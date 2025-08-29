
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
	Prof_Function();
	
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
