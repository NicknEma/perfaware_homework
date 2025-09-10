#include "repetition_tester_shared.h"
#include "repetition_tester.h"

#include "repetition_tester_shared.cpp"
#include "repetition_tester.cpp"

extern "C" void read_cache_skip64(u64, u8 *, u64);
extern "C" void read_cache_noskip(u64, u8 *, u64);

extern "C" void read_cache_masked(u64, u8 *, u64);

#pragma comment(lib, "repetition_tester_cache_loops.lib")

typedef void Test_Proc(u64 count, u8 *data, u64 mask);

struct Test_Target {
	char *name;
	Test_Proc *test_proc;
};

static Test_Target targets[] = {
#if 0
	"read_cache_noskip", read_cache_noskip,
	"read_cache_skip64", read_cache_skip64,
#endif
	
	"read_cache_masked", read_cache_masked,
};

static u64 masks[] = {
	0,
	0xFF,
	0x1FF,
	0x3FF,
	0xFFFFFF,
	0xFFFFFFFF,
	~0ULL,
};

int main() {
	int exit_code = 0;
	
	Buffer buffer = alloc_buffer(1*1024*1024*1024 + 8);
	if (is_valid(buffer)) {
		u64 cpu_freq = estimate_cpu_timer_frequency();
		
		Repetition_Tester testers[array_count(masks)][array_count(targets)] = {};
		for (;;) {
			for (u32 mask_index = 0; mask_index < array_count(masks); mask_index += 1) {
				u64 mask = masks[mask_index];
				
				for (u32 target_index = 0; target_index < array_count(targets); target_index += 1) {
					printf("--- Now testing: %s(0x%I64x) ---\n", targets[target_index].name, mask);
					
					Repetition_Tester *tester = &testers[mask_index][target_index];
					start_test_wave(tester, buffer.len, cpu_freq, 5);
					
					while (is_testing(tester)) {
						begin_timed_block(tester);
						{
							targets[target_index].test_proc(buffer.len, buffer.data, mask);
						}
						end_timed_block(tester);
						
						accumulate_byte_count(tester, buffer.len);
					}
					
					printf("\n");
				}
			}
		}
	} else {
		fprintf(stderr, "Out of memory.\n");
		exit_code = 1;
	}
	
	return exit_code;
}
