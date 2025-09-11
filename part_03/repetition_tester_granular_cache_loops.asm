global read_cache_repeat

section .text

; rcx: count (total)
; rdx: data
;  r8: count (of the subsection)
;  r9: rep count

read_cache_repeat:
	align 64

.outer_loop:
    mov r9, r8   ; Reset the reads-per-block counter
    mov rax, rdx ; Reset the read pointer to the beginning of the block

.inner_loop:
    ; Read 256 bytes
    vmovdqu ymm0, [rax]
    vmovdqu ymm0, [rax + 0x20]
    vmovdqu ymm0, [rax + 0x40]
    vmovdqu ymm0, [rax + 0x60]
    vmovdqu ymm0, [rax + 0x80]
    vmovdqu ymm0, [rax + 0xa0]
    vmovdqu ymm0, [rax + 0xc0]
    vmovdqu ymm0, [rax + 0xe0]
    add rax, 0x100 ; Advance the read pointer by 256 bytes
    dec r9
    jnz .inner_loop

    dec rcx
    jnz .outer_loop
    ret