USE16

SECTION .text

global main, _main
main:
_main:
    push bx
    push ax

    mov  bx, sp       ; Save the stack pointer
    mov  ax, [bx+6]   ; Get arg
    mov  [_argv], ax

    pop  ax
    pop  bx

    pusha
    call _entry
    popa
    ret

extern _entry, _argv
