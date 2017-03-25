# NANO S16

NANO S16 is a small operating system for 16-bit x86 architectures which operates in real mode. Major functions of this operating system include:
* Memory and system resources management
* User programs execution management, providing them system services
* Implements a basic user interface and application programmer interface

System requirements:
* x86 compatible computer
* 128Kb of RAM (1MB recommended)
* 1.44Mb disk
* VGA graphics card

Realtek 8029AS network card is supported.
PS2 mice are supported.

Developer notes:
This software is a hobby operating system. Creator makes no warranty for the use of its products and assumes no responsibility for any errors which may appear in this document nor does it make a commitment to update the information contained herein.

## System description
This operating system is composed of several components, detailed in this section.

### The kernel
The kernel is the core of the operating system. It connects the application software to the hardware of a computer. The kernel manages memory access for user programs, determines how programs get access to hardware resources, and organizes the data for long-term non-volatile storage with file systems on disks.

This operating system implements a monolithic kernel written in C and NASM, which includes all of its services itself. This reduces the amount of messaging and synchronization involved.

#### Program execution
The operating system provides an interface between an application program and the computer hardware, so that an application program can interact with the hardware by obeying rules and procedures programmed into the operating system. The operating system provides also a set of services which simplify development and execution of application programs. Executing an application program involves the creation of a process by the operating system kernel which assigns memory space and other resources, loads program binary code into memory, and initiates execution of the application program which then interacts with the user and with hardware devices.

This operating system implements a monotasking task model. Monotasking systems, also referred to as single-tasking systems, are operating systems in which only one thread of execution is run at a given time. When an application is executed, it takes control of the whole computer, save for the 'resident' part of the operating system which handles system calls and hardware interrupts.

#### Operation Mode
Operating systems determine CPU operation mode. Modern CPUs support multiple modes. Real Mode is a simplistic 16-bit mode that is present on all x86 processors. It was the first x86 mode design and was used by many early operating systems before the birth of Protected Mode. For compatibility purposes, all x86 processors begin execution in Real Mode, emulating an Intel 8088 microprocessor. Real Mode allows unrestricted access to hardware.

This operating system operates kernel and applications in Real Mode.

#### Memory management
An operating system kernel is responsible for managing the system memory. This ensures that a program does not interfere with memory already in use. In Real Mode, there is a little over 1 MB of "addressable" memory (if A20 line is enabled), which can be accessed using segmentation. There are 16 segments of 64KB each available.

This operating system enables A20 line (if available) and uses a mixed memory model: one segment for kernel (code, stack and data), one segment for user programs (code and stack) and multiple user programs data segments.

Memory map
* 0x00000000-0x000003FF	(1KB)   - Interrupt Vector Table
* 0x00000400-0x000004FF	(256B)  - BIOS Data Area
* 0x00007C00-0x00007FFF	(1KB)   - Boot data
* 0x00008000-0x00017FFF	(64KB)  - Kernel segment (code, stack and data)
* 0x00018000-0x00027FFF (64KB)  - User programs code and stack segment
* 0x00028000-0x0009FC00 (490KB) - User programs data
* 0x0009FC00-0x0009FFFF	(1KB)   - Extended BIOS Data Area
* 0x000A0000-0x000FFFFF (384KB) - Video memory, ROM Area

Inside the kernel mapping area there is a dedicated buffer for performing disk operations, and another for kernel heap memory allocation.

#### Disk access and file systems
File systems allow users and programs to organize and sort files on a computer. Computers usually store data on disks using files. The specific way in which files are stored on a disk is called a file system, and enables files to have names and attributes.

With an appropriate device driver, the kernel can then access the contents of the disk in a connected storage device in raw format. A file system driver is used to translate this raw data into a set of structures that the operating system can understand. Programs can then deal with these file systems on the basis of filenames.

This operating system supports hard disks and removable disks, and implements a custom file system (NSFS). The NSFS file system is simple, offers good performance and allows light-weight implementations.

NSFS divides disk space into logical blocks of contiguous space, following this layout:

[boot block | super block | entries table | data blocks]
* Boot block (block 0): Boot sector
* Super block (block 1): Contains information about the layout of the file system
* Entries table (blocks 2-n): Table of file and directory entries
* Data blocks (blocks n-end): Data blocks referenced by file entries

### User Interface
Every computer that is to be operated by a human requires a user interface. One of the most common forms of a user interface is the command-line interface (CLI), where computer commands are typed out line-by-line.

This operating system implements a CLI model, in which each command is typed out using the keyboard after the 'prompt', and then its output appears below, in a compatible display device. The User Manual section contains a more detailed description of the CLI.

### Boot process
When a computer is switched on or reset, it runs through a series of diagnostics called POST (Power-On Self-Test). This sequence culminates in locating a bootable device, such as a removable disk or a hard disk in the order that the firmware is configured to. A device is detected as bootable if it carries a boot sector (512 bytes) with the byte sequence 0x55, 0xAA in bytes 511 and 512 respectively. When the BIOS finds such a boot sector, it loads it into memory at 0x0000:0x7C00 (segment 0, offset 0x7C00). Execution is then transferred to this address.

The NANO system disk contains a bootloader in this sector. This bootloader scans then the boot disk and loads the kernel file at memory location 0x0800:0x0000. Once the bootloader has loaded the kernel, it jumps to this memory location to begin executing it.

The kernel execution starts, sets up needed drivers and initializes the Command Line Interface.

### Executable format
NANO executable file format does not contain metadata or sections. It only contains non-relocatable raw 16-bit x86 machine code assumed to be loaded at 0x1800:0x0000 address. Execution starts also at this address through a special far call instruction. The CLI requires a file name to contain the `.bin` suffix before starting the execution of such file. This is the only safety check performed in order to differentiate executable files.

## Building
The building process is expected to be executed in a Linux system. In Windows 10 or greater it can be built using Windows Subsystem for Linux.

1. Install required software:
    * make, gcc, bcc, nasm and ld86 to build the operating system and user programs
    * optionally install qemu x86 emulator to test the images in a virtual machine
    * optionally install exuberant-ctags to generate tags
    * optionally use dd to write disk images on storage devices

2. Get full source code tree. The tree contains the following directories:
    * fstools: disk image generation tool
    * images: output folder for generated disk images
    * source: source code
        * boot: code for the boot sector image
        * ulib: library to develop user programs
        * programs: user programs

3. Build: Customize `Makefile` and `source/Makefile` files. Run `make` from the root directory to build everything. Images will be generated in the `images` directory.

Note: official dev86 is hosted at https://github.com/lkundrak/dev86

## Testing
After building, run `make qemu` (linux) or `qemu.bat` (windows) from the root directory to test the operating system in qemu. Other virtual machines have been successfully tested. To test the system using VirtualBox, create a new `Other/DOS` machine and start it with `images/os_fd.img` as floppy image.

The network system has been only tested in qemu under Windows, using the Tap-windows driver provided [here](https://openvpn.net/index.php/download/community-downloads.html). This virtual device must be renamed to `tap` and bridged to the actual nic in order to make the default `qemu.bat` script work as expected.

If debug mode is activated in the operating system configuration, it will output debug information through the first serial port in real time. This can be useful for system and user application developers. This serial port is configured to work at 2400 bauds, 8 data bits, odd parity and 1 stop bit.

Using the provided qemu scripts, the serial port will be automatically mapped to the process standard input/output. For VirtualBox users, it is possible for example to telnet from putty if COM1 is set to TCP mode without a pipe, and the same port if specified in both programs.

The system can operate real hardware if images are written to physical disks. Writing images to disks to test them in real hardware is dangerous and can cause loss of data or boot damage, among other undesired things, in all involved computers. So, it is not recommended to proceed unless this risk is understood and assumed. To write images to physical disks, `dd` can be used in linux:
```
dd status=noxfer conv=notrunc if=images/os-fd.img of=/dev/sdb status=none
```
Replace `/dev/sdb` with the actual target storage device. Usually `root` privileges will be needed to run this command.

## User manual
### First boot
A system disk is needed to start the operating system for first time. This disk contains special files required for the computer to boot. If the system disk is removable media like a floppy diskette or an USB drive, it must be introduced in the computer before the boot sequence starts (for example, before turning the computer on). Check also the settings in the BIOS to ensure proper boot device configuration.

See later sections for information about how to install (clone system) once it is running.

### Command Line Interface
Once the computer is turned on, and after boot sequence, the operating system automatically starts the Command Line Interface (CLI). The prompt shows an open angle bracket `>` symbol and a blinking cursor (underscore) where a command is expected to be introduced using the keyboard.

A command is usually a short text string constituted of several parts. The first part, is the command name (just a word), which allows the system to identify which command must be executed. Sometimes, after the name, one or more parameters must be provided, depending on the specific syntax of each command. For example, `copy doc.txt doc-new.txt` is a command made of three parts: the name (`copy`) and two parameters (`doc.txt` and `doc-new.txt`), whose meaning is specific to the copy command syntax. System commands syntax is discussed in a later section.

There are two different types of valid commands:
* The path of an executable file (`.bin`) will cause the system to execute it
* There are also some built-in commands in the CLI (see later section).

The `.bin` suffix can be omitted for executable files. For example, to run `edit.bin`, it's enough to write `edit`. Always, after typing a command, `ENTER` must be pressed in order to execute it.

### File system and paths
Information is stored on disks using a files and directories analogy, called file system. Directories perform a classifying function, and can contain other directories and files. Files are the actual data holders, and can only contain information, such as text, images or even executable code. A disk contains at least a top level directory, usually referred to as root directory. This term comes from an analogy where a file system is a tree, the directories are its branches and the files are its leaves.

Since most commands perform files or directories operations, a way to identify which files or directories to operate is needed. This is achieved through a specific type of structured text strings that univocally refer a file or directory, named paths. Paths are usually used as command parameters.

Paths can be specified as absolute paths or relative to the system disk. When specified as absolute, they must begin with a disk identifier. Possible disk identifiers are:
* fd0 - First floppy disk
* fd1 - Second floppy disk
* hd0 - First hard disk
* hd1 - Second hard disk

After the optional disk identifier, paths are formed of a sequence of components. Each component, represents a branch in the tree (a directory name), describing the full path from the root to a given branch or leave. Path components are separated with slashes `/`. The root directory of a disk can be omitted or referred as `.`.

Examples of valid paths:
```
fd0
hd0/.
hd0/documents/file.txt
docs/other/readme.txt
```

Note that all files and directories have their own name. This name must not contain spaces or slashes, and must be unique (differentiation is case sensitive) inside its parent directory. The fact that two files can be identically named if they are located in different directories, explains the need of using paths to absolutely identify them.

When booting the operating system from a flash drive, the BIOS emulates instead a floppy disk or a hard disk, so the flash drive can still be accessed using one of the previous identifiers.

### CLI Built-in commands

The CLI provides some built-in commands, detailed in this section. When executed with wrong syntax, a syntax reminder is printed to screen.

#### CLONE
Clone the system disk in another disk. The target disk, after being formatted, will be able to boot and will contain a copy of all the files in the current system disk. Any previously existing data in the target disk will be lost. One parameter is expected: the target disk identifier.

Example:
```
clone hd0
```

#### CLS
Clear the screen.

#### CONFIG
Show, set or save configuration parameter values. To show current values, call it without arguments. To set values, two parameters are expected: the parameter name to set, and its value. To save configuration to file, use `save` parameter. Saved configuration will be automatically read when the system starts.

Example:
```
config
config save
config debug enabled
config graphics disabled
```

#### COPY
Copy files. Two parameters are expected: the path of the file to copy, and the path of the new copy.

Example:
```
copy doc.txt doc-copy.txt
```

#### DELETE
Delete a file or a directory. One parameter is expected: the path of the file or directory to delete.

Example:
```
delete doc.txt
```

#### HELP
Show basic help.

#### INFO
Show system version and hardware information.

#### LIST
List the contents of a directory. One parameter is expected: the path of the directory to list. If this parameter is omitted, the contents of the system disk root directory will be listed.

Example:
```
list fd0/documents
```

#### MAKEDIR
Create a new directory. One parameter is expected: the path of the new directory.

Example:
```
makedir documents/newdir
```

#### MOVE
Move files. Two parameters are expected: the current path of the file to move, and its new path.

Example:
```
move fd0/doc.txt hd0/documents/doc.txt
```

#### READ
Display the contents of a file. The path of the file to display is expected as only parameter. Optionally, if `hex`  is passed as first parameter, contents will be dumped in hexadecimal instead of ASCII.

Example:
```
read documents/doc.txt
read hex sample.bin
```

#### SHUTDOWN
If called without arguments, shutdowns the computer or halts it if APM is not supported. If called with `reboot` argument, restarts the computer.

Example:
```
shutdown reboot
```

#### TIME
Show current date and time.

### System configuration
The following parameters can be configured using the `config` command interface:

* `debug`: Enable/disable debug information output through the serial port
* `graphics`: Enable/disable graphics mode
* `net_IP`: Specify host network IP
* `net_gate`: Specify network gateway

## User programs cross development

The easiest way to cross develop new user programs is:

1. Setup the development system so it contains a copy of the full source tree and it's able to build and test it. See the building and testing pages.

2. Create a new `programname.c` file in `source/programs` folder with this code:
    ```
    /*
     * User program: programname
     */

     #include "types.h"
     #include "ulib/ulib.h"

     uint main(uint argc, uchar* argv[])
     {
       return 0;
     }
     ```
3. Edit this line of `source/Makefile` and add `$(PROGDIR)programname.bin` at the end, like this:

    ```
    programs: $(PROGDIR)edit.bin $(PROGDIR)programname.bin
    ```
4. Edit this line of `Makefile` and add `$(SOURCEDIR)programs/programname.bin` at the end, like this:

    ```
    USERFILES := $(SOURCEDIR)programs/edit.bin $(SOURCEDIR)programs/programname.bin
    ```
5. Add code to `programname.c`. See all available system functions in `ulib/ulib.h`
6. Build and test
