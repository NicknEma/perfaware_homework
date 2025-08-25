; ========================================================================
;
; This is a modification of listing 55 from Computer Enhance's
; Performance-Aware Programming Course.
;
; Please see https://computerenhance.com for further information
;
; ========================================================================

bits 16

; Start image after one row, to avoid overwriting our code!
mov bp, 64*4

; Draw the black strip, 21 pixels
mov dx, 21
y_loop_start_b:
	
	mov cx, 128
	x_loop_start_b:
		mov byte [bp + 0], 0   ; Red
		mov byte [bp + 1], 0   ; Green
		mov byte [bp + 2], 0   ; Blue
		mov byte [bp + 3], 255 ; Alpha
		add bp, 4
		
		loop x_loop_start_b
	
	sub dx, 1
	jnz y_loop_start_b

; Draw the white strip, 22 pixels
mov dx, 22
y_loop_start_w:
	
	mov cx, 128
	x_loop_start_w:
		mov byte [bp + 0], 255 ; Red
		mov byte [bp + 1], 255 ; Green
		mov byte [bp + 2], 255 ; Blue
		mov byte [bp + 3], 255 ; Alpha
		add bp, 4
		
		loop x_loop_start_w
	
	sub dx, 1
	jnz y_loop_start_w

; Draw the green strip, 21 pixels
mov dx, 21
y_loop_start_g:
	
	mov cx, 128
	x_loop_start_g:
		mov byte [bp + 0], 0   ; Red
		mov byte [bp + 1], 255 ; Green
		mov byte [bp + 2], 0   ; Blue
		mov byte [bp + 3], 255 ; Alpha
		add bp, 4
		
		loop x_loop_start_g
	
	sub dx, 1
	jnz y_loop_start_g

; Reset base pointer to the start
mov bp, 64*4

; Draw the red triangle
mov dx, 32
y_loop_start_r:
	
	mov cx, 33
	sub cx, dx
	x_loop_start_r:
		mov byte [bp + 0], 255 ; Red
		mov byte [bp + 1], 0   ; Green
		mov byte [bp + 2], 0   ; Blue
		mov byte [bp + 3], 255 ; Alpha
		add bp, 4
		
		loop x_loop_start_r
	
	mov ax, 128
	sub ax, dx
	shl ax, 2
	add bp, ax

	sub dx, 1
	jnz y_loop_start_r

; Add the line rectangle green
; mov bp, 64*4 + 4*64 + 4
; mov bx, bp
; mov cx, 62
; outline_loop_start:
; 	
; 	mov byte [bp + 1], 255 ; Top line
; 	mov byte [bp + 61*64*4 + 1], 255 ; Bottom line
; 	mov byte [bx + 1], 255 ; Left line
; 	mov byte [bx + 61*4 + 1], 255 ; Right  line
; 	
; 	add bp, 4
; 	add bx, 4*64
; 			
; 	loop outline_loop_start
