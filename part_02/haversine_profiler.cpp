
	Profiler_Anchor::Profiler_Anchor() {}

Profiler_Anchor::Profiler_Anchor(char *name, char *file, u32 line, u32 index) {
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

Profiler_Anchor::~Profiler_Anchor() {
	u64 cpu_end = read_cpu_timer();
	this->record->total_time += cpu_end - this->cpu_start;
}
