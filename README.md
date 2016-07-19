#NANO S16

NANO S16 is a small operating system for 16-bit x86 architectures which
operates in real mode. Most notable features are:

-It's written in C and NASM.
-Partially based on DOS.
-Implements a subset of the ext2 file system specification.
-Relies on the BIOS functions for hardware handling.

This software is intended to be just a hobby OS. Creator makes no warranty for
the use of its products and assumes no responsibility for any errors which may
appear in this document nor does it make a commitment to update the information
contained herein.

System requirements:
x86 Compatible computer
64Kb of RAM
1.44Mb disk
VGA graphics card

Building
========
The building process is expected to be run in a Linux system.

1. Install required software:
    -bcc, nasm and ld86 to build the OS and apps.
    -dd and mkfs with ext2 support to create images.
    -qemu to test the images in a virtual machine.

2. Get full source code tree (see previous section). In this tree there are the
   following directories:
    -images: output folder for generated images.
    -source: source code.
    -source/boot: boot sector.
    -source/clib: code related to interface with external programs.
    -source/filesystem: native file system support.
    -source/programs: external programs, such as text editor.

3. Build:
    Customize source/Makefile and build.sh scripts. Run build.sh to build
    everything. Images will be generated in the "images" directory.

Testing
=======
After building, customize and run "test.sh" script to test the OS in a virtual
machine.

It's also possible to just run a virtual machine using a previously generated
system image, without the need of building the system.

The system can be tested in real hardware if images are copied to physical
disks.

User manual
===========
If the OS is not yet installed on the target computer, a system disk in a
proper drive will be needed to start it for first time. See later sections for
information about how to install (clone system) once it is running.

Once the computer is turned on, and after boot sequence, the operating system
will automatically start the Command Line Interface (CLI). The prompt will show
the active drive (at start it is the one which has performed the system boot)
and the current path. Possible drive names are:

fd0 - First floppy disk drive
fd1 - Second floppy disk drive
hd0 - First hard disk drive
hd1 - Second hard disk drive

In general, paths can be specified as relative to the current working directory
or as absolute paths. After typing a command, ENTER must be pressed in order to
execute it. There are different types of valid commands:

-A valid drive name will cause the system to set it as active drive.
-The path of a binary file will cause the system to execute it.
-There are also some built-in commands in the CLI. See next subsection.

CLI Built-in commands

CD
Change current working directory. One parameter is expected: the new path.

Example:
cd dir

CLS
Clear the screen.

COPY
Copy files. Two parameters are expected: the path of the file to copy, and the
path where the copy must be created.

Example:
copy rme.txt rmecopy.txt

DEL
Delete a file. One parameter is expected: the path of the file to delete.

Example:
del rme.txt

DIR
View the contents of the current working directory in the file system.

MD
Create a new directory. One parameter is expected: the path of the new
directory.

Example:
md dir

REN
Rename a file. Two parameters are expected: the path of the file to rename, and
the new name.

Example:
ren /dir/rme.txt readme.txt

SYS
Clone the system disk in another disk. The target disk will be able to boot and
will contain a copy of the files in the current system device. Any data in the
target drive will be lost. The target disk must have at least 1.44 MB of space.
One parameter is expected: the target drive name.

Example:
sys hd0

TIME
Show current date and time.

TYPE
Display the contents of a file. The path of the file to display is expected as
only parameter.

Example:
type rme.txt

XCOPY
Copy files in different drives. Four parameters are expected: drive name where
the file to copy is located, path of the file to copy inside this drive, drive
name where the copy must be created, and the path inside this drive where the
copy must be created.

Example:
xcopy fd0 /rme.txt hd0 /readme.txt






See readme.txt for more information.
