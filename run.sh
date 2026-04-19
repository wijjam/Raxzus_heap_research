#!/bin/bash
echo "Building MyOS..."

# Compile assembly
i686-elf-as boot.s -o boot.o
i686-elf-as interrupt_stubs.s -o interrupt_stubs.o

# Compile all C source files
echo "Compiling library files..."
i686-elf-gcc -c lib/io.c -o lib/io.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/rtc.c -o lib/rtc.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/vga.c -o lib/vga.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/interrupts.c -o lib/interrupts.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/pic.c -o lib/pic.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/keyboard.c -o lib/keyboard.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/keymap.c -o lib/keymap.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/raxzus_shell.c -o lib/raxzus_shell.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/memory.c -o lib/memory.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/process_manager.c -o lib/process_manager.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/forgeproc.c -o lib/forgeproc.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/paging_manager.c -o lib/paging_manager.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c lib/raxzus_shell_ui.c -o lib/raxzus_shell_ui.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

echo "Compiling kernel..."
i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

# Link everything together
i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o interrupt_stubs.o lib/io.o lib/rtc.o lib/vga.o lib/interrupts.o lib/pic.o lib/keyboard.o lib/keymap.o lib/raxzus_shell.o lib/memory.o lib/process_manager.o lib/forgeproc.o lib/exceptions.o lib/paging_manager.o lib/raxzus_shell_ui.o -lgcc
# Create ISO
echo "Creating ISO..."
cp myos.bin isodir/boot/
grub-mkrescue -o myos.iso isodir

# Run
echo "Starting QEMU..."
qemu-system-i386 -cdrom myos.iso