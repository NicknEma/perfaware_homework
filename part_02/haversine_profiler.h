#ifndef HAVERSINE_PROFILER_H
#define HAVERSINE_PROFILER_H

#define Prof_Function() Prof_Block(__FUNCTION__)
#define Prof_Block(name) \
Profiler_Block prof_anchor##__FILE__##__LINE__(name, __FILE__, __LINE__, __COUNTER__ + 1)

struct Profiler_Record {
	char *name, *file;
	u32 line;
	u32 hit_count;
	u64 cpu_elapsed_exclusive;
	u64 cpu_elapsed_inclusive;
};

struct Profiler_Block {
	u64 cpu_start;
	u64 cpu_elapsed_inclusive;
	u32 index;
	u32 parent_index;
	
	char *name, *file;
	u32   line;
	
	Profiler_Block(char *name, char *file, u32 line, u32 index);
	~Profiler_Block();
};

struct Profiler {
	Profiler_Record records[1024];
	u64 cpu_start;
	u64 cpu_end;
};

static Profiler profiler;
static u32 current_profiler_block_index;

void begin_profile();
void end_and_print_profile();

#endif
