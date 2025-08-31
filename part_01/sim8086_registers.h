#ifndef SIM8086_REGISTERS_H
#define SIM8086_REGISTERS_H

//////////////////////////////////////////
// Registers

typedef register_access register_access_t;

typedef i32 register_name_t;
enum {
	Register_None,
	
	// NOTE(ema): In the manual the order of the first 4 registers is "a, c, d, b",
	// but since I'm using Casey's decoding library I have to reproduce his order
	// if I don't want to do a remapping step every time.
	
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
} register_name_enum_t;

typedef union register_file_t register_file_t;
union register_file_t {
	// NOTE(ema): See above comment about register order.
	
	struct {
		u16 _none;
		union {struct {u8 al, ah;}; u16 ax;};
		union {struct {u8 bl, bh;}; u16 bx;};
		union {struct {u8 cl, ch;}; u16 cx;};
		union {struct {u8 dl, dh;}; u16 dx;};
		u16 sp;
		u16 bp;
		u16 si;
		u16 di;
		u16 es;
		u16 cs;
		u16 ss;
		u16 ds;
		u16 ip;
		u16 flags;
	};
	
	u8  as_bytes[Register_Count][2];
	u16 as_words[Register_Count];
};

static u8 *get_register_pointer(register_file_t *registers, register_access_t access);
static memory_t pretend_register_is_memory(register_file_t *registers, register_access_t access);

static u16 read_register(register_file_t *registers, register_access_t access);
static u16 read_register_u16(register_file_t *registers, u32 index);

//////////////////////////////////////////
// Flags

typedef u8 cpu_flag_shifts_t;
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
} cpu_flag_shifts_enum_t;

typedef u16 cpu_flags_t;
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
} cpu_flags_enum_t;

#endif
