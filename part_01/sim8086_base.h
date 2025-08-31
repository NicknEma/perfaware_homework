#ifndef SIM8086_BASE_H
#define SIM8086_BASE_H

#define array_count(a) (sizeof(a)/sizeof((a)[0]))

#define str_expand_pfirst(s) (s), sizeof(s)
#define str_expand_sfirst(s) sizeof(s), (s)

static int count_ones_i8(i8 n);

typedef struct buffer_t buffer_t;
struct buffer_t {
	u64 len;
	u8 *data;
};

static buffer_t make_buffer(u8 *data, u64 len);

static u64 read_file_into_buffer(buffer_t buffer, char *name);
static u64 write_buffer_to_file(buffer_t buffer, char *name);

#endif
