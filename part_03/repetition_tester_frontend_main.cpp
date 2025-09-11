#include "repetition_tester_shared.h"
#include "repetition_tester.h"

#include "repetition_tester_shared.cpp"
#include "repetition_tester.cpp"

typedef void Test_Proc(u64 count, u8 *data);

static     void write_forward(u64, u8 *);
extern "C" void write_mov(u64, u8 *);
extern "C" void write_nop(u64, u8 *);
extern "C" void write_cmp(u64, u8 *);
extern "C" void write_dec(u64, u8 *);

extern "C" void nop3x1_loop(u64, u8 *);
extern "C" void nop1x3_loop(u64, u8 *);
extern "C" void nop1x9_loop(u64, u8 *);

#pragma comment(lib, "repetition_tester_frontend.lib")

static void write_forward(u64 count, u8 *data) {
	for (u64 byte_index = 0; byte_index < count; byte_index += 1) {
		data[byte_index] = (u8)byte_index;
	}
}

struct Test_Target {
	char *label;
	Test_Proc *test_proc;
};

static Test_Target targets[] = {
#if 0
	"write_forward", write_forward,
	"write_mov",     write_mov,
	"write_nop",     write_nop,
	"write_cmp",     write_cmp,
	"write_dec",     write_dec,
#endif
	
#if 1
	"1 nop,  each 3 bytes", nop3x1_loop,
	"3 nops, each 1 byte",  nop1x3_loop,
	"9 nops, each 1 byte",  nop1x9_loop,
#endif
};

int main() {
	int exit_code = 0;
	
	Buffer buffer = alloc_buffer(GIGABYTE + 8);
	if (is_valid(buffer)) {
		u64 cpu_freq = estimate_cpu_timer_frequency();
		
		Repetition_Tester testers[array_count(targets)] = {};
		for (;;) {
			for (u32 target_index = 0; target_index < array_count(targets); target_index += 1) {
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
