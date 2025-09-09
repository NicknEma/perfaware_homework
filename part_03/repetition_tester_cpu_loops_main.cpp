#include "repetition_tester_shared.h"
#include "repetition_tester.h"

#include "repetition_tester_shared.cpp"
#include "repetition_tester.cpp"

extern "C" void write_mov_asm(u64, u8 *);
extern "C" void write_nop_asm(u64, u8 *);
extern "C" void write_cmp_asm(u64, u8 *);
extern "C" void write_dec_asm(u64, u8 *);
extern "C" void write_skip_nop_asm(u64, u8 *);

extern "C" void read_ports_8x1(u64, u8 *);
extern "C" void read_ports_8x2(u64, u8 *);
extern "C" void read_ports_8x3(u64, u8 *);
extern "C" void read_ports_8x4(u64, u8 *);
extern "C" void read_ports_1x2(u64, u8 *);
extern "C" void read_ports_2x2(u64, u8 *);
extern "C" void read_ports_4x2(u64, u8 *);

extern "C" void write_ports_8x1(u64, u8 *);
extern "C" void write_ports_8x2(u64, u8 *);
extern "C" void write_ports_8x3(u64, u8 *);
extern "C" void write_ports_8x4(u64, u8 *);

extern "C" void read_bandw_32x2(u64, u8 *);
extern "C" void read_bandw_64x2(u64, u8 *);
extern "C" void read_bandw_128x2(u64, u8 *);
extern "C" void read_bandw_256x2(u64, u8 *);

#pragma comment(lib, "repetition_tester_cpu_loops.lib")

struct Test_Buffer {
	u64 len;
	u8 *data;
};

enum Test_Pattern : u32 {
    Test_Pattern_ZEROS,
	Test_Pattern_ONES,
	Test_Pattern_MOD2,
	Test_Pattern_MOD3,
	Test_Pattern_MOD4,
	Test_Pattern_CRTRAND,
	Test_Pattern_OSRAND,
	
	Test_Pattern_COUNT,
};

static char *describe_pattern(Test_Pattern pattern) {
    char *result = 0;
    switch (pattern) {
        case Test_Pattern_ZEROS: {result = "never taken";} break;
        case Test_Pattern_ONES: {result = "always taken";} break;
		case Test_Pattern_MOD2: {result = "every 2";} break;
		case Test_Pattern_MOD3: {result = "every 3";} break;
		case Test_Pattern_MOD4: {result = "every 4";} break;
		case Test_Pattern_CRTRAND: {result = "CRT rand";} break;
		case Test_Pattern_OSRAND: {result = "OS rand";} break;
        default: {result = "UNKNOWN";} break;
    }
    
    return result;
}

static void fill_buffer(Test_Buffer buffer, Test_Pattern pattern) {
	switch (pattern) {
        case Test_Pattern_ZEROS:
        case Test_Pattern_ONES:
		case Test_Pattern_MOD2:
		case Test_Pattern_MOD3:
		case Test_Pattern_MOD4:
		case Test_Pattern_CRTRAND: {
			
			for (u64 index = 0; index < buffer.len; index += 1) {
				u8 value = 0;
				
				switch (pattern) {
					case Test_Pattern_ZEROS: {value = 0;} break;
					case Test_Pattern_ONES: {value = 1;} break;
					case Test_Pattern_MOD2: {value = (index % 2) == 0;} break;
					case Test_Pattern_MOD3: {value = (index % 3) == 0;} break;
					case Test_Pattern_MOD4: {value = (index % 4) == 0;} break;
					case Test_Pattern_CRTRAND: {value = (u8)rand();} break;
					default: break;
				}
				
				buffer.data[index] = value;
			}
			
		} break;
		
		// case Test_Pattern_OSRAND: {result = "OS rand";} break;
		
        default: {
			fprintf(stderr, "ERROR: Unknown fill pattern.\n");
		} break;
    }
}

typedef void Test_Proc(u64 count, u8 *data);

struct Test_Target {
	char *name;
	Test_Proc *test_proc;
};

static void write_forward(u64 count, u8 *data) {
	for (u64 byte_index = 0; byte_index < count; byte_index += 1) {
		data[byte_index] = (u8)byte_index;
	}
}

static Test_Target targets[] = {
#if 0
	"write_forward", write_forward,
	"write_mov", write_mov_asm,
	"write_nop", write_nop_asm,
	"write_cmp", write_cmp_asm,
	"write_dec", write_dec_asm,
	"write_skip_nop", write_skip_nop_asm,
#endif
	
#if 0
	"read_ports_8x1", read_ports_8x1,
	"read_ports_8x2", read_ports_8x2,
	"read_ports_8x3", read_ports_8x3,
	"read_ports_8x4", read_ports_8x4,
	"read_ports_1x2", read_ports_1x2,
	"read_ports_2x2", read_ports_2x2,
	"read_ports_4x2", read_ports_4x2,
#endif
	
#if 0
	"write_ports_8x1", write_ports_8x1,
	"write_ports_8x2", write_ports_8x2,
	"write_ports_8x3", write_ports_8x3,
	"write_ports_8x4", write_ports_8x4,
#endif
	
#if 1
	"read_bandw_32x2", read_bandw_32x2,
	"read_bandw_64x2", read_bandw_64x2,
	"read_bandw_128x2", read_bandw_128x2,
	"read_bandw_256x2", read_bandw_256x2,
#endif
};

#if !defined(TEST_FILL_PATTERNS)
#define TEST_FILL_PATTERNS 0
#endif

int main() {
	Test_Buffer buffer = {};
	buffer.len = 1*1024*1024*1024 + 8;
	buffer.data = (u8 *) calloc(1, buffer.len);
	// NOTE(ema): I use calloc so that I don't have to call fill_buffer()
	// in case I'm not testing fill patterns.
	
	u64 cpu_freq = estimate_cpu_timer_frequency();
	
	Repetition_Tester testers[Test_Pattern_COUNT][array_count(targets)] = {};
	for (;;) {
#if TEST_FILL_PATTERNS
		for (u32 pattern_type = 0; pattern_type < Test_Pattern_COUNT; pattern_type += 1) {
			Test_Pattern pattern = (Test_Pattern)pattern_type;
			fill_buffer(buffer, pattern);
#else
			u32 pattern_type = 0;
#endif
			
			for (int target_index = 0; target_index < array_count(targets); target_index += 1) {
#if TEST_FILL_PATTERNS
				printf("--- Now testing: %s%s%s ---\n", describe_pattern(pattern), " + ",
					   targets[target_index].name);
#else
				printf("--- Now testing: %s ---\n", targets[target_index].name);
#endif
				
				Repetition_Tester *tester = &testers[pattern_type][target_index];
				start_test_wave(tester, buffer.len, cpu_freq, 5);
				
				while (is_testing(tester)) {
					begin_timed_block(tester);
					{
						targets[target_index].test_proc(buffer.len, buffer.data);
					}
					end_timed_block(tester);
					
					accumulate_byte_count(tester, buffer.len);
				}
				
				printf("\n");
			}
#if TEST_FILL_PATTERNS
		}
#endif
	}
	
	// return 0;
}
