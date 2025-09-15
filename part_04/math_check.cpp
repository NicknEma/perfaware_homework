
///////////////////////////
// Tests against hardcoded values

static void print_spaces(u32 space_count) {
	for (u32 space_index = 0; space_index < space_count; space_index += 1) {
		printf(" ");
	}
}

static void test_against_hardcoded_values(char *name, Math_Func *func, u32 ref_count, Reference_Answer *refs) {
	printf("%s:\n", name);
	
	for (u32 ref_index = 0; ref_index < ref_count; ref_index += 1) {
		f64 ref_x = refs[ref_index].x;
		f64 ref_y = refs[ref_index].y;
		
		u32 space_count = printf("f(%+.24f) ", ref_x);
		printf("= %+.24f [reference]\n", ref_y);
		
		f64 got_y = func(ref_x);
		f64 error = fabs(got_y - ref_y);
		
		print_spaces(space_count);
		printf("= %+.24f (off by %.24f) [%s]\n", got_y, error, name);
	}
	
	printf("\n");
}

///////////////////////////
// Tests against reference implementation

static void print_result(Math_Test result);
static f64  compute_average_diff(Math_Test from);
static void print_decimal_bars();
static int  compare_tests_for_qsort(void *context, const void *a_index, const void *b_index);

// TODO(ema): I can't seem to find a decent name for this function, they're all confusing...
static b32  try_start_precision_test(Math_Tester *tester, f64 min_input, f64 max_input, u32 step_count) {
    if (tester->testing) {
        tester->step_index += 1;
    } else {
        // NOTE(ema): This is a new test
        tester->testing = true;
        tester->step_index  = 0;
    }
	
    if (tester->step_index < step_count) {
        tester->current_test_offset = 0;
        
        f64 t_step = (f64)tester->step_index / (f64)(step_count - 1);
        tester->input_value = (1.0 - t_step)*min_input + t_step*max_input;
    } else {
        // NOTE(ema): We did all the steps, mark vacant tests as completed
		tester->completed_test_count += tester->current_test_offset;
        
		// NOTE(ema): Log error
		if (tester->completed_test_count > array_count(tester->tests)) {
            tester->completed_test_count = array_count(tester->tests);
            fprintf(stderr, "Out of room to store math test results.\n");
        }
        
		// NOTE(ema): Print newly completed tests
        if (tester->printed_test_count < tester->completed_test_count) {
            print_decimal_bars();
            while (tester->printed_test_count < tester->completed_test_count) {
                print_result(tester->tests[tester->printed_test_count]);
				tester->printed_test_count += 1;
            }
        }
        
        tester->testing = false;
    }
    
    b32 result = tester->testing;
    return result;
}

static void compare_outputs(Math_Tester *tester, f64 expected, f64 output, char const *format, ...) {
	// NOTE(ema): Select the test we want to update
	Math_Test *test = &tester->error_test;
    u32 test_index = tester->completed_test_count + tester->current_test_offset;
	if (test_index < array_count(tester->tests)) {
		test = &tester->tests[test_index];
	}
    
	// NOTE(ema): If it's the first time using it, write the label
    if (tester->step_index == 0) {
        *test = {};
        va_list  args;
        va_start(args, format);
        vsnprintf(test->label, sizeof(test->label), format, args);
        va_end(args);
    }
    
	// NOTE(ema): Update the test
    f64 diff = fabs(expected - output);
    test->total_diff += diff;
    test->diff_count += 1;
    if (test->max_diff < diff) {
        test->max_diff = diff;
        test->input_value_at_max_diff = tester->input_value;
        test->output_value_at_max_diff = output;
        test->expected_value_at_max_diff = expected;
    }
    
	// NOTE(ema): Go to next test
    tester->current_test_offset += 1;
}

static void print_results(Math_Tester *tester) {
    if (tester->completed_test_count > 0) {
        printf("\nSorted by maximum error:\n");
        
        print_decimal_bars();
		
        u32 ranking[array_count(tester->tests)];
        for (u32 test_index = 0; test_index < tester->completed_test_count; test_index += 1) {
            ranking[test_index] = test_index;
        }
        
        qsort_s(ranking, tester->completed_test_count, sizeof(ranking[0]), compare_tests_for_qsort, tester);
        
        for (u32 test_index = 0; test_index < tester->completed_test_count; test_index += 1) {
			Math_Test test = tester->tests[ranking[test_index]];
            
            printf("%+.24f (%+.24f) [%s", test.max_diff, compute_average_diff(test), test.label);
            while ((test_index + 1) < tester->completed_test_count) {
				Math_Test next_test = tester->tests[ranking[test_index + 1]];
                if (next_test.max_diff   == test.max_diff &&
					next_test.total_diff == test.total_diff) {
                    printf(", %s", next_test.label);
                    test_index += 1;
                } else {
                    break;
                }
            }
            printf("]\n");
        }
    }
}

static void print_result(Math_Test test) {
    printf("%+.24f (%+.24f) at %+.24f [%s] \n", test.max_diff, compute_average_diff(test), test.input_value_at_max_diff, test.label);
}

static f64 compute_average_diff(Math_Test from) {
    f64 result = (from.diff_count) ? (from.total_diff / (f64)from.diff_count) : 0;
    return result;
}

static void print_decimal_bars() {
    printf("   ________________             ________________\n");
}

static int compare_tests_for_qsort(void *context, const void *a_index, const void *b_index) {
	Math_Tester *tester = (Math_Tester *)context;
    
	Math_Test *a = tester->tests + (*(u32 *)a_index);
	Math_Test *b = tester->tests + (*(u32 *)b_index);
    
    int result = 0;
    if        (a->max_diff   > b->max_diff) {
        result =  1;
    } else if (a->max_diff   < b->max_diff) {
        result = -1;
    } else if (a->total_diff > b->total_diff) {
        result =  1;
    } else if (a->total_diff < b->total_diff) {
        result = -1;
    }
    
    return result;
}
