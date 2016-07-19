#
# Makefile
#

CC86  = bcc
LD86  = ld86
NASM  = nasm

all: boot/boot.bin kernel.bin programs

CFLAGS  := -0 -O -ansi 				# 8086 target, optimize and ansi C
LDFLAGS := -d -s 					# delete the header and strip symbols
NFLAGS  := -w+orphan-labels -f as86 # generate as86 object file

programs: programs/echo.bin programs/copy.bin programs/edit.bin

programs/echo.bin: programs/echo.c clib/clib.o clib/x86.o clib/entry.o
	@$(CC86) $(CFLAGS) -o programs/echo.o -c programs/echo.c
	@$(LD86) $(LDFLAGS) -T 0xD000 -o $@ clib/entry.o programs/echo.o clib/clib.o clib/x86.o

programs/copy.bin: programs/copy.c clib/clib.o clib/x86.o clib/entry.o
	@$(CC86) $(CFLAGS) -o programs/copy.o -c programs/copy.c
	@$(LD86) $(LDFLAGS) -T 0xD000 -o $@ clib/entry.o programs/copy.o clib/clib.o clib/x86.o

programs/edit.bin: programs/edit.c clib/clib.o clib/x86.o clib/entry.o
	@$(CC86) $(CFLAGS) -o programs/edit.o -c programs/edit.c
	@$(LD86) $(LDFLAGS) -T 0xD000 -o $@ clib/entry.o programs/edit.o clib/clib.o clib/x86.o

boot/boot.bin: boot/boot.s boot/ext2.inc
	@$(NASM) -O0 -i boot/ -w+orphan-labels -f bin -o $@ boot/boot.s

kernel.o: kernel.h kernel.c
	@$(CC86) $(CFLAGS) -o $@ -c kernel.c

utils.o: utils.h utils.c
	@$(CC86) $(CFLAGS) -o $@ -c utils.c

clib/clib.o: clib/clib.h clib/clib.c
	@$(CC86) $(CFLAGS) -o $@ -c clib/clib.c

clib/x86.o:	clib/x86.s
	@$(NASM) $(NFLAGS) -o $@ clib/x86.s

clib/entry.o: clib/entry.s
	@$(NASM) $(NFLAGS) -o $@ clib/entry.s

hw86.o: hw86.s
	@$(NASM) $(NFLAGS) -o $@ hw86.s

filesystem/ext2.o: filesystem/ext2.h filesystem/ext2.c
	@$(CC86) $(CFLAGS) -o $@ -c filesystem/ext2.c

load.o: kernel.h load.s
	@$(NASM) $(NFLAGS) -o $@ load.s

kernel.bin: load.o hw86.o kernel.o utils.o clib/clib.o clib/x86.o filesystem/ext2.o
	@$(LD86) $(LDFLAGS) -o $@ load.o hw86.o kernel.o clib/clib.o \
		clib/x86.o filesystem/ext2.o utils.o

clean:
	@find . -name "*.o" -type f -delete
	@find . -name "*.bin" -type f -delete
