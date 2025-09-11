#include "repetition_tester_shared.h"
#include "repetition_tester.h"

#include "repetition_tester_shared.cpp"
#include "repetition_tester.cpp"

typedef void Test_Proc(u64 outer_loop_count, u8 *data, u64 inner_loop_count);

extern "C" void read_cache_repeat(u64 outer_loop_count, u8 *data, u64 inner_loop_count);

#pragma comment(lib, "repetition_tester_granular_cache_loops.lib")

struct Test_Target {
	char *label;
	Test_Proc *test_proc;
};

static Test_Target target = {
	"read_cache_repeat", read_cache_repeat,
};

// NOTE(ema): This is how many bytes we read in a single iteration of the loop.
#define INNER_LOOP_SIZE 256

static void test_for_region_size(Repetition_Tester *tester, Buffer buffer, u64 region_size, u64 cpu_freq) {
	u64 outer_loop_count = buffer.len / region_size;
	u64 inner_loop_count = region_size / INNER_LOOP_SIZE;
	u64 adjusted_size = outer_loop_count * region_size;
	
	printf("--- Now testing: %s(0x%I64x) ---\n", target.label, region_size);
	start_test_wave(tester, adjusted_size, cpu_freq, 1);
	
	while (is_testing(tester)) {
		begin_timed_block(tester);
		{
			target.test_proc(outer_loop_count, buffer.data, inner_loop_count);
		}
		end_timed_block(tester);
		
		accumulate_byte_count(tester, adjusted_size);
	}
	
	printf("\n");
}

static f64 bandwidth_from_tester(Repetition_Tester *tester, u64 cpu_freq) {
	f64 seconds = (f64)tester->min_time / (f64)cpu_freq;
	f64 gigabyte = (1024.0f * 1024.0f * 1024.0f);
	f64 bandwidth = (f64)tester->bytes_expected / (gigabyte * seconds);
	return bandwidth;
}

int main() {
	int exit_code = 0;
	
	Buffer buffer = alloc_buffer(64 * MEGABYTE);
	if (is_valid(buffer)) {
		u64 cpu_freq = estimate_cpu_timer_frequency();
		
		Repetition_Tester testers[30] = {};
		u32 tester_count = 0;
		
		u64 region_sizes[array_count(testers)] = {};
		
		if (tester_count < array_count(testers)) {
			u64 region_size = 1024;
			Repetition_Tester *tester = &testers[tester_count];
			region_sizes[tester_count] = region_size;
			tester_count += 1;
			test_for_region_size(tester, buffer, region_size, cpu_freq);
		}
		
		if (tester_count < array_count(testers)) {
			u64 region_size = buffer.len - 8;
			Repetition_Tester *tester = &testers[tester_count];
			region_sizes[tester_count] = region_size;
			tester_count += 1;
			test_for_region_size(tester, buffer, region_size, cpu_freq);
		}
		
		while (tester_count < array_count(testers)) {
			f64 max_bandwidth_diff = 0;
			u32 max_bandwidth_diff_index = 0;
			for (u32 tester_index = 0; tester_index < tester_count - 1; tester_index += 1) {
				Repetition_Tester *tester0 = &testers[tester_index];
				Repetition_Tester *tester1 = &testers[tester_index + 1];
				
				f64 bandwidth0 = bandwidth_from_tester(tester0, cpu_freq);
				f64 bandwidth1 = bandwidth_from_tester(tester1, cpu_freq);
				
				f64 bandwidth_diff = fabs(bandwidth1 - bandwidth0);
				if (max_bandwidth_diff < bandwidth_diff) {
					max_bandwidth_diff = bandwidth_diff;
					max_bandwidth_diff_index = tester_index;
				}
			}
			
			u64 region_size = (region_sizes[max_bandwidth_diff_index] / 2) + (region_sizes[max_bandwidth_diff_index + 1] / 2);
			
			for (u32 tester_index = tester_count; tester_index > max_bandwidth_diff_index + 1; tester_index -= 1) {
				testers[tester_index] = testers[tester_index - 1];
				region_sizes[tester_index] = region_sizes[tester_index - 1];
			}
			
			tester_count += 1;
			
			Repetition_Tester *tester = &testers[max_bandwidth_diff_index + 1];
			region_sizes[max_bandwidth_diff_index + 1] = region_size;
			
			memset(tester, 0, sizeof(*tester));
			test_for_region_size(tester, buffer, region_size, cpu_freq);
		}
		
		printf("Region Size,gb/s\n");
        for (u32 tester_index = 0; tester_index < tester_count; tester_index += 1) {
			Repetition_Tester *tester = &testers[tester_index];
			u64 region_size = region_sizes[tester_index];
			
			f64 bandwidth = bandwidth_from_tester(tester, cpu_freq);
			printf("%llu,%f\n", region_size, bandwidth);
        }
	} else {
		fprintf(stderr, "Out of memory.\n");
		exit_code = 1;
	}
	
	return exit_code;
}
