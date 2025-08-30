
#if HAVERSINE_PROFILER

Profiler_Block::Profiler_Block(char *name, char *file, u32 line, u32 index, u64 byte_count) {
	this->name = name;
	this->file = file;
	this->line = line;
	
	this->index = index;
	this->parent_index = current_profiler_block_index;
	current_profiler_block_index = this->index; // "Now I'm the current block being profiled"
	
	Profiler_Record *record = &profiler_records[this->index];
	this->cpu_elapsed_inclusive = record->cpu_elapsed_inclusive;
	record->processed_byte_count = byte_count;
	
	// NOTE(ema): This is the last thing that happens in the constructor so that the other
	// bookkeeping work is not counted in the profile.
	this->cpu_start = read_cpu_timer();
}

Profiler_Block::~Profiler_Block() {
	// NOTE(ema): This is the first thing that happens in the destructor so that the other
	// bookkeeping work is not counted in the profile.
	u64 cpu_end = read_cpu_timer();
	u64 elapsed = cpu_end - this->cpu_start;
	
	current_profiler_block_index = this->parent_index; // "Now I'm not the current block being profiled anymore"
	
	Profiler_Record *parent = &profiler_records[this->parent_index];
	parent->cpu_elapsed_exclusive -= elapsed;
	
	Profiler_Record *record = &profiler_records[this->index];
	record->cpu_elapsed_inclusive = this->cpu_elapsed_inclusive + elapsed;
	record->cpu_elapsed_exclusive += elapsed;
	record->hit_count += 1;
	
	record->name = this->name;
	record->file = this->file;
	record->line = this->line;
}

static void print_profiler_records(u64 cpu_total, u64 cpu_freq) {
	for (u32 i = 0; i < array_count(profiler_records); i += 1) {
		Profiler_Record *record = &profiler_records[i];
		if (record->hit_count != 0) {
			u64 cpu_elapsed_self = record->cpu_elapsed_exclusive;
			f64 percent = (f64)(cpu_elapsed_self) * 100.0 / (f64)cpu_total;
			printf("  %s (%s:%u), %u hits: %llu (%.4f%%", record->name, record->file, record->line,
				   record->hit_count, cpu_elapsed_self, percent);
			if (cpu_elapsed_self != record->cpu_elapsed_inclusive) {
				f64 percent_with_children = (f64)(record->cpu_elapsed_inclusive) * 100.0 / (f64)cpu_total;
				printf(", %.4f%% w/children", percent_with_children);
			}
			printf(")");
			
			if (record->processed_byte_count != 0) {
				f64 megabyte = 1024.0 * 1024.0;
				f64 gigabyte = 1024.0 * megabyte;
				
				f64 seconds_elapsed = (f64)record->cpu_elapsed_inclusive / (f64)cpu_freq;
				f64 bytes_per_second = (f64)record->processed_byte_count / seconds_elapsed;
				f64 megabytes_processed = (f64)record->processed_byte_count / megabyte;
				f64 gigabytes_per_second = bytes_per_second / gigabyte;
				
				printf("  %.3fmb at %.2fgb/s", megabytes_processed, gigabytes_per_second);
			}
			
			printf("\n");
		}
	}
}

#else

#endif

static void begin_profile() {
	profiler.cpu_start = read_cpu_timer();
}

static void end_and_print_profile() {
	profiler.cpu_end = read_cpu_timer();
	
	u64 cpu_total = profiler.cpu_end - profiler.cpu_start;
	u64 cpu_freq = estimate_cpu_timer_frequency(100);
	if (cpu_freq != 0) {
		printf("Total time: %.4fms (CPU freq %llu)\n", (f64)cpu_total/(f64)cpu_freq, cpu_freq);
	}
	
	printf("Profiler blocks: %s\n", HAVERSINE_PROFILER ? "on" : "off");
	
	print_profiler_records(cpu_total, cpu_freq);
}
