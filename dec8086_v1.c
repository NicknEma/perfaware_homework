/*
** This program implements decoding for register-register mov instructions
** for the 8086 architecture. All other instructions are ignored.
** Malformed instructions (the ones that are missing non-optional bytes) are not
** handled gracefully.
*/

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef  uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

static void decode_8086(char *name, u8 *data, int len) {
	printf("; %s disassembly:\nbits 16\n", name);
	
	char *nreg_table[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh", }; // Narrow registers
	char *wreg_table[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", }; // Wide registers
	
	int idx = 0;
	while (idx < len) {
		u8 b0 = data[idx];
		idx += 1;
		
		u8 b0_m6 = b0 >> 2;
		u8 b0_l2 = b0 & 0x3;
		
		//
		// At this point in the course I have no idea how different the various instructions
		// can get, in terms of length/bit pattern of the subsequent bytes, so for now I'm
		// going to make every instruction have its own switch case.
		// Some things can probabily be collapsed out (like the Mod/RM byte decoding)
		// but I'm going to do that when I'm more familiar with the instruction set.
		//
		
		switch (b0_m6) {
			case 0x22: {
				assert(idx < len);
				
				u8 b1 = data[idx];
				idx += 1;
				
				u8 b1_mod = b1 >> 6;
				u8 b1_reg = (b1 >> 3) & 0x7;
				u8 b1_rm  = b1 & 0x7;
				
				assert(b1_mod == 0x3);
				
				char **reg_table = ((b0_l2 & 0x1) == 1) ? wreg_table : nreg_table;
				
				int dst_idx = 0, src_idx = 0;
				if ((b0_l2 & 0x2) == 0) {
					dst_idx = b1_reg;
					src_idx = b1_rm;
				} else {
					dst_idx = b1_rm;
					src_idx = b1_reg;
				}
				
				char *dst = reg_table[dst_idx];
				char *src = reg_table[src_idx];
				
				printf("mov %s, %s\n", dst, src);
			} break;
			
			default: {
				fprintf(stderr, "Unknown instruction\n");
			} break;
		}
	}
}

int main(int argc, char **argv) {
	if (argc > 1) {
		FILE *file = fopen(argv[1], "rb");
		if (file) {
			fseek(file, 0, SEEK_END);
			size_t file_size = ftell(file);
			fseek(file, 0, SEEK_SET);
			
			u8 *file_data = malloc(file_size);
			if (file_data && fread(file_data, 1, file_size, file) == file_size) {
				decode_8086(argv[1], file_data, (int) file_size);
			} else {
				fprintf(stderr, "Error reading the file\n");
			}
			
			fclose(file);
		} else {
			fprintf(stderr, "Error opening the file\n");
		}
	} else {
		fprintf(stderr, "Please specify a binary file\n");
	}
	
	return 0;
}
