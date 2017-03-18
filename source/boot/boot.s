; Boot Record for BIOS-based PCs

; Code location constants
%define ORG_LOC         0x7C00 ; Initial MBR position in memory
%define STAGE_LOC       0x8000 ; Location of stage
%define BDISK_LOC       0x0660 ; Location to store boot disk in memory

[ORG ORG_LOC]
[BITS 16]
  jmp  0x0000:start

; Start of the bootstrap code
start:
  ; Set up a stack
  mov  ax, 0

  cli                   ; Disable interrupts while changing stack
  mov  ss, ax
  mov  ds, ax
  mov  es, ax
  mov  sp, ORG_LOC
  sti

.setup_data:
  mov  [BDISK_LOC], dl

  mov  ah, 8            ; Get disk parameters
  int  13h
  jc   error
  and  cx, 0x3F         ; Maximum sector number
  mov  [SECTORS], cx    ; Sector numbers start at 1
  movzx dx, dh          ; Maximum head number
  add  dx, 1            ; Head numbers start at 0 - add 1 for total
  mov  [SIDES], dx

  mov  eax, 0
  mov  ax, 1

  mov  bx, (STAGE_LOC>>4)
  mov  es, bx

.read_next:
  push ax
  call disk_lba_to_hts

  mov  bx, [BUFFER]

  mov  ah, 2
  mov  al, 1

  pusha

.read_loop:
  popa
  pusha

  stc                   ; Some BIOSes do not set properly on error
  int  0x13             ; Read sectors

  jnc  .read_finished
  call disk_reset       ; Reset controller and try again
  jnc  .read_loop       ; Disk reset OK?

  popa
  jmp  error            ; Fatal double error

.read_finished:
  popa                  ; Restore registers from main loop
  pop  ax
  cmp  ax, 1
  jne  .inc_loop        ; If it was the super block
  mov  bx, [BUFFER]
  mov  ax, [es:bx+12]
  mov  bx, ax
  add  bx, 127
  mov  [LASTBLOCK], bx
  jmp  .read_next       ; find where the bootable image starts
.inc_loop:
  inc  ax
  mov  bx, [BUFFER]
  add  bx, 512
  mov  [BUFFER], bx
  cmp  ax, [LASTBLOCK]
  jne  .read_next

  ; Jump to stage
  mov  dl, [BDISK_LOC]
  jmp  (STAGE_LOC>>4):0x0000

; Reset disk
disk_reset:
  push ax
  push dx
  mov  ax, 0
  mov  dl, byte [BDISK_LOC]
  stc
  int  0x13
  pop  dx
  pop  ax
  ret

; disk_lba_to_hts -- Calculate head, track and sector for int 0x13
; IN: logical sector in AX; OUT: correct registers for int 0x13
disk_lba_to_hts:
  push bx
  push ax

  mov  bx, ax           ; Save logical sector

  mov  dx, 0            ; First the sector
  div  word [SECTORS]   ; Sectors per track
  add  dl, 0x01         ; Physical sectors start at 1
  mov  cl, dl           ; Sectors belong in CL for int 0x13
  mov  ax, bx

  mov  dx, 0            ; Now calculate the head
  div  word [SECTORS]   ; Sectors per track
  mov  dx, 0
  div  word [SIDES]     ; Disk sides
  mov  dh, dl           ; Head/side
  mov  ch, al           ; Track

  pop  ax
  pop  bx

  mov  dl, [BDISK_LOC]  ; Set disk

  ret

SECTORS   dw 18
SIDES     dw 2
BUFFER    dw 0
LASTBLOCK dw 240

error:
  mov  si, disk_error   ; If not, print error message
  call print_string
  jmp  error

print_string:           ; Output string in SI to screen
  pusha
  mov  ah, 0x0E         ; int 10h teletype function

.repeat:
  lodsb                 ; Get char from string
  cmp  al, 0
  je   .done					  ; If char is zero, end of string
  int  0x10             ; Otherwise, print it
  jmp  short .repeat

.done:
  popa
  ret

  disk_error db "Disk error", 0

; ------------------------------------------------------------------
; END OF BOOT SECTOR

  times 510-($-$$) db 0  ; Pad remainder of boot sector with zeros
  dw 0xAA55              ; Boot signature
