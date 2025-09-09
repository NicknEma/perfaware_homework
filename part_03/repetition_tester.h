#ifndef HAVERSINE_REPETITION_TESTER_H
#define HAVERSINE_REPETITION_TESTER_H

enum Repetition_Tester_State : u32 {
	Repetition_Tester_State_UNINITIALIZED,
	Repetition_Tester_State_TESTING,
	Repetition_Tester_State_COMPLETED,
	Repetition_Tester_State_ERROR,
	Repetition_Tester_State_COUNT,
};

struct Repetition_Tester {
	Repetition_Tester_State state;
	
	// NOTE(ema): Configuration data
	u64 bytes_expected;
	u64 cpu_freq;
	
	// NOTE(ema): Per-wave data
	u64 cpu_time_to_try;
	u64 cpu_start;
	
	// NOTE(ema): Per-test data
	u64 bytes_processed_on_this_test;
	u64 time_elapsed_on_this_test;
	u32 open_block_count;
	u32 closed_block_count;
	
	// NOTE(ema): Test results
	u64 test_count;
	u64 total_time;
	u64 max_time;
	u64 min_time;
};

static void record_error(Repetition_Tester *tester, char *error);
static void start_test_wave(Repetition_Tester *tester, u64 byte_count, u64 cpu_freq, u32 seconds_to_try = 10);

static void begin_timed_block(Repetition_Tester *tester);
static void end_timed_block(Repetition_Tester *tester);
static void accumulate_byte_count(Repetition_Tester *tester, u64 byte_count);
static void end_repetition(Repetition_Tester *tester);
static bool is_testing(Repetition_Tester *tester);

#endif
