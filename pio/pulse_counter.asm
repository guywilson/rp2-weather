.program pio_pulse_counter
.side_set 1

loop:
	wait 0 pin 0
	wait 1 pin 0
	jmp x_dec loop
	
	pull
	mov x, osr
	