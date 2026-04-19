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
