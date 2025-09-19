#include "shared.h"
#include "math.h"

#include "shared.cpp"
#include "math.cpp"
#include "reference_haversine.cpp"

struct Func_Range {
	f64 min, max;
};

struct Haversine_Ranges {
	Func_Range sqrt, asin, sin, cos;
};

static void update_range(f64 x, Func_Range *range) {
	if (range->min > x) { range->min = x; }
	if (range->max < x) { range->max = x; }
}

static void update_haversine_ranges(f64 x0, f64 y0, f64 x1, f64 y1, Haversine_Ranges *ranges) {
	f64 lat1 = y0;
    f64 lat2 = y1;
    f64 lon1 = x0;
    f64 lon2 = x1;
    
    f64 dLat = radians_from_degrees_f64(lat2 - lat1);
    f64 dLon = radians_from_degrees_f64(lon2 - lon1);
    lat1 = radians_from_degrees_f64(lat1);
    lat2 = radians_from_degrees_f64(lat2);
    
	update_range(dLat/2.0, &ranges->sin);
	update_range(dLon/2.0, &ranges->sin);
	
	update_range(lat1, &ranges->cos);
	update_range(lat2, &ranges->cos);
	
    f64 a = square_f64(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*square_f64(sin(dLon/2.0));
    
	update_range(a, &ranges->sqrt);
	update_range(sqrt(a), &ranges->asin);
	
	return;
}

int main(int argc, char **argv) {
	int exit_code = 0;
	
	if (argc == 2) {
		char *file_name = argv[1];
		u64 file_size = get_file_size(file_name);
		if (file_size > 0) {
			Buffer file_buf = alloc_buffer(file_size);
			if (is_valid(file_buf)) {
				FILE *file = fopen(file_name, "rb");
				if (file) {
					if (fread(file_buf.data, 1, file_buf.len, file) == file_buf.len) {
						
						f64 *points = (f64 *) file_buf.data;
						u64  point_count = file_buf.len / sizeof(f64);
						
						Func_Range inv_inf = {DBL_MAX, -DBL_MAX};
						Haversine_Ranges ranges = {};
						ranges.sqrt = inv_inf;
						ranges.asin = inv_inf;
						ranges.sin = inv_inf;
						ranges.cos = inv_inf;
						
						for (u64 point_index = 0; point_index < point_count - 3; point_index += 4) {
							f64 x0 = points[point_index];
							f64 y0 = points[point_index + 1];
							f64 x1 = points[point_index + 2];
							f64 y1 = points[point_index + 3];
							
							update_haversine_ranges(x0, y0, x1, y1, &ranges);
						}
						
						printf("Ranges:\n");
						printf("  sqrt: [%f, %f]\n", ranges.sqrt.min, ranges.sqrt.max);
						printf("  asin: [%f, %f]\n", ranges.asin.min, ranges.asin.max);
						printf("  sin: [%f, %f]\n", ranges.sin.min, ranges.sin.max);
						printf("  cos: [%f, %f]\n", ranges.cos.min, ranges.cos.max);
					} else {
						fprintf(stderr, "Error: Cannot read file.\n");
						exit_code = 1;
					}
					
					fclose(file);
					fclose(file);
				} else {
					fprintf(stderr, "Error: Cannot open file.\n");
					exit_code = 1;
				}
			} else {
				fprintf(stderr, "Error: Out of memory.\n");
				exit_code = 1;
			}
		} else {
			fprintf(stderr, "Error: Cannot use an input file of size 0.\n");
			exit_code = 1;
		}
	} else {
		fprintf(stderr, "Usage:\n\t%s <pairs_file.f64> <answers_file.f64>\n", argv[0]);
		exit_code = 1;
	}
	
	return exit_code;
}
