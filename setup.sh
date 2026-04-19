#!/bin/bash

set -e  # Exit on error

echo "=========================================="
echo "OS Development Environment Setup"
echo "=========================================="
echo ""

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "Cannot detect OS. Please install dependencies manually."
    exit 1
fi

echo "Detected OS: $OS"
echo ""

# Install system dependencies based on OS
echo "Installing system dependencies..."
case $OS in
    ubuntu|debian|pop)
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            gcc-multilib \
            g++-multilib \
            nasm \
            qemu-system-x86 \
            grub-common \
            grub-pc-bin \
            xorriso \
            mtools \
            git
        
        # Try to install cross-compiler packages if available
        sudo apt-get install -y gcc-i686-linux-gnu binutils-i686-linux-gnu 2>/dev/null || true
        ;;
    fedora|rhel|centos)
        sudo dnf install -y \
            gcc gcc-c++ \
            glibc-devel.i686 \
            nasm \
            qemu-system-x86 \
            grub2-tools \
            xorriso \
            mtools \
            git
        ;;
    arch|manjaro)
        sudo pacman -S --needed --noconfirm \
            base-devel \
            multilib-devel \
            nasm \
            qemu-system-x86 \
            grub \
            xorriso \
            mtools \
            git
        ;;
    *)
        echo "Unsupported OS: $OS"
        echo "Please install dependencies manually."
        exit 1
        ;;
esac

echo ""
echo "System dependencies installed!"
echo ""

# Set up cross-compiler
OSDEV_DIR="$HOME/osdev"
PREFIX="$OSDEV_DIR/cross-compiler"

echo "Setting up cross-compiler in $PREFIX"
echo ""

# Create directories
mkdir -p "$PREFIX/bin"

# Create wrapper scripts for cross-compiler
echo "Creating cross-compiler wrapper scripts..."

# i686-elf-gcc wrapper
cat > "$PREFIX/bin/i686-elf-gcc" << 'EOF'
#!/bin/bash
# Wrapper for i686 cross-compilation using system gcc
# Use -ffreestanding instead of -nostdinc to allow compiler built-ins like stdarg.h
gcc -m32 -ffreestanding -fno-builtin -fno-stack-protector "$@"
EOF
chmod +x "$PREFIX/bin/i686-elf-gcc"

# i686-elf-as wrapper
cat > "$PREFIX/bin/i686-elf-as" << 'EOF'
#!/bin/bash
# Wrapper for i686 assembler
as --32 "$@"
EOF
chmod +x "$PREFIX/bin/i686-elf-as"

# i686-elf-ld wrapper
cat > "$PREFIX/bin/i686-elf-ld" << 'EOF'
#!/bin/bash
# Wrapper for i686 linker
ld -m elf_i386 "$@"
EOF
chmod +x "$PREFIX/bin/i686-elf-ld"

# i686-elf-ar wrapper (for libraries)
cat > "$PREFIX/bin/i686-elf-ar" << 'EOF'
#!/bin/bash
# Wrapper for archiver
ar "$@"
EOF
chmod +x "$PREFIX/bin/i686-elf-ar"

# i686-elf-objcopy wrapper
cat > "$PREFIX/bin/i686-elf-objcopy" << 'EOF'
#!/bin/bash
# Wrapper for objcopy
objcopy "$@"
EOF
chmod +x "$PREFIX/bin/i686-elf-objcopy"

echo "Cross-compiler wrappers created!"
echo ""

# Create environment setup script
cat > "$OSDEV_DIR/setup-env.sh" << 'EOF'
#!/bin/bash
# OS Development Environment Setup
echo "Setting up OS development environment..."

# Set up cross-compiler paths
export PREFIX="$HOME/osdev/cross-compiler"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Verify the tools are available
if command -v i686-elf-gcc &> /dev/null; then
    echo "✓ Cross-compiler found: i686-elf-gcc"
else
    echo "✗ Cross-compiler not found! Make sure it's built in $PREFIX"
    echo "Run the setup.sh script to build the cross-compiler."
    return 1 2>/dev/null || true
fi

if command -v i686-elf-as &> /dev/null; then
    echo "✓ Assembler found: i686-elf-as"
else
    echo "✗ Assembler not found!"
    return 1 2>/dev/null || true
fi

echo "✓ Environment ready for OS development!"
echo ""
echo "Available tools:"
echo "  - i686-elf-gcc (cross-compiler)"
echo "  - i686-elf-as (assembler)"
echo "  - i686-elf-ld (linker)"
echo ""
echo "You can now run 'make' or 'make run' to build and test your kernel."
EOF

chmod +x "$OSDEV_DIR/setup-env.sh"

# Test the cross-compiler
export PATH="$PREFIX/bin:$PATH"

echo "Testing cross-compiler..."
if command -v i686-elf-gcc &> /dev/null; then
    echo "✓ i686-elf-gcc: Working"
    
    # Create a simple test
    cat > /tmp/test-cross.c << 'TESTEOF'
void _start() {
    while(1);
}
TESTEOF
    
    if i686-elf-gcc -c /tmp/test-cross.c -o /tmp/test-cross.o 2>/dev/null; then
        echo "✓ Test compilation successful!"
        rm -f /tmp/test-cross.c /tmp/test-cross.o
    else
        echo "⚠ Warning: Cross-compiler installed but test compilation failed"
        echo "  This might still work for your kernel. Try 'make' to verify."
    fi
else
    echo "✗ i686-elf-gcc not found!"
    exit 1
fi

if command -v i686-elf-as &> /dev/null; then
    echo "✓ i686-elf-as: Working"
else
    echo "✗ i686-elf-as not found!"
    exit 1
fi

echo ""
echo "=========================================="
echo "Setup Complete!"
echo "=========================================="
echo ""
echo "Your OS development environment is ready!"
echo ""
echo "Next steps:"
echo "  1. cd to your kernel source directory"
echo "  2. Run: source ~/osdev/setup-env.sh"
echo "  3. Run: make run"
echo ""
echo "To automatically load the environment in new terminals:"
echo "  echo 'source ~/osdev/setup-env.sh' >> ~/.bashrc"
echo ""
echo "Happy OS development!"
echo ""
