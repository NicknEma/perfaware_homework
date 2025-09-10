#include "repetition_tester_shared.h"
#include "repetition_tester.h"

#include "repetition_tester_shared.cpp"
#include "repetition_tester.cpp"

struct Test_Parameters {
    char *file_name;
	Buffer buffer;
};

typedef void Test_Proc(Repetition_Tester *Tester, Test_Parameters *params);

static void read_via_fread(Repetition_Tester *tester, Test_Parameters *params) {
	while (is_testing(tester)) {
		FILE *file = fopen(params->file_name, "rb");
		if (file) {
			Buffer buffer = params->buffer;
			
			size_t bytes_read = 0;
			
			begin_timed_block(tester);
			{
				bytes_read = fread(buffer.data, 1, buffer.len, file);
			}
			end_timed_block(tester);
			
			if (bytes_read == buffer.len) {
				accumulate_byte_count(tester, bytes_read);
			} else {
				record_error(tester, "fread failed");
			}
			
			fclose(file);
		} else {
			record_error(tester, "fopen failed");
		}
	}
}

static void read_via_ReadFile(Repetition_Tester *tester, Test_Parameters *params) {
	while (is_testing(tester)) {
#if _WIN32
		HANDLE file = CreateFileA(params->file_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (file && file != INVALID_HANDLE_VALUE) {
			Buffer buffer = params->buffer;
			
			bool ok = false;
			u32 bytes_read = 0;
			
			begin_timed_block(tester);
			{
				ok = ReadFile(file, buffer.data, (DWORD)buffer.len, (DWORD *)&bytes_read, 0);
			}
			end_timed_block(tester);
			
			if (ok && (u64)bytes_read == buffer.len) {
				accumulate_byte_count(tester, (u64)bytes_read);
			} else {
				record_error(tester, "ReadFile failed");
			}
			
			CloseHandle(file);
		} else {
			record_error(tester, "CreateFileA failed");
		}
	}
#else
	(void) tester;
	(void) params;
#endif
}

static void write_forward(Repetition_Tester *tester, Test_Parameters *params) {
	while (is_testing(tester)) {
		Buffer buffer = params->buffer;
		
		begin_timed_block(tester);
		{
			for (u64 byte_index = 0; byte_index < buffer.len; byte_index += 1) {
				buffer.data[byte_index] = (u8)byte_index;
			}
		}
		end_timed_block(tester);
		
		accumulate_byte_count(tester, buffer.len);
	}
}

struct Test_Target {
	char *name;
	Test_Proc *test_proc;
};

static Test_Target targets[] = {
	{"fread", read_via_fread},
	{"ReadFile", read_via_ReadFile},
	{"write_forward", write_forward},
};

int main(int argc, char **argv) {
	int exit_code = 0;
	if (argc == 2) {
		Test_Parameters params = {};
		params.file_name = argv[1];
		
		u64 file_size = get_file_size(params.file_name);
		if (file_size != 0) {
			params.buffer = alloc_buffer(file_size);
			if (is_valid(params.buffer)) {
				u64 cpu_freq = estimate_cpu_timer_frequency();
				
				Repetition_Tester testers[array_count(targets)] = {};
				for (;;) {
					for (u32 target_index = 0; target_index < array_count(targets); target_index += 1) {
						printf("--- Now testing: %s ---\n", targets[target_index].name);
						
						start_test_wave(&testers[target_index], params.buffer.len, cpu_freq);
						targets[target_index].test_proc(&testers[target_index], &params);
						
						printf("\n");
					}
					
					printf("====================\n\n");
				}
			} else {
				fprintf(stderr, "Out of memory.\n");
				exit_code = 1;
			}
		} else {
			fprintf(stderr, "Cannot test on data with a size of 0.\n");
			exit_code = 1;
		}
	} else {
		fprintf(stderr, "Usage: %s [file]\n", argv[0]);
		exit_code = 1;
	}
	
	return exit_code;
}
