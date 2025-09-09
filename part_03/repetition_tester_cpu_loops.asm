global write_mov_asm
global write_nop_asm
global write_cmp_asm
global write_dec_asm
global write_skip_nop_asm

global read_ports_8x1
global read_ports_8x2
global read_ports_8x3
global read_ports_8x4
global read_ports_1x2
global read_ports_2x2
global read_ports_4x2

global write_ports_8x1
global write_ports_8x2
global write_ports_8x3
global write_ports_8x4

global read_bandw_32x2
global read_bandw_64x2
global read_bandw_128x2
global read_bandw_256x2

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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Read port counting

; These loops read from the same memory location over and over, but they're unrolled
; a different amount of times. The speed of each successive loop should increase
; until we've reached the point where no more movs can be scheduled because no
; ports are available.

; NOTE(ema): The count is in rcx, the data pointer in rdx

read_ports_8x1:
	align 64
.loop:
	mov rax, [rdx]
	sub rcx, 1
	jnle .loop
	ret

read_ports_8x2:
	align 64
.loop:
	mov rax, [rdx]
	mov rax, [rdx]
	sub rcx, 2
	jnle .loop
	ret

read_ports_8x3:
	align 64
.loop:
	mov rax, [rdx]
	mov rax, [rdx]
	mov rax, [rdx]
	sub rcx, 3
	jnle .loop
	ret

read_ports_8x4:
	align 64
.loop:
	mov rax, [rdx]
	mov rax, [rdx]
	mov rax, [rdx]
	mov rax, [rdx]
	sub rcx, 4
	jnle .loop
	ret

read_ports_1x2:
	align 64
.loop:
	mov al, [rdx]
	mov al, [rdx]
	sub rcx, 1
	jnle .loop
	ret

read_ports_2x2:
	align 64
.loop:
	mov ax, [rdx]
	mov ax, [rdx]
	sub rcx, 1
	jnle .loop
	ret

read_ports_4x2:
	align 64
.loop:
	mov eax, [rdx]
	mov eax, [rdx]
	sub rcx, 1
	jnle .loop
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Write port counting

write_ports_8x1:
	align 64
.loop:
	mov [rdx], rax
	sub rcx, 1
	jnle .loop
	ret

write_ports_8x2:
	align 64
.loop:
	mov [rdx], rax
	mov [rdx], rax
	sub rcx, 2
	jnle .loop
	ret

write_ports_8x3:
	align 64
.loop:
	mov [rdx], rax
	mov [rdx], rax
	mov [rdx], rax
	sub rcx, 3
	jnle .loop
	ret

write_ports_8x4:
	align 64
.loop:
	mov [rdx], rax
	mov [rdx], rax
	mov [rdx], rax
	mov [rdx], rax
	sub rcx, 4
	jnle .loop
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Read bandwith limits through SIMD movs

; Each of these loops does 2 movs per iteration, but with different register widths. This is to test that
; the reading speed is not only limited by the amount of ports that can issue reads, but also by the
; memory bandwidth.

read_bandw_32x2:
	align 64
.loop:
	mov eax, [rdx ]
	mov eax, [rdx + 4]
	sub rcx, 8
	jnle .loop
	ret

read_bandw_64x2:
	align 64
.loop:
	mov rax, [rdx ]
	mov rax, [rdx + 8]
	sub rcx, 16
	jnle .loop
	ret

read_bandw_128x2:
	align 64
.loop:
	vmovdqu xmm0, [rdx ]
	vmovdqu xmm0, [rdx + 16]
	sub rcx, 32
	jnle .loop
	ret

; NOTE(ema): For some reason this is getting the same exact results as the 128x2 version on this
; machine. Are we hitting the memory bandwidth limit?

read_bandw_256x2:
	align 64
.loop:
	vmovdqu ymm0, [rdx ]
	vmovdqu ymm0, [rdx + 32]
	sub rcx, 64
	jnle .loop
	ret
