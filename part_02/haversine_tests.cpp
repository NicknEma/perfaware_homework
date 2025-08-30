#include "haversine_base.h"
#include "haversine_timing.h"
#include "haversine_profiler.h"
#include "haversine_repetition_tester.h"

#include "haversine_base.cpp"
#include "haversine_timing.cpp"
#include "haversine_profiler.cpp"
#include "haversine_repetition_tester.cpp"

struct Test_Buffer {
	u64 len;
	u8 *data;
};

struct Test_Parameters {
    char *file_name;
	Test_Buffer buffer;
};

typedef void Read_Proc(Repetition_Tester *Tester, Test_Parameters *params);

static void read_via_fread(Repetition_Tester *tester, Test_Parameters *params) {
	while (is_testing(tester)) {
		FILE *file = fopen(params->file_name, "rb");
		if (file) {
			Test_Buffer buffer = params->buffer;
			
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
		
		// end_repetition(tester);
	}
}

static void read_via_ReadFile(Repetition_Tester *tester, Test_Parameters *params) {
	while (is_testing(tester)) {
		HANDLE file = CreateFileA(params->file_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (file && file != INVALID_HANDLE_VALUE) {
			Test_Buffer buffer = params->buffer;
			
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
		
		// end_repetition(tester);
	}
}

static void write_forward(Repetition_Tester *tester, Test_Parameters *params) {
	while (is_testing(tester)) {
		Test_Buffer buffer = params->buffer;
		
		begin_timed_block(tester);
		{
			for (u64 byte_index = 0; byte_index < buffer.len; byte_index += 1) {
				buffer.data[byte_index] = (u8)byte_index;
			}
		}
		end_timed_block(tester);
		
		accumulate_byte_count(tester, buffer.len);
		// end_repetition(tester);
	}
}

struct Test_Target {
	char *name;
	Read_Proc *read_proc;
};

static Test_Target targets[] = {
	"fread", read_via_fread,
	"ReadFile", read_via_ReadFile,
	// "write_forward", write_forward,
};

int main(int argc, char **argv) {
	int exit_code = 0;
	if (argc == 2) {
		Test_Parameters params = {};
		params.file_name = argv[1];
		
		params.buffer.len = get_file_size(params.file_name);
		if (params.buffer.len != 0) {
			params.buffer.data = (u8 *) malloc(params.buffer.len);
			
			u64 cpu_freq = estimate_cpu_timer_frequency();
			
			Repetition_Tester testers[array_count(targets)] = {};
			for (;;) {
				for (int target_index = 0; target_index < array_count(targets); target_index += 1) {
					printf("--- Now testing: %s ---\n", targets[target_index].name);
					
					start_test_wave(&testers[target_index], params.buffer.len, cpu_freq);
					targets[target_index].read_proc(&testers[target_index], &params);
					
					printf("\n");
				}
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
