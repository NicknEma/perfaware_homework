global read_bandw_32x2
global read_bandw_64x2
global read_bandw_128x2
global read_bandw_256x2

section .text

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
