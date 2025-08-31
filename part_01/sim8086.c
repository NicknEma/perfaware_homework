#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include "sim86_shared.h"
#pragma comment (lib, "sim86_shared_debug.lib")

#include "sim8086_base.h"
#include "sim8086_memory.h"
#include "sim8086_registers.h"

#include "sim8086_base.c"
#include "sim8086_memory.c"
#include "sim8086_registers.c"

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
					immediate *imm = &operand->Immediate;
					
					char *imm_format = "%i";
					i32 imm_value = imm->Value;
					
					if (imm->Flags & Immediate_RelativeJumpDisplacement) {
						imm_format = "$%+i";
						imm_value += 2;
					}
					
					printf(imm_format, imm_value);
				} break;
			}
		}
	}
}

//////////////////////////////////////////
// Simulation

typedef u8 arithmetic_op_flags;
enum {
	Aop_flag_writeback = 1 << 0,
	Aop_flag_addish    = 1 << 1, // Can be considered an add for the purposes of computing the OF
	Aop_flag_subbish   = 1 << 2, // Can be considered a sub for the purposes of computing the OF
	Aop_flag_shift     = 1 << 3, // You get the idea
} arithmetic_op_flags_enum;

typedef struct arithmetic_op_info arithmetic_op_info;
struct arithmetic_op_info {
	operation_type type;
	i32 source_op_index;
	i32 dest_op_index;
	arithmetic_op_flags flags;
	cpu_flags_t flags_affected;
	u16 (*impl)(u16 op1, u16 op2);
};

#pragma warning(push)
#pragma warning(disable: 4100)
static u16 nop_impl(u16 op1, u16 op2) { return 0; }
static u16 mov_impl(u16 op1, u16 op2) { return op2; }
static u16 add_impl(u16 op1, u16 op2) { return op1 + op2; }
static u16 sub_impl(u16 op1, u16 op2) { return op1 - op2; }
static u16 shl_impl(u16 op1, u16 op2) { return op1 << op2; }
#pragma warning(pop)

static arithmetic_op_info arithmetic_ops[] = {
	{Op_None, 0, 0, 0,                                   0,                                         nop_impl},
	{Op_mov,  1, 0, Aop_flag_writeback,                  0,                                         mov_impl},
	{Op_add,  1, 0, Aop_flag_writeback|Aop_flag_addish,  Flag_A|Flag_C|Flag_O|Flag_P|Flag_S|Flag_Z, add_impl},
	{Op_sub,  1, 0, Aop_flag_writeback|Aop_flag_subbish, Flag_A|Flag_C|Flag_O|Flag_P|Flag_S|Flag_Z, sub_impl},
	{Op_cmp,  1, 0,                    Aop_flag_subbish, Flag_A|Flag_C|Flag_O|Flag_P|Flag_S|Flag_Z, sub_impl},
	{Op_shl,  1, 0, Aop_flag_writeback|Aop_flag_shift,                 Flag_O                     , shl_impl},
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

static void print_cpu_flags(cpu_flags_t flags) {
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
		cpu_flags_t flag = 1 << i;
		if (flag & flags) {
			printf("%c", flags_chars[i]);
		}
	}
}

static bool simulate_8086_instruction(instruction instr, register_file_t *registers, memory_t memory) {
	bool should_halt = 0;
	
	typedef struct operand_access_t operand_access_t;
	struct operand_access_t {
		memory_t memory;
		pointer_variable_t pointer;
		u32 value;
		b32 unaligned;
	};
	
	operand_access_t accesses[2] = {0};
	
	for (int operand_index = 0; operand_index < array_count(instr.Operands); operand_index += 1) {
		instruction_operand *operand = &instr.Operands[operand_index];
		
		if (operand->Type != Operand_None) {
			switch (operand->Type) {
				case Operand_None: break;
				
				case Operand_Memory: {
					accesses[operand_index].memory  = memory;
					accesses[operand_index].pointer = pointer_from_components(operand->Address, infer_default_segment(operand), registers);
					
					accesses[operand_index].unaligned |= (accesses[operand_index].pointer.offset & 1);
					accesses[operand_index].value   = memory_read_u16(memory, physical_address_from_pointer_variable(accesses[operand_index].pointer));
				} break;
				
				case Operand_Register: {
					assert( operand->Register.Offset <= 1);
					assert((operand->Register.Count  >= 1) && (operand->Register.Count <= 2));
					assert((operand->Register.Offset + operand->Register.Count) <= 2);
					
					accesses[operand_index].memory = pretend_register_is_memory(registers, operand->Register);
					accesses[operand_index].value  = read_register(registers, operand->Register);
				} break;
				
				case Operand_Immediate: {
					accesses[operand_index].memory = memory;
					accesses[operand_index].value  = operand->Immediate.Value;
				} break;
			}
		}
	}
	
	u32 width      = (instr.Flags & Inst_Wide) ? 2 : 1;
	u32 width_mask = (1 << (8 * width)) - 1;
	u32 sign_bit   =  1 << (8 * width - 1);
	
	u32 aux_sign_bit = 1 << 3;
	
	bool cf_set = (registers->flags & Flag_C) != 0;
	bool pf_set = (registers->flags & Flag_P) != 0;
	bool af_set = (registers->flags & Flag_A) != 0;
	bool zf_set = (registers->flags & Flag_Z) != 0;
	bool sf_set = (registers->flags & Flag_S) != 0;
	bool tf_set = (registers->flags & Flag_T) != 0;
	bool if_set = (registers->flags & Flag_I) != 0;
	bool df_set = (registers->flags & Flag_D) != 0;
	bool of_set = (registers->flags & Flag_O) != 0;
	
	operand_access_t op0 = accesses[0];
	operand_access_t op1 = accesses[1];
	
	u32 v0 = op0.value;
	u32 v1 = op1.value;
	
	bool jump_condition_true = 0;
	
	switch (instr.Op) {
		case Op_None: break;
		
		case Op_mov: {
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)v1, width);
		} break;
		
		case Op_add: {
			u32 result = (v0 & width_mask) + (v1 & width_mask);
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			cf_set = (((v0 & v1) | ((v0 ^ v1) & ~result)) & sign_bit) != 0;
			af_set = (((v0 & v1) | ((v0 ^ v1) & ~result)) & aux_sign_bit) != 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = (~(v0 ^ v1) & (v1 ^ result) & sign_bit) != 0;
		} break;
		
		case Op_adc: {
			u32 result = (v0 & width_mask) + (v1 & width_mask) + cf_set ? 1 : 0;
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			cf_set = (((v0 & v1) | ((v0 ^ v1) & ~result)) & sign_bit) != 0;
			af_set = (((v0 & v1) | ((v0 ^ v1) & ~result)) & aux_sign_bit) != 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = (~(v0 ^ v1) & (v1 ^ result) & sign_bit) != 0;
		} break;
		
		case Op_inc: {
			v1 = 1;
			u32 result = (v0 & width_mask) + (v1 & width_mask);
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			// NOTE(ema): Don't affect CF
			af_set = (((v0 & v1) | ((v0 ^ v1) & ~result)) & aux_sign_bit) != 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = (~(v0 ^ v1) & (v1 ^ result) & sign_bit) != 0;
		} break;
		
		case Op_sub: {
			u32 result = (v0 & width_mask) - (v1 & width_mask);
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			cf_set = (((v1 & result) | ((v1 ^ result) & ~v0)) & sign_bit) != 0;
			af_set = (((v1 & result) | ((v1 ^ result) & ~v0)) & aux_sign_bit) != 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = ((v0 ^ v1) & ~(v1 ^ result) & sign_bit) != 0;
		} break;
		
		case Op_sbb: {
			u32 result = (v0 & width_mask) - (v1 & width_mask) - cf_set ? 1 : 0;
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			cf_set = (((v1 & result) | ((v1 ^ result) & ~v0)) & sign_bit) != 0;
			af_set = (((v1 & result) | ((v1 ^ result) & ~v0)) & aux_sign_bit) != 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = ((v0 ^ v1) & ~(v1 ^ result) & sign_bit) != 0;
		} break;
		
		case Op_dec: {
			v1 = 1;
			u32 result = (v0 & width_mask) - (v1 & width_mask);
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			// NOTE(ema): Don't affect CF
			af_set = (((v1 & result) | ((v1 ^ result) & ~v0)) & aux_sign_bit) != 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = ((v0 ^ v1) & ~(v1 ^ result) & sign_bit) != 0;
		} break;
		
		case Op_neg: {
			v0 = 0;
			u32 result = (v0 & width_mask) - (v1 & width_mask);
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			cf_set = v1 != 0;
			af_set = (((v1 & result) | ((v1 ^ result) & ~v0)) & aux_sign_bit) != 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = ((v0 ^ v1) & ~(v1 ^ result) & sign_bit) != 0;
		} break;
		
		case Op_cmp: {
			u32 result = (v0 & width_mask) - (v1 & width_mask);
			
			cf_set = (((v1 & result) | ((v1 ^ result) & ~v0)) & sign_bit) != 0;
			af_set = (((v1 & result) | ((v1 ^ result) & ~v0)) & aux_sign_bit) != 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = ((v0 ^ v1) & ~(v1 ^ result) & sign_bit) != 0;
		} break;
		
		case Op_mul: {
			memory_t ax_memory = pretend_register_is_memory_u16(registers, Register_a);
			u32      ax_value  = read_register_u16(registers, Register_a);
			
			memory_t dx_memory = pretend_register_is_memory_u16(registers, Register_d);
			
			u32 result = (u32)(v0 & width_mask) * (u32)(ax_value & width_mask);
			if (width == 1) {
				memory_write_u16(ax_memory, 0, (u16)result);
			} else {
				memory_write_u16(ax_memory, 0, (u16)result);
				memory_write_u16(dx_memory, 0, (u16)(result >> 8));
			}
			
			if (result >> (width * 8)) {
				cf_set = 1;
				of_set = 1;
			} else {
				cf_set = 0;
				of_set = 0;
			}
		} break;
		
		case Op_imul: {
			memory_t ax_memory = pretend_register_is_memory_u16(registers, Register_a);
			u32      ax_value  = read_register_u16(registers, Register_a);
			
			memory_t dx_memory = pretend_register_is_memory_u16(registers, Register_d);
			
			u32 result = (u32)((i32)(u32)(v0 & width_mask) * (i32)(u32)(ax_value & width_mask));
			if (width == 1) {
				memory_write_u16(ax_memory, 0, (u16)result);
			} else {
				memory_write_u16(ax_memory, 0, (u16)result);
				memory_write_u16(dx_memory, 0, (u16)(result >> 8));
			}
			
			u32 sign = result & sign_bit;
			u32 sign_extension = (u32)((i32)sign >> (width * 8));
			u32 result_hi = result >> (width * 8);
			if (result_hi != sign_extension) {
				cf_set = 1;
				of_set = 1;
			} else {
				cf_set = 0;
				of_set = 0;
			}
		} break;
		
		case Op_not: {
			u32 result = (~v0) & width_mask;
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
		} break;
		
		case Op_and: {
			u32 result = (v0 & width_mask) & (v1 & width_mask);
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			cf_set = 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = 0;
		} break;
		
		case Op_or: {
			u32 result = (v0 & width_mask) | (v1 & width_mask);
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			cf_set = 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = 0;
		} break;
		
		case Op_xor: {
			u32 result = (v0 & width_mask) ^ (v1 & width_mask);
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			cf_set = 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = 0;
		} break;
		
		case Op_test: {
			u32 result = (v0 & width_mask) & (v1 & width_mask);
			
			cf_set = 0;
			sf_set = (result & sign_bit) != 0;
			zf_set = result == 0;
			pf_set = count_ones_i8(result & 0xFF) % 2 == 0;
			of_set = 0;
		} break;
		
		case Op_shl: {
			u32 result = (v0 & width_mask) << (v1 & width_mask);
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			if ((v0 & sign_bit) == (result & sign_bit)) {
				of_set = 0;
			}
		} break;
		
		case Op_shr: {
			u32 result = (v0 & width_mask) >> (v1 & width_mask);
			memory_write_n(op0.memory, physical_address_from_pointer_variable(op0.pointer), (u16)result, width);
			
			if ((v0 & sign_bit) == (result & sign_bit)) {
				of_set = 0;
			}
		} break;
		
#if 0
		case Op_je: {
			u16 flag_expr = registers[Register_flags] >> Flag_Z_shift & 1;
			jump_condition_true = flag_expr == 1;
		} goto do_jump;
		
		case Op_jl: {
			u16 flag_expr = ((registers[Register_flags] >> Flag_S_shift) ^
							 (registers[Register_flags] >> Flag_O_shift)) & 1;
			jump_condition_true = flag_expr == 1;
		} goto do_jump;
		
		case Op_jle: {
			u16 flag_expr = (((registers[Register_flags] >> Flag_S_shift) ^
							  (registers[Register_flags] >> Flag_O_shift)) |
							 (registers[Register_flags] >> Flag_Z_shift)) & 1;
			jump_condition_true = flag_expr == 1;
		} goto do_jump;
		
		case Op_jb: {
			u16 flag_expr = registers[Register_flags] >> Flag_C_shift & 1;
			jump_condition_true = flag_expr == 1;
		} goto do_jump;
		
		case Op_jbe: {
			u16 flag_expr = ((registers[Register_flags] >> Flag_C_shift) |
							 (registers[Register_flags] >> Flag_Z_shift)) & 1;
			jump_condition_true = flag_expr == 1;
		} goto do_jump;
		
		case Op_jp: {
			u16 flag_expr = registers[Register_flags] >> Flag_P_shift & 1;
			jump_condition_true = flag_expr == 1;
		} goto do_jump;
		
		case Op_jo: {
			u16 flag_expr = registers[Register_flags] >> Flag_O_shift & 1;
			jump_condition_true = flag_expr == 1;
		} goto do_jump;
		
		case Op_js: {
			u16 flag_expr = registers[Register_flags] >> Flag_S_shift & 1;
			jump_condition_true = flag_expr == 1;
		} goto do_jump;
		
		case Op_jne: {
			u16 flag_expr = registers[Register_flags] >> Flag_Z_shift & 1;
			jump_condition_true = flag_expr == 0;
		} goto do_jump;
		
		case Op_jnl: {
			u16 flag_expr = ((registers[Register_flags] >> Flag_S_shift) ^
							 (registers[Register_flags] >> Flag_O_shift)) & 1;
			jump_condition_true = flag_expr == 0;
		} goto do_jump;
		
		case Op_jg: {
			u16 flag_expr = (((registers[Register_flags] >> Flag_S_shift) ^
							  (registers[Register_flags] >> Flag_O_shift)) |
							 (registers[Register_flags] >> Flag_Z_shift)) & 1;
			jump_condition_true = flag_expr == 0;
		} goto do_jump;
		
		case Op_jnb: {
			u16 flag_expr = registers[Register_flags] >> Flag_C_shift & 1;
			jump_condition_true = flag_expr == 0;
		} goto do_jump;
		
		case Op_ja: {
			u16 flag_expr = ((registers[Register_flags] >> Flag_C_shift) |
							 (registers[Register_flags] >> Flag_Z_shift)) & 1;
			jump_condition_true = flag_expr == 0;
		} goto do_jump;
		
		case Op_jnp: {
			u16 flag_expr = registers[Register_flags] >> Flag_P_shift & 1;
			jump_condition_true = flag_expr == 0;
		} goto do_jump;
		
		case Op_jno: {
			u16 flag_expr = registers[Register_flags] >> Flag_O_shift & 1;
			jump_condition_true = flag_expr == 0;
		} goto do_jump;
		
		case Op_jns: {
			u16 flag_expr = registers[Register_flags] >> Flag_S_shift & 1;
			jump_condition_true = flag_expr == 0;
		} goto do_jump;
		
		case Op_loop: {
			registers[Register_c] -= 1;
			jump_condition_true = registers[Register_c] != 0;
		} goto do_jump;
		
		case Op_loopz: {
			registers[Register_c] -= 1;
			u16 flag_expr = registers[Register_flags] >> Flag_Z_shift & 1;
			jump_condition_true = (registers[Register_c] != 0 && flag_expr == 1);
		} goto do_jump;
		
		case Op_loopnz: {
			registers[Register_c] -= 1;
			u16 flag_expr = registers[Register_flags] >> Flag_Z_shift & 1;
			jump_condition_true = (registers[Register_c] != 0 && flag_expr == 0);
		} goto do_jump;
		
		case Op_jcxz: {
			jump_condition_true = registers[Register_c] == 0;
			
			do_jump:;
			if (jump_condition_true) {
				i8 offset = (i8) instr.Operands[0].Immediate.Value;
				registers[Register_ip] += offset;
			}
		} break;
#endif
		
		case Op_hlt: {
			should_halt = 1;
		} break;
		
		default: {
			fprintf(stderr, "Error: Unimplemented instruction.\n");
		} break;
	}
	
	u16 cf_bit = !!cf_set << Flag_C_shift;
	u16 pf_bit = !!pf_set << Flag_P_shift;
	u16 af_bit = !!af_set << Flag_A_shift;
	u16 zf_bit = !!zf_set << Flag_Z_shift;
	u16 sf_bit = !!sf_set << Flag_S_shift;
	u16 tf_bit = !!tf_set << Flag_T_shift;
	u16 if_bit = !!if_set << Flag_I_shift;
	u16 df_bit = !!df_set << Flag_D_shift;
	u16 of_bit = !!of_set << Flag_O_shift;
	
	registers->flags = (cf_bit | pf_bit | af_bit | zf_bit | sf_bit |
						tf_bit | if_bit | df_bit | of_bit);
	
	return should_halt;
}

static_assert(-4 >> 1 == -2, ">> doesn't do sign extension");

static void simulate_8086(memory_t memory, u32 code_offset, u32 code_len, bool exec, bool show) {
	u8 *code = memory.bytes.data + code_offset;
	
	register_file_t registers = {0};
	
	registers.ip = 0;
	while (registers.ip < code_len) {
		instruction decoded = {0};
		Sim86_Decode8086Instruction(code_len - registers.ip, code + registers.ip, &decoded);
		if (decoded.Op) {
			register_file_t old_registers = {0};
			if (show) {
				old_registers = registers;
			}
			
			registers.ip += (u16)decoded.Size;
			
			if (show) {
				print_8086_instruction(decoded);
			}
			if (exec) {
				if (show) {
					printf(" ; ");
				}
				
				bool halt = simulate_8086_instruction(decoded, &registers, memory);
				
				if (show) {
					register_file_t new_registers = registers;
					
					for (int reg_index = 0; reg_index < Register_Count; reg_index += 1) {
						if (reg_index != Register_flags && old_registers.as_words[reg_index] != new_registers.as_words[reg_index]) {
							register_access reg = {reg_index, 0, 2};
							printf("%s: 0x%x->0x%x, ", Sim86_RegisterNameFromOperand(&reg),
								   old_registers.as_words[reg_index], new_registers.as_words[reg_index]);
						}
					}
					
					cpu_flags_t old_flags = old_registers.as_words[Register_flags];
					cpu_flags_t new_flags = new_registers.as_words[Register_flags];
					
					if (old_flags != new_flags) {
						printf("flags: ");
						print_cpu_flags(old_flags);
						printf("->");
						print_cpu_flags(new_flags);
					}
				}
				
				if (halt) break;
			}
			
			if (show) {
				printf("\n");
			}
		} else {
			fprintf(stderr, "Unrecognized instruction\n");
			break;
		}
	}
	
	printf("\nFinal registers:\n");
	for (int reg_index = 1; reg_index < Register_Count; reg_index += 1) {
		if (reg_index != Register_flags) {
			register_access reg = {reg_index, 0, 2};
			printf("\t%s: 0x%x (%i)\n", Sim86_RegisterNameFromOperand(&reg),
				   registers.as_words[reg_index], registers.as_words[reg_index]);
		}
	}
	
	printf("    flags: ");
	print_cpu_flags(registers.as_words[Register_flags]);
	printf("\n");
	
	printf("\n");
}

int main(int argc, char **argv) {
	bool ok = 1;
	
	u32 version = Sim86_GetVersion();
	printf("Sim86 Version: %u (expected %u)\n", version, SIM86_VERSION);
	if (version != SIM86_VERSION) {
		printf("ERROR: Header file version doesn't match DLL.\n");
		ok = 0;
	}
	
	buffer_t bytes = {0};
	bytes.len = 1024 * 64;
	bytes.data = calloc(1, bytes.len);
	
	memory_t memory = make_memory(bytes, 20);
	
	u32 code_len = 0;
	
	char *file_name = "";
	bool exec = 0;
	bool dump = 0;
	bool show = 0;
	
	if (ok) {
		// NOTE(ema): This command-line parsing is really stupid and it only keeps the last string
		// that it thinks is a file. If multiple files are specified, all but the last one are
		// ignored.
		for (int i = 1; i < argc; i += 1) {
			if (memcmp(argv[i], str_expand_pfirst("-exec")) == 0) {
				exec = 1;
			}
			
			if (memcmp(argv[i], str_expand_pfirst("-dump")) == 0) {
				dump = 1;
			}
			
			if (memcmp(argv[i], str_expand_pfirst("-show")) == 0) {
				show = 1;
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
		if (memory.bytes.data) {
			code_len = (u32)read_file_into_buffer(memory.bytes, file_name);
			if (code_len == 0) ok = 0;
		} else {
			fprintf(stderr, "Out of memory");
			ok = 0;
		}
	}
	
	if (ok) {
		simulate_8086(memory, 0, code_len, exec, show);
		
		if (dump) {
			write_buffer_to_file(memory.bytes, "dump.data");
		}
	}
	
	return !ok;
}
