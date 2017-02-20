# Makefile

# Directories
IMAGEDIR := images/
SOURCEDIR := source/
FSTOOLSDIR := fstools/

# User files and args for mkfs
USERFILES := $(SOURCEDIR)programs/edit.bin $(SOURCEDIR)programs/hello.bin $(SOURCEDIR)programs/chat.bin
MKFSARGS := $(SOURCEDIR)boot/boot.bin $(SOURCEDIR)kernel.n16 $(USERFILES)

# Make source and create images
all: $(FSTOOLSDIR)mkfs
	$(MAKE) $@ -C $(SOURCEDIR) --no-print-directory
	mkdir -p $(IMAGEDIR)
	$(FSTOOLSDIR)mkfs $(IMAGEDIR)os-fd.img 2880 $(MKFSARGS)
	$(FSTOOLSDIR)mkfs $(IMAGEDIR)os-hd.img 28800 $(MKFSARGS)

# mkfs generates disk images
$(FSTOOLSDIR)mkfs: $(FSTOOLSDIR)mkfs.c $(SOURCEDIR)fs.h
	gcc -Werror -Wall -I$(SOURCEDIR) -o $(FSTOOLSDIR)mkfs $(FSTOOLSDIR)mkfs.c

# Generate tags
ctags:
	ctags -R

# run in emulators
# Specify QEMU path here
QEMU = qemu-system-i386

QEMUOPTS = -drive file=$(IMAGEDIR)os-fd.img,if=floppy,media=disk,format=raw \
	-drive file=$(IMAGEDIR)os-hd.img,media=disk,format=raw \
	-boot menu=on -serial mon:stdio -m 1

qemu: all
	$(QEMU) $(QEMUOPTS)

# Clean
clean:
	rm -f $(FSTOOLSDIR)mkfs
	rm -f tags
	rm -f $(IMAGEDIR)os-fd.img $(IMAGEDIR)os-hd.img
	$(MAKE) $@ -C $(SOURCEDIR) --no-print-directory

.PHONY: all ctags qemu clean
