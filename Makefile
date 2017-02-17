IMAGEDIR := images/
SOURCEDIR := source/
FSTOOLSDIR := fstools/

MKSRCVARS := IMGDIR=$(IMAGEDIR)

all: $(FSTOOLSDIR)mkfs
	$(MAKE) $@ -C $(SOURCEDIR) --no-print-directory $(MKSRCVARS)
	mkdir -p $(IMAGEDIR)
	$(FSTOOLSDIR)mkfs $(IMAGEDIR)os-fd.img 2880 $(SOURCEDIR)kernel.n16 $(SOURCEDIR)programs/edit.bin
	dd status=noxfer conv=notrunc if=$(SOURCEDIR)boot/boot.bin of=$(IMAGEDIR)os-fd.img status=none
	$(FSTOOLSDIR)mkfs $(IMAGEDIR)os-hd.img 28800 $(SOURCEDIR)kernel.n16 $(SOURCEDIR)programs/edit.bin
	dd status=noxfer conv=notrunc if=$(SOURCEDIR)boot/boot.bin of=$(IMAGEDIR)os-hd.img status=none

$(FSTOOLSDIR)mkfs: $(FSTOOLSDIR)mkfs.c $(SOURCEDIR)fs.h
	gcc -Werror -Wall -I$(SOURCEDIR) -o $(FSTOOLSDIR)mkfs $(FSTOOLSDIR)mkfs.c

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

clean:
	rm -f $(FSTOOLSDIR)mkfs
	rm -f tags
	rm -f $(IMAGEDIR)os-fd.img $(IMAGEDIR)os-hd.img
	$(MAKE) $@ -C $(SOURCEDIR) --no-print-directory $(MKSRCVARS)

.PHONY: all ctags qemu clean
