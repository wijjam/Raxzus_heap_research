.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

# ==========================================
# THE 1MB WORLD (PHYSICAL MEMORY)
# ==========================================

.section .boot_data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.align 4096
boot_page_directory:
    # Identity map 0-4MB (Flags: Present, R/W, 4MB Page)
    .long 0x00000083
    .fill 767, 4, 0
    # Higher half map 3GB->0-4MB (Flags: Present, R/W, 4MB Page)
    .long 0x00000083
    .fill 255, 4, 0

# We put the entry point in its own boot section so it stays at 1MB!
.section .boot_text, "ax"
.global _start
.type _start, @function
_start:
    # 1. Load our page directory
    mov $boot_page_directory, %ecx
    mov %ecx, %cr3

    # 2. Enable Page Size Extension (PSE) so our 4MB pages work
    mov %cr4, %ecx
    or $0x00000010, %ecx
    mov %ecx, %cr4

    # 3. Enable Paging!
    mov %cr0, %ecx
    or $0x80000000, %ecx
    mov %ecx, %cr0

    # 4. Blast off into the higher half
    lea higher_half, %ecx
    jmp *%ecx

.size _start, . - _start

# ==========================================
# THE 3GB WORLD (VIRTUAL MEMORY)
# ==========================================

.section .text
higher_half:
    # Now we are safely executing at 0xC0000000+
    mov $stack_top, %esp
    
    # We unmap the 0-4MB identity map right away so NULL pointers crash safely
    movl $0, boot_page_directory
    mov %cr3, %ecx
    mov %ecx, %cr3   # Flush the TLB

    call kernel_main
    
    cli
1:  hlt
    jmp 1b

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:
