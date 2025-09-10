
#include <sys/stat.h>

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

static u64 get_file_size(char *name) {
	struct __stat64 info = {};
	_stat64(name, &info);
	return info.st_size;
}

static Buffer alloc_buffer(u64 size) {
	Buffer result = {};
	result.data = (u8 *) VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	result.len  = size;
	
	return result;
}

#else

#include <x86intrin.h>
#include <sys/time.h>
#include <sys/mman.h>

static u64 get_os_timer_frequency() {
	return 1000000;
}

static u64 read_os_timer() {
	timeval value = {};
	gettimeofday(&value, 0);
	
	u64 result = get_os_timer_frequency()*(u64)value.tv_sec + (u64)value.tv_usec;
	return result;
}

static u64 get_file_size(char *name) {
	struct stat info = {};
	stat(name, &info);
	return info.st_size;
}

static Buffer alloc_buffer(u64 size) {
	Buffer result = {};
	result.data = (u8 *) mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	result.len  = size;
	
	return result;
}

#endif

static u64 read_cpu_timer() {
	return __rdtsc();
}

static u64 estimate_cpu_timer_frequency(u64 milliseconds_to_wait) {
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

static bool is_valid(Buffer buffer) {
	return buffer.data != 0 || buffer.len == 0;
}
