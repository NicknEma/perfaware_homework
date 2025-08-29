#include "haversine_base.cpp"
#include "haversine_timing.cpp"

int main() {
	u64 os_freq = get_os_timer_frequency();
	u64 cpu_freq = estimate_cpu_timer_frequency(10);
	
	u64 os_start = read_os_timer();
	(void) os_start, os_freq, cpu_freq;
	
	return 0;
}
