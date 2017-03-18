; Sample nas program
; Clear the screen and show "Hello"

ORG 0x0000         ; Origin address

main:
  push bx          ; save reg state

  ; Clear screen syscall
  mov  bx, 0x00    ; second param to syscall: unused
  push bx
  push bx          ; push param to the stak
  mov  bx, [cls]   ; first param to syscall: service code
  push bx          ; push param to the stak
  call sysca       ; make syscall
  pop  bx          ; Pop prevously pushed params
  pop  bx
  pop  bx

  ; Print string

  ; Long pointer(x) = 0x00018000 + x
  mov  dx, 0x0001  ; Push hi word of long pointer
  push dx          ; so it's in the stack until
                   ; string is printed

  mov  bx, hstr    ; Put initial string address in bx
  mov  ax, [ochar] ; out char service code in ax

repeat:            ; loop for each char
  mov  dx, bx
  add  dx, 0x8000
  push dx
  push ax          ; push params
  call sysca       ; make syscall
  pop  ax          ; pop params
  pop  dx

  add  bx, 1       ; inc bx so it points next char of str
  mov  dx, 0
  cmp  [bx], dx    ; compare current char and 0
  jne  repeat      ; if not equal, continue loop

  ; Otherwise, we are done
  pop  dx          ; pop
  pop  bx          ; restore reg
  retf             ; Finish

; Syscall function
sysca:
  mov  ax, 0x1800  ; push cs
  push ax
  int  0x80
  pop ax           ; pop cs
  ret

; "Hello\n\r" string (ascii)
hstr  db 0x48, 0x65, 0x6C, 0x6C, 0x6F, 10, 13, 0, 0

; syscall codes
cls   dw 0x03    ; clear screen
ochar dw 0x10    ; out char
