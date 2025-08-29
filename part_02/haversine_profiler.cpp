#define Prof_Function() Prof_Block(__FUNCTION__)
#define Prof_Block(name) \
Profiler_Anchor prof_anchor##__FILE__##__LINE__(name, __FILE__, __LINE__, __COUNTER__)

struct Profiler_Record {
	char *name, *file;
	u32 line;
	u32 hit_count;
	u64 total_time;
};

static Profiler_Record profiler_records[1024];
static u32 profiler_record_count;

struct Profiler_Anchor {
	u64 cpu_start;
	Profiler_Record *record;
	
	Profiler_Anchor() {}
	
	Profiler_Anchor(char *name, char *file, u32 line, u32 index) {
		this->record = &profiler_records[index];
		this->record->name = name;
		this->record->file = file;
		this->record->line = line;
		this->record->hit_count += 1;
		this->cpu_start = read_cpu_timer();
		if (profiler_record_count < index) {
			profiler_record_count = index;
		}
	}
	
	~Profiler_Anchor() {
		u64 cpu_end = read_cpu_timer();
		this->record->total_time += cpu_end - this->cpu_start;
	}
};
