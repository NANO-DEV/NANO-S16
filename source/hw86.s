;
; Hardware utilities for x86
;

[BITS 16]

;
; void dump_regs()
;
;
global _dump_regs
_dump_regs:
  pusha

  push cs
  push ds
  push es
  push ss

  push .dumpstr
  call _sputstr
  pop  ax

  pop  ax
  pop  ax
  pop  ax
  pop  ax

  popa
  ret

.dumpstr db "REG DUMP:",13,10,"SS=%x ES=%x DS=%x CS=%x",13,10,"DI=%x SI=%x BP=%x SP=%x",13,10,"BX=%x DX=%x CX=%x AX=%x",13,10, 0
extern _sputstr


;
; void io_set_text_mode()
; Set the text mode to 80x25 16 colors
; and cursor shape to blinking
;
global _io_set_text_mode
_io_set_text_mode:
push ax
push bx

mov  al, 0x03
mov  ah, 0x00
int  0x10

mov  ax, 0x1112       ; Use this to set 80x50 video mode
mov  bl, 0
int  0x10

pop  bx
pop  ax
ret


;
; void io_clear_screen()
; Clears the text screen for text mode
;
global _io_clear_screen
_io_clear_screen:
  pusha

  mov  dx, 0            ; Position cursor at top-left
  mov  bh, 0
  mov  ah, 2
  int  10h              ; BIOS interrupt to move cursor

  mov  ah, 6            ; Scroll full-screen
  mov  al, 0            ; Normal white on black
  mov  bh, 0x07         ; scolor
  mov  cx, 0            ; Top-left
  mov  dh, [_screen_height] ; Bottom-right
  sub  dh, 1
  mov  dl, [_screen_width]
  sub  dl, 1
  int  10h

  popa
  ret

extern _screen_width, _screen_height


;
; void io_out_char(uchar c)
; Write a char to display in teletype mode
; Parameters are passed on the stack
global  _io_out_char
_io_out_char:
  push bx
  push ax

  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+6]       ; Get char from string
  cmp  al, 0x09         ; Special '\t' case
  jne  .print
  mov  al, 0x20         ; Replace '\t' with a space
.print:
  mov  bx, 0x01
  mov  ah, 0x0E         ; int 10h teletype function
  int  0x10             ; Print it

  pop  ax
  pop  bx
  ret


;
; void io_out_char_attr(uint x, uint y, uchar c, uchar color)
; Write a char to display in specific position
;
global  _io_out_char_attr
_io_out_char_attr:
  push ax
  push bx
  push cx
  push dx

  ; Set cursos position
  mov  bx, sp
  mov  dl, [bx+10]
  mov  dh, [bx+12]
  mov  ah, 2
  mov  bh, 0
  int  0x10

  ; Print char
  mov  bx, sp
  mov  al, [bx+14]
  cmp  al, 0x09         ; Special '\t' case
  jne  .print
  mov  al, 0x20         ; Replace '\t' with a space
.print:
  mov  bl, [bx+16]
  mov  ah, 9
  mov  bh, 0
  mov  cx, 1
  int  0x10

  pop  dx
  pop  cx
  pop  bx
  pop  ax

  ret


;
; void io_out_char_serial(uchar c)
; Write a char to serial port
;
global  _io_out_char_serial
_io_out_char_serial:
  push bx
  push ax
  push dx

  test byte [_serial_status], 0x80
  jne  .skip
  mov  dx, 0            ; Port number
  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+8]       ; Get char from string
  mov  ah, 0x01         ; int 14h ah=01h: write
  int  0x14             ; Send it

  mov byte [_serial_status], ah

.skip:
  pop  dx
  pop  ax
  pop  bx
  ret

;
; uchar io_in_char_serial()
; Read a char from serial port
;
global  _io_in_char_serial
_io_in_char_serial:
  push dx
  push ax

  test byte [_serial_status], 0x80
  jne  .skip
  mov  dx, 0            ; Port number
  mov  al, 0
  mov  ah, 0x02         ; int 14h ah=02h: read
  int  0x14             ; Get it

  ;mov byte [_serial_status], ah
  test byte ah, 0x80
  mov  ah, 0
  je   .skip
  mov  ax, 0

.skip:
  pop  dx
  pop  dx
  ret

extern _serial_status

;
; void io_hide_cursor()
; Hide the cursor in text mode using BIOS
;
global  _io_hide_cursor
_io_hide_cursor:
  push ax
  push cx
  mov  ch, 0x20
  mov  ah, 0x01
  int  0x10
  pop  cx
  pop  ax
  ret


;
; void io_show_cursor()
; Show the cursor in text mode using BIOS
;
;
global  _io_show_cursor
_io_show_cursor:
  pusha

  mov  ch, 6
  mov  cl, 7
  mov  ah, 1
  mov  al, 3
  int  0x10

  popa
  ret


;
; void io_get_cursor_position(uint* x, uint* y)
; Get the cursor position
;
;
global  _io_get_cursor_pos
_io_get_cursor_pos:
  push ax
  push bx
  push dx

  mov  ah, 3
  mov  bh, 0
  int  0x10

  mov  bx, sp
  mov  bx, [bx+8]
  mov  byte [bx], dl
  mov  byte [bx+1], 0
  mov  bx, sp
  mov  bx, [bx+10]
  mov  byte [bx], dh
  mov  byte [bx+1], 0

  pop  dx
  pop  bx
  pop  ax
  ret


;
; void io_set_cursor_position(uint x, uint y)
; Set the cursor position
;
;
global  _io_set_cursor_pos
_io_set_cursor_pos:
  push ax
  push bx
  push dx

  mov  bx, sp
  mov  byte dl, [bx+8]
  mov  byte dh, [bx+10]
  mov  ah, 2
  mov  bh, 0
  int  0x10

  pop  dx
  pop  bx
  pop  ax
  ret


;
; uint io_in_key()
;
;
global _io_in_key
_io_in_key:
  mov  ax, 0            ; BIOS call to check if ther is a key in buffer
  mov  ah, 0x11
  int  0x16
  jz   .no_key

  mov  ah, 0x10         ; BIOS call to wait for key
  int  0x16             ; Since there is one in buffer, there is no wait
  ret

.no_key:                ; No key pressed, return 0
  mov  ax, 0
  ret


;
; void halt()
;
;
global _halt
_halt:
  cli
  hlt


;
; void get_time(uchar* BDCtime, uchar* date)
; Get system time
;
global _get_time
_get_time:
  pusha

  mov  ah, 0x02
  int  0x1A
  mov  bx, sp
  mov  ax, [bx+18]
  mov  bx, ax
  mov  [bx], ch
  inc  bx
  mov  [bx], cl
  inc  bx
  mov  [bx], dh

  mov  ah, 0x04
  int  0x1A
  mov  bx, sp
  mov  ax, [bx+20]
  mov  bx, ax
  mov  [bx], cl
  inc  bx
  mov  [bx], dh
  inc  bx
  mov  [bx], dl

  popa
  ret


;
; uint read_disk_sector(uint disk, uint sector, uint n, uchar* buff)
;
;
global _read_disk_sector
_read_disk_sector:
  pusha
  cli

  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+18]
  mov  [tdev], al
  call set_disk_params
  mov  ax, [bx+20]      ; Ax = start logical sector

  cmp  byte [dsects], 0
  je   .param_failure
  cmp  byte [dsides], 0
  je   .param_failure

  call disk_lba_to_hts

  mov  ah, 2            ; Params for int 13h: read disk sectors
  mov  al, [bx+22]      ; Number of sectors to read
  mov  si, [bx+24]      ; Set ES:BX to point the buffer
  mov  bx, ds
  mov  es, bx
  mov  bx, si

  mov  word [.n], 0

  pusha                 ; Prepare to enter loop

.read_loop:
  popa
  pusha

  stc                   ; A few BIOSes do not set properly on error
  int  0x13             ; Read sectors

  jnc  .read_finished

  inc  word [.n]
  cmp  word [.n], 2
  jg   .read_failure

  call disk_reset       ; Reset controller and try again
  jnc  .read_loop       ; Disk reset OK?
  jmp  .read_failure    ; Fatal double error

.read_finished:
  sti
  popa                  ; Restore registers from main loop
  popa                  ; And restore from start of this system call
  mov  ax, 0            ; Return 0 (for success)
  ret

.read_failure:
  sti
  mov  [.n], ax
  popa
  popa
  mov  ax, [.n]         ; Return 1 (for failure)
  ret

.param_failure:
  sti
  popa
  mov  ax, 1
  ret

.n dw 0


;
; void turn_off_floppy_motors()
; Since IRQ0 is overwritten, this must be done manually
;
global _turn_off_floppy_motors
_turn_off_floppy_motors:
  push ax
  push dx

  mov  dx, 3F2h
  mov  al, 0
  out  dx, al

  pop  dx
  pop  ax
  ret


;
; uint write_disk_sector(uint disk, uint sector, uint n, uchar* buff)
;
;
global _write_disk_sector
_write_disk_sector:
  pusha
  cli

  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+18]
  mov  [tdev], al
  call set_disk_params
  mov  ax, [bx+20]      ; Ax = start logical sector

  cmp  byte [dsects], 0
  je   .param_failure
  cmp  byte [dsides], 0
  je   .param_failure

  call disk_lba_to_hts

  mov  ah, 3            ; Params for int 13h: write disk sectors
  mov  al, [bx+22]      ; Number of sectors to read
  mov  si, [bx+24]      ; Set ES:BX to point the buffer
  mov  bx, ds
  mov  es, bx
  mov  bx, si

  stc                   ; A few BIOSes do not set properly on error
  int  0x13             ; Read sectors

  jc   .write_failure

  sti
  popa                  ; And restore from start of this system call
  mov  ax, 0            ; Return 0 (for success)
  ret

.write_failure:
  sti
  mov  [.n], ax
  popa
  mov  ax, [.n]         ; Return error code
  ret

.param_failure:
  sti
  popa
  mov  ax, 1
  ret

.n dw 0

tdev db 0


;
; Reset disk
;
;
disk_reset:
  push ax
  push dx
  mov  ax, 0

  mov  dl, [tdev]

  stc
  int  0x13
  pop  dx
  pop  ax
  ret


;
; disk_lba_to_hts -- Calculate head, track and sector for int 13h
; IN: logical sector in AX; OUT: correct registers for int 13h
;
disk_lba_to_hts:
  push bx
  push ax

  mov  bx, ax           ; Save logical sector

  mov  dx, 0            ; First the sector
  div  word [dsects]    ; Sectors per track
  add  dl, 01           ; Physical sectors start at 1
  mov  cl, dl           ; Sectors belong in CL for int 13h
  mov  ax, bx

  mov  dx, 0            ; Now calculate the head
  div  word [dsects]    ; Sectors per track
  mov  dx, 0
  div  word [dsides]    ; Disk sides
  mov  dh, dl           ; Head/side
  mov  ch, al           ; Track

  pop  ax
  pop  bx

  mov  dl, [tdev]       ; Set disk

  ret


struc DISKINFO
  .id         resw 1
  .name       resb 4
  .fstype     resw 1
  .fssize     resw 2
  .sectors    resw 1
  .sides      resw 1
  .cylinders  resw 1
  .disk_size  resw 2
  .last_accss resw 2
  .size:
endstruc

;
; set_disk_params
; To allow disk read and write
;
set_disk_params:
  pusha

  mov  byte bl, [tdev]
  mov  bh, 0
  push bx
  call _disk_to_index
  pop  bx
  mov  bl, DISKINFO.size
  mul  bl
  mov  bx, [_disk_info + eax + DISKINFO.sectors]
  mov  [dsects], bx
  mov  bx, [_disk_info + eax + DISKINFO.sides]
  mov  [dsides], bx
  mov  ebx, [_system_timer_ms]
  mov  [_disk_info + eax + DISKINFO.last_accss], ebx

  popa
  ret

extern _disk_to_index
extern _disk_info


;
; uint get_disk_info(uint disk, uint* st, uint* hd, , uint* cl)
;
;
global _get_disk_info
_get_disk_info:
  pusha
  cli

  clc
  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+18]
  mov  [tdev], al

  mov  di, 0
  mov  byte dl, [tdev]
  mov  ah, 8            ; Get disk parameters
  int  0x13
  jc   .error           ; Detect possible errors
  cmp  dl, 0
  je   .error
  cmp  ah, 0
  jne  .error

  push cx
  mov  bx, sp           ; Save the stack pointer
  mov  ax, [bx+22]
  and  cx, 0x3F         ; Maximum sector number
  mov  bx, ax
  mov  [bx], cx         ; Sector numbers start at 1
  mov  bx, sp           ; Save the stack pointer
  mov  ax, [bx+24]
  mov  bx, ax
  movzx dx, dh          ; Maximum head number
  add  dx, 1            ; Head numbers start at 0 - add 1 for total
  mov  [bx], dx
  pop  cx
  mov  ax, cx           ; Cylinders
  and  ax, 0xC0
  shl  ax, 2
  and  cx, 0xFF00
  shr  cx, 8
  or   cx, ax
  mov  ax, cx
  inc  ax
  mov  cx, ax
  mov  bx, sp
  mov  ax, [bx+24]
  mov  bx, ax
  mov  [bx], cx

  sti
  popa
  mov  ax, 0
  ret

.error:
  sti
  popa
  mov  ax, 1
  ret


dsects dw 0             ; Current disk sectors per track
dsides dw 0             ; Current disk sides


;
; void lmem_setbyte(lptr addr, uchar b)
; Set far memory byte
;
global _lmem_setbyte
_lmem_setbyte:
  cli
  push es
  push cx
  push bx
  push ax

  mov  bx, sp
  mov  ecx, [bx+12]
  sal  cx, 8
  mov  es, cx
  mov  cx, [bx+10]
  mov  al, [bx+14]
  mov  bx, cx

  mov  [es:bx], al

  pop  ax
  pop  bx
  pop  cx
  pop  es
  sti
  ret


;
; uchar lmem_getbyte(lptr addr)
; Get far memory byte
;
global _lmem_getbyte
_lmem_getbyte:
  cli
  push es
  push cx
  push bx

  mov  bx, sp
  mov  cx, [bx+10]
  sal  cx, 8
  mov  es, cx
  mov  cx, [bx+8]
  mov  bx, cx
  mov  ax, 0

  mov  al, [es:bx]

  pop  bx
  pop  cx
  pop  es
  sti
  ret


;
; void outb(uchar value, uint port);
; write byte to port
;
global _outb
_outb:
  push ax
  push bx
  push dx

	mov	 bx, sp
	mov	 ax, [bx+8]
	mov	 dx, [bx+10]
	out	 dx, al

  pop  dx
  pop  bx
  pop  ax
	ret


;
; uchar inb(uint port);
; Read byte from port
;
global _inb
_inb:
  push bx
  push dx

  mov  ah, 0
  mov	 bx, sp
	mov	 dx, [bx+6]
	in	 al, dx

	pop  dx
  pop  bx
  ret

; All PIT related:
; http://wiki.osdev.org/Programmable_Interval_Timer
;
; void timer_init(uint32_t freq)
; Init PIT
; freq = desired PIT frequency in Hz
;
global _timer_init
_timer_init:
  pushad

  mov  eax, 0
  mov  bx, sp
  mov  ax, [bx+32+2]
  mov  ebx, eax

  ; Check input freq
  mov  eax, 0x10000     ; eax = reload value for slowest possible frequency (65536)
  cmp  ebx, 18          ; If freq is too low, use slowest possible frequency
  jbe  .gotReloadValue

  mov  eax, 1           ; ax = reload value for fastest possible frequency (1)
  cmp  ebx, 1193181     ; If freq is too high, use fastest possible frequency
  jae  .gotReloadValue

  ; Calculate the reload value
  mov  eax, 3579545
  mov  edx, 0           ; edx:eax = 3579545
  div  ebx              ; eax = 3579545 / frequency, edx = remainder
  cmp  edx, 3579545/2   ; Is the remainder more than half?
  jb   .l1              ; no, round down
  inc  eax              ; yes, round up
.l1:
  mov  ebx, 3
  mov  edx, 0           ; edx:eax = 3579545 * 256 / frequency
  div  ebx              ; eax = (3579545 * 256 / 3 * 256) / frequency
  cmp  edx, 3/2         ; Is the remainder more than half?
  jb   .l2              ; no, round down
  inc  eax              ; yes, round up
.l2:

; Store the reload value and calculate the actual frequency
.gotReloadValue:
  push eax              ; Store reload_value for later
  mov  [PIT_reload_value], ax ; Store the reload value for later
  mov  ebx, eax         ; ebx = reload value

  mov  eax, 3579545
  mov  edx, 0           ; edx:eax = 3579545
  div  ebx              ; eax = 3579545 / reload_value, edx = remainder
  cmp  edx, 3579545/2   ; Is the remainder more than half?
  jb   .l3              ; no, round down
  inc  eax              ; yes, round up
.l3:
  mov  ebx, 3
  mov  edx, 0           ; edx:eax = 3579545 / reload_value
  div  ebx              ; eax = (3579545 / 3) / frequency
  cmp  edx, 3/2         ; Is the remainder more than half?
  jb   .l4              ; no, round down
  inc  eax              ; yes, round up
.l4:
  mov  [_IRQ0_frequency], eax ; Store the actual frequency for displaying later

; Calculate the amount of time between IRQs in 32.32 fixed point
;
; Note: The basic formula is:
;           time in ms = reload_value / (3579545 / 3) * 1000
;       This can be rearranged in the follow way:
;           time in ms = reload_value * 3000 / 3579545
;           time in ms = reload_value * 3000 / 3579545 * (2^42)/(2^42)
;           time in ms = reload_value * 3000 * (2^42) / 3579545 / (2^42)
;           time in ms * 2^32 = reload_value * 3000 * (2^42) / 3579545 / (2^42) * (2^32)
;           time in ms * 2^32 = reload_value * 3000 * (2^42) / 3579545 / (2^10)

  pop  ebx              ; ebx = reload_value
  mov  eax, 0xDBB3A062  ; eax = 3000 * (2^42) / 3579545
  mul  ebx              ; edx:eax = reload_value * 3000 * (2^42) / 3579545
  shrd eax, edx, 10
  shr  edx, 10          ; edx:eax = reload_value * 3000 * (2^42) / 3579545 / (2^10)

  mov  [IRQ0_ms], edx   ; Set whole ms between IRQs
  mov  [IRQ0_fractions], eax  ; Set fractions of 1 ms between IRQs

; Program the PIT channel
  pushfd
  cli                   ; Disabled interrupts (just in case)

  mov  al, 00110100b    ; channel 0, lobyte/hibyte, rate generator
  out  0x43, al

  mov  ax, [PIT_reload_value] ; ax = 16 bit reload value
  out  0x40, al         ; Set low byte of PIT reload value
  mov  al, ah           ; ax = high 8 bits of reload value
  out  0x40, al         ; Set high byte of PIT reload value

  popfd

  popad
  ret

PIT_reload_value dw 0 ; Current PIT reload value
IRQ0_fractions   dd 0 ; Fractions of 1 ms between IRQs
IRQ0_ms          dd 0 ; Number of whole ms between IRQs
system_timer_fractions dd 0 ; Fractions of 1 ms since timer initialized
extern _IRQ0_frequency, _system_timer_ms

;
; Handler for the IRQ0
; Used by timer (PIT)
;
IRQ0_handler:
	pushad
  pushfd

  cli
  cld
	mov  eax, [IRQ0_fractions]
  mov  ebx, [IRQ0_ms]                ; eax.ebx = amount of time between IRQs
  add  [system_timer_fractions], eax ; Update system timer tick fractions
  adc  [_system_timer_ms], ebx       ; Update system timer tick milli-seconds

;  mov  ax, cs                        ; Reset segments ¿What happens?
;  mov  ds, ax
;  mov  es, ax
;  mov  ss, ax
  call _kernel_time_tick

  mov  al, PIC_EOI
  out  PORT_MPIC_COMMAND, al         ; Send the EOI to the PIC

  popfd
	popad

	iret

  extern _kernel_time_tick

;
; void apm_shutdown()
;	Power off system using APM
;
global _apm_shutdown
_apm_shutdown:
  mov  ax, 0x5301
  mov  bx, 0
  int  0x15
  jc   .error

  mov  ax, 0x5308
  mov  bx, 0x0001
  mov  cx, 0x0001
  int  0x15
  jc   .error

  mov  ax, 0x5307
  mov  bx, 0x0001
  mov  cx, 0x0003
  int  0x15

.error:
  ret


;
; void PIC_init()
; Initialize PIC
;
PORT_MPIC_COMMAND equ 0x20
PORT_MPIC_DATA equ 0x21
PORT_SPIC_COMMAND equ 0xA0
PORT_SPIC_DATA equ 0xA1

PIC_EOI equ 0x20 ; End-of-interrupt command code

ICW1_ICW4	equ 0x01		; ICW4 (not) needed
ICW1_SINGLE	equ 0x02		; Single (cascade) mode
ICW1_INTERVAL4 equ 0x04		; Call address interval 4 (8)
ICW1_LEVEL equ 0x08		; Level triggered (edge) mode
ICW1_INIT equ 0x10		; Initialization - required!

ICW4_8086 equ 0x01		; 8086/88 (MCS-80/85) mode
ICW4_AUTO equ 0x02		; Auto (normal) EOI
ICW4_BUF_SLAVE equ 0x08		; Buffered mode/slave
ICW4_BUF_MASTER equ 0x0C		; Buffered mode/master
ICW4_SFNM equ 0x10		; Special fully nested (not)

global _PIC_init
_PIC_init:
  pusha
  cli

  ; Save masks
  in   al, PORT_MPIC_DATA   ; a1 = inb(PIC1_DATA);
  push ax
  in   al, PORT_SPIC_DATA   ; a2 = inb(PIC2_DATA);
  push ax

  ; Starts the initialization sequence (in cascade mode)
  mov  al, ICW1_INIT+ICW1_ICW4
  out  PORT_MPIC_COMMAND, al ; outb(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4);
	out  PORT_SPIC_COMMAND, al ; outb(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);

  ; PIC vector offsets
  mov  al, 0x08
	out  PORT_MPIC_DATA, al ; outb(PIC1_DATA, offset1);
  mov  al, 0x70
	out  PORT_SPIC_DATA, al ; outb(PIC2_DATA, offset2);


  ; ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
  mov  al, 4
	out  PORT_MPIC_DATA, al ; outb(PIC1_DATA, 4);
  ; ICW3: tell Slave PIC its cascade identity (0000 0010)
  mov  al, 2
	out  PORT_SPIC_DATA, al ; outb(PIC2_DATA, 2);

  mov  al, ICW4_8086
	out  PORT_MPIC_DATA, al ; outb(PIC1_DATA, ICW4_8086);
	out  PORT_SPIC_DATA, al ; outb(PIC2_DATA, ICW4_8086);

  ; Restore masks
	pop  ax
  out  PORT_SPIC_DATA, al ; outb(PIC2_DATA, a2);
  pop  ax
  out  PORT_MPIC_DATA, al ; outb(PIC1_DATA, a1);

  sti
  popa
  ret

;
; Install IRS
;
;
INT_CODE_SYSCALL equ 0x70
INT_CODE_MPIC_BASE equ 0x08

global _install_ISR
_install_ISR:
  cli                   ; hardware interrupts are now stopped
  mov  ax, 0
  mov  es, ax

  ; add routine to interrupt vector table (SYSCALL)
  mov  dx, SYS_ISR
  mov  [es:INT_CODE_SYSCALL*4], dx
  mov  ax, cs
  mov  [es:INT_CODE_SYSCALL*4+2], ax

  ; add routine to interrupt vector table (IRQ0)
  mov  dx, IRQ0_handler
  mov  [es:INT_CODE_MPIC_BASE*4], dx
  mov  ax, cs
  mov  [es:INT_CODE_MPIC_BASE*4+2], ax

  ; Set IRQ0 (PIC PIT timer) unmasked
  in   al, PORT_MPIC_DATA ; Get current mask
  and  al, 11111110     ; Unmask IRQ0
  out  PORT_MPIC_DATA, al

  sti
  ret


;
; SYS_ISR
; Syscall Interrput Service Routine
;
SYS_ISR:
  pushad
  cld                   ; clear on function entry
  push cx
  push ax
  call _kernel_service
  pop  cx
  pop  cx
  mov  [result], ax
  popad
  mov  ax, [result]

  iret

result dw 0             ; Result of ISR
extern _kernel_service
