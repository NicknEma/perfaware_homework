
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

void print_profiler_records(u64 cpu_total) {
	u64 cpu_freq = estimate_cpu_timer_frequency(100);
	if (cpu_freq != 0) {
		printf("Total time: %.4fms (CPU freq %llu)\n", (f64)cpu_total/(f64)cpu_freq, cpu_freq);
	}
	
	for (u32 i = 0; i < profiler_record_count; i += 1) {
		Profiler_Record *record = &profiler_records[i];
		if (record->hit_count != 0) {
			printf("  %s (%s:%u): %llu (%.4f%%, %u hits)\n", record->name, record->file, record->line,
				   record->total_time, (f64)(record->total_time) * 100.0 / (f64)cpu_total, record->hit_count);
		}
	}
}
