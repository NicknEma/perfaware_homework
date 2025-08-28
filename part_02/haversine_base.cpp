#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

//
// Base
//

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

static string make_string(i64 len, u8 *data) {
	string result = {len, data};
	return result;
}

static bool string_equals(string a, string b) {
	bool result = false;
	if (a.len == b.len) {
		result = memcmp(a.data, b.data, sizeof(u8) * a.len) == 0;
	}
	return result;
}

struct Temp_Storage {
	u8 *ptr;
	i64 cap;
	i64 pos;
};

static Temp_Storage temp_storage = {};

static void init_temp_storage() {
	temp_storage.cap = 1024*1024*1024;
	temp_storage.ptr = (u8 *) calloc(1, temp_storage.cap);
	temp_storage.pos = 0;
}

static void free_temp_storage() {
	temp_storage.pos = 0;
}

static void *temp_push_nozero(i64 size) {
	void *result = 0;
	
	if (temp_storage.pos + size < temp_storage.cap) {
		result = temp_storage.ptr + temp_storage.pos;
		temp_storage.pos += size;
	}
	
	assert(result);
	
	return result;
}

static void *temp_push(i64 size) {
	void *result = temp_push_nozero(size);
	memset(result, 0, size);
	
	return result;
}

static char *tsprintf(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	
	size_t needed = vsnprintf(0, 0, fmt, args) + 1;
	char  *buffer = (char *) temp_push(sizeof(char) * (i64) needed);
	
	vsnprintf(buffer, needed, fmt, args);
	
	va_end(args);
	return buffer;
}

static string read_entire_file(char *name) {
	string result = {};
	
	FILE *file = fopen(name, "rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		u32 file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		result.data = (u8 *) malloc(file_size);
		if (fread(result.data, 1, file_size, file) == file_size) {
			result.len = file_size;
		} else {
			fprintf(stderr, "Error reading file '%s'\n", name);
		}
		
		fclose(file);
	} else {
		fprintf(stderr, "Error opening file '%s'\n", name);
	}
	
	return result;
}
