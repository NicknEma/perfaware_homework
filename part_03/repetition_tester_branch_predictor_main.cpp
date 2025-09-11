#include "repetition_tester_shared.h"
#include "repetition_tester.h"

#include "repetition_tester_shared.cpp"
#include "repetition_tester.cpp"

typedef void Test_Proc(u64 count, u8 *data);

extern "C" void write_conditional_nop(u64, u8 *);

#pragma comment(lib, "repetition_tester_branch_predictor.lib")

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
		
		case Test_Pattern_EVERY_2: {
			for (u64 index = 0; index < buffer.len - 1; index += 2) {
				buffer.data[index + 0] = ((index + 0) % 2) == 0;
				buffer.data[index + 1] = ((index + 1) % 2) == 0;
			}
		} break;
		
		case Test_Pattern_EVERY_3: {
			for (u64 index = 0; index < buffer.len - 1; index += 2) {
				buffer.data[index + 0] = ((index + 0) % 3) == 0;
				buffer.data[index + 1] = ((index + 1) % 3) == 0;
			}
		} break;
		
		case Test_Pattern_EVERY_4: {
			for (u64 index = 0; index < buffer.len - 1; index += 2) {
				buffer.data[index + 0] = ((index + 0) % 4) == 0;
				buffer.data[index + 1] = ((index + 1) % 4) == 0;
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

struct Test_Target {
	char *label;
	Test_Proc *test_proc;
};

static Test_Target targets[] = {
	"conditionally skip a nop", write_conditional_nop,
};

int main() {
	int exit_code = 0;
	
	Buffer buffer = alloc_buffer(GIGABYTE + 8);
	if (is_valid(buffer)) {
		u64 cpu_freq = estimate_cpu_timer_frequency();
		
		Repetition_Tester testers[Test_Pattern_COUNT][array_count(targets)] = {};
		for (;;) {
			for (u32 pattern_index = 0; pattern_index < Test_Pattern_COUNT; pattern_index += 1) {
				Test_Pattern pattern = (Test_Pattern)pattern_index;
				
				char *pattern_desc = describe_pattern(pattern);
				fill_buffer(buffer, pattern);
				
				for (u32 target_index = 0; target_index < array_count(targets); target_index += 1) {
					printf("--- Now testing: %s (%s) ---\n", targets[target_index].label, pattern_desc);
					
					Repetition_Tester *tester = &testers[pattern_index][target_index];
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
			}
			
			printf("====================\n\n");
		}
	} else {
		fprintf(stderr, "Out of memory.\n");
		exit_code = 1;
	}
	
	return exit_code;
}
