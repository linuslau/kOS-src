# Makefile for building a simple operating system bootloader and kernel

# Compiler and linker settings
ASM			= nasm
CC			= gcc
LD			= ld

# Compiler and linker flags
ASMFLAGS	= -I ../include/ -f elf
CFLAGS		= -I ../include/ -c -fno-builtin -fno-stack-protector -Wall -m32 -fno-stack-protector
LDFLAGS		= -Ttext 0x1000 -s -m elf_i386

# Library and binary paths
LIB			= ../lib/lib_crt.a
BIN			= echo pwd
BINTAR		= hdloader.bin kernel.bin install.tar
KERNEL		= kernel.bin
HDLOADER	= hdloader.bin

# Path to the target disk image
HD			= ../80m.img

.PHONY : bin clean cleanall all install

# Build and install targets
install: all clean
	cp ../$(KERNEL) .; cp ../boot/$(HDLOADER) .
	tar vcf install.tar $(KERNEL) $(BIN) $(HDLOADER)
	dd if=install.tar of=$(HD) seek=`echo "obase=10;ibase=16;(\`egrep -e '^ROOT_BASE' ../boot/include/bootloader.inc | sed -e 's/.*0x//g'\`+\`egrep -e '#define[[:space:]]*INSTALL_START_SECT' ../include/sys/config.h | sed -e 's/.*0x//g'\`)*200" | bc` bs=1 count=`ls -l install.tar | awk -F " " '{print $$5}'` conv=notrunc
	rm $(HDLOADER) $(KERNEL)

# Binary targets
bin : $(BIN)

# Build all targets
all : cleanall bin

# Clean object files
clean :
	rm -f *.o

# Clean all generated files
cleanall :
	rm -f $(BIN) $(BINTAR) *.o *.map

# Build start.o from assembly source
start.o : start.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

# Build echo.o from C source
echo.o: echo.c ../include/stdint.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

# Link echo binary
echo : echo.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $? -Map $@.map

# Build pwd.o from C source
pwd.o: pwd.c ../include/stdint.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

# Link pwd binary
pwd : pwd.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $? -Map $@.map
