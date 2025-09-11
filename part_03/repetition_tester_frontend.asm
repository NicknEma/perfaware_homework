global write_mov
global write_nop
global write_cmp
global write_dec

global nop3x1_loop
global nop1x3_loop
global nop1x9_loop

section .text

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Instruction-level parallelism tests

write_mov:
	xor rax, rax
.loop:
	mov [rdx + rax], al
	inc rax
	cmp rax, rcx
	jb .loop
	ret

write_nop:
	xor rax, rax
.loop:
	db 0x0f, 0x1f, 0x00
	inc rax
	cmp rax, rcx
	jb .loop
	ret

write_cmp:
	xor rax, rax
.loop:
	inc rax
	cmp rax, rcx
	jb .loop
	ret

write_dec:
.loop:
	dec rcx
	jnz .loop
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Instruction decoding limits

; These tests stress the front-end decoder speed by inserting a different number of nops in the
; middle of the loop, forcing it to spend time decoding instructions that we don't need.

nop3x1_loop:
	xor rax, rax
.loop:
	db 0x0f, 0x1f, 0x00
	inc rax
	cmp rax, rcx
	jb .loop
	ret

nop1x3_loop:
	xor rax, rax
.loop:
	nop
	nop
	nop
	inc rax
	cmp rax, rcx
	jb .loop
	ret

nop1x9_loop:
	xor rax, rax
.loop:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	inc rax
	cmp rax, rcx
	jb .loop
	ret
