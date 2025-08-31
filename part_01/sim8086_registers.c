
static u8 *get_register_pointer(register_file_t *registers, register_access_t access) {
    assert( access.Offset <= 1);
    assert((access.Offset + access.Count) <= 2);
    
	u8 *result = registers->as_bytes[access.Index % array_count(registers->as_words)] + access.Offset;
    return result;
}

static memory_t pretend_register_is_memory(register_file_t *registers, register_access_t access) {
	// NOTE(ema): The register offset (used to distinguish between *l and *h parts) is not explicitly
	// encoded in the segmented access, as it is used to compute the pointer
	// in pointer_from_register().
	memory_t result = make_memory(make_buffer(get_register_pointer(registers, access), access.Count),
								  access.Count - 1);
	
	return result;
}

static u16 read_register(register_file_t *registers, register_access_t access) {
	assert((access.Count >= 1) && (access.Count <= 2));
    
    u8 *Reg8 = get_register_pointer(registers, access);
    u16 *Reg16 = (u16 *)Reg8;
    u16 Result = (access.Count == 2) ? *Reg16 : *Reg8;
    return Result;
}

static u16 read_register_u16(register_file_t *registers, u32 index) {
	register_access_t access = {index, 0, 2};
	u16 result = read_register(registers, access);
    return result;
}
