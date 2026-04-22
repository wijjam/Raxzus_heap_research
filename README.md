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

Each domain has its own page directory. When an allocation is requested, the allocator uses our multi mapped functionality and, pops the free list head and returns. The entire operation is O(1) by construction — there is no searching, no coalescing, no walking of any structure.

Each domain's physical frames are mapped into both the domain's own page directory and the kernel's page directory at the same time. This means returned pointers stay valid in kernel space without needing a CR3 switch. The fast path — when the free list has blocks available — never touches CR3 at all. CR3 switching only happens on the slow path when a new physical page needs to be mapped.

kfree derives the correct domain purely from the pointer address. No size argument is needed. No header is read.
Free List.

Free blocks store a pointer to the next free block in their own first 4 bytes. When a block is handed to the caller, those bytes become user data. When freed, they become list linkage again. There is no separate bookkeeping structure — freed memory is the bookkeeping.

This is a LIFO free list. The most recently freed block is the next to be allocated.

Performance

Tests were run on a 32-bit x86 system without PCID support.

Steady state (free list warm, 32 samples):

    FRAGMENTED kmalloc(1024) + kfree: min=26 max=28 avg=26 stddev=0 cycles 
    CLEAN      kmalloc(1024) + kfree: min=24 max=36 avg=26 stddev=2 cycles
    O(1) delta (avg): 0 cycles
Images of this and more tests can be seen at the bottom of the README.

Round trip cost (kmalloc + kfree combined) is 26 +- 2 cycles on 32-bit x86 without PCID.

Fast path on 64-bit with PCID: expected sub-20 cycles since CR3 switch cost disappears.

Slow path on 64-bit with PCID: significantly cheaper since TLB flush on page mapping is eliminated.

When the free list is exhausted, a new physical page is mapped on demand. This slow path costs approximately 5000-6500 cycles and is amortized over N allocations where N = 4096 / block_size.

Cache Behavior

The entire allocator fits within a standard 32KB L1 instruction cache. This contributes directly to the low stddev observed in benchmarks.

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

The 64-bit port will use a finer-grained size class scheme with 8-byte alignment to reduce worst-case internal fragmentation below 25%.

Below here are the test run images of the heap with <4KB blocks.

<img width="808" height="606" alt="image" src="https://github.com/user-attachments/assets/ae9112e6-717e-490c-9bd9-a8413093b70a" />

We see here the same data as written in the top section. With the avg cycles for malloc and free being 26 cycles.

<img width="808" height="606" alt="image" src="https://github.com/user-attachments/assets/f3f48c76-9496-4053-b57a-e6941bf60182" />

As we can see the avg and stddev stay consistant through runs.

<img width="808" height="606" alt="image" src="https://github.com/user-attachments/assets/5b360769-39d6-4677-a967-01148ea4ee5b" />



