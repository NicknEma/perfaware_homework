
static int count_ones_i8(i8 n) {
	int ones = 0;
	for (int i = 0; i < sizeof(i8) * 8; i += 1) {
		if (n & (1 << i)) {
			ones += 1;
		}
	}
	return ones;
}

static buffer_t make_buffer(u8 *data, u64 len) {
	buffer_t buffer = {
		.data = data,
		.len  = len,
	};
	
	return buffer;
}

static u64 read_file_into_buffer(buffer_t buffer, char *name) {
	u64 bytes_read = 0;
	
	FILE *file = fopen(name, "rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		u64 file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		u64 to_read = file_size;
		if (to_read > buffer.len) to_read = buffer.len;
		
		bytes_read = fread(buffer.data, 1, to_read, file);
		if (bytes_read != to_read) {
			fprintf(stderr, "Error reading file '%s'\n", name);
		}
		
		fclose(file);
	} else {
		fprintf(stderr, "Error opening file '%s'\n", name);
	}
	
	return bytes_read;
}

static u64 write_buffer_to_file(buffer_t buffer, char *name) {
	u64 bytes_written = 0;
	
	FILE *file = fopen(name, "wb");
	if (file) {
		bytes_written = (u64) fwrite(buffer.data, 1, buffer.len, file);
		if (bytes_written != buffer.len) {
			fprintf(stderr, "Error writing to file '%s'\n", name);
		}
		
		fclose(file);
	} else {
		fprintf(stderr, "Error opening file '%s'\n", name);
	}
	
	return bytes_written;
}
