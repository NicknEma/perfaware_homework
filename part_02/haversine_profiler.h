#ifndef HAVERSINE_PROFILER_H
#define HAVERSINE_PROFILER_H

#define Prof_Function() Prof_Block(__FUNCTION__)
#define Prof_Block(name) \
Profiler_Anchor prof_anchor##__FILE__##__LINE__(name, __FILE__, __LINE__, __COUNTER__)

struct Profiler_Record {
	char *name, *file;
	u32 line;
	u32 hit_count;
	u64 total_time;
};

struct Profiler_Anchor {
	u64 cpu_start;
	Profiler_Record *record;
	
	Profiler_Anchor(char *name, char *file, u32 line, u32 index);
	~Profiler_Anchor();
};

static Profiler_Record profiler_records[1024];
static u32 profiler_record_count;

void print_profiler_records(u64 cpu_total);

#endif
