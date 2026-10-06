@echo off
setlocal
set "LLVM_PATH=C:\Program Files\LLVM\bin"
set "QEMU_PATH=C:\Program Files\qemu"
set "PATH=%LLVM_PATH%;%QEMU_PATH%;%PATH%"
mkdir build 2>nul

echo [1/9] boot.s + gdt_flush...
clang --target=i686-none-elf -O2 -c boot\boot.s -o build\boot.o
if errorlevel 1 goto fail
clang --target=i686-none-elf -c kernel\gdt_flush.s -o build\gdt_flush.o
if errorlevel 1 goto fail

echo [2/9] ASM stubs...
clang --target=i686-none-elf -c kernel\idt_flush.s -o build\idt_flush.o
if errorlevel 1 goto fail
clang --target=i686-none-elf -c kernel\interrupt.s -o build\interrupt.o
if errorlevel 1 goto fail
clang --target=i686-none-elf -c kernel\kbd_irq.s -o build\kbd_irq.o
if errorlevel 1 goto fail
clang --target=i686-none-elf -c kernel\mouse_irq.s -o build\mouse_irq.o
if errorlevel 1 goto fail
clang --target=i686-none-elf -c kernel\sched_irq.s -o build\sched_irq.o
if errorlevel 1 goto fail
clang --target=i686-none-elf -c kernel\syscall.s -o build\syscall.o
if errorlevel 1 goto fail

echo [3/9] C core...
set "CFLAGS=--target=i686-none-elf -std=gnu99 -ffreestanding -O2 -fno-builtin -fno-stack-protector -Ikernel -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx"
clang %CFLAGS% -c kernel\serial.c -o build\serial.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\gdt.c -o build\gdt.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\idt.c -o build\idt.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\isrs.c -o build\isrs.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\irq.c -o build\irq.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\timer.c -o build\timer.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\rtc.c -o build\rtc.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\pmm.c -o build\pmm.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\kmalloc.c -o build\kmalloc.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\ata.c -o build\ata.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\elf.c -o build\elf.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\net.c -o build\net.o
if errorlevel 1 goto fail

echo [4/9] C drivers + fs + editor + tasks...
clang %CFLAGS% -c kernel\vga.c -o build\vga.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\keyboard.c -o build\keyboard.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\mouse.c -o build\mouse.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\speaker.c -o build\speaker.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\splash.c -o build\splash.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\fs.c -o build\fs.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\editor.c -o build\editor.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\task.c -o build\task.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\sched.c -o build\sched.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\shell.c -o build\shell.o
if errorlevel 1 goto fail
clang %CFLAGS% -c kernel\kernel.c -o build\kernel.o
if errorlevel 1 goto fail

echo [5/9] Linking kernel...
ld.lld -T kernel.ld -nostdlib build\boot.o build\gdt_flush.o build\idt_flush.o build\interrupt.o build\kbd_irq.o build\mouse_irq.o build\sched_irq.o build\syscall.o build\serial.o build\gdt.o build\idt.o build\isrs.o build\irq.o build\timer.o build\rtc.o build\pmm.o build\kmalloc.o build\ata.o build\elf.o build\net.o build\vga.o build\keyboard.o build\mouse.o build\speaker.o build\splash.o build\fs.o build\editor.o build\task.o build\sched.o build\shell.o build\kernel.o -o build\nexusos.elf
if errorlevel 1 goto fail

echo [6/9] Flat binary...
llvm-objcopy -O binary build\nexusos.elf build\nexusos.bin
if errorlevel 1 goto fail

echo [7/9] Boot sector...
clang --target=i686-none-elf -c boot\bootsec.s -o build\bootsec.o
if errorlevel 1 goto fail
ld.lld -T bootsec.ld -nostdlib build\bootsec.o -o build\bootsec.elf
if errorlevel 1 goto fail
llvm-objcopy -O binary build\bootsec.elf build\boot.bin
if errorlevel 1 goto fail

echo [8/9] User programs...
set "UFLAGS=--target=i686-none-elf -std=gnu99 -ffreestanding -O2 -fno-builtin -fno-stack-protector -Iuser -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx"
clang --target=i686-none-elf -c user\crt0.s -o build\crt0.o
if errorlevel 1 goto fail
clang %UFLAGS% -c user\hello.c -o build\hello.o
if errorlevel 1 goto fail
clang %UFLAGS% -c user\sysinfo.c -o build\sysinfo.o
if errorlevel 1 goto fail
ld.lld -T user\user.ld -nostdlib build\crt0.o build\hello.o -o build\hello.elf
if errorlevel 1 goto fail
ld.lld -T user\user.ld -nostdlib build\crt0.o build\sysinfo.o -o build\sysinfo.elf
clang %UFLAGS% -c user\myprog.c -o build\myprog.o
if errorlevel 1 goto fail
ld.lld -T user\user.ld -nostdlib build\crt0.o build\myprog.o -o build\myprog.elf
if errorlevel 1 goto fail
if errorlevel 1 goto fail

echo [9/9] Done.
echo [SUCCESS] Stage 9 built.
goto end
:fail
echo [FAILED] Check output above.
pause
:end
endlocal
