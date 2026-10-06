# NexusOS

![Cardinal](logo.png)

A tiny operating system for i386, written **from scratch** ??? bootloader, kernel,
drivers, filesystem, multitasking, network stack and a built-in BASIC ??? with no
libc, no existing kernel, no copy-paste frameworks. Just C, x86 assembly and QEMU.

> Every stage was debugged by hand on real interrupts,
> page faults and panic dumps. See the changelog below.

## Highlights

- Custom MBR bootloader -> 32-bit protected mode (GDT, IDT, PIC, PIT, RTC)
- Paging (4 MB pages) + physical memory manager + `kmalloc` heap
- Preemptive round-robin multitasking (`ps` / `spawn` / `kill`)
- Hierarchical filesystem with directories, persisted to disk via ATA PIO
- ELF loader + `int 0x80` syscalls for user programs
- Network stack from scratch: PCI -> e1000 MMIO -> Ethernet -> ARP -> IPv4 -> ICMP (`ping` works)
- VGA text UI, PS/2 keyboard & mouse, PC speaker, text editor
- **NexusBASIC** ??? a built-in interpreter (LET / PRINT / IF..THEN..ELSE / GOTO / LIST)
- Red-screen panic handler with register dump (it actually helps debug)

## Build & run (Windows)

Requirements: LLVM/Clang (`clang`, `ld.lld`, `llvm-objcopy`) and QEMU on PATH.

```bat
build.bat
