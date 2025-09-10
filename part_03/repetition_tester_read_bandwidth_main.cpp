#include "repetition_tester_shared.h"
#include "repetition_tester.h"

#include "repetition_tester_shared.cpp"
#include "repetition_tester.cpp"

typedef void Test_Proc(u64 count, u8 *data);

extern "C" void read_bandw_32x2(u64, u8 *);
extern "C" void read_bandw_64x2(u64, u8 *);
extern "C" void read_bandw_128x2(u64, u8 *);
extern "C" void read_bandw_256x2(u64, u8 *);

#pragma comment(lib, "repetition_tester_read_bandwidth.lib")

struct Test_Target {
	char *label;
	Test_Proc *test_proc;
};

static Test_Target targets[] = {
	{"2 reads, 4 byte each",  read_bandw_32x2},
	{"2 reads, 8 byte each",  read_bandw_64x2},
	{"2 reads, 16 byte each", read_bandw_128x2},
	{"2 reads, 32 byte each", read_bandw_256x2},
};

int main() {
	int exit_code = 0;
	
	Buffer buffer = alloc_buffer(GIGABYTE + 8);
	if (is_valid(buffer)) {
		u64 cpu_freq = estimate_cpu_timer_frequency();
		
		printf("Testing read bandwidth for this machine, assuming 2 read ports:\n"
			   "2 movs are issued each iteration, but with different widths.\n\n");
		
		Repetition_Tester testers[array_count(targets)] = {};
		for (;;) {
			for (int target_index = 0; target_index < array_count(targets); target_index += 1) {
				printf("--- Now testing: %s ---\n", targets[target_index].label);
				
				Repetition_Tester *tester = &testers[target_index];
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
		}
	} else {
		fprintf(stderr, "Out of memory.\n");
		exit_code = 1;
	}
	
	return exit_code;
}
