#ifndef HAVERSINE_BASE_H
#define HAVERSINE_BASE_H

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <string.h>
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

#define KILOBYTE (1024)
#define MEGABYTE (1024 * KILOBYTE)
#define GIGABYTE (1024 * MEGABYTE)

#define array_count(a) (sizeof(a)/sizeof((a)[0]))

#define sl_expand_pfirst(s) (s), sizeof(s)
#define sl_expand_sfirst(s) sizeof(s), (s)
#define sl(s) make_string(sizeof(s)-1, (u8 *) (s))
#define slct(s) (string){sizeof(s)-1, (u8 *) (s)}
#define slc(s) {sizeof(s)-1, (u8 *) (s)}

#define ss_expand_pfirst(s) (s).data, (s).len
#define ss_expand_sfirst(s) (s).len, (s).data

struct string {
	i64 len;
	u8 *data;
};

static string make_string(i64 len, u8 *data);
static bool string_equals(string a, string b);

struct Temp_Storage {
	u8 *ptr;
	i64 cap;
	i64 pos;
};

static Temp_Storage temp_storage = {};

static void init_temp_storage();
static void free_temp_storage();
static void *temp_push_nozero(i64 size);
static void *temp_push(i64 size);

static char *tsprintf(char *fmt, ...);

static string read_entire_file(char *name);
static u64 get_file_size(char *name);

#endif
