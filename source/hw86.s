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
; void io_set_text_mode();
; Set the text mode to 80x25 16 colors
; and cursor shape to blinking
;
global _io_set_text_mode
_io_set_text_mode:
  push ax
  mov  al, 0x03
  mov  ah, 0x00
  int  0x10
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
  mov  dh, 24           ; Bottom-right
  mov  dl, 79
  int  10h

  popa
  ret


;
; void io_out_char(uchar c);
; Write a char to display in teletype mode
; Parameters are passed on the stack
global  _io_out_char
_io_out_char:
  push bx
  push ax

  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+6]       ; Get char from string
  mov  ah, 0Eh          ; int 10h teletype function
  int  10h              ; Print it

  pop  ax
  pop  bx
  ret


;
; void io_out_char_attr(uint x, uint y, uchar c, uchar color);
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
  int  10h

  ; Print char
  mov  bx, sp
  mov  al, [bx+14]
  mov  bl, [bx+16]
  mov  ah, 9
  mov  bh, 0
  mov  cx, 1
  int  10h

  pop  dx
  pop  cx
  pop  bx
  pop  ax

  ret


;
; void io_out_char_serial(uchar c);
; Write a char to serial port
;
global  _io_out_char_serial
_io_out_char_serial:
  push bx
  push ax
  push dx

  test byte [_serial_status], 0x80
  jne  .skip
  mov  dx, 0
  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+8]       ; Get char from string
  mov  ah, 01h          ; int 14h n
  int  0x14             ; Print it

  mov byte [_serial_status], ah

.skip:
  pop  dx
  pop  ax
  pop  bx
  ret

extern _serial_status

;
; void io_hide_cursor();
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
; void io_show_cursor();
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
  int  10h

  popa
  ret


;
; void io_get_cursor_position(uint* x, uint* y);
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
  int  10h

  mov  bx, sp
  mov  bx, [bx+8]
  mov  byte [bx], dl
  mov  bx, [bx+10]
  mov  byte [bx], dh

  pop  dx
  pop  bx
  pop  ax
  ret


;
; void io_set_cursor_position(uint x, uint y);
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
  int  10h

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
  mov  ax, 0
  mov  ah, 10h          ; BIOS call to wait for key
  int  16h
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

  mov  ah, 02h
  int  1ah
  mov  bx, sp
  mov  ax, [bx+18]
  mov  bx, ax
  mov  [bx], ch
  inc  bx
  mov  [bx], cl
  inc  bx
  mov  [bx], dh

  mov  ah, 04h
  int  1ah
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

  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+18]
  mov  [tdev], al
  call set_disk_params
  mov  ax, [bx+20]      ; Ax = start logical sector

  call disk_lba_to_hts

  mov  ah, 2            ; Params for int 13h: read disk sectors
  mov  al, [bx+22]      ; Number of sectors to read
  mov  si, [bx+24]      ; Set ES:BX to point the buffer
  mov  bx, ds
  mov  es, bx
  mov  bx, si

  pusha                 ; Prepare to enter loop

.read_loop:
  popa
  pusha

  stc                   ; A few BIOSes do not set properly on error
  int  13h              ; Read sectors

  jnc  .read_finished

  inc  word [.n]
  cmp  word [.n], 2
  jg   .read_failure

  call disk_reset       ; Reset controller and try again
  jnc  .read_loop       ; Disk reset OK?
  jmp  .read_failure    ; Fatal double error

.read_finished:
  popa                  ; Restore registers from main loop
  popa                  ; And restore from start of this system call
  mov  ax, 0            ; Return 0 (for success)
  ret

.read_failure:
  mov  [.n], ax
  popa
  popa
  mov  ax, [.n]         ; Return 1 (for failure)
  ret

.n dw 0


;
; uint write_disk_sector(uint disk, uint sector, uint n, uchar* buff)
;
;
global _write_disk_sector
_write_disk_sector:
  pusha

  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+18]
  mov  [tdev], al
  call set_disk_params
  mov  ax, [bx+20]      ; Ax = start logical sector

  call disk_lba_to_hts

  mov  ah, 3            ; Params for int 13h: write disk sectors
  mov  al, [bx+22]      ; Number of sectors to read
  mov  si, [bx+24]      ; Set ES:BX to point the buffer
  mov  bx, ds
  mov  es, bx
  mov  bx, si

  stc                   ; A few BIOSes do not set properly on error
  int  13h              ; Read sectors

  jc   .write_failure

  popa                  ; And restore from start of this system call
  mov  ax, 0            ; Return 0 (for success)
  ret

.write_failure:
  mov  [.n], ax
  popa
  mov  ax, [.n]         ; Return error code
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
  int  13h
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

  popa
  ret

extern _disk_to_index
extern _disk_info


;
; uint get_disk_info(uint disk, uint* st, uint* hd, , uint* cl);
;
;
global _get_disk_info
_get_disk_info:
  pusha

  clc
  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+18]
  mov  [tdev], al

  mov  di, 0
  mov  byte dl, [tdev]
  mov  ah, 8            ; Get disk parameters
  int  13h
  jc   .error           ; Detect possible errors
  cmp  dl, 0
  je   .error
  cmp  ah, 0
  jne  .error

  push cx
  mov  bx, sp           ; Save the stack pointer
  mov  ax, [bx+22]
  and  cx, 3Fh          ; Maximum sector number
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
  and  ax, 0C0h
  shl  ax, 2
  and  cx, 0FF00h
  shr  cx, 8
  or   cx, ax
  mov  ax, cx
  inc  ax
  mov  cx, ax
  mov  bx, sp
  mov  ax, [bx+24]
  mov  bx, ax
  mov  [bx], cx

  popa
  mov  ax, 0
  ret

.error:
  popa
  mov  ax, 1
  ret


dsects dw 0             ; Current disk sectors per track
dsides dw 0             ; Current disk sides


;
; Install IRS
;
;
INT_CODE equ 0x70
global _install_ISR
_install_ISR:
  cli                   ; hardware interrupts are now stopped
  xor  ax, ax
  mov  es, ax
  mov  bx, [es:INT_CODE*4]

  ; add routine to interrupt vector table
  mov  dx, _ISR
  mov  [es:INT_CODE*4], dx
  mov  ax, cs
  mov  [es:INT_CODE*4+2], ax
  sti

  ret


;
; IRS
; Interrput Service Routine
;
_ISR:
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
