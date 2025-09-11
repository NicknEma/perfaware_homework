global write_mov_asm
global write_nop_asm
global write_cmp_asm
global write_dec_asm
global write_skip_nop_asm

section .text

write_mov_asm:
	xor rax, rax
.loop:
	mov [rdx + rax], al
	inc rax
	cmp rax, rcx
	jb .loop
	ret

write_nop_asm:
	xor rax, rax
.loop:
	db 0x0f, 0x1f, 0x00
	inc rax
	cmp rax, rcx
	jb .loop
	ret

write_cmp_asm:
	xor rax, rax
.loop:
	inc rax
	cmp rax, rcx
	jb .loop
	ret

write_dec_asm:
.loop:
	dec rcx
	jnz .loop
	ret

write_skip_nop_asm:
	xor rax, rax
.loop:
	mov r10, [rdx + rax]
	inc rax
	test r10, 1
	jnz .skip
	nop
.skip:
	cmp rax, rcx
	jb .loop
	ret
