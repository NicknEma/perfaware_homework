#include "repetition_tester_shared.h"
#include "repetition_tester.h"

#include "repetition_tester_shared.cpp"
#include "repetition_tester.cpp"

typedef void Test_Proc(u64 count, u8 *data);

extern "C" void  read_ports_8x1(u64, u8 *);
extern "C" void  read_ports_8x2(u64, u8 *);
extern "C" void  read_ports_8x3(u64, u8 *);
extern "C" void  read_ports_8x4(u64, u8 *);

extern "C" void  read_ports_1x2(u64, u8 *);
extern "C" void  read_ports_2x2(u64, u8 *);
extern "C" void  read_ports_4x2(u64, u8 *);

extern "C" void write_ports_8x1(u64, u8 *);
extern "C" void write_ports_8x2(u64, u8 *);
extern "C" void write_ports_8x3(u64, u8 *);
extern "C" void write_ports_8x4(u64, u8 *);

#pragma comment(lib, "repetition_tester_read_write_ports.lib")

struct Test_Target {
	char *name;
	Test_Proc *test_proc;
};

static Test_Target targets[] = {
#if 1
	{"1 read,  8 bytes each", read_ports_8x1},
	{"2 reads, 8 bytes each", read_ports_8x2},
	{"3 reads, 8 bytes each", read_ports_8x3},
	{"4 reads, 8 bytes each", read_ports_8x4},
#endif
	
#if 0
	{"2 reads, 1 byte  each", read_ports_1x2},
	{"2 reads, 2 bytes each", read_ports_2x2},
	{"2 reads, 4 bytes each", read_ports_4x2},
	{"2 reads, 8 bytes each", read_ports_8x2},
#endif
	
#if 0
	{"1 write,  8 bytes each", write_ports_8x1},
	{"2 writes, 8 bytes each", write_ports_8x2},
	{"3 writes, 8 bytes each", write_ports_8x3},
	{"4 writes, 8 bytes each", write_ports_8x4},
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
				printf("--- Now testing: %s ---\n", targets[target_index].name);
				
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
