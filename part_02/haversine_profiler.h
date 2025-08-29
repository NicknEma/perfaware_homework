#ifndef HAVERSINE_PROFILER_H
#define HAVERSINE_PROFILER_H

#if !defined(HAVERSINE_PROFILER)
#define HAVERSINE_PROFILER 0
#endif

#if HAVERSINE_PROFILER

#define Name_Concat2(A, B) A##B
#define Name_Concat(A, B) Name_Concat2(A, B)

#define Prof_Function() Prof_Block(__FUNCTION__)
#define Prof_Block(name) \
Profiler_Block Name_Concat(prof_anchor, __LINE__)(name, __FILE__, __LINE__, __COUNTER__ + 1)

#define TU_End_Prof_Static_Assert() static_assert(__COUNTER__ < array_count(profiler_records))

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
	
	char *name;
	char *file;
	u32   line;
	
	Profiler_Block(char *name, char *file, u32 line, u32 index);
	~Profiler_Block();
};

static Profiler_Record profiler_records[1024];
static u32 current_profiler_block_index;

#else

#define Prof_Function()
#define Prof_Block(name)

#define TU_End_Prof_Static_Assert()

#define print_profiler_records(...)

#endif

struct Profiler {
	u64 cpu_start;
	u64 cpu_end;
};

static Profiler profiler;

static void begin_profile();
static void end_and_print_profile();

#endif
