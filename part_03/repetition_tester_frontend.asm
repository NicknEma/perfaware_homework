global write_mov
global write_nop
global write_cmp
global write_dec

section .text

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
