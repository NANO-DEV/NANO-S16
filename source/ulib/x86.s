;
; x86 Utilities for ulib
;

[BITS 16]

;
; bcc compiler uses this func as entry point
;
;
global ___mkargv
___mkargv:
  jmp _main

extern _main


INT_CODE equ 070h

;
; uint syscall(uint s, void* p)
; Generate OS interrupt with parameter
;
global _syscall
_syscall:
  push cx
  push bx
  push ax

  mov  bx, sp           ; Save the stack pointer
  mov  ax, [bx+8]       ; Get parameters
  mov  cx, [bx+10]
  int  INT_CODE         ; Call it

  pop  bx
  pop  bx
  pop  cx

  ret


;
; lptr lp(void* ptr)
; Convert pointer to lptr
;
global _lp
_lp:
  push bx

  mov  edx, 0
  mov  dx, es
  shl  edx, 4

  mov  eax, 0
  mov  bx, sp
  mov  ax, [bx+4]

  add  edx, eax
  mov  ax, dx
  shr  edx, 20

  pop  bx
  ret


;
; BCC Compiler needs these functions
;
;
global idiv_
idiv_:
  cwd
  idiv bx
  ret

global idiv_u
idiv_u:
  xor  dx, dx
  div  bx
  ret

global imod
imod:
  cwd
  idiv bx
  mov  ax, dx
  ret

global imodu
imodu:
  xor	 dx, dx
	div	 bx
	mov	 ax, dx
	ret

global imul_, imul_u
imul_:
imul_u:
 	imul bx
 	ret

global isl, islu
isl:
islu:
 	mov	 cl, bl
 	shl	 ax, cl
 	ret

global isr
isr:
 	mov	 cl, bl
 	sar	 ax, cl
 	ret

global isru
isru:
 	mov	 cl, bl
 	shr	 ax, cl
 	ret

global laddl,	laddul
laddl:
laddul:
	add	 ax, [di]
	adc	 bx, [di+2]
	ret

global landl, landul
landl:
landul:
	and	 ax, [di]
	and	 bx, [di+2]
	ret

global lcmpl, lcmpul
lcmpl:
lcmpul:
	sub	 bx, [di+2]
	je   LCMP_NOT_SURE
	ret
LCMP_NOT_SURE:
	cmp	 ax, [di]
	jb   LCMP_B_AND_LT
	jge	 LCMP_EXIT
	inc	 bx
LCMP_EXIT:
	ret
LCMP_B_AND_LT:
	dec	 bx
	ret

global lcoml, lcomul
lcoml:
lcomul:
	not	 ax
	not	 bx
	ret

global ldecl, ldecul
ldecl:
ldecul:
	cmp	 word [bx], 0
	je	 LDEC_BOTH
	dec	 word [bx]
	ret
LDEC_BOTH:
	dec	 word [bx]
	dec	 word [bx+2]
	ret

global ldivl
ldivl:
	mov	 cx, [di]
	mov	 di, [di+2]
	call ldivmod
	xchg ax, cx
	xchg bx, di
	ret

global ldivul
ldivul:
	mov  cx, [di]
	mov  di, [di+2]
	call ludivmod
	xchg ax, cx
	xchg bx, di
	ret

global leorl, leorul
leorl:
leorul:
	xor	 ax, [di]
	xor	 bx, [di+2]

global lincl, lincul
lincl:
lincul:
	inc	 word [bx]
	je   LINC_HIGH_WORD
	ret
LINC_HIGH_WORD:
	inc  word [bx+2]
	ret

global lmodl
lmodl:
	mov	 cx, [di]
	mov  di, [di+2]
	call ldivmod
	ret

global lmodul
lmodul:
	mov  cx, [di]
	mov	 di, [di+2]
	call ludivmod
	ret

global lmull, lmulul
lmull:
lmulul:
	mov	 cx, ax
	mul	 word [di+2]
	xchg ax, bx
	mul	 word [di]
	add	 bx, ax
	mov	 ax, [di]
	mul	 cx
	add	 bx, dx
	ret

global lnegl, lnegul
lnegl:
lnegul:
	neg	 bx
	neg	 ax
	sbb	 bx, 0
	ret

global lorl, lorul
lorl:
lorul:
	or   ax, [di]
	or   bx, [di+2]
	ret

global lsll, lslul
lsll:
lslul:
	mov	 cx, di
	jcxz LSL_EXIT
	cmp  cx, 32
	jae  LSL_ZERO
LSL_LOOP:
	shl	 ax, 1
	rcl	 bx, 1
	loop LSL_LOOP
LSL_EXIT:
	ret
LSL_ZERO:
	xor  ax, ax
	mov	 bx, ax
	ret

global lsrl
lsrl:
	mov	 cx, di
	jcxz LSR_EXIT
	cmp	 cx, 32
	jae	 LSR_SIGNBIT
LSR_LOOP:
	sar	 bx, 1
	rcr	 ax, 1
	loop LSR_LOOP
LSR_EXIT:
	ret
LSR_SIGNBIT:
	mov	 cx, 32
	jmp	 LSR_LOOP

global lsrul
lsrul:
	mov	 cx, di
	jcxz LSRU_EXIT
	cmp	 cx, 32
	jae	 LSRU_ZERO
LSRU_LOOP:
	shr	 bx, 1
	rcr	 ax, 1
	loop LSRU_LOOP
LSRU_EXIT:
	ret
LSRU_ZERO:
	xor	 ax, ax
	mov	 bx, ax
	ret

global lsubl, lsubul
lsubl:
lsubul:
	sub	 ax, [di]
	sbb	 bx, [di+2]
	ret

global ltstl, ltstul
ltstl:
ltstul:
	test bx, bx
	je   LTST_NOT_SURE
	ret
LTST_NOT_SURE:
	test ax, ax
	js   LTST_FIX_SIGN
	ret
LTST_FIX_SIGN:
	inc	 bx
	ret

global ldivmod
ldivmod:
	mov	 dx, di            ; sign byte of b in dh
	mov  dl, bh            ; sign byte of a in dl
	test di, di
	jns	 set_asign
	neg	 di
	neg	 cx
	sbb	 di,0
set_asign:
	test bx, bx
	jns  got_signs         ; leave r = a positive
	neg  bx
	neg	 ax
	sbb	 bx, 0
	jmp	 got_signs

global ludivmod
ludivmod:
  xor  dx, dx           ; both sign bytes 0
got_signs:
	push bp
	push si
	mov  bp, sp
	push di                ; remember b
	push cx


b0	EQU	-4
b16	EQU	-2

	test di, di
	jne  divlarge
	test cx, cx
	je   divzero
	cmp	 bx, cx
	jae	 divlarge          ; would overflow
	xchg dx, bx            ; a in dx:ax, signs in bx
	div	 cx
	xchg cx, ax            ; q in di:cx, junk in ax
	xchg ax, bx            ; signs in ax, junk in bx
	xchg ax, dx            ; r in ax, signs back in dx
	mov	 bx, di            ; r in bx:ax
	jmp	 zdivu1

divzero:                 ; return q = 0 and r = a
	test dl, dl
	jns  return
	jmp  negr              ; a initially minus, restore it

divlarge:
	push dx                ; remember sign bytes
	mov	 si, di            ; w in si:dx, initially b from di:cx
	mov	 dx, cx
	xor	 cx, cx            ; q in di:cx, initially 0
	mov	 di, cx
	; r in bx:ax, initially a
	; use di:cx rather than dx:cx in order to
	; have dx free for a byte pair later
	cmp	 si, bx
	jb   loop1
	ja   zdivu             ; finished if b > r
	cmp  dx, ax
	ja   zdivu

; rotate w (= b) to greatest dyadic multiple of b <= r

loop1:
	shl	 dx, 1             ; w = 2*w
	rcl	 si, 1
	jc   loop1_exit        ; w was > r counting overflow (unsigned)
	cmp	 si, bx            ; while w <= r (unsigned)
	jb   loop1
	ja   loop1_exit
	cmp	 dx, ax
	jbe	 loop1             ; else exit with carry clear for rcr
loop1_exit:
	rcr	 si, 1
	rcr	 dx, 1
loop2:
	shl	 cx, 1             ; q = 2*q
	rcl	 di, 1
	cmp	 si, bx            ; if w <= r
	jb   loop2_over
	ja   loop2_test
	cmp	 dx, ax
	ja   loop2_test
loop2_over:
	add  cx, 1             ; q++
	adc  di, 0
	sub  ax, dx            ; r = r-w
	sbb  bx, si
loop2_test:
	shr	 si, 1             ; w = w/2
	rcr	 dx, 1
	cmp	 si, [bp+b16]      ; while w >= b
	ja   loop2
	jb   zdivu
	cmp	 dx, [bp+b0]
	jae	 loop2

zdivu:
	pop	 dx                ; sign bytes
zdivu1:
	test dh, dh
	js   zbminus
	test dl, dl
	jns  return            ; else a initially minus, b plus
	mov	 dx, ax            ; -a = b * q + r ==> a = b * (-q) + (-r)
	or   dx, bx
	je   negq              ; use if r = 0
	sub	 ax, [bp+b0]       ; use a = b * (-1 - q) + (b - r)
	sbb	 bx, [bp+b16]
	not	 cx                ; q = -1 - q (same as complement)
	not	 di
negr:
	neg	 bx
	neg	 ax
	sbb	 bx, 0
return:
	mov	 sp, bp
	pop	 si
	pop	 bp
	ret
zbminus:
	test dl, dl            ; (-a) = (-b) * q + r ==> a = b * q + (-r)
	js   negr              ; use if initial a was minus
	mov  dx, ax            ; a = (-b) * q + r ==> a = b * (-q) + r
	or   dx, bx
	je   negq              ; use if r = 0
	sub	 ax, [bp+b0]       ; use a = b * (-1 - q) + (b + r) (b is now -b)
	sbb	 bx, [bp+b16]
	not	 cx
	not	 di
	mov	 sp, bp
	pop	 si
	pop	 bp
	ret
negq:
	neg	 di
	neg	 cx
	sbb	 di, 0
	mov	 sp, bp
	pop	 si
	pop	 bp
	ret
