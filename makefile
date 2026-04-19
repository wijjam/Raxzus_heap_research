# Compiler and tools
CC = i686-elf-gcc
AS = i686-elf-as
LD = i686-elf-gcc

# Compiler flags
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude -g

# Linker flags  
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -nostartfiles -nodefaultlibs -lgcc

# Source files
ASM_SOURCES = boot.s interrupt_stubs.s
C_SOURCES = $(wildcard *.c) $(wildcard lib/*.c) $(wildcard kernel/*.c) $(wildcard drivers/*.c)

# Object files
ASM_OBJECTS = $(ASM_SOURCES:.s=.o)
C_OBJECTS = $(C_SOURCES:.c=.o)
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# Output files
KERNEL_ELF = myos.elf
KERNEL = myos.bin
ISO = myos.iso
DISK = disk.img
DISK_SIZE = 64

# Default target
all: $(ISO)

# Build ISO
$(ISO): $(KERNEL)
	@echo "Creating ISO..."
	@mkdir -p isodir/boot/grub
	@cp $(KERNEL) isodir/boot/
	@echo 'set timeout=3' > isodir/boot/grub/grub.cfg
	@echo 'set default=0' >> isodir/boot/grub/grub.cfg
	@echo '' >> isodir/boot/grub/grub.cfg
	@echo 'menuentry "MyOS" {' >> isodir/boot/grub/grub.cfg
	@echo '    multiboot /boot/$(KERNEL)' >> isodir/boot/grub/grub.cfg
	@echo '    boot' >> isodir/boot/grub/grub.cfg
	@echo '}' >> isodir/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO) isodir 2>/dev/null || grub-mkrescue -o $(ISO) isodir
	@echo "ISO created: $(ISO)"

# Create binary from ELF (for bootloader)
$(KERNEL): $(KERNEL_ELF)
	@echo "Creating binary from ELF..."
	@cp $(KERNEL_ELF) $(KERNEL)

# Link kernel (creates ELF with debug symbols)
$(KERNEL_ELF): $(OBJECTS)
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) boot.o $(filter-out boot.o, $(OBJECTS))
	@echo "Kernel ELF built: $(KERNEL_ELF)"

$(DISK):
	@echo "Creating blank disk image ($(DISK_SIZE)MB)..."
	@dd if=/dev/zero of=$(DISK) bs=1M count=$(DISK_SIZE)
	@echo "Disk image created: $(DISK)"

# Compile C files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble assembly files
%.o: %.s
	@echo "Assembling $<..."
	$(AS) $< -o $@

# Run in QEMU
run: $(ISO) $(DISK)
	@echo "Starting QEMU..."
	qemu-system-i386 -cdrom $(ISO) -drive file=$(DISK),format=raw,if=ide

run-serial: $(ISO) $(DISK)
	@echo "Starting QEMU with serial output..."
	qemu-system-i386 -cdrom $(ISO) -drive file=$(DISK),format=raw -serial stdio -m 4G

debug: $(ISO) $(DISK)
	@echo "Starting QEMU with debugging..."
	qemu-system-i386 -cdrom $(ISO) -drive file=$(DISK),format=raw -d int,cpu_reset -no-reboot -no-shutdown

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(OBJECTS) $(KERNEL) $(KERNEL_ELF) $(ISO)
	@rm -rf isodir
	@echo "Clean complete"

clean-disk:
	@rm -f $(DISK)
	@echo "Disk image removed"

# Clean everything including downloads
distclean: clean
	@echo "Removing all generated files..."
	@rm -rf build-* binutils-* gcc-* *.tar.xz

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build ISO (default)"
	@echo "  run          - Build and run in QEMU"
	@echo "  debug        - Run with debugging output"
	@echo "  run-serial   - Run with serial console"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove everything including downloads"
	@echo "  help         - Show this help message"

# Install kernel to a location (useful for testing on real hardware)
install: $(ISO)
	@echo "Installing $(ISO) to /tmp/myos.iso"
	@cp $(ISO) /tmp/myos.iso
	@echo "You can now burn /tmp/myos.iso to a USB or CD"

# Rebuild everything from scratch
rebuild: clean all

# Show compiler version
version:
	@$(CC) --version
	@$(AS) --version

# Count lines of code
count:
	@echo "Lines of code:"
	@wc -l *.c *.h *.s lib/*.c lib/*.h include/*.h 2>/dev/null | tail -1

# Run with GDB debugging
gdb: $(ISO)
	@echo "Starting QEMU with GDB server..."
	@echo "In another terminal, run: gdb myos.elf"
	@echo "Then type: target remote localhost:1234"
	qemu-system-i386 -s -S -cdrom $(ISO)

# Declare phony targets (not actual files)
.PHONY: all clean distclean run debug run-serial help install rebuild version count
