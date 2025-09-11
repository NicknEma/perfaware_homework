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
	char *label;
	Test_Proc *test_proc;
};

#if 0
static Test_Target targets[] = {
#if 0
	"read_cache_noskip", read_cache_noskip,
	"read_cache_skip64", read_cache_skip64,
#endif
	
	"read_cache_masked", read_cache_masked,
};
#else
static Test_Target target = {"read_cache_masked", read_cache_masked};
#endif

// NOTE(ema): These are in bytes
static u64 sizes[] = {
	256ULL << 0,
	256ULL << 1,
	256ULL << 2,
	256ULL << 3,
	256ULL << 4,
	256ULL << 5,
	256ULL << 6,
	256ULL << 7,
	256ULL << 8,
	256ULL << 9,
	256ULL << 10,
	256ULL << 11,
	256ULL << 12,
	256ULL << 13,
	256ULL << 14,
	256ULL << 15,
	256ULL << 16,
	256ULL << 17,
	256ULL << 18,
	256ULL << 19,
	256ULL << 20,
	256ULL << 21,
};

int main() {
	int exit_code = 0;
	
	Buffer buffer = alloc_buffer(GIGABYTE + 8);
	if (is_valid(buffer)) {
		u64 cpu_freq = estimate_cpu_timer_frequency();
		
		Repetition_Tester testers[array_count(sizes)] = {};
		for (u32 size_index = 0; size_index < array_count(sizes); size_index += 1) {
			u64 size = sizes[size_index];
			u64 mask = size - 1;
			
			printf("--- Now testing: %s(%I64u, 0x%I64x) ---\n", target.label, size, size);
			
			Repetition_Tester *tester = &testers[size_index];
			start_test_wave(tester, buffer.len, cpu_freq, 1);
			
			while (is_testing(tester)) {
				begin_timed_block(tester);
				{
					target.test_proc(buffer.len, buffer.data, mask);
				}
				end_timed_block(tester);
				
				accumulate_byte_count(tester, buffer.len);
			}
			
			printf("\n");
		}
		
		printf("Region Size,gb/s\n");
		for (u32 size_index = 0; size_index < array_count(sizes); size_index += 1) {
			u64 size = sizes[size_index];
			
			Repetition_Tester *tester = &testers[size_index];
			
			f64 seconds = (f64)tester->min_time / (f64)cpu_freq;
			f64 gigabyte = (1024.0f * 1024.0f * 1024.0f);
			f64 bandwidth = (f64)tester->bytes_expected / (gigabyte * seconds);
			
			printf("%llu,%f\n", size, bandwidth);
		}
	} else {
		fprintf(stderr, "Out of memory.\n");
		exit_code = 1;
	}
	
	return exit_code;
}
