#ifndef MATH_CHECK_H
#define MATH_CHECK_H

///////////////////////////
// Tests against hardcoded values

typedef f64 Math_Func(f64);

struct Reference_Answer {
	f64 x, y;
};

static void test_against_hardcoded_values(char *name, Math_Func *func, u32 ref_count, Reference_Answer *refs);

///////////////////////////
// Tests against reference implementation

struct Math_Test {
    f64 max_diff;
    f64 total_diff;
    u32 diff_count;
    
    f64 input_value_at_max_diff;
    f64 output_value_at_max_diff;
    f64 expected_value_at_max_diff;
    
    char label[64];
};

struct Math_Tester {
	Math_Test tests[256];
	Math_Test error_test; // NOTE(ema): Fallback test if we're out of space
    
    u32 completed_test_count;
    u32 current_test_offset;
    u32 printed_test_count;
	
    b32 testing;
    u32 step_index;
	
    f64 input_value;
};

static b32  try_start_precision_test(Math_Tester *tester, f64 min_input, f64 max_input, u32 step_count = 1000000);
static void compare_outputs(Math_Tester *tester, f64 expected, f64 output, char const *format, ...);
static void print_results(Math_Tester *tester);

// NOTE(ema): Example:
//
// while (try_start_precision_test(tester, -Pi, Pi) {
//     // This will write to test at index 0
//     compare_outputs(tester, sin(tester.input_value), my_sin(tester.input_value), "my_sin");
// }
//
// while (try_start_precision_test(tester, -Pi/2, Pi/2) {
//     // This will write to test at index 1
//     compare_outputs(tester, cos(tester.input_value), my_cos(tester.input_value), "my_cos");
//
//     // This will write to test at index 2
//     compare_outputs(tester, tan(tester.input_value), my_tan(tester.input_value), "my_tan");
// }

#endif
