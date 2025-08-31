#ifndef SIM8086_MEMORY_H
#define SIM8086_MEMORY_H

//////////////////////////////////////////
// Memory

typedef register_access register_access_t;
typedef union register_file_t register_file_t;

typedef struct memory_t memory_t;
struct memory_t {
	buffer_t bytes;
	u32 mask;
};

typedef u32 physical_address_t;

static memory_t make_memory(buffer_t bytes, u32 pointer_bits);
static u8 *get_memory_ptr(memory_t memory, physical_address_t address);

static void memory_write_u8(memory_t memory, physical_address_t address, u8 value);
static u8   memory_read_u8(memory_t memory, physical_address_t address);

static void memory_write_u16(memory_t memory, physical_address_t address, u16 value);
static u16 memory_read_u16(memory_t memory, physical_address_t address);

static void memory_write_n(memory_t memory, physical_address_t address, u16 value, u32 size);

//////////////////////////////////////////
// Memory segments

typedef struct segmented_access_t segmented_access_t;
struct segmented_access_t {
	u16 base;
	u16 offset;
};

typedef segmented_access_t pointer_variable_t;
typedef effective_address_expression effective_address_expression_t;

static physical_address_t physical_address_from_pointer_variable(pointer_variable_t pointer);
static pointer_variable_t pointer_from_components(effective_address_expression_t effective_address, register_access_t default_segment, register_file_t *registers);

#endif
