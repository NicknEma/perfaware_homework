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

#pragma comment(lib, "repetition_tester_cpu_loops.lib")

enum Test_Pattern : u32 {
    Test_Pattern_NEVER_TAKEN,
	Test_Pattern_ALWAYS_TAKEN,
	Test_Pattern_EVERY_2,
	Test_Pattern_EVERY_3,
	Test_Pattern_EVERY_4,
	Test_Pattern_CRTRAND,
	Test_Pattern_OSRAND,
	
	Test_Pattern_COUNT,
};

static char *describe_pattern(Test_Pattern pattern) {
    char *result = 0;
    switch (pattern) {
        case Test_Pattern_NEVER_TAKEN:  {result =  "never taken";} break;
        case Test_Pattern_ALWAYS_TAKEN: {result = "always taken";} break;
		case Test_Pattern_EVERY_2: {result =  "every 2";} break;
		case Test_Pattern_EVERY_3: {result =  "every 3";} break;
		case Test_Pattern_EVERY_4: {result =  "every 4";} break;
		case Test_Pattern_CRTRAND: {result = "CRT rand";} break;
		case Test_Pattern_OSRAND:  {result =  "OS rand";} break;
        default: {result = "UNKNOWN";} break;
    }
    return result;
}

static void fill_buffer(Buffer buffer, Test_Pattern pattern) {
	switch (pattern) {
        case Test_Pattern_NEVER_TAKEN: {
			memset(buffer.data, 0, sizeof(u8)*buffer.len);
		} break;
		
        case Test_Pattern_ALWAYS_TAKEN: {
			memset(buffer.data, 1, sizeof(u8)*buffer.len);
		} break;
		
		case Test_Pattern_EVERY_2:
		case Test_Pattern_EVERY_3: 
		case Test_Pattern_EVERY_4: {
			u64 divisor = 0;
			switch (pattern) {
				case Test_Pattern_EVERY_2: {divisor = 2;} break;
				case Test_Pattern_EVERY_3: {divisor = 3;} break;
				case Test_Pattern_EVERY_4: {divisor = 4;} break;
				default: break;
			}
			
			assert(divisor != 0);
			
			for (u64 index = 0; index < buffer.len - 1; index += 2) {
				buffer.data[index + 0] = ((index + 0) % divisor) == 0;
				buffer.data[index + 1] = ((index + 1) % divisor) == 0;
			}
		} break;
		
		case Test_Pattern_CRTRAND: {
			for (u64 index = 0; index < buffer.len - 1; index += 2) {
				buffer.data[index + 0] = (u8)rand();
				buffer.data[index + 1] = (u8)rand();
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
#endif
	"write_skip_nop", write_skip_nop_asm,
	
#if 0
	"read_ports_8x1", read_ports_8x1,
	"read_ports_8x2", read_ports_8x2,
	"read_ports_8x3", read_ports_8x3,
	"read_ports_8x4", read_ports_8x4,
	"read_ports_1x2", read_ports_1x2,
	"read_ports_2x2", read_ports_2x2,
	"read_ports_4x2", read_ports_4x2,
#endif
	
#if 1
	"write_ports_8x1", write_ports_8x1,
	"write_ports_8x2", write_ports_8x2,
	"write_ports_8x3", write_ports_8x3,
	"write_ports_8x4", write_ports_8x4,
#endif
};

#if !defined(TEST_FILL_PATTERNS)
#define TEST_FILL_PATTERNS 1
#endif

int main() {
	int exit_code = 0;
	
	Buffer buffer = alloc_buffer(GIGABYTE + 8);
	if (is_valid(buffer)) {
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
				
				printf("====================\n\n");
#if TEST_FILL_PATTERNS
			}
#endif
		}
	} else {
		fprintf(stderr, "Out of memory.\n");
		exit_code = 1;
	}
	
	return exit_code;
}
