Author: William B.
Date: April 2026
Status: Prototype — 32-bit, single core

Raxzus Flow — A Novel MMU-Backed Domain Allocation Method

Raxzus Flow is the name of the allocation method. This document describes its first implementation in RaxzusOS, a custom x86 32-bit kernel.

This is a novel heap allocator implemented in a custom x86 32-bit kernel (RaxzusOS). The core idea is simple: instead of using software data structures to manage allocations, we delegate that responsibility to the hardware MMU itself.

Virtual address regions encode size class directly. The address of a pointer tells the allocator everything it needs to know — no per-allocation metadata is stored for small allocations.
Design

The heap is divided into seven size-class domains:
Domain	Block Size	Virtual Region
heap_64	64 bytes	0x10000000
heap_128	128 bytes	0x20000000
heap_256	256 bytes	0x30000000
heap_512	512 bytes	0x40000000
heap_1k	1024 bytes	0x50000000
heap_2k	2048 bytes	0x60000000
heap_4k	4096 bytes	0x70000000

Each domain has its own page directory. When an allocation is requested, the allocator switches CR3 to the appropriate domain, pops the free list head, and restores the kernel CR3. The entire operation is O(1) by construction — there is no searching, no coalescing, no walking of any structure.

kfree derives the correct domain purely from the pointer address. No size argument is needed. No header is read.
Free List

Free blocks store a pointer to the next free block in their own first 4 bytes. When a block is handed to the caller, those bytes become user data. When freed, they become list linkage again. There is no separate bookkeeping structure — freed memory is the bookkeeping.

This is a LIFO free list. The most recently freed block is the next to be allocated.
Performance

Tests were run on a 32-bit x86 system without PCID support.

Steady state (free list warm, 32 samples):

    kmalloc(64) + kfree: 636 cycles, jitter 8 cycles
    kmalloc(256) + kfree: 638 cycles, jitter 14 cycles
    kmalloc(1024) + kfree: 640 cycles, jitter 40 cycles

Under heap pressure (24,000 live allocs, 24,000 freed holes):

    Cost delta vs steady state: 2 cycles
    O(1) confirmed — heap state has zero effect on allocation cost

Round trip cost (kmalloc + kfree combined) is 636 ± 40 cycles on 32-bit x86 without PCID. Per operation this is approximately 300 ± 20 cycles.

The majority of this cost is TLB flush overhead from CR3 switching, not allocator logic. The actual allocation and free operations are 2-3 instructions each.

On 64-bit with PCID support, CR3 switches carry no TLB flush cost. The round trip cost is expected to drop to sub-100 cycles, as the allocator logic itself is approximately 5-10 cycles.
Cache Behavior

The entire allocator fits within a standard 32KB L1 instruction cache. This contributes directly to the low jitter observed in benchmarks.

A notable observation: under heap pressure, allocation cost occasionally measures below steady state. This is consistent with LIFO free list cache locality — recently freed blocks are returned first and remain hot in L1 data cache, making the subsequent allocation faster than a cold steady state access.
Fragmentation

Internal fragmentation is bounded by size class geometry. Worst case is just under 50% on 32-bit (a 65-byte allocation consuming a 128-byte block). In practice the average case is significantly lower.

External fragmentation does not occur. Blocks within a domain are fixed size — there are no adjacent free blocks of different sizes to fragment.
Memory Protection

Each domain has its own page directory. The MMU enforces domain boundaries in hardware. A pointer from the 64-byte domain cannot reach the 512-byte domain's memory regardless of software behavior.

Kernel stack lives at 0xC0000000+, heap domains at 0x10000000–0x80000000. Stack-heap collision is architecturally impossible — not probabilistically unlikely, not canary-protected, impossible.

Per-process isolation follows the same pattern. Each process receives the same virtual layout backed by different physical frames. Processes cannot access each other's memory by page table construction.
Large Allocations

Allocations above 4096 bytes are handled by a separate large domain at 0x80000000. Pages are mapped on demand — there is no pre-committed ceiling. A 4-byte page count header is stored inline, which is the one known metadata overhead in the current prototype.

Large allocation cost is approximately 7470 cycles per page, dominated by page table mapping overhead.
Limitations

    Large allocations carry a 4-byte inline header
    Physical frames are not returned to the PMM on large free (prototype limitation)
    Without PCID, CR3 switching cost dominates the cycle count
    No multicore support yet — planned for the 64-bit port

What's Next

The 64-bit port is the priority. With PCID support and a wider address space, the Fibonacci size class scheme (32, 64, 96, 128 ... 4096) becomes viable, tightening worst-case fragmentation to under 25%. The same design, in the environment it was built for.

Images of tests of <4KB allocation and frees can be seen below:

<img width="1043" height="782" alt="image" src="https://github.com/user-attachments/assets/ff08392d-4fad-4fe8-8d07-c36fd4936ceb" />

<img width="1043" height="782" alt="image" src="https://github.com/user-attachments/assets/ea6f9f03-7ad9-46fc-92ac-b971ed911b30" />

Below are tests made where we test the allocation of blocks bigger than 4KB to see how it handles them, the result is the following:

<img width="1043" height="782" alt="image" src="https://github.com/user-attachments/assets/3553bbf5-4e37-47b2-b78e-4040bd9ff807" />
<img width="1043" height="782" alt="image" src="https://github.com/user-attachments/assets/dfbf5b54-1e48-44b6-8331-caf57711ca62" />
<img width="1210" height="907" alt="image" src="https://github.com/user-attachments/assets/8df72b3d-0cbd-4e45-88b5-de2a20118c2d" />
All these tests were taken at different times.
