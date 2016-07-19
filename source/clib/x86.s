;
; x86 Utilities for clib
;

[BITS 16]

; -------------------------------------------------------------------------------------

INT_CODE equ 070h

;
; void clib_syscall(int s, void* p, int* r);
; Generate OS interrupt with parameter
;
global _clib_syscall
_clib_syscall:
    push dx
    push cx
    push bx
    push ax

    mov bx, sp        ; Save the stack pointer
    mov ax, [bx+10]   ; Get parameters
    mov cx, [bx+12]
    mov dx, [bx+14]
    int INT_CODE      ; Call it

    pop ax
    mov bx, dx
    mov ax, [bx]
    pop bx
    pop cx
    pop dx

    ret


;
; int mod(int D, int d);
; Get division modulus
;
global _mod
_mod:
    push dx
    push cx
    push bx

    mov bx, sp        ; Save the stack pointer
    mov dx, 0
    mov ax, [bx+8]    ; Get parameters
    mov cx, [bx+10]
    div word cx
    mov ax, dx

    pop bx
    pop cx
    pop dx

    ret
