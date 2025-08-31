
//////////////////////////////////////////
// Memory

static memory_t make_memory(buffer_t bytes, u32 pointer_bits) {
	memory_t memory = {
		.bytes = bytes,
		.mask  = (1 << pointer_bits) - 1,
	};
	
	return memory;
}

static u8 *get_memory_ptr(memory_t memory, physical_address_t address) {
	u8 *result = 0;
	physical_address_t masked = address & memory.mask;
	if (masked < memory.bytes.len) {
		result = memory.bytes.data + masked;
	}
	return result;
}

static void memory_write_u8(memory_t memory, physical_address_t address, u8 value) {
	*get_memory_ptr(memory, address) = value;
}

static u8 memory_read_u8(memory_t memory, physical_address_t address) {
	u8 result = *get_memory_ptr(memory, address);
	return result;
}

static void memory_write_u16(memory_t memory, physical_address_t address, u16 value) {
    memory_write_u8(memory, address + 0,  value       & 0xff);
    memory_write_u8(memory, address + 1, (value >> 8) & 0xff);
}

static u16 memory_read_u16(memory_t memory, physical_address_t address) {
    u16 result = (u16)memory_read_u8(memory, address) | ((u16)memory_read_u8(memory, address + 1) << 8);
    return result;
}

static void memory_write_n(memory_t memory, physical_address_t address, u16 value, u32 size) {
    if (size == 1) {
        memory_write_u8(memory, address, value & 0xff);
    } else {
        memory_write_u16(memory, address, value);
    }
}

//////////////////////////////////////////
// Memory segments

// Computes a physical 20-bit address from a logical 32-bit address composed of a base and an offset.
// See page 2-13 of the manual.
static physical_address_t physical_address_from_pointer_variable(pointer_variable_t pointer) {
	physical_address_t result = ((u32)pointer.base << 4) | (u32)pointer.offset;
	return result;
}

// Constructs a logical address from an expression.
// Page 2-68 (83 in the scan) of the manual explains addressing modes.
static pointer_variable_t pointer_from_components(effective_address_expression_t effective_address, register_access_t default_segment, register_file_t *registers) {
	pointer_variable_t result = {0};
	
	result.base = read_register(registers, default_segment);
	if (effective_address.Flags & Address_ExplicitSegment) {
		result.base = read_register_u16(registers, effective_address.ExplicitSegment);
	}
	
	result.offset = (u16)effective_address.Displacement;
	for (int term_index = 0; term_index < array_count(effective_address.Terms); term_index += 1) {
		effective_address_term term = effective_address.Terms[term_index];
		u16 value = (u16)(term.Scale * read_register(registers, term.Register));
		result.offset += value;
	}
	
	return result;
}

static register_access_t infer_default_segment(instruction_operand *operand) {
	register_access_t result = {
		.Index  = (operand->Address.Terms[0].Register.Index == Register_bp) ? Register_ss : Register_ds,
		.Offset = 0,
		.Count  = 2,
	};
	
	return result;
}
