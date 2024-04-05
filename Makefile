#########################
# Makefile for kOS #
#########################

# Entry point of kOS
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
ENTRYPOINT	= 0x1000

FD		= a.img
HD		= 80m.img
SUBDIR		= ./app

# Programs, flags, etc.
ASM		= nasm
DASM		= objdump
CC		= gcc
LD		= ld
ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -I include/sys/ -f elf
CFLAGS		= -I include/ -I include/sys/ -c -fno-builtin -Wall -m32 -fno-stack-protector
#CFLAGS		= -I include/ -c -fno-builtin -fno-stack-protector -fpack-struct -Wall -m32 -fno-stack-protector
LDFLAGS		= -Ttext $(ENTRYPOINT) -Map $(KERNELMAP) -m elf_i386
DASMFLAGS	= -D
ARFLAGS		= rcs

# This Program
BOOT		= boot/fdboot.bin boot/hdboot.bin boot/fdloader.bin boot/hdloader.bin
KERNEL		= kernel.bin
KERNELMAP   	= kernel.map
LIB		= lib/lib_crt.a

OBJS		= kernel/kernel.o\
			kernel/global.o kernel/descriptor.o kernel/process.o kernel/debug.o\
			kernel/kliba.o kernel/klib.o kernel/ipc.o\
			device/8259.o device/timer.o device/keyboard.o device/console.o device/hd.o\
			lib/fs/syslog.o\
			task/pm/fork.o task/pm/exec.o task/pm/exit.o task/pm/wait.o task/pm/misc.o\
			task/fs/fs.o task/fs/open_close.o task/fs/misc.o task/fs/read_write.o\
		    task/fs/unlink.o task/fs/disklog.o task/fs/stat.o\
			shell/shell.o\
			init/main.o\
			task/task_fs.o task/task_hd.o task/task_pm.o task/task_sys.o task/task_tty.o
LOBJS		=  lib/sys/syscall.o lib/sys/getpid.o lib/sys/ipc.o\
			lib/misc/printf.o lib/misc/vsprintf.o lib/misc/string_a.o lib/misc/string.o\
			lib/misc/assert.o\
			lib/fs/open.o lib/fs/read.o lib/fs/write.o lib/fs/close.o lib/fs/unlink.o\
			lib/fs/lseek.o lib/fs/stat.o\
			lib/pm/fork.o lib/pm/exit.o lib/pm/wait.o lib/pm/exec.o
DASMOUTPUT	= kernel.bin.asm

# All Phony Targets
.PHONY : everything final image clean cleanall disasm all buildimg buildimg-sub $(SUBDIR)

# Default starting position
all : cleanall everything $(SUBDIR)

$(SUBDIR): echo
	# @cd $(SUBDIR) && $(MAKE) all
	$(MAKE) all -C $(SUBDIR)

image : cleanall everything clean buildimg buildimg-sub

# Dir ./app
buildimg-sub: echo
	$(MAKE) -C $(SUBDIR)

echo:
	@echo
	@echo
	@echo Building dir: $(SUBDIR)

bochs : image
	bochs

everything : $(BOOT) $(KERNEL)

clean :
	rm -f $(OBJS) $(LOBJS)

cleanall :
	rm -f $(OBJS) $(LOBJS) $(LIB) $(BOOT) $(KERNEL) $(KERNELMAP)

disasm :
	$(DASM) $(DASMFLAGS) $(KERNEL) > $(DASMOUTPUT)

buildimg :
	dd if=boot/fdboot.bin of=$(FD) bs=512 count=1 conv=notrunc
	dd if=boot/hdboot.bin of=$(HD) bs=1 count=446 conv=notrunc
	dd if=boot/hdboot.bin of=$(HD) seek=510 skip=510 bs=1 count=2 conv=notrunc
#	dd if=boot/hdboot.bin of=$(HD) seek=`echo "obase=10;ibase=16;\`egrep -e '^ROOT_BASE' boot/include/load.inc | sed -e 's/.*0x//g'\`*200" | bc` bs=1 count=446 conv=notrunc
#	dd if=boot/hdboot.bin of=$(HD) seek=`echo "obase=10;ibase=16;\`egrep -e '^ROOT_BASE' boot/include/load.inc | sed -e 's/.*0x//g'\`*200+1FE" | bc` skip=510 bs=1 count=2 conv=notrunc
	sudo mount -o loop $(FD) /mnt/floppy/
	sudo cp -fv boot/fdloader.bin /mnt/floppy/
	sudo cp -fv kernel.bin /mnt/floppy
	sudo umount /mnt/floppy

boot/fdboot.bin : boot/fdboot.asm boot/include/bootloader.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/hdboot.bin : boot/hdboot.asm boot/include/bootloader.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/fdloader.bin : boot/fdloader.asm boot/include/bootloader.inc boot/include/descriptor.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/hdloader.bin : boot/hdloader.asm boot/include/bootloader.inc boot/include/descriptor.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

$(KERNEL) : $(OBJS) $(LIB)
	$(LD) $(LDFLAGS) -o $(KERNEL) $^

$(LIB) : $(LOBJS)
	$(AR) $(ARFLAGS) $@ $^

kernel/kernel.o : kernel/kernel.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

init/main.o: init/main.c
	$(CC) $(CFLAGS) -o $@ $<

device/timer.o: device/timer.c
	$(CC) $(CFLAGS) -o $@ $<

device/keyboard.o: device/keyboard.c
	$(CC) $(CFLAGS) -o $@ $<

device/console.o: device/console.c
	$(CC) $(CFLAGS) -o $@ $<

device/8259.o: device/8259.c
	$(CC) $(CFLAGS) -o $@ $<

device/hd.o: device/hd.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/global.o: kernel/global.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/descriptor.o: kernel/descriptor.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/process.o: kernel/process.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/kliba.o : kernel/kliba.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/ipc.o: kernel/ipc.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/debug.o: kernel/debug.c
	$(CC) $(CFLAGS) -o $@ $<

kernel/klib.o: kernel/klib.c
	$(CC) $(CFLAGS) -o $@ $<

lib/misc/printf.o: lib/misc/printf.c
	$(CC) $(CFLAGS) -o $@ $<

lib/misc/vsprintf.o: lib/misc/vsprintf.c
	$(CC) $(CFLAGS) -o $@ $<

lib/misc/string.o: lib/misc/string.c
	$(CC) $(CFLAGS) -o $@ $<

lib/misc/assert.o: lib/misc/assert.c
	$(CC) $(CFLAGS) -o $@ $<

lib/misc/string_a.o : lib/misc/string_a.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/fs/open.o: lib/fs/open.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/read.o: lib/fs/read.c
	$(CC) $(CFLAGS) -o $@ $<

lib//fs/write.o: lib/fs/write.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/close.o: lib/fs/close.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/unlink.o: lib/fs/unlink.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/syslog.o: lib/fs/syslog.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/stat.o: lib/fs/stat.c
	$(CC) $(CFLAGS) -o $@ $<

lib/fs/lseek.o: lib/fs/lseek.c
	$(CC) $(CFLAGS) -o $@ $<

lib/sys/getpid.o: lib/sys/getpid.c
	$(CC) $(CFLAGS) -o $@ $<

lib/sys/ipc.o: lib/sys/ipc.c
	$(CC) $(CFLAGS) -o $@ $<

lib/sys/syscall.o : lib/sys/syscall.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/pm/fork.o: lib/pm/fork.c
	$(CC) $(CFLAGS) -o $@ $<

lib/pm/exit.o: lib/pm/exit.c
	$(CC) $(CFLAGS) -o $@ $<

lib/pm/wait.o: lib/pm/wait.c
	$(CC) $(CFLAGS) -o $@ $<

lib/pm/exec.o: lib/pm/exec.c
	$(CC) $(CFLAGS) -o $@ $<

task/pm/fork.o: task/pm/fork.c
	$(CC) $(CFLAGS) -o $@ $<

task/pm/exec.o: task/pm/exec.c
	$(CC) $(CFLAGS) -o $@ $<

task/pm/exit.o: task/pm/exit.c
	$(CC) $(CFLAGS) -o $@ $<

task/pm/wait.o: task/pm/wait.c
	$(CC) $(CFLAGS) -o $@ $<

task/pm/misc.o: task/pm/misc.c
	$(CC) $(CFLAGS) -o $@ $<

task/fs/fs.o: task/fs/fs.c
	$(CC) $(CFLAGS) -o $@ $<

task/fs/open_close.o: task/fs/open_close.c
	$(CC) $(CFLAGS) -o $@ $<

task/fs/read_write.o: task/fs/read_write.c
	$(CC) $(CFLAGS) -o $@ $<

task/fs/unlink.o: task/fs/unlink.c
	$(CC) $(CFLAGS) -o $@ $<

task/fs/disklog.o: task/fs/disklog.c
	$(CC) $(CFLAGS) -o $@ $<

task/fs/stat.o: task/fs/stat.c
	$(CC) $(CFLAGS) -o $@ $<

shell/shell.o: shell/shell.c
	$(CC) $(CFLAGS) -o $@ $<

task/task_fs.o: task/task_fs.c
	$(CC) $(CFLAGS) -o $@ $<

task/task_hd.o: task/task_hd.c
	$(CC) $(CFLAGS) -o $@ $<

task/task_pm.o: task/task_pm.c
	$(CC) $(CFLAGS) -o $@ $<

task/task_sys.o: task/task_sys.c
	$(CC) $(CFLAGS) -o $@ $<

task/task_tty.o: task/task_tty.c
	$(CC) $(CFLAGS) -o $@ $<
