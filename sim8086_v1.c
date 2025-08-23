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

static void simulate_8086_instruction(instruction instr, u16 *registers, u32 register_count) {
	u16 imm_value = 0;
	u16 *operand_ptrs[array_count(instr.Operands)] = {0};
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
					
					u16 *address = (u16 *) &registers[operand->Register.Index];
					address = (u16 *) ((u8 *) address + operand->Register.Offset);
					operand_ptrs[operand_index] = address;
					
					operand_strings[operand_index] = (char *) Sim86_RegisterNameFromOperand(&operand->Register);
				} break;
				
				case Operand_Immediate: {
					assert((operand->Immediate.Value & 0xFFFF0000) == 0);
					
					imm_value = (u16) operand->Immediate.Value;
					operand_ptrs[operand_index] = &imm_value;
				} break;
			}
		}
	}
	
	switch (instr.Op) {
		case Op_None: break;
		
		case Op_mov: {
			assert(size != 0);
			
			u16 source_val = 0;
			u16 dest_val = 0;
			
			memcpy(&source_val, operand_ptrs[1], size);
			memcpy(&dest_val, operand_ptrs[0], size);
			
			char *dest_str = operand_strings[0];
			printf("%s: 0x%x (%i) -> ", dest_str, dest_val, dest_val);
			
			u16 result = source_val;
			
			memcpy(operand_ptrs[0], &result, size);
			
			u16 new_dest_val = 0;
			memcpy(&new_dest_val, operand_ptrs[0], size);
			printf("0x%x (%i)", new_dest_val, new_dest_val);
		} break;
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
