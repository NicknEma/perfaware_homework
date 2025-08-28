#if _WIN32

#include <intrin.h>
#include <windows.h>

static u64 get_os_timer_frequency() {
	LARGE_INTEGER freq = {};
	QueryPerformanceFrequency(&freq);
	return freq.QuadPart;
}

static u64 read_os_timer() {
	LARGE_INTEGER value = {};
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

#else

#include <x86intrin.h>
#include <sys/time.h>

static u64 get_os_timer_frequency() {
	return 1000000;
}

static u64 read_os_timer() {
	timeval value = {};
	gettimeofday(&value, 0);
	
	u64 result = get_os_timer_frequency()*(u64)value.tv_sec + (u64)value.tv_usec;
	return result;
}

#endif

static u64 read_cpu_timer() {
	return __rdtsc();
}

static u64 estimate_cpu_timer_frequency(u64 milliseconds_to_wait = 1000) {
	u64 os_freq = get_os_timer_frequency();
	
	u64 cpu_start = read_cpu_timer();
	u64 os_start = read_os_timer();
	u64 os_end = 0;
	u64 os_elapsed = 0;
	u64 os_wait_time = os_freq * milliseconds_to_wait / 1000;
	while (os_elapsed < os_wait_time) {
		os_end = read_os_timer();
		os_elapsed = os_end - os_start;
	}
	
	u64 cpu_end = read_cpu_timer();
	u64 cpu_elapsed = cpu_end - cpu_start;
	u64 cpu_freq = 0;
	if (os_elapsed != 0) {
		cpu_freq = os_freq * cpu_elapsed / os_elapsed;
	}
	
	return cpu_freq;
}
