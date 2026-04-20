.section .text
.global isr_wrapper_130
.global isr_wrapper_129
.global isr_wrapper_33
.global isr_wrapper_32
.global isr_wrapper_14
.global isr_wrapper_13
.global isr_wrapper_8
.global isr_wrapper_0

# New keyboard interrupt wrapper (IRQ 1 = interrupt 33)
isr_wrapper_33:
    
    
    pusha                           # Save all registers
    call keyboard_interrupt_handler # Call our C keyboard function
    popa                            # Restore all registers
    
    iret                            # Return from interrupt


# New Process switch interrupt wrapper
isr_wrapper_130:


    pusha
    
    # LOAD next ESP
    movl current_process, %ebx

    #movl (%ebx), %esp


    movl 4(%ebx), %ecx         # load page_dir value into ecx
    subl $0xC0000000, %ecx
    movl %ecx, %cr3            # now move from register to CR3
    popa
    iret



# New timer interrupt wrapper (IRQ 0 = interrupt 32)
isr_wrapper_32:
    pusha
    movl %esp, (esp_address_variable)
    call timer_interrupt_handler
    
    # Compare current and next
    movl current_process, %eax
    movl next_process, %ebx
    cmpl %eax, %ebx
    je skip_switch              # Skip if same
    
    # SAVE current ESP into the current process's PCB (offset 0 = saved_esp)
    movl %esp, (%eax)

    # SWITCH PAGE DIRECTORY (CR3) *before* loading the new stack pointer.
    #
    # Why this order matters:
    #   The new process's stack is mapped at PROC_STACK_VIRT (0xBFFFF000), a
    #   user-space address that is ONLY present in that process's page directory.
    #   If we loaded ESP first and then switched CR3, the CPU would try to read
    #   the new stack while the OLD page directory was still active, find no
    #   mapping, and triple-fault.
    #
    #   We read page_dir from %ebx (the PCB pointer in kernel space) before we
    #   touch ESP, so all memory accesses here are still in kernel space and safe.
    movl 4(%ebx), %ecx          # next_process->page_dir (virtual, offset 4 in PCB)
    subl $0xC0000000, %ecx      # convert to physical address (CR3 must be physical)
    movl %ecx, %cr3             # switch page directory — TLB is flushed here

    # Update current_process *after* CR3 is live but *before* we load new ESP.
    # current_process is a kernel global (0xC0000000+), still reachable.
    movl %ebx, current_process

    # NOW load the new stack pointer.  The correct page directory is active, so
    # PROC_STACK_VIRT is mapped and popa/iret below will work correctly.
    movl (%ebx), %esp           # next_process->saved_esp (offset 0 in PCB)

skip_switch:
    popa
    iret

# System call interrupt wrapper (IRQ 128 = interrupt 129)
isr_wrapper_129:
    
    pusha
    pushl %esp        #← push esp as argument to the function
    
    call system_call_interrupt_handler
    addl $4, %esp   #  ← clean up the argument
    popa
    iret



# divide by zero exception wrapper 
isr_wrapper_0:
    push 0
    pusha                           # Save all registers
    call divide_by_zero_handler # Call our C divide by zero function
    popa                            # Restore all registers
    addl $4, %esp
    iret                            # Return from interrupt
# general protection fault exception wrapper
isr_wrapper_13:
    pusha                           # Save all registers
    pushl %esp
    call general_protection_fault_handler # Call our C general_protection_fault_handler
    addl $4, %esp
    popa                            # Restore all registers
    addl $4, %esp
    iret                            # Return from interrupt
# page fault exception wrapper
isr_wrapper_14:
    pusha                           # Save all registers
    pushl %esp
    call page_fault_handler # Call our C page_fault_handler
    addl $4, %esp
    popa                            # Restore all registers
    addl $4, %esp
    iret                            # Return from interrupt
# double fault exception wrapper
isr_wrapper_8:
    pusha                           # Save all registers
    pushl %esp
    call double_fault_handler # Call our C page_fault_handler
    addl $4, %esp
    popa                            # Restore all registers
    addl $4, %esp
    iret                            # Return from interrupt