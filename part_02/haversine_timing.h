#ifndef HAVERSINE_TIMING_H
#define HAVERSINE_TIMING_H

static u64 get_os_timer_frequency();
static u64 read_os_timer();
static u64 read_cpu_timer();
static u64 estimate_cpu_timer_frequency(u64 milliseconds_to_wait = 100);

#endif
