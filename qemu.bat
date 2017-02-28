"c:\Program Files\qemu\qemu-system-i386" -drive file=images\os-fd.img,if=floppy,media=disk,format=raw -drive file=images\os-hd.img,media=disk,format=raw -boot menu=on -serial mon:stdio -m 1 -vga std 
