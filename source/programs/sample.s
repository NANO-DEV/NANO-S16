; Sample nas program
; Clear the screen and show "Hello"

ORG 0xE000         ; Origin address

main:
  push bx          ; save reg state

  ; Clear screen syscall
  mov  bx, 0x00    ; second param to syscall: unused
  push bx          ; push param to the stak
  mov  bx, [cls]   ; first param to syscall: service code
  push bx          ; push param to the stak
  call sysca       ; make syscall
  pop  bx          ; Pop prevously pushed params
  pop  bx

  ; Print string
  mov  bx, hstr    ; String address

repeat:            ; loop each char
  mov  ax, [ochar] ; out char service code

  push bx
  push ax          ; push params
  call sysca       ; make syscall
  pop  ax          ; pop params
  pop  ax

  add  bx, 1       ; inc bx so it points next char of str
  mov  cx, 0
  cmp  [bx], cx    ; compare current char and 0
  jne  repeat      ; if not equal, continue loop

  ; Otherwise, we are done
  pop  bx          ; restore reg
  ret              ; Finish

; Syscall function
sysca:
  int 0x70
  ret

; "Hello\n\r" string (ascii)
hstr  db 0x48, 0x65, 0x6C, 0x6C, 0x6F, 10, 13, 0, 0

; syscall codes
cls   dw 0x03    ; clear screen
ochar dw 0x10    ; out char
