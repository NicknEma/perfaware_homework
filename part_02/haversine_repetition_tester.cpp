
static void begin_timed_block(Repetition_Tester *tester) {
	tester->open_block_count += 1;
    tester->time_elapsed_on_this_test -= read_cpu_timer();
}

static void end_timed_block(Repetition_Tester *tester) {
	tester->closed_block_count += 1;
    tester->time_elapsed_on_this_test += read_cpu_timer();
}

static void accumulate_byte_count(Repetition_Tester *tester, u64 byte_count) {
	tester->bytes_processed_on_this_test += byte_count;
}

static void record_error(Repetition_Tester *tester, char *error) {
	fprintf(stderr, "Error: %s\n", error);
	tester->state = Repetition_Tester_State_ERROR;
}

static void start_test_wave(Repetition_Tester *tester, u64 byte_count, u64 cpu_freq, u32 seconds_to_try) {
	if (tester->state == Repetition_Tester_State_UNINITIALIZED) {
		tester->state = Repetition_Tester_State_TESTING;
		
		// NOTE(ema): Configure tester
		tester->bytes_expected = byte_count;
		tester->cpu_freq = cpu_freq;
		
		tester->min_time = ~0ULL;
	} else if (tester->state == Repetition_Tester_State_COMPLETED) {
		tester->state = Repetition_Tester_State_TESTING;
		
		// NOTE(ema): Check that configuration didn't change
		if (tester->bytes_expected != byte_count) {
			record_error(tester, "Byte count changed");
		}
		
		if (tester->cpu_freq != cpu_freq) {
			record_error(tester, "CPU frequency changed");
		}
	}
	
	// NOTE(ema): Setup new wave
	tester->cpu_time_to_try = seconds_to_try * cpu_freq;
	tester->cpu_start = read_cpu_timer();
}



static void _print_time_stat(char *stat_label, f64 cpu_time_measured, u64 cpu_freq, u64 bytes_processed) {
    printf("%s: %.0f", stat_label, (f64)cpu_time_measured);
    if (cpu_freq != 0) {
        f64 seconds = (f64)cpu_time_measured / (f64)cpu_freq;
        printf(" (%fms)", 1000.0 * seconds);
		
        if (bytes_processed != 0) {
            f64 bandwidth = bytes_processed / ((f64)GIGABYTE * seconds);
            printf(" %fgb/s", bandwidth);
        }
    }
}

static void _print_time_stat(char *stat_label, u64 cpu_time_measured, u64 cpu_freq, u64 bytes_processed) {
	_print_time_stat(stat_label, (f64)cpu_time_measured, cpu_freq, bytes_processed);
}

static void end_repetition(Repetition_Tester *tester) {
    if (tester->state == Repetition_Tester_State_TESTING) {
		u64 cpu_now = read_cpu_timer();
        
		// NOTE(ema): We don't count tests that had no timing blocks - we assume they took some other path
        if (tester->open_block_count != 0) {
            if (tester->open_block_count != tester->closed_block_count) {
                record_error(tester, "Unbalanced calls to begin/end_timed_block");
            }
            
            if (tester->bytes_processed_on_this_test != tester->bytes_expected) {
                record_error(tester, "Processed byte count mismatch");
            }
			
            if (tester->state == Repetition_Tester_State_TESTING) {
                u64 elapsed_time = tester->time_elapsed_on_this_test;
				tester->test_count += 1;
				tester->total_time += elapsed_time;
                if (tester->max_time < elapsed_time) {
					tester->max_time = elapsed_time;
                }
                
                if (tester->min_time > elapsed_time) {
					tester->min_time = elapsed_time;
                    
                    // NOTE(casey): Whenever we get a new minimum time, we reset the clock to the full trial time
                    tester->cpu_start = cpu_now;
                    
					{
						u64 bytes_processed = tester->bytes_expected;
						_print_time_stat("Min", tester->min_time, tester->cpu_freq, bytes_processed);
					}
					printf("               \r");
				}
                
				// NOTE(ema): Reset per-test data
                tester->open_block_count = 0;
                tester->closed_block_count = 0;
                tester->time_elapsed_on_this_test = 0;
                tester->bytes_processed_on_this_test = 0;
            }
        }
        
		// NOTE(ema): Check if we should stop testing
        if ((cpu_now - tester->cpu_start) > tester->cpu_time_to_try) {
            tester->state = Repetition_Tester_State_COMPLETED;
            
            printf("                                                          \r");
			{
				u64 bytes_processed = tester->bytes_expected;
				
				_print_time_stat("Min", tester->min_time, tester->cpu_freq, bytes_processed);
				printf("\n");
				
				_print_time_stat("Max", tester->max_time, tester->cpu_freq, bytes_processed);
				printf("\n");
				
				if (tester->test_count != 0) {
					f64 avg_time = (f64)tester->total_time / (f64)tester->test_count;
					_print_time_stat("Avg", avg_time, tester->cpu_freq, bytes_processed);
					printf("\n");
				}
			}
        }
    }
}

static bool is_testing(Repetition_Tester *tester) {
	end_repetition(tester);
	
	bool result = (tester->state == Repetition_Tester_State_TESTING);
    return result;
}
