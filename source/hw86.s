;
; Hardware utilities for x86
;

[BITS 16]

;
; void dump_regs()
; Dump register values through debug output
;
global _dump_regs
_dump_regs:
  pusha

  push cs
  push ds
  push es
  push ss

  push .dumpstr
  call _debugstr
  pop  ax

  pop  ax
  pop  ax
  pop  ax
  pop  ax

  popa
  ret

.dumpstr db "REG DUMP:",13,10,"SS=%x ES=%x DS=%x CS=%x",13,10,"DI=%x SI=%x BP=%x SP=%x",13,10,"BX=%x DX=%x CX=%x AX=%x",13,10, 0
extern _debugstr



extern _screen_width_c, _screen_height_c, _screen_width_px, _screen_height_px, _graphics_mode
extern _video_show_cursor, _video_hide_cursor, _video_get_cursor_pos, _video_set_cursor_pos
extern _video_out_char, _video_out_char_attr, _video_clear_screen, _video_window_y, _video_font_h
extern _video_enable, _video_disable

;
; void io_set_text_mode()
; Set the text mode to 80x50 16 colors
; and cursor shape to blinking
;
global _io_set_text_mode
_io_set_text_mode:
  push ax
  push bx

  call _video_disable

  mov  al, 0x03         ; 80x25 text mode
  mov  ah, 0x00
  int  0x10

  mov  ah, 0x05         ; Set active page = 0
  mov  al, 0
  int  0x10

  mov  ax, 0x1112       ; Set 8x8 font
  mov  bl, 0
  int  0x10

  mov  word [_graphics_mode], 0
  mov  word [_screen_width_px], 0
  mov  word [_screen_height_px], 0
  mov  word [_screen_width_c], 80
  mov  word [_screen_height_c], 50

  pop  bx
  pop  ax
  ret

;
; void io_set_graphics_mode()
; Set the graphics mode
;
global _io_set_graphics_mode
_io_set_graphics_mode:
  push ax
  push bx

  mov  ah, 0x12         ; Enable default palette loading
  mov  bl, 0x31
  mov  al, 0x00
  int  0x10

  mov  ax, 0x4F02       ; Set graphics mode (VESA)
  mov  bx, 0x103
  int  0x10

  mov  ah, 0x12         ; Enable memory access
  mov  bl, 0x32
  mov  al, 0
  int  0x10

  mov  ax, 0x1123       ; Set grphics font memory
  mov  bl, 0x00         ; and specify rows
  mov  dl, 75
  int  0x10

  mov  word [_graphics_mode], 1
  mov  word [_screen_width_px], 800
  mov  word [_screen_height_px], 600
  mov  word [_screen_width_c], 100
  mov  word [_screen_height_c], 75
  mov  word [_video_window_y], 0
  mov  word [_video_font_h], 8

  call _video_enable
  call _video_clear_screen

  pop  bx
  pop  ax
  ret


;
; void io_set_vesa_bank(uint bank)
; Set active VESA bank
;
global  _io_set_vesa_bank
_io_set_vesa_bank:
  cli
  push ax
  push bx
  push dx

  mov  ax, 0x4F05
  mov  bx, sp
  mov  edx, [bx+8]
  mov  ebx, 0
  int  0x10

  pop  dx
  pop  bx
  pop  ax
  sti
  ret


;
; lp_t io_get_bios_font(uint* offset)
; Get BIOS font pointer and offset
;
global _io_get_bios_font
_io_get_bios_font:
  push bx
  push es
  push bp

  mov  bx, sp           ; Fill font offset info
  mov  cx, 0x40
  mov  es, cx
  mov  ax, [es:0x85]
  mov  [bx+8], ax

  mov  ax, 0x1130       ; Get the pointer in es:bp
  mov  bh, 0x03
  int  0x10

  mov  ebx, 0           ; Multiply segment by 0x10 and move to ebx
  mov  bx, es
  sal  ebx, 4

  mov  eax, 0           ; Add offset to ebx, so it contains the pointer
  mov  ax, bp
  add  ebx, eax

  mov  ax, bx           ; Return pointer in dx:ax
  shr  ebx, 16
  mov  dx, bx

  pop  bp
  pop  es
  pop  bx
  ret


;
; void io_clear_screen()
; Clear the screen
;
global _io_clear_screen
_io_clear_screen:
  cmp  word [_graphics_mode], 1
  je  _video_clear_screen

  pusha

  mov  dx, 0            ; Position cursor at top-left
  push dx
  push dx
  call _io_set_cursor_pos
  pop  dx
  pop  dx

  mov  ah, 6            ; Scroll full-screen
  mov  al, 0            ; Normal white on black
  mov  bh, 0x07         ; Clear attributes
  mov  cx, 0            ; Top-left
  mov  dh, [_screen_height_c] ; Bottom-right
  sub  dh, 1
  mov  dl, [_screen_width_c]
  sub  dl, 1
  int  10h

  popa
  ret

;
; void io_scroll_screen()
; Scrolls the screen
;
global _io_scroll_screen
_io_scroll_screen:
  pusha

  cmp  word [_graphics_mode], 1
  jne  .scroll_text_mode

  mov  ax, 0x4F07       ; Set display start line
  mov  bl, 0x00
  mov  cx, 0
  mov  dx, [_video_window_y]
  add  dx, [_video_font_h]
  int  0x10
  mov  [_video_window_y], dx

  jmp  .end

.scroll_text_mode:
  mov  ah, 6            ; Scroll screen
  mov  al, 1
  mov  bh, 0x07         ; text clear color
  mov  cx, 0            ; Top-left
  mov  dh, [_screen_height_c] ; Bottom-right
  sub  dh, 1
  mov  dl, [_screen_width_c]
  sub  dl, 1
  int  10h

.end:
  popa
  ret


;
; void io_out_char(uchar c)
; Write a char to display in teletype mode
;
global  _io_out_char
_io_out_char:
  cmp  word [_graphics_mode], 1
  je   _video_out_char

  push bx
  push ax

  mov  bx, sp           ; Save the stack pointer
  mov  al, [bx+6]       ; Get char from string
  cmp  al, 0x09         ; Special '\t' case
  jne  .print
  mov  al, 0x20         ; Replace '\t' with a space
.print:
  mov  bx, 0x07         ; Gray?
  mov  ah, 0x0E         ; int 10h teletype function
  int  0x10             ; Print it

  pop  ax
  pop  bx
  ret


;
; void io_out_char_attr(uint col, uint row, uchar c, uchar attr)
; Write a char to display in specific position
;
global  _io_out_char_attr
_io_out_char_attr:
  cmp  word [_graphics_mode], 1
  je   _video_out_char_attr

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
; void io_hide_cursor()
; Hide the cursor
;
global  _io_hide_cursor
_io_hide_cursor:
  cmp  word [_graphics_mode], 1
  je   _video_hide_cursor

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
; Show the cursor
;
global  _io_show_cursor
_io_show_cursor:
  cmp  word [_graphics_mode], 1
  je   _video_show_cursor

  push ax
  push cx

  mov  ch, 6
  mov  cl, 7
  mov  ah, 1
  mov  al, 3
  int  0x10

  pop  cx
  pop  ax
  ret


;
; void io_get_cursor_position(uint* col, uint* row)
; Get the cursor position
;
global  _io_get_cursor_pos
_io_get_cursor_pos:
  cmp  word [_graphics_mode], 1
  je   _video_get_cursor_pos

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
; void io_set_cursor_position(uint col, uint row)
; Set the cursor position
;
;
global  _io_set_cursor_pos
_io_set_cursor_pos:
  cmp  word [_graphics_mode], 1
  je   _video_set_cursor_pos

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
; Get key press
;
global _io_in_key
_io_in_key:
  mov  ax, 0            ; BIOS call to check if there is a key in buffer
  mov  ah, 0x11
  int  0x16
  jz   .no_key

  mov  ah, 0x10         ; BIOS call to wait for key
  int  0x16             ; Since there is one in buffer, there is no wait
  sti
  ret

.no_key:                ; No key pressed, return 0
  mov  ax, 0
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
  mov  ah, 0x01         ; int 0x14 ah=0x01: write
  int  0x14             ; Send it

  mov byte [_serial_status], ah

.skip:
  pop  dx
  pop  ax
  pop  bx
  ret

extern _serial_status


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
; Read a disk sector
;
global _read_disk_sector
_read_disk_sector:
  pusha

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

  mov  ah, 2            ; Params for int 0x13: read disk sectors
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

.param_failure:
  popa
  mov  ax, 1
  ret

.n dw 0


;
; void turn_off_floppy_motors()
; Since IRQ0 is used for timer, this must be done manually
;
global _turn_off_fd_motors
_turn_off_fd_motors:
  push ax
  push dx

  mov  dx, 0x3F2
  mov  al, 0
  out  dx, al

  pop  dx
  pop  ax
  ret


;
; uint write_disk_sector(uint disk, uint sector, uint n, uchar* buff)
; Write disk sector
;
global _write_disk_sector
_write_disk_sector:
  pusha

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

  mov  ah, 3            ; Params for int 0x13: write disk sectors
  mov  al, [bx+22]      ; Number of sectors to read
  mov  si, [bx+24]      ; Set ES:BX to point the buffer
  mov  bx, ds
  mov  es, bx
  mov  bx, si

  stc                   ; A few BIOSes do not set properly on error
  int  0x13             ; Read sectors

  jc   .write_failure

  popa                  ; And restore from start of this system call
  mov  ax, 0            ; Return 0 (for success)
  ret

.write_failure:
  mov  [.n], ax
  popa
  mov  ax, [.n]         ; Return error code
  ret

.param_failure:
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
; disk_lba_to_hts -- Calculate head, track and sector for int 0x13
; IN: logical sector in AX; OUT: correct registers for int 0x13
;
disk_lba_to_hts:
  push bx
  push ax

  mov  bx, ax           ; Save logical sector

  mov  dx, 0            ; First the sector
  div  word [dsects]    ; Sectors per track
  add  dl, 01           ; Physical sectors start at 1
  mov  cl, dl           ; Sectors belong in CL for int 0x13
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
  .fssize     resd 1
  .sectors    resw 1
  .sides      resw 1
  .cylinders  resw 1
  .disk_size  resd 1
  .last_accss resd 1
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
; uint get_disk_info(uint disk, uint* st, uint* hd, uint* cl)
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
; void outb(uchar value, uint port)
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
; uchar inb(uint port)
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


;
; void outw(uint value, uint port)
; write word to port
;
global _outw
_outw:
  push ax
  push bx
  push dx

	mov	 bx, sp
	mov	 ax, [bx+8]
	mov	 dx, [bx+10]
	out	 dx, ax

  pop  dx
  pop  bx
  pop  ax
	ret


;
; uint inw(uint port)
; Read word from port
;
global _inw
_inw:
  push bx
  push dx

  mov  ax, 0
  mov	 bx, sp
	mov	 dx, [bx+6]
	in	 ax, dx

	pop  dx
  pop  bx
  ret


;
; void outl(ul_t value, uint port)
; write long to port
;
global _outl
_outl:
  push eax
  push bx
  push dx

	mov	 bx, sp
	mov	 eax, [bx+10]
	mov	 dx, [bx+14]
	out	 dx, eax

  pop  dx
  pop  bx
  pop  eax
	ret


;
; ul_t inl(uint port)
; Read long from port
;
global _inl
_inl:
  push bx

  mov  eax, 0
  mov	 bx, sp
	mov	 dx, [bx+4]
	in	 eax, dx
  mov  edx, eax
  shr  edx, 16

  pop  bx
  ret


;
; void apm_shutdown()
;	Power off system using APM
;
global _apm_shutdown
_apm_shutdown:
  push ax
  push bx
  push cx
  clc

  ; Disconnect from any APM interface
  mov  ax, 0x5304
  mov  bx, 0
  int  0x15
  clc

  ; Connect to real mode interface
  mov  ax, 0x5301
  mov  bx, 0
  int  0x15
  jc   .error

  ; Enable power management
  mov  ax, 0x5308
  mov  bx, 0x0001
  mov  cx, 0x0001
  int  0x15
  jc   .error

  ; Power off
  mov  ax, 0x5307
  mov  bx, 0x0001
  mov  cx, 0x0003
  int  0x15

.error:
  clc
  pop  cx
  pop  bx
  pop  ax
  ret


;
; void reboot()
;	Reboot system
;
global _reboot
_reboot:
  push ax
  push dx

  mov  dx, 0x64         ; Use keyboard line
  mov  al, 0xFE
  out  dx, al

  pop dx
  pop ax
  ret


;
; void lmem_setbyte(lp_t addr, uchar b)
; Set far memory byte
;
global _lmem_setbyte
_lmem_setbyte:
  cli
  push es
  push ecx
  push bx
  push ax

  mov  bx, sp
  mov  cx, [bx+14]
  sal  cx, 12
  mov  es, cx
  mov  cx, [bx+12]
  mov  al, [bx+16]
  mov  bx, cx

  mov  [es:bx], al

  pop  ax
  pop  bx
  pop  ecx
  pop  es
  sti
  ret


;
; uchar lmem_getbyte(lp_t addr)
; Get far memory byte
;
global _lmem_getbyte
_lmem_getbyte:
  cli
  push es
  push ecx
  push bx

  mov  bx, sp
  mov  cx, [bx+12]
  sal  cx, 12
  mov  es, cx
  mov  cx, [bx+10]
  mov  bx, cx
  mov  ax, 0

  mov  al, [es:bx]

  pop  bx
  pop  ecx
  pop  es
  sti
  ret


;
; Enter kernel mode
; Replace stack and data segments
;
global _enter_kernel
_enter_kernel:
  pop  dx               ; Save ret address
  push es
  push ds
  pushad
  pushfd
  cli

  mov  bx, KERNSEG      ; Kernel data segment
  mov  es, bx
  mov  ds, bx

  mov  [.retadd], dx

  mov  ax, ss           ; Store current stack
  mov  bx, sp
  cmp  ax, KERNSEG
  je   .nset

  mov  dx, KERNSEG      ; Set the kernel stack
  mov  ss, dx
  mov  dx, [kstack]
  mov  sp, dx

.nset:                  ; Push old stack
  push ax
  push bx

  mov  ax, [.retadd]    ; Return
  push ax
  sti
  cld
  ret

.retadd  dw 0


;
; Leave kernel mode
; Restore stack and data segments
;
global _leave_kernel
_leave_kernel:
  pop  ax
  mov  [.retadd], ax
  cli

  pop  bx               ; Pop old stack
  pop  ax
  mov  sp, bx
  mov  ss, ax

  popfd
  popad                  ; Restore previous to call
  mov  dx, [.retadd]
  pop  ds
  pop  es
  push dx               ; and ret
  ret

.retadd  dw 0

kstack  dw 0            ; Last kernel stack pointer


;
; uint uprog_call(uint argc, uchar* argv[])
; User program far call
;
USERSEG equ 0x1800
KERNSEG equ 0x0800

global _uprog_call
_uprog_call:
  pusha

  ; Get args
  mov  bx, sp
  mov  dx, [bx+18]
  mov  [.arg0], dx
  mov  dx, [bx+20]
  mov  [.arg1], dx

  cli
  mov  ax, sp
  mov  dx, USERSEG
  mov  ss, dx
  mov  dx, 0xFF7F
  mov  sp, dx
  push ax
  mov  [kstack], ax
  sti

  mov  dx, [.arg1]      ; Push args
  push dx
  mov  dx, [.arg0]
  push dx

  mov  dx, USERSEG      ; Set data segment
  mov  es, dx
  mov  ds, dx

  call USERSEG:0x0000   ; Call

  mov  dx, KERNSEG      ; Restore data segment
  mov  es, dx
  mov  ds, dx

  pop  dx               ; Remove pushed args
  pop  dx

  cli
  pop  dx               ; Restore kernel stack
  mov  sp, dx
  mov  dx, KERNSEG
  mov  ss, dx
  sti

  popa

  ret

.arg0 dw 0
.arg1 dw 0


; All PIT related:
; http://wiki.osdev.org/Programmable_Interval_Timer
;
; void timer_init(ul_t freq)
; Init PIT
; freq = desired PIT frequency in Hz
;
global _timer_init
_timer_init:
  pushad
  push es

  mov  eax, 0
  mov  bx, sp
  mov  eax, [bx+32+4]
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
  mov  [_system_timer_freq], eax ; Store the actual frequency for displaying later

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

  ; Add routine to interrupt vector table (IRQ0)
  mov  ax, 0
  mov  es, ax
  mov  dx, IRQ0_handler
  mov  [es:INT_CODE_MPIC_BASE*4], dx
  mov  ax, cs
  mov  [es:INT_CODE_MPIC_BASE*4+2], ax

  ; Set IRQ0 (PIC PIT timer) unmasked
  in   al, PORT_MPIC_DATA ; Get current mask
  and  al, 11111110b     ; Unmask IRQ0
  out  PORT_MPIC_DATA, al

  popfd

  pop  es
  popad

  ret

PIT_reload_value dw 0 ; Current PIT reload value
IRQ0_fractions   dd 0 ; Fractions of 1 ms between IRQs
IRQ0_ms          dd 0 ; Number of whole ms between IRQs
system_timer_fractions dd 0 ; Fractions of 1 ms since timer initialized
extern _system_timer_freq, _system_timer_ms


;
; Handler for the IRQ0
; Used by timer (PIT)
;
IRQ0_handler:
  pushad
  call _enter_kernel

	mov  eax, [IRQ0_fractions]
  mov  ebx, [IRQ0_ms]                ; eax.ebx = amount of time between IRQs
  add  [system_timer_fractions], eax ; Update system timer tick fractions
  adc  [_system_timer_ms], ebx       ; Update system timer tick milli-seconds

  call _kernel_time_tick

  mov  al, PIC_EOI
  out  PORT_MPIC_COMMAND, al         ; Send the EOI to the PIC

  call _leave_kernel
  popad
	iret

  extern _kernel_time_tick


;
; Handler for the IRQ12
; Used by mouse
;
IRQ12_handler:
  pushad
	call _enter_kernel

  call _mouse_handler

  mov  al, PIC_EOI
  out  PORT_SPIC_COMMAND, al         ; Send the EOI to the PIC
  out  PORT_MPIC_COMMAND, al         ; Send the EOI to the PIC

  call _leave_kernel
  popad
	iret

extern _mouse_handler


;
; Handler for the net IRQ
; Used by network
;
IRQNET_handler:
  pushad
	call _enter_kernel

  call _net_handler

  mov  al, PIC_EOI
  out  PORT_SPIC_COMMAND, al         ; Send the EOI to the PIC
  out  PORT_MPIC_COMMAND, al         ; Send the EOI to the PIC

  call _leave_kernel
  popad
	iret

extern _net_handler


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

INT_CODE_MPIC_BASE equ 0x08
INT_CODE_SPIC_BASE equ 0x70

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
; void install_mouse_IRQ_handler()
; Add mouse routine to interrupt vector table (IRQ12)
;
global _install_mouse_IRQ_handler
_install_mouse_IRQ_handler:
  pusha
  push es
  cli

  ; Install handler
  mov  ax, 0
  mov  es, ax
  mov  dx, IRQ12_handler
  mov  [es:(INT_CODE_SPIC_BASE+4)*4], dx
  mov  ax, cs
  mov  [es:(INT_CODE_SPIC_BASE+4)*4+2], ax

  ; Set IRQ12 (mouse) unmasked
  in   al, PORT_SPIC_DATA
  and  al, 11101111b
  out  PORT_SPIC_DATA, al

  ; Set IRQ2 (Slave PIC) unmasked
  in   al, PORT_MPIC_DATA
  and  al, 11111101b
  out  PORT_MPIC_DATA, al

  sti
  pop  es
  popa

  ret


;
; void install_net_IRQ_handler()
; Add routine to interrupt vector table
;
global _install_net_IRQ_handler
_install_net_IRQ_handler:
  pusha
  push es
  cli

  ; Install handler
  mov  bx, [_net_irq]
  sub  bl, 8
  add  bl, INT_CODE_SPIC_BASE
  mov  al, 4
  mul  bl
  mov  bx, ax
  mov  ax, 0
  mov  es, ax
  mov  dx, IRQNET_handler
  mov  [es:bx], dx
  mov  ax, cs
  mov  [es:bx+2], ax

  ; Set IRQ unmasked
  mov  cx, [_net_irq]
  sub  cl, 8
  in   al, PORT_SPIC_DATA
  mov  bl, 1
  shl  bl, cl
  not  bl
  and  al, bl
  out  PORT_SPIC_DATA, al

  sti
  pop  es
  popa

  ret

extern _net_irq ; netword IRQ number, assumed > 8

;
; Install IRS
;
;
INT_CODE_SYSCALL equ 0x80
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

  sti
  ret


;
; SYS_ISR
; Syscall Interrput Service Routine
;
SYS_ISR:
  cli
  push es
  push ds
  pushad

  mov  bx, KERNSEG      ; Kernel data segment
  mov  es, bx
  mov  ds, bx

  ; Get call params from current stack
  mov  cx, ds
  mov  dx, ss
  mov  bx, sp

  mov  ds, dx
  mov  ax, [bx+32+6+12] ; Get param 1 (hi)
  mov  ds, cx
  mov  [.arg3], ax
  mov  ds, dx
  mov  ax, [bx+32+6+10] ; Get param 1 (lo)
  mov  ds, cx
  mov  [.arg2], ax
  mov  ds, dx
  mov  ax, [bx+32+6+8]  ; Get param 0
  mov  ds, cx
  mov  [.arg1], ax
  mov  ds, dx
  mov  ax, [bx+32+6+4]  ; Get cs
  mov  ds, cx
  mov  [.arg0], ax

  ; Store current stack
  mov  ax, ss
  mov  bx, sp
  cmp  ax, KERNSEG
  je   .nset

  ; Set the kernel stack
  mov  dx, KERNSEG
  mov  ss, dx
  mov  dx, [kstack]
  mov  sp, dx

  ; Push old stack
.nset:
  push ax
  push bx

  ; Push args
  mov  ax, [.arg3]
  push ax
  mov  ax, [.arg2]
  push ax
  mov  ax, [.arg1]
  push ax
  mov  ax, [.arg0]
  push ax

  ; Service
  sti
  call _kernel_service  ; Service
  cli

  ; Pop args
  pop  cx
  pop  cx
  pop  cx
  pop  cx

  ; Store result
  mov  [.result], ax

  ; Pop old stack
  pop  bx
  pop  ax
  mov  sp, bx
  mov  ss, ax

  ; Restore previous to call
  popad
  mov  ax, [.result]
  pop  ds
  pop  es

  sti
  iret

.result dw 0            ; Result of ISR
.arg0   dw 0            ; Args
.arg1   dw 0
.arg2   dw 0
.arg3   dw 0
extern _kernel_service
