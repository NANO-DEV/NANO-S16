# Makefile

INCDIR  := ./
BOOTDIR := boot/
PROGDIR := programs/
ULIBDIR := ulib/

CC86  = bcc
LD86  = ld86
NASM  = nasm

CFLAGS  := -0 -O -ansi -I$(INCDIR) # 8086 target, optimize and ansi C
LDFLAGS := -d -s # delete the header and strip symbols
NFLAGS  := -w+orphan-labels -f as86 # generate as86 object file

all: $(BOOTDIR)boot.bin kernel.n16 programs net

programs: $(PROGDIR)edit.bin $(PROGDIR)nas.bin $(PROGDIR)unet.bin

$(PROGDIR)%.bin: $(PROGDIR)%.c $(ULIBDIR)ulib.o $(ULIBDIR)x86.o
	$(CC86) $(CFLAGS) -o $(PROGDIR)$*.o -c $(PROGDIR)$*.c
	$(LD86) $(LDFLAGS) -T 0x0000 -o $@ $(ULIBDIR)x86.o $(PROGDIR)$*.o $(ULIBDIR)ulib.o

$(BOOTDIR)boot.bin: $(BOOTDIR)boot.s
	$(NASM) -O0 -w+orphan-labels -f bin -o $@ $(BOOTDIR)boot.s

kernel.n16: load.o hw86.o kernel.o cli.o $(ULIBDIR)ulib.o $(ULIBDIR)x86.o fs.o video.o net.o pci.o
	$(LD86) $(LDFLAGS) -o $@ load.o hw86.o kernel.o cli.o $(ULIBDIR)ulib.o $(ULIBDIR)x86.o fs.o video.o net.o pci.o

load.o: load.s
	$(NASM) $(NFLAGS) -o $@ load.s

hw86.o: hw86.s
	$(NASM) $(NFLAGS) -o $@ hw86.s

$(ULIBDIR)x86.o: $(ULIBDIR)x86.s
	$(NASM) $(NFLAGS) -o $@ ulib/x86.s

$(ULIBDIR)ulib.o: $(ULIBDIR)ulib.h $(ULIBDIR)ulib.c types.h syscall.h
	$(CC86) $(CFLAGS) -o $@ -c $(ULIBDIR)ulib.c

video.o: video.c video.h types.h
	$(CC86) $(CFLAGS) -o $@ -c video.c

cli.o: cli.c cli.h kernel.h types.h hw86.h syscall.h $(ULIBDIR)ulib.h fs.h
	$(CC86) $(CFLAGS) -o $@ -c cli.c

kernel.o: kernel.h kernel.c types.h hw86.h syscall.h $(ULIBDIR)ulib.h fs.h cli.h
	$(CC86) $(CFLAGS) -o $@ -c kernel.c

fs.o: fs.h fs.c types.h kernel.h $(ULIBDIR)ulib.h
	$(CC86) $(CFLAGS) -o $@ -c fs.c

net.o: net.h net.c
	$(CC86) $(CFLAGS) -o $@ -c net.c

pci.o: pci.h pci.c
	$(CC86) $(CFLAGS) -o $@ -c pci.c

clean:
	@find . -name "*.o" -type f -delete
	@find . -name "*.bin" -type f -delete
	@find . -name "*.n16" -type f -delete

.PHONY: all programs net clean
