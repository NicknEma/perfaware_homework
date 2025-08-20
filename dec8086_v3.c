/*
** This program implements decoding for most variants of the mov instruction
** and of some arithmetic instructions for the 8086 architecture. All other
 ** instructions are ignored.
** Malformed instructions (the ones that are missing non-optional bytes) are not
** handled gracefully.
*/

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define array_count(a) (sizeof(a)/sizeof((a)[0]))

#define strlit_expand_pn(s) (s), sizeof(s)
#define strlit_expand_np(s) sizeof(s), (s)

#define U8_MAX (u8) 0xFF

typedef  uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef   int8_t  i8;
typedef  int16_t i16;
typedef  int32_t i32;
typedef  int64_t i64;

typedef  char   bool;

static i8  transmute_i8 (u8  u) { return *(i8  *) &u; }
static i16 transmute_i16(u16 u) { return *(i16 *) &u; }

static u8  transmute_u8 (i8  i) { return *(u8  *) &i; }
static u16 transmute_u16(i16 i) { return *(u16 *) &i; }

static i8  safe_truncate_u32_to_i8 (u32 u) {
	i8 x = (i8) u;
	assert((u32) x == u);
	
	return x;
}

static i16 safe_truncate_u32_to_i16(u32 u) {
	i16 x = (i16) u;
	assert((u32) x == u);
	
	return x;
}

static u8  safe_truncate_u32_to_u8 (u32 u) {
	u8 x = (u8) u;
	assert((u32) x == u);
	
	return x;
}

static i16 safe_truncate_u32_to_u16(u32 u) {
	u16 x = (u16) u;
	assert((u32) x == u);
	
	return x;
}

static int digit_count_i8(i8 n) {
	int count = 0;
	
	do {
		n /= 10;
		count += 1;
	} while (n != 0);
	
	if (count == 0) count = 1;
	
	return count;
}

static int xor_digits_i32(i32 n) {
	int result = 0;
	
	do {
		int d = n % 10;
		n /= 10;
		
		result ^= d;
	} while (n != 0);
	
	return result;
}

//
// Basic temp storage
//

static u8 *temp_ptr = 0;
static i64 temp_pos = 0;
static i64 temp_cap = 0;

static void temp_init() {
	temp_cap = 1024 * 1024 * 4;
	temp_ptr = calloc(1, temp_cap);
}

static void *temp_push_nozero(i64 size) {
	void *ptr = 0;
	
	if (temp_pos + size < temp_cap) {
		ptr = temp_ptr + temp_pos;
		temp_pos += size;
	}
	
	assert(ptr);
	
	return ptr;
}

static void *temp_push(i64 size) {
	void  *ptr = temp_push_nozero(size);
	memset(ptr, 0, size);
	return ptr;
}

static void temp_reset() { temp_pos = 0; }

//
// String helpers
//

static int antoi(char *str, size_t len) {
	char *cstr = temp_push(sizeof(char) * (len + 1));
	memcpy(cstr, str, sizeof(char) * len);
	cstr[len] = '\0';
	
	int i = atoi(cstr);
	return i;
}

static char *asprintf(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	
	size_t needed = vsnprintf(0, 0, fmt, args) + 1;
    char  *buffer = malloc(sizeof(char) * needed);
    vsnprintf(buffer, needed, fmt, args);
    buffer[needed] = '\0';
	
	va_end(args);
	return buffer;
}

static char *tsprintf(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	
	size_t needed = vsnprintf(0, 0, fmt, args) + 1;
    char  *buffer = temp_push(sizeof(char) * needed);
    vsnprintf(buffer, needed, fmt, args);
    buffer[needed] = '\0';
	
	va_end(args);
	return buffer;
}

//
// Label table
//

typedef struct Label_Node Label_Node;
struct Label_Node {
	Label_Node *next;
	char *label;
	int   addr;
};

typedef struct Label_List Label_List;
struct Label_List {
	Label_Node *head;
};

static i8 label_counter = 0;
static Label_List labels[32];

static char *label_from_absolute_address(int addr, bool create_if_missing) {
	int hash = xor_digits_i32(addr);
	int slot = hash % array_count(labels);
	Label_List *list = &labels[slot];
	Label_Node *node = list->head;
	while (node) {
		if (node->addr == addr)
			break;
		node = node->next;
	}
	
	char *label = 0;
	if (!node && create_if_missing) {
		node = malloc(sizeof(Label_Node));
		node->label = asprintf("label_%i", label_counter);
		node->addr  = addr;
		
		if (list->head)  node->next = list->head;
		else             node->next = 0;
		list->head = node;
		
		label_counter += 1;
	}
	
	if (node) label = node->label;
	
	return label;
}

//
// Main decoder
//

static char *arithmetic_mnemonic_from_3bit_opcode(u8 opcode) {
	char *mnemonic = 0;
	switch (opcode) {
		case 0b000: { mnemonic = "add"; } break;
		case 0b010: { mnemonic = "adc"; } break;
		case 0b101: { mnemonic = "sub"; } break;
		case 0b011: { mnemonic = "sbb"; } break;
		case 0b111: { mnemonic = "cmp"; } break;
		case 0b100: { mnemonic = "and"; } break;
		case 0b001: { mnemonic = "or "; } break;
		case 0b110: { mnemonic = "xor"; } break;
		default: break;
	}
	return mnemonic;
}

static char *unconditional_jump_mnemonic_from_opcode(u8 opcode) {
	char *mnemonic = 0;
	switch (opcode) {
		case 0b01110100: { mnemonic = "jz    "; } break;
		case 0b01111100: { mnemonic = "jl    "; } break;
		case 0b01111110: { mnemonic = "jle   "; } break;
		case 0b01110010: { mnemonic = "jb    "; } break;
		case 0b01110110: { mnemonic = "jbe   "; } break;
		case 0b01111010: { mnemonic = "jp    "; } break;
		case 0b01110000: { mnemonic = "jo    "; } break;
		case 0b01111000: { mnemonic = "js    "; } break;
		case 0b01110101: { mnemonic = "jnz   "; } break;
		case 0b01111101: { mnemonic = "jge   "; } break;
		case 0b01111111: { mnemonic = "jg    "; } break;
		case 0b01110011: { mnemonic = "jae   "; } break;
		case 0b01110111: { mnemonic = "ja    "; } break;
		case 0b01111011: { mnemonic = "jnp   "; } break;
		case 0b01110001: { mnemonic = "jno   "; } break;
		case 0b01111001: { mnemonic = "jns   "; } break;
		
		case 0b11100010: { mnemonic = "loop  "; } break;
		case 0b11100001: { mnemonic = "loopz "; } break;
		case 0b11100000: { mnemonic = "loopnz"; } break;
		case 0b11100011: { mnemonic = "jcxz  "; } break;
		default: break;
	}
	return mnemonic;
}

static char ibuffer[1024 * 32] = {0};
static int  ibuffer_cap = array_count(ibuffer);
static int  ibuffer_len = 0;
static int  ibuffer_idx = 0;

static void decode_8086(char *name, u8 *data, int len, bool print_labels) {
	printf("; %s disassembly:\nbits 16\n", name);
	
	char *nreg_table[] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh", }; // Narrow registers
	char *wreg_table[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", }; // Wide registers
	
	char *addr_table[] = { // Effective address calculations
		"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
	};
	
	memset(ibuffer, 0, sizeof(ibuffer));
	ibuffer_len = 0;
	ibuffer_idx = 0;
	
	int idx = 0;
	while (idx < len) {
		ibuffer_len += snprintf(ibuffer + ibuffer_len, ibuffer_cap, "%i ", idx);
		
		u8 b0 = data[idx];
		idx += 1;
		
		if        ((b0 >> 2 == 0x22) || (b0 >> 1 == 0x63)) {
			// Register/memory to/from register/memory OR Immediate to register/memory
			assert(idx < len);
			
			u8 b1 = data[idx], b2 = 0, b3 = 0, b4 = 0, b5 = 0;
			idx += 1;
			
			char **reg_table = 0;
			
			if (b0 & 0x01)  reg_table = wreg_table;
			else            reg_table = nreg_table;
			
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
						
						dst = tsprintf("[%s %c %i]", addr_table[b1 & 0x07], sign, disp);
					} else if ((b1 & 0x07) == 0x06) {
						assert(idx + 1 < len);
						
						b2 = data[idx], b3 = data[idx + 1];
						idx += 2;
						
						// When the displacement is actually just a direct address, it only makes sense for it
						// to be unsigned.
						u16 disp = ((u16) b2 | ((u16) b3 << 8));
						
						dst = tsprintf("[%u]", disp);
					} else {
						dst = tsprintf("[%s]", addr_table[b1 & 0x07]);
					}
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
					
					src = tsprintf("%u", (u32) imm);
				}
				
				// If the D flag is set AND we're in the first variant of the mov:
				if ((b0 >> 2 == 0x22) && (b0 & 0x02)) {
					char *tmp = dst; dst = src; src = tmp;
				}
			}
			
			// If it is either a reg/mem->reg/mem or an imm->reg, no need for an explicit size
			if ((b0 >> 2 == 0x22) || (b1 >> 6 == 0x03)) {
				ibuffer_len += snprintf(ibuffer + ibuffer_len, ibuffer_cap, "mov %s, %s\n", dst, src);
			} else {
				ibuffer_len += snprintf(ibuffer + ibuffer_len, ibuffer_cap, "mov %s, %s %s\n", dst, size, src);
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
			
			ibuffer_len += snprintf(ibuffer + ibuffer_len, ibuffer_cap, "mov %s, %u\n", dst, (u32) imm);
		} else if (b0 >> 2 == 0x28) { // Memory to accumulator/Accumulator to memory
			assert(idx + 1 < len);
			
			u8 b1 = data[idx], b2 = data[idx + 1];
			idx += 2;
			
			u16  addr = b1 | (b2 << 8);
			char *fmt = (b0 & 0x02) ? "mov [%u], ax\n" : "mov ax, [%u]\n";
			
			ibuffer_len += snprintf(ibuffer + ibuffer_len, ibuffer_cap, fmt, (u32) addr);
		} else if (((b0 >> 2) & ~0b00001110) == 0 || ((b0 >> 2) & ~0b00001110) == 0x20) {
			// Register/memory to/from register/memory OR Immediate to register/memory
			assert(idx < len);
			
			u8 b1 = data[idx], b2 = 0, b3 = 0, b4 = 0, b5 = 0;
			idx += 1;
			
			u8 opcode = 0;
			if ((b0 & 0x80) == 0) { opcode = (b0 >> 3) & 0x07; }
			else                  { opcode = (b1 >> 3) & 0x07; }
			
			char *mnemonic = arithmetic_mnemonic_from_3bit_opcode(opcode);
			
			char **reg_table = 0;
			
			if (b0 & 0x01)  reg_table = wreg_table;
			else            reg_table = nreg_table;
			
			char *dst = 0, *src = 0;
			char *size = "byte";
			
			bool is_1st_variant = ((b0 >> 2) & ~0b1110) == 0;
			
			{
				// First assume that the D flag is 0, then swap source and destination if it
				// turns out to be 1.
				
				if (is_1st_variant) {
					// The first variant of instruction, the source is a register
					src = reg_table[(b1 >> 3) & 0x07]; // Extract from Reg field
				} else {
					// The second variant of instruction, the source is an immediate; do nothing for now
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
						
						dst = tsprintf("[%s %c %i]", addr_table[b1 & 0x07], sign, disp);
					} else if ((b1 & 0x07) == 0x06) {
						assert(idx + 1 < len);
						
						b2 = data[idx], b3 = data[idx + 1];
						idx += 2;
						
						// When the displacement is actually just a direct address, it only makes sense for it
						// to be unsigned.
						u16 disp = ((u16) b2 | ((u16) b3 << 8));
						
						dst = tsprintf("[%u]", disp);
					} else {
						dst = tsprintf("[%s]", addr_table[b1 & 0x07]);
					}
				}
				
				if (is_1st_variant) {
					// The first variant of mov, the source has already been extracted
					;
				} else {
					// The second variant of mov, extract the immediate
					assert(idx < len);
					
					b4 = data[idx];
					idx += 1;
					
					u16 imm = b4;
					
					if ((b0 & 0x02) == 0x01) {
						assert(idx < len);
						
						b5 = data[idx];
						idx += 1;
						
						imm = imm | (b5 << 8);
						size = "word";
					}
					
					src = tsprintf("%u", (u32) imm);
				}
				
				// If the D flag is set AND we're in the first variant of the instruction:
				if (is_1st_variant && (b0 & 0x02)) {
					char *tmp = dst; dst = src; src = tmp;
				}
			}
			
			if (is_1st_variant || (b1 >> 6 == 0x03)) {
				ibuffer_len += snprintf(ibuffer + ibuffer_len, ibuffer_cap, "%s %s, %s\n", mnemonic, dst, src);
			} else {
				ibuffer_len += snprintf(ibuffer + ibuffer_len, ibuffer_cap, "%s %s, %s %s\n", mnemonic, dst, size, src);
			}
		} else if (((b0 >> 1) & ~0b00011100) == 0b10) { // Arithmetic immediate w/ accumulator
			assert(idx < len);
			
			u8 b1 = data[idx], b2 = 0;
			idx += 1;
			
			char *dst = 0;
			
			if (b0 & 0x01)  dst = "ax";
			else            dst = "al";
			
			u16 imm = b1;
			
			if (b0 & 0x01) {
				assert(idx < len);
				
				b2 = data[idx];
				idx += 1;
				
				imm = imm | (b2 << 8);
			}
			
			u8 op = b0 >> 3;
			char *mnemonic = arithmetic_mnemonic_from_3bit_opcode(op);
			
			ibuffer_len += snprintf(ibuffer + ibuffer_len, ibuffer_cap, "%s %s, %u\n", mnemonic, dst, (u32) imm);
		} else {
			char *mnemonic = unconditional_jump_mnemonic_from_opcode(b0);
			if (mnemonic) {
				assert(idx < len);
				
				u8 b1 = data[idx];
				idx += 1;
				
				i8 disp = transmute_i8(b1);
				
				if (print_labels) {
					char *label = label_from_absolute_address(idx + (int) disp, 1);
					ibuffer_len += snprintf(ibuffer + ibuffer_len, ibuffer_cap, "%s %s ; %i\n", mnemonic, label, (i32) disp);
				} else {
					ibuffer_len += snprintf(ibuffer + ibuffer_len, ibuffer_cap, "%s %i\n", mnemonic, (i32) disp);
				}
			} else {
				fprintf(stderr, "Unknown instruction\n");
			}
		}
		
		temp_reset();
	}
	
	while (ibuffer_idx < ibuffer_len) {
		int line_start = ibuffer_idx;
		int space = ibuffer_idx;
		
		while (ibuffer[space] != ' ') {
			space += 1;
		}
		
		int addr = antoi(ibuffer + line_start, space - line_start);
		char *label = label_from_absolute_address(addr, 0);
		if (label) {
			printf("%s:\n", label);
		}
		
		int newline = space + 1;
		while (ibuffer[newline] != '\n') {
			newline += 1;
		}
		
		printf("%.*s\n", newline - space - 1, ibuffer + space + 1);
		ibuffer_idx = newline + 1;
	}
}

//
// Entry point
//

int main(int argc, char **argv) {
	temp_init();
	
	if (argc > 1) {
		FILE *file = fopen(argv[1], "rb");
		if (file) {
			fseek(file, 0, SEEK_END);
			size_t file_size = ftell(file);
			fseek(file, 0, SEEK_SET);
			
			u8 *file_data = malloc(file_size);
			if (file_data && fread(file_data, 1, file_size, file) == file_size) {
				bool print_labels = 0;
				if (argc > 2 && memcmp(argv[2], strlit_expand_pn("-labels")) == 0)
					print_labels = 1;
				
				decode_8086(argv[1], file_data, (int) file_size, print_labels);
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
