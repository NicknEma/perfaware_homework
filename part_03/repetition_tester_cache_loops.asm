global read_cache_skip64
global read_cache_noskip
global read_cache_masked

section .text

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Cache testing

; These loops were written as challenge homework to test different cache behaviours. The two functions are
; identical except for the way the loop counters are incremented: in the first one, the internal loop
; counter advances by 1 every time and the external one advances by 64. In the second one,
; the increments (and the value they are compared against) are swapped.
; This shows how important the cache is, as the second function is 10 times slower than the first.

read_cache_noskip:
	xor rax, rax
	align 64
.i_loop:
	xor r9, r9

	.j_loop:
		lea r10, [rax + r9]
		mov r8, [rdx + r10]
		add r9, 1
		cmp r9, 64
		jb .j_loop

	add rax, 64
	cmp rax, rcx
	jb .i_loop
	ret

read_cache_skip64:
	xor rax, rax
	align 64
.i_loop:
	xor r9, r9

	.j_loop:
		lea r10, [rax + r9]
		mov r8, [rdx + r10]
		add r9, 64
		cmp r9, rcx
		jb .j_loop

	add rax, 1
	cmp rax, 64
	jb .i_loop
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Better cache testing

; To test actual cache behaviour (not only see *that* missing the cache is slow, but how much does it slow down
; *depending on how often we miss it*), we do a normal loop where we read the entire buffer's size worth
; of data, but we include the possibility of re-reading the same bytes over and over instead of always
; reading new bytes. The additional parameter that comes in r8 is a mask that controls this effect.

read_cache_masked:
	xor r9, r9
	align 64
.loop:
	mov rax, r9
	and rax, r8
	add rax, rdx
		vmovdqu ymm0, [rax +  0]
		vmovdqu ymm0, [rax + 32]
		vmovdqu ymm0, [rax + 64]
		vmovdqu ymm0, [rax + 96]
	add r9, 128
	cmp r9, rcx
	jb .loop
	ret
