ENTRY(_start)

SECTIONS
{
    /* Start putting things at physical 1 Megabyte */
    . = 1M;

    .boot_data : ALIGN(4K)
    {
        *(.boot_data)
    }

    .boot_text : ALIGN(4K)
    {
        *(.boot_text)
    }

    /* Shift the location counter up to 3 Gigabytes */
    . += 0xC0000000;

    /* AT(...) tells GRUB to keep loading it continuously in physical RAM */
    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000)
    {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000)
    {
        *(.rodata)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)

        . = ALIGN(4096);
        _kernel_page_dir = .;
        . += 4096;
        _kernel_page_table = .;
        . += 4096;

        /*
         * Pre-allocate the 256 page-table frames that map_kernel_memory()
         * needs (one per page-directory entry 768-1023, covering the full
         * 1 GB kernel virtual range).  Keeping them here in BSS — BEFORE
         * _heap_start — means they never overlap with the kernel heap.
         *
         * Without this reservation, get_next_free_kernel_frame() would hand
         * out frames starting at physical(_heap_start), the same bytes that
         * kmalloc() writes to.  Every kmalloc call would then silently
         * corrupt the live page tables.  The TLB hides the damage until the
         * next CR3 write (every context switch), at which point the CPU
         * re-reads the corrupted entries and triple-faults.
         *
         * 256 tables × 4 KB = 1 MB.
         */
        . = ALIGN(4096);
        _kernel_page_tables_base = .;
        . += (256 * 4096);
    }

    _heap_start = ALIGN(4K);
    _kernel_end = .;
}