#ifndef SHARED_H
#define SHARED_H

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

typedef  uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef   int8_t  i8;
typedef  int16_t i16;
typedef  int32_t i32;
typedef  int64_t i64;

typedef    float f32;
typedef   double f64;

typedef      u32 b32;

#define KILOBYTE (1024)
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)

#define array_count(a) (sizeof(a)/sizeof((a)[0]))

struct Buffer {
	u64 len;
	u8 *data;
};

static Buffer alloc_buffer(u64 size);
static void free_buffer(Buffer *buffer);
static bool is_valid(Buffer buffer);

static u64 get_file_size(char *name);

///////////////////////////
// Platform info

static u64 get_os_timer_frequency();
static u64 read_os_timer();
static u64 read_cpu_timer();
static u64 estimate_cpu_timer_frequency(u64 milliseconds_to_wait = 100);

#endif
