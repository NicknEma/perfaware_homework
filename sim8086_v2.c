#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include "sim86_shared.h"
#pragma comment (lib, "sim86_shared_debug.lib")

#define array_count(a) (sizeof(a)/sizeof((a)[0]))

#define str_expand_pfirst(s) (s), sizeof(s)
#define str_expand_sfirst(s) sizeof(s), (s)

static u32 read_file_into_buffer(u8 *buffer, u32 buffer_size, char *name) {
	u32 bytes_read = 0;
	
	FILE *file = fopen(name, "rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		u32 file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		u32 to_read = file_size;
		if (to_read > buffer_size) to_read = buffer_size;
		
		bytes_read = (u32) fread(buffer, 1, to_read, file);
		if (bytes_read != to_read) {
			fprintf(stderr, "Error reading the file\n");
		}
		
		fclose(file);
	} else {
		fprintf(stderr, "Error opening the file\n");
	}
	
	return bytes_read;
}

static void print_8086_instruction(instruction instr) {
	char *mnemonic = (char *) Sim86_MnemonicFromOperationType(instr.Op);
	printf("%s ", mnemonic);
	
	char *separator = "";
	for (int operand_index = 0; operand_index < array_count(instr.Operands); operand_index += 1) {
		instruction_operand *operand = &instr.Operands[operand_index];
		
		if (operand->Type != Operand_None) {
			printf(separator);
			separator = ", ";
			
			switch (operand->Type) {
				case Operand_None: break;
				
				case Operand_Register: {
					char *reg_name = (char *) Sim86_RegisterNameFromOperand(&operand->Register);
					printf("%s", reg_name);
				} break;
				
				case Operand_Immediate: {
					char *imm_format = "%i";
					
					immediate *imm = &operand->Immediate;
					if (imm->Flags & Immediate_RelativeJumpDisplacement) {
						imm_format = "$%+i";
					}
					
					printf(imm_format, imm->Value);
				} break;
			}
		}
	}
}

typedef i32 register_name;
enum {
	Register_None,
	
	Register_a,
	Register_b,
	Register_c,
	Register_d,
	Register_sp,
	Register_bp,
	Register_si,
	Register_di,
	Register_es,
	Register_cs,
	Register_ss,
	Register_ds,
	Register_ip,
	Register_flags,
	
	Register_Count,
} register_name_enum;

typedef u8 cpu_flag_shifts;
enum {
	Flag_C_shift = 0,
	Flag_P_shift = 2,
	Flag_A_shift = 4,
	Flag_Z_shift = 6,
	Flag_S_shift = 7,
	Flag_T_shift = 8,
	Flag_I_shift = 9,
	Flag_D_shift = 10,
	Flag_O_shift = 11,
} cpu_flag_shifts_enum;

typedef u16 cpu_flags;
enum {
	Flag_C = 1 << Flag_C_shift,
	Flag_P = 1 << Flag_P_shift,
	Flag_A = 1 << Flag_A_shift,
	Flag_Z = 1 << Flag_Z_shift,
	Flag_S = 1 << Flag_S_shift,
	Flag_T = 1 << Flag_T_shift,
	Flag_I = 1 << Flag_I_shift,
	Flag_D = 1 << Flag_D_shift,
	Flag_O = 1 << Flag_O_shift,
} cpu_flags_enum;

typedef struct arithmetic_op_info arithmetic_op_info;
struct arithmetic_op_info {
	operation_type type;
	i32 source_op_index;
	i32 dest_op_index;
	bool writeback;
	cpu_flags flags_affected;
	i32 (*impl)(i32 op1, i32 op2);
};

#pragma warning(push)
#pragma warning(disable: 4100)
static i32 nop_impl(i32 op1, i32 op2) { return 0; }
static i32 mov_impl(i32 op1, i32 op2) { return op2; }
static i32 add_impl(i32 op1, i32 op2) { return op1 + op2; }
static i32 sub_impl(i32 op1, i32 op2) { return op1 - op2; }
#pragma warning(pop)

static arithmetic_op_info arithmetic_ops[] = {
	{Op_None, 0, 0, 0, 0, nop_impl},
	{Op_mov, 1, 0, 1, 0, mov_impl},
	{Op_add, 1, 0, 1, Flag_A|Flag_C|Flag_O|Flag_P|Flag_S|Flag_Z, add_impl},
	{Op_sub, 1, 0, 1, Flag_A|Flag_C|Flag_O|Flag_P|Flag_S|Flag_Z, sub_impl},
	{Op_cmp, 1, 0, 0, Flag_A|Flag_C|Flag_O|Flag_P|Flag_S|Flag_Z, sub_impl},
};

static arithmetic_op_info arithmetic_op_info_from_op(operation_type op) {
	arithmetic_op_info info = {0};
	for (int i = 0; i < array_count(arithmetic_ops); i += 1) {
		if (op == arithmetic_ops[i].type) {
			info = arithmetic_ops[i];
			break;
		}
	}
	return info;
}

static void print_cpu_flags(cpu_flags flags) {
	static char flags_chars[] = {
		[Flag_C_shift] = 'C',
		[Flag_P_shift] = 'P',
		[Flag_A_shift] = 'A',
		[Flag_Z_shift] = 'Z',
		[Flag_S_shift] = 'S',
		[Flag_T_shift] = 'T',
		[Flag_I_shift] = 'I',
		[Flag_D_shift] = 'D',
		[Flag_O_shift] = 'O',
	};
	for (int i = 0; i < array_count(flags_chars); i += 1) {
		cpu_flags flag = 1 << i;
		if (flag & flags) {
			printf("%c", flags_chars[i]);
		}
	}
}

static int count_ones_i8(i8 n) {
	int ones = 0;
	for (int i = 0; i < sizeof(i8) * 8; i += 1) {
		if (n & (1 << i)) {
			ones += 1;
		}
	}
	return ones;
}

static void simulate_8086_instruction(instruction instr, u16 *registers, u32 register_count) {
	i32 imm_value = 0;
	i32 *operand_ptrs[array_count(instr.Operands)] = {0};
	u32 size = 0;
	
	char *operand_strings[array_count(instr.Operands)] = {0};
	
	for (int operand_index = 0; operand_index < array_count(instr.Operands); operand_index += 1) {
		instruction_operand *operand = &instr.Operands[operand_index];
		
		if (operand->Type != Operand_None) {
			switch (operand->Type) {
				case Operand_None: break;
				
				case Operand_Register: {
					assert(operand->Register.Index < register_count);
					
					size = operand->Register.Count;
					
					i32 *address = (i32 *) &registers[operand->Register.Index];
					address = (i32 *) ((u8 *) address + operand->Register.Offset);
					operand_ptrs[operand_index] = address;
					
					operand_strings[operand_index] = (char *) Sim86_RegisterNameFromOperand(&operand->Register);
				} break;
				
				case Operand_Immediate: {
					imm_value = operand->Immediate.Value;
					operand_ptrs[operand_index] = &imm_value;
				} break;
			}
		}
	}
	
	cpu_flags old_flags = registers[Register_flags];
	
	switch (instr.Op) {
		case Op_None: break;
		
		case Op_mov:
		case Op_add:
		case Op_sub:
		case Op_cmp: {
			assert(size != 0);
			
			arithmetic_op_info info = arithmetic_op_info_from_op(instr.Op);
			
			i32 source_val = 0;
			i32 dest_val = 0;
			
			memcpy(&source_val, operand_ptrs[info.source_op_index], size);
			memcpy(&dest_val, operand_ptrs[info.dest_op_index], size);
			
			char *dest_str = operand_strings[info.dest_op_index];
			printf("%s:%x (%i) -> ", dest_str, dest_val, dest_val);
			
			i32 result = info.impl(dest_val, source_val);
			
			if (info.writeback) {
				memcpy(operand_ptrs[info.dest_op_index], &result, size);
			}
			
			i32 new_dest_val = 0;
			memcpy(&new_dest_val, operand_ptrs[info.dest_op_index], size);
			printf("%x (%i)", new_dest_val, new_dest_val);
			
			if (info.flags_affected & Flag_C) {
				
			}
			
			if (info.flags_affected & Flag_A) {
				
			}
			
			if (info.flags_affected & Flag_S) {
				if (new_dest_val < 0) {
					registers[Register_flags] |= Flag_S;
				} else {
					registers[Register_flags] &= ~Flag_S;
				}
			}
			
			if (info.flags_affected & Flag_Z) {
				if (new_dest_val == 0) {
					registers[Register_flags] |= Flag_Z;
				} else {
					registers[Register_flags] &= ~Flag_Z;
				}
			}
			
			if (info.flags_affected & Flag_P) {
				if (count_ones_i8(new_dest_val & 0xFF) % 2 == 0) {
					registers[Register_flags] |= Flag_P;
				} else {
					registers[Register_flags] &= ~Flag_P;
				}
			}
			
			if (info.flags_affected & Flag_O) {
				
			}
			
		} break;
	}
	
	cpu_flags new_flags = registers[Register_flags];
	
	if (old_flags != new_flags) {
		printf("\tFlags: ");
		print_cpu_flags(old_flags);
		printf("->");
		print_cpu_flags(new_flags);
	}
}

static void simulate_8086(u8 *memory, u32 memory_size, u32 code_offset, u32 code_len, bool exec) {
	u8 *code = memory + code_offset;
	
	u16 registers[Register_Count] = {0};
	
	u32 offset = 0;
	while (offset < code_len) {
		instruction decoded = {0};
		Sim86_Decode8086Instruction(code_len - offset, code + offset, &decoded);
		if (decoded.Op) {
			offset += decoded.Size;
			
			print_8086_instruction(decoded);
			if (exec) {
				printf(" ; ");
				simulate_8086_instruction(decoded, registers, array_count(registers));
			}
			printf("\n");
		} else {
			fprintf(stderr, "Unrecognized instruction\n");
			break;
		}
	}
	
	printf("\nFinal registers:\n");
	for (int reg_index = 1; reg_index < Register_Count; reg_index += 1) {
		register_access reg = {reg_index, 0, 2};
		printf("\t%s: 0x%x (%i)\n", Sim86_RegisterNameFromOperand(&reg),
			   registers[reg_index], registers[reg_index]);
	}
	
	printf("\n");
	
	(void) memory_size;
}

int main(int argc, char **argv) {
	bool ok = 1;
	
	u32 version = Sim86_GetVersion();
	printf("Sim86 Version: %u (expected %u)\n", version, SIM86_VERSION);
	if (version != SIM86_VERSION) {
		printf("ERROR: Header file version doesn't match DLL.\n");
		ok = 0;
	}
	
	u32 memory_size = 1024 * 4;
	u8 *memory = 0;
	
	u32 code_len = 0;
	
	char *file_name = "";
	bool exec = 0;
	
	if (ok) {
		// NOTE(ema): This command-line parsing is really stupid and it only keeps the last string
		// that it thinks is a file. If multiple files are specified, all but the last one are
		// ignored.
		for (int i = 1; i < argc; i += 1) {
			if (memcmp(argv[i], str_expand_pfirst("-exec")) == 0) {
				exec = 1;
			}
			
			if (argv[i][0] != '-') {
				file_name = argv[i];
			}
		}
		
		if (!file_name) {
			fprintf(stderr, "Please specify a binary file");
			ok = 0;
		}
	}
	
	if (ok) {
		memory = calloc(1, memory_size);
		if (memory) {
			code_len = read_file_into_buffer(memory, memory_size, file_name);
			if (code_len == 0) ok = 0;
		} else {
			fprintf(stderr, "Out of memory");
			ok = 0;
		}
	}
	
	if (ok) {
		simulate_8086(memory, memory_size, 0, code_len, exec);
	}
	
	return !ok;
}
