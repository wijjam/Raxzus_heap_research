     Copyright (c) 2026 William Berglind
     Raxzus Flow   MMU backed domain heap allocator
     Licensed under the Apache License 2.0
     
## Disclaimer: 
This started as a hobby kernel project. I thought the MMU was underused at the heap level and wanted to see if the idea worked. It did. I published it because to my knowledge nobody had done it this way before. The 64-bit port is coming, multicore is coming, more tests are coming. Follow the repo if you're curious where it goes.


# Raxzus Flow   A Novel MMU Backed Domain Allocation Method

Author: William B.
Date: April 2026
Status: Prototype    32-bit, single core

[![DOI](https://zenodo.org/badge/1215019950.svg)](https://doi.org/10.5281/zenodo.19697609)

     To the author's knowledge, no prior work describes a heap allocator using 
     separate page directories per size class with kernel address space double 
     mapping, making this the first of its kind.
     
     Note: After initial publication, related systems were identified that use 
     subsets of these techniques: SLAB_VIRTUAL (virtual partitioning without 
     separate page directories), xMP (EPT based isolation by security domain 
     rather than size class), and KPTI (dual CR3 for user/kernel isolation). 
     None combine the complete Raxzus Flow architecture.


Raxzus Flow is the name of the allocation method. This document describes its first implementation in RaxzusOS, a custom x86  32-bit kernel.

This is a novel heap allocator implemented in a custom x86  32-bit kernel (RaxzusOS). The core idea is simple: instead of using software data structures to manage allocations, we delegate that responsibility to the hardware MMU itself.

Virtual address regions encode size class directly. The address of a pointer tells the allocator everything it needs to know   no per allocation metadata is stored for small allocations.

## Design

The heap is divided into seven size class domains:
```
Domain	Block Size	Virtual Region
heap_64	64 bytes	0x10000000
heap_128	128 bytes	0x20000000
heap_256	256 bytes	0x30000000
heap_512	512 bytes	0x40000000
heap_1k	1024 bytes	0x50000000
heap_2k	2048 bytes	0x60000000
heap_4k	4096 bytes	0x70000000
```

Each domain has its own page directory. When an allocation is requested, the allocator uses the multi mapped functionality and, pops the free list head and returns. The entire operation is O(1) by construction   there is no searching, no coalescing, no walking of any structure.

Each domain's physical frames are mapped into both the domain's own page directory and the kernel's page directory at the same time. This means returned pointers stay valid in kernel space without needing a CR3 switch. The fast path   when the free list has blocks available   never touches CR3 at all. CR3 switching is not required at any point. The page_directory pointer passed to map_page is a higher half virtual address   map_page never dereferences domain virtual addresses, so the kernel CR3 remains active throughout. The separate page directories exist solely for hardware isolation.

kfree derives the correct domain purely from the pointer address. No size argument is needed. No header is read.
Free List.

Free blocks store a pointer to the next free block in their own first 4 bytes. When a block is handed to the caller, those bytes become user data. When freed, they become list linkage again. There is no separate bookkeeping structure   freed memory is the bookkeeping.

This is a LIFO free list. The most recently freed block is the next to be allocated.

Each domain occupies a 256MB virtual region, sufficient for millions of small block allocations before virtual exhaustion.

## Performance

Tests were run on a  32-bit x86 system.

Steady state (free list warm, 32 samples):

    FRAGMENTED kmalloc(1024) + kfree: min=24 max=62 avg=27 stddev=6 cycles 
    CLEAN      kmalloc(1024) + kfree: min=26 max=28 avg=27 stddev=1 cycles
    O(1) delta (avg): 0 cycles
Images of this and more tests can be seen at the bottom of the README.

Round trip avg cost (kmalloc + kfree combined) is approximately 27 cycles on  32-bit x86.

When the free list is exhausted, a new physical page is mapped on demand. This slow path costs approximately 738–1770 cycles depending on page table state, dominated by PMM frame allocation and page table writes.

## Cache Behavior

The entire allocator fits within a standard 32KB L1 instruction cache. This contributes directly to the low stddev observed in benchmarks.

A notable observation: under heap pressure, allocation cost occasionally measures below steady state. This is consistent with LIFO free list cache locality   recently freed blocks are returned first and remain hot in L1 data cache, making the subsequent allocation faster than a cold steady state access.
Fragmentation

Internal fragmentation is bounded by size class geometry. Worst case is just under 50% on  32-bit (a 65 byte allocation consuming a 128 byte block). In practice the average case is lower.

External fragmentation does not occur. Blocks within a domain are fixed size there are no adjacent free blocks of different sizes to fragment.
Memory Protection

Each domain has its own page directory. The MMU enforces domain boundaries in hardware. A pointer from the 64 byte domain cannot reach the 512 byte domain's memory regardless of software behavior.

Per process isolation follows the same pattern. Each process receives the same virtual layout backed by different physical frames. Processes cannot access each other's memory by page table construction.

Fault Elimination: Physical frames are mapped when the LIFO empties, before any pointer is returned to the caller. This eliminates demand paging page faults entirely   the slow path is triggered by an empty free list, not by a hardware exception. Traditional demand paged allocators pay 10,000+ cycles for a fault handler round trip. Raxzus Flow's worst case is 600–700 cycles with no exception overhead.

## Large Allocations

Large allocation cost is O(n) where n is the number of 4KB pages required. First page costs approximately 200–738 cycles, subsequent pages approximately 1047–1770 cycles per page as page table structures are established.
Limitations

    Large allocations carry a 4 byte inline header
    No multicore support yet   planned for the 64 bit port

More to come regarding Large Allocations
## What's Next

Optimizing the PMM to go from a nestled loop search for each new page to using a linked list LIFO, this should make worst path take less cycles.
The 64 bit port will use a finer grained size class scheme with 8 byte alignment to attempt to reduce worst case internal fragmentation below 25%.

Below here are the test run images of the heap with 1024 domain blocks.

<img width="977" height="733" alt="image" src="https://github.com/user-attachments/assets/531dae44-6e81-4dc4-a982-5fb02ebbb8b6" />


We see here the same data as written in the top section. We can see it being delta between fragmented and clean around 0 to 3 cycles


<img width="977" height="733" alt="image" src="https://github.com/user-attachments/assets/271d077c-4ac5-4e03-bc72-cce5252511e0" />
Above is the image of large allocs currently.

