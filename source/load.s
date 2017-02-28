;
; Description: This file contains the kernel startup
;
; The primary boot loader code supplies the following parameters:
; dl = Boot-disk
;

USE16


SECTION .text

global main, _main
main:
_main:
  mov  ax, LOADSEG
  mov  ds, ax                 ; ds = header
  mov  es, ax
  cli                         ; Ignore interrupts while stack is not build
  mov  ss, ax
  mov  ax, kernel_stack_top
  mov  sp, ax                 ; Set sp at the top of all that
  sti                         ; Stack ok now
  cld                         ; Set the direction flag

  ; Copy primary boot parameters to variables
  mov  [_system_disk], dl     ; Boot disk is the system disk

  ; Install ISR
  call _install_ISR

  ; Init serial port
  mov  dx, 0
  mov  ah, 0
  mov  al, 10101011b          ; 2400 baud, 8 data bits, odd parity, 1 stop bit
  int  0x14
  mov  [_serial_status], ah


  ; Enable A20 line
  mov  ax,2401h               ; A20-Gate Activate by BIOS
  int  15h

  cli
  push es                     ; Save segment, disable interrupts

  mov  bx, 0x0000             ; Write byte 0x00 to 0x0000:0x0500
  mov  es, bx
  mov  bx, 0x0500
  mov  byte [es:bx], 0x00

  mov  bx, 0xFFFF             ; Write byte 0xFF to 0xFFFF:0x0510
  mov  es, bx
  mov  bx, 0x0510
  mov  byte [es:bx], 0xFF

  mov  bx, 0x0000             ; If the memory wraps around
  mov  es, bx                 ; these positions will refer to the same byte.
  mov  bx, 0x0500             ; If the byte at 0x0000:0x0510 is now 0xFF
  mov  al, [es:bx]            ; then the memory wrapped around
  cmp  al, 0x00

  pop  es                      ; Restore segment, enable interrupts
  sti
  jne  a20_failed             ; couldn't activate the gate

a20_activated:
  mov  byte [_a20_enabled], 1
  jmp  kernel_call
a20_failed:
  mov  byte [_a20_enabled], 0
kernel_call:
  call _kernel


;
; Imported functions and variables
;
extern _kernel, _system_disk, _serial_status, _a20_enabled, _install_ISR

LOADSEG EQU 0x0800 ; Where this code is loaded

SECTION .bss

kernel_stack:
resb 0x2000                    ; kernel stack
kernel_stack_top:
global _disk_buff
_disk_buff:
resb 512 ; SECTOR_SIZE in kernel.h
