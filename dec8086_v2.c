/*
** This program implements decoding for most variants of the mov instruction
** for the 8086 architecture. All other instructions are ignored.
** Malformed instructions (the ones that are missing non-optional bytes) are not
** handled gracefully.
*/

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define array_count(a) (sizeof(a)/sizeof((a)[0]))

typedef  uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef   int8_t  i8;
typedef  int16_t i16;
typedef  int32_t i32;
typedef  int64_t i64;

// Since displacements can be negative I do these pointer-casting shenanigans
// to transmute an unsigned value to a signed one.
static i8  transmute_i8 (u8  u) { return *(i8  *) &u; }
static i16 transmute_i16(u16 u) { return *(i16 *) &u; }

static void decode_8086(char *name, u8 *data, int len) {
	printf("; %s disassembly:\nbits 16\n", name);
	
	char *nreg_table[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh", }; // Narrow registers
	char *wreg_table[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", }; // Wide registers
	
	char *addr_table[] = { // Effective address calculations
		"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
	};
	
	int idx = 0;
	while (idx < len) {
		u8 b0 = data[idx];
		idx += 1;
		
		//
		// Since the instruction opcodes are of varying length I replaced the switch statement
		// with a chain of if-else statements, so that each if condition can shift and mask
		// the byte in its own way.
		//
		
		if        ((b0 >> 2 == 0x22) || (b0 >> 1 == 0x63)) {
			// Register/memory to/from register/memory OR Immediate to register/memory
			assert(idx < len);
			
			u8 b1 = data[idx], b2 = 0, b3 = 0, b4 = 0, b5 = 0;
			idx += 1;
			
			char **reg_table = 0;
			
			if (b0 & 0x01)  reg_table = wreg_table;
			else            reg_table = nreg_table;
			
			char dst_buf[32] = {0};
			int  dst_buf_cap =  array_count(dst_buf);
			int  dst_buf_len = 0;
			
			char src_buf[32] = {0};
			int  src_buf_cap =  array_count(src_buf);
			int  src_buf_len = 0;
			
			char *dst = 0, *src = 0;
			char *size = "byte";
			
			{
				// First assume that the D flag is 0, then swap source and destination if it
				// turns out to be 1.
				
				if (b0 >> 2 == 0x22) {
					// The first variant of mov, the source is a register
					src = reg_table[(b1 >> 3) & 0x07]; // Extract from Reg field
				} else {
					// The second variant of mov, the source is an immediate; do nothing for now
					;
				}
				
				if (b1 >> 6 == 0x03) {
					// The destination is a register
					dst = reg_table[b1 & 0x07]; // Extract from R/M field
				} else {
					// The destination is memory: extract the formula and the displacement
					// by checking the Mod field and wether the R/M field is 110.
					
					if ((b1 >> 6) != 0x00) {
						char sign = ' ';
						i32  disp = 0;
						
						if ((b1 >> 6) == 0x02) {
							assert(idx + 1 < len);
							
							b2 = data[idx], b3 = data[idx + 1];
							idx += 2;
							
							disp = (i32) transmute_i16((u16) b2 | ((u16) b3 << 8));
						} else {
							assert(idx < len);
							
							b2 = data[idx];
							idx += 1;
							
							disp = (i32) transmute_i8(b2); // Sign-extend by increasing the size *after* changing the signedness.
						}
						
						// This is done to get the same exact formatting that appears in Casey's listings.
						// The disp is stored as i32 to avoid overflow when flipping the sign.
						sign = disp < 0 ? '-' : '+';
						disp = disp < 0 ? -disp : disp;
						
						dst_buf_len = snprintf(dst_buf, dst_buf_cap, "[%s %c %i]", addr_table[b1 & 0x07], sign, disp);
					} else if ((b1 & 0x07) == 0x06) {
						assert(idx + 1 < len);
						
						b2 = data[idx], b3 = data[idx + 1];
						idx += 2;
						
						// When the displacement is actually just a direct address, it only makes sense for it
						// to be unsigned.
						u16 disp = ((u16) b2 | ((u16) b3 << 8));
						
						dst_buf_len = snprintf(dst_buf, dst_buf_cap, "[%u]", disp);
					} else {
						dst_buf_len = snprintf(dst_buf, dst_buf_cap, "[%s]", addr_table[b1 & 0x07]);
					}
					
					dst = dst_buf;
				}
				
				if (b0 >> 2 == 0x22) {
					// The first variant of mov, the source has already been extracted
					;
				} else {
					// The second variant of mov, extract the immediate
					assert(idx < len);
					
					b4 = data[idx];
					idx += 1;
					
					u16 imm = b4;
					
					if (b0 & 0x01) {
						assert(idx < len);
						
						b5 = data[idx];
						idx += 1;
						
						imm = imm | (b5 << 8);
						size = "word";
					}
					
					src_buf_len = snprintf(src_buf, src_buf_cap, "%u", (u32) imm);
					src = src_buf;
				}
				
				// If the D flag is set AND we're in the first variant of the mov:
				if ((b0 >> 2 == 0x22) && (b0 & 0x02)) {
					char *tmp = dst; dst = src; src = tmp;
				}
			}
			
			// If it is either a reg/mem->reg/mem or an imm->reg, no need for an explicit size
			if ((b0 >> 2 == 0x22) || (b1 >> 6 == 0x03)) {
				printf("mov %s, %s\n", dst, src);
			} else {
				printf("mov %s, %s %s\n", dst, size, src);
			}
		} else if (b0 >> 4 == 0x0B) { // Immediate to register
			assert(idx < len);
			
			u8 b1 = data[idx], b2 = 0;
			idx += 1;
			
			u16 imm = b1;
			char **reg_table = 0;
			
			if (b0 & 0x08) {
				assert(idx < len);
				
				b2 = data[idx];
				idx += 1;
				
				imm = imm | (b2 << 8);
				
				reg_table = wreg_table;
			} else {
				reg_table = nreg_table;
			}
			
			char *dst = reg_table[b0 & 0x07];
			
			printf("mov %s, %u\n", dst, (u32) imm);
		} else if (b0 >> 2 == 0x28) { // Memory to accumulator/Accumulator to memory
			assert(idx + 1 < len);
			
			u8 b1 = data[idx], b2 = data[idx + 1];
			idx += 2;
			
			u16  addr = b1 | (b2 << 8);
			char *fmt = (b0 & 0x02) ? "mov [%u], ax\n" : "mov ax, [%u]\n";
			
			printf(fmt, (u32) addr);
		} else {
			fprintf(stderr, "Unknown instruction\n");
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
