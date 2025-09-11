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

section .text

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Read port counting

; These loops read from the same memory location over and over, but they're unrolled
; a different amount of times. The speed of each successive loop should increase
; until we've reached the point where no more movs can be scheduled because no
; ports are available.

; NOTE(ema): The count is in rcx, the data pointer in rdx

align 64
read_ports_8x1:
.loop:
	mov rax, [rdx]
	sub rcx, 1
	jnle .loop
	ret

align 64
read_ports_8x2:
.loop:
	mov rax, [rdx]
	mov rax, [rdx]
	sub rcx, 2
	jnle .loop
	ret

align 64
read_ports_8x3:
.loop:
	mov rax, [rdx]
	mov rax, [rdx]
	mov rax, [rdx]
	sub rcx, 3
	jnle .loop
	ret

align 64
read_ports_8x4:
.loop:
	mov rax, [rdx]
	mov rax, [rdx]
	mov rax, [rdx]
	mov rax, [rdx]
	sub rcx, 4
	jnle .loop
	ret

align 64
read_ports_1x2:
.loop:
	mov al, [rdx]
	mov al, [rdx]
	sub rcx, 1
	jnle .loop
	ret

align 64
read_ports_2x2:
.loop:
	mov ax, [rdx]
	mov ax, [rdx]
	sub rcx, 1
	jnle .loop
	ret

align 64
read_ports_4x2:
.loop:
	mov eax, [rdx]
	mov eax, [rdx]
	sub rcx, 1
	jnle .loop
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Write port counting

write_ports_8x1:
	xor rax, rax
	align 64
.loop:
	mov [rdx], rax
	sub rcx, 1
	jnle .loop
	ret

write_ports_8x2:
	xor rax, rax
	align 64
.loop:
	mov [rdx], rax
	mov [rdx], rax
	sub rcx, 2
	jnle .loop
	ret

write_ports_8x3:
	xor rax, rax
	align 64
.loop:
	mov [rdx], rax
	mov [rdx], rax
	mov [rdx], rax
	sub rcx, 3
	jnle .loop
	ret

write_ports_8x4:
	xor rax, rax
	align 64
.loop:
	mov [rdx], rax
	mov [rdx], rax
	mov [rdx], rax
	mov [rdx], rax
	sub rcx, 4
	jnle .loop
	ret
