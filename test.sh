#!/bin/bash

smenu="on"

if [ $# -ge 1 ]; then
    if [ $1 = "menu" ]; then
        smenu="on"
    fi
fi

# Test with system floppy image, empty hard disk, and serial port output
qemu-system-i386 -boot order=ac,menu=$smenu -fda images/os-fd.flp -hda images/hd.img -serial mon:stdio
