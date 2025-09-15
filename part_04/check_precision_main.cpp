#include "shared.h"
#include "math.h"
#include "math_check.h"

#include "shared.cpp"
#include "math.cpp"
#include "math_check.cpp"
#include "reference_haversine.cpp"

//
// 1) Make sure the *reference* functions are correct, by comparing them against some
// hand-picked results
// 2) Make sure the *custom* implementations are correct *in the range we care about*, by
// comparing them against the reference ones.
//

static Reference_Answer sqrt_ref_answers[] = {
	{0.00, 0.0},
	{0.25, 0.5},
	{0.50, 0.7071067811865476},
	{0.75, 0.8660254037844386},
	{1.00, 1.0},
};

#if 0
static void test_against_reference_implem(char *ref_name, Math_Func *ref_func, char *name, Math_Func *func,
										  f64 domain_min, f64 domain_max, u32 sample_count = 100000) {
	
	printf("%s (comparing against %s):\n", name, ref_name);
	
	f64 acc_error = 0;
	f64 max_error = 0;
	f64 max_error_at = 0;
	for (u32 sample_index = 0; sample_index < sample_count; sample_index += 1) {
		f64 t = (f64)sample_index / (f64)(sample_count - 1);
		f64 x = (1.0 - t) * domain_min + t * domain_max;
		
		f64 ref_y = ref_func(x);
		f64 got_y = func(x);
		
		f64 error = fabs(got_y - ref_y);
		acc_error += error;
		if (max_error < error) {
			max_error = error;
			max_error_at = x;
		}
	}
	
	f64 avg_error = sample_count != 0 ? acc_error / sample_count : 0;
	
	printf("Max error: %+.24f (found when x = %+.24f)\n", max_error, max_error_at);
	printf("Avg error: %+.24f\n", avg_error);
	
	printf("\n");
}
#endif

int main() {
	{
#if 0
		test_against_hardcoded_values("sqrt", sqrt, array_count(sqrt_ref_answers), sqrt_ref_answers);
		test_against_hardcoded_values("asin", asin, 0, 0);
		test_against_hardcoded_values("sin",  sin,  0, 0);
		test_against_hardcoded_values("cos",  cos,  0, 0);
#endif
	}
	
	Math_Tester tester = {};
	
	while (try_start_precision_test(&tester, -PI64, PI64)) {
		compare_outputs(&tester, sin(tester.input_value), sin_q(tester.input_value), "sin_q");
	}
	
	while (try_start_precision_test(&tester, -PI64, PI64)) {
		compare_outputs(&tester, sin(tester.input_value), sin_q_half(tester.input_value), "sin_q_half");
	}
	
	while (try_start_precision_test(&tester, -PI64, PI64)) {
		compare_outputs(&tester, sin(tester.input_value), sin_q_quarter(tester.input_value), "sin_q_quarter");
	}
	
	while (try_start_precision_test(&tester, -PI64, PI64)) {
		compare_outputs(&tester, cos(tester.input_value), cos_q_quarter(tester.input_value), "cos_q_quarter");
	}
	
	print_results(&tester);
	
	return 0;
}
