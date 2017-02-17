#NANO S16

NANO S16 is a small operating system for 16-bit x86 architectures which operates in real mode. Most notable features are:
* It's written in C and NASM
* Implements a custom file system (NSFS)
* Relies on the BIOS functions for hardware handling

This software is intended to be just a hobby OS. Creator makes no warranty for the use of its products and assumes no responsibility for any errors which may appear in this document nor does it make a commitment to update the information contained herein.

System requirements:
* x86 compatible computer
* 64Kb of RAM
* 1.44Mb disk
* VGA graphics card

##Building
The building process is expected to be executed in a Linux system. In Windows 10 it can be built using Windows Subsystem for Linux.

1. Install required software:
    * make, gcc, bcc, nasm and ld86 to build the OS and apps
    * dd to create disk images
    * optionally install qemu x86 emulator to test the images in a virtual machine
    * optionally install exuberant-ctags to generate tags

2. Get full source code tree. In this tree there are several directories:
    * fstools: disk image generation tool
    * images: output folder for generated disk images
    * source: source code
        * boot: code for the boot sector image
        * ulib: library to develop user programs
        * programs: user programs

3. Build: Customize `Makefile` and `source/Makefile` files. Run `make` from the root directory to build everything. Images will be generated in the "images" directory.

##Testing
After building, run `make qemu` (linux) or `qemu.bat` (windows) from the root directory to test the OS in a virtual machine.

The system can operate real hardware if images are written to physical disks.

##User manual
A system disk is needed to start the OS for first time. See later sections for information about how to install (clone system) once it is running.

Once the computer is turned on, and after boot sequence, the operating system automatically starts the Command Line Interface (CLI). The prompt will show a `>` symbol and a blinking cursor where a command is expected to be introduced. There are two different types of valid commands:
* The path of an executable file (`.bin`) will cause the system to execute it
* There are also some built-in commands in the CLI. See next subsection

The `.bin` suffix can be omitted for executable files. After typing a command, `ENTER` must be pressed in order to execute it.

Paths can be specified as absolute paths or relative to the system disk. When specified as absolute, they must begin with a disk identifier. Possible disk identifiers are:
* fd0 - First floppy disk
* fd1 - Second floppy disk
* hd0 - First hard disk
* hd1 - Second hard disk

Path components are separated with slashes `/`. The root directory of a disk can be omitted or referred as `.` when it's the last path component.

Examples of valid paths:
```
fd0
hd0/.
hd0/documents/file.txt
```

When booting the operating system from a flash drive, the BIOS emulates instead a floppy disk or a hard disk, so the flash drive can still be accessed using one of the previous identifiers.

###CLI Built-in commands

####CLONE
Clone the system disk in another disk. The target disk, after being formatted, will be able to boot and will contain a copy of the files in the current system disk. Any previously existing data in the target disk will be lost. One parameter is expected: the target disk identifier.

Example:
```
clone hd0
```

####CLS
Clear the screen.

####COPY
Copy files. Two parameters are expected: the path of the file to copy, and the path of the new copy.

Example:
```
copy doc.txt doc-copy.txt
```

####DELETE
Delete a file or a directory. One parameter is expected: the path of the file or directory to delete.

Example:
```
delete doc.txt
```

####HELP
Show basic help.

####INFO
Show system version and hardware information.

####LIST
List the contents of a directory. One parameter is expected: the path of the directory to list.

Example:
```
list fd0/documents
```

####MAKEDIR
Create a new directory. One parameter is expected: the path of the new directory.

Example:
```
makedir documents/newdir
```

####MOVE
Move files. Two parameters are expected: the current path of the file to move, and its new path.

Example:
```
move fd0/doc.txt hd0/documents/doc.txt
```

####READ
Display the contents of a file. The path of the file to display is expected as only parameter.

Example:
```
read documents/doc.txt
```

####TIME
Show current date and time.
