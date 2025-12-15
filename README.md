# PeachOS-Game

A tiny 32-bit hobby OS kernel written in C/ASM for i686. Builds a bootable FAT16 disk image with a simple user program and drops a text file onto it.

## Prerequisites
- nasm
- make
- sudo (used to mount the image while building)
- Cross toolchain: i686-elf-gcc, i686-elf-ld
- Optional: qemu-system-i386 (to run)

## Build
```bash
./build.sh        # sets PATH to your cross toolchain and runs make all
# or: make all
```
Build outputs go to `bin/`:
- `boot.bin` – boot sector
- `kernel.bin` – kernel binary
- `os.bin` – combined 16MB disk image (boot + kernel + padding + copied hello.txt)

## Run in QEMU
```bash
qemu-system-i386 -hda bin/os.bin
```

## Clean
```bash
make clean
```

## Layout
- `src/` – kernel sources (boot, GDT/IDT, paging, heap, filesystem, tasks, keyboard, screen)
- `programs/blank/` – sample user program
- `build/` – object files
- `bin/` – boot sector, kernel, and final disk image
