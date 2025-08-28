#include "haversine_base.cpp"

//
// Haversine computation
//

union Point {
	struct { f64 x, y; };
	f64 v[2];
};

union Pair {
	struct { f64 x0, y0, x1, y1; };
	struct { Point p0, p1; };
	Point points[2];
};

#define TAU 44.0/7.0
static f64 radians_from_degrees(f64 deg) { return deg * TAU / 360.0; }

#define EARTH_RADIUS 6372.8

static f64 haversine_of_degrees(f64 x0, f64 y0, f64 x1, f64 y1, f64 r) {
	f64 dY = radians_from_degrees(y1 - y0);
	f64 dX = radians_from_degrees(x1 - x0);
	y0 = radians_from_degrees(y0);
	y1 = radians_from_degrees(y1);
	
	f64 sindy2 = sin(dY/2);
	f64 sindx2 = sin(dX/2);
	
	f64 root_term = (sindy2*sindy2) + cos(y0)*cos(y1)*(sindx2*sindx2);
	f64 result = 2*r*asin(sqrt(root_term));
	
	return result;
}

//
// Input generation
//

enum Random_Method : u32 {
	Random_Uniform,
	Random_Cluster,
	Random_COUNT,
};

#define CLUSTER_COUNT  6
#define CLUSTER_RADIUS 10.0

struct Random_Method_State {
	Random_Method method;
	union {
		struct { u8 _unused; } uniform;
		struct {
			i32 max_pairs; // Max pairs per cluster, computed from the total number of pairs
			i32 num_pairs; // Num pairs in the current cluster
			Point origin;
			i32 num_clusters;
			i32 leftover_pairs;
		} cluster;
	};
};

static void init_random_method_state(Random_Method_State *state, Random_Method method, u32 pair_count) {
	memset(state, 0, sizeof(*state));
	state->method = method;
	switch (method) {
		case Random_Uniform: break;
		
		case Random_Cluster: {
			state->cluster.max_pairs = pair_count / CLUSTER_COUNT;
			state->cluster.leftover_pairs = pair_count - state->cluster.max_pairs * CLUSTER_COUNT;
		} break;
	}
}

static Point normalize(Point p) {
	p.x = fmod(p.x, 180.0);
	p.y = fmod(p.y, 90.0);
	return p;
}

static f64 rand_f64() {
	return ((f64) rand() / (f64) RAND_MAX);
}

static f64 rand_f64_range(f64 min, f64 max) {
	return ((f64) rand() / (f64) RAND_MAX) * (max - min) + min;
}

static Pair generate_pair(Random_Method_State *state) {
	Pair pair = {};
	
	switch (state->method) {
		case Random_Uniform: {
			pair.x0 = rand_f64_range(-180.0, 180.0);
			pair.y0 = rand_f64_range(-90.0, 90.0);
			pair.x1 = rand_f64_range(-180.0, 180.0);
			pair.y1 = rand_f64_range(-90.0, 90.0);
		} break;
		
		case Random_Cluster: {
			if (state->cluster.num_clusters < CLUSTER_COUNT) {
				if (state->cluster.num_pairs >= state->cluster.max_pairs) {
					state->cluster.origin.x = rand_f64_range(-180.0, 180.0);
					state->cluster.origin.y = rand_f64_range(-90.0, 90.0);
					
					state->cluster.num_pairs = 0;
					state->cluster.num_clusters += 1;
				}
				
				for (int i = 0; i < array_count(pair.points); i += 1) {
					f64 angle = rand_f64_range(0, 360.0);
					f64 dist = rand_f64_range(0, CLUSTER_RADIUS);
					
					pair.points[i].x = cos(angle) * dist;
					pair.points[i].y = sin(angle) * dist;
					
					pair.points[i].x += state->cluster.origin.x;
					pair.points[i].y += state->cluster.origin.y;
					
					pair.points[i] = normalize(pair.points[i]);
				}
				
				state->cluster.num_pairs += 1;
			} else {
				pair.x0 = rand_f64_range(-180.0, 180.0);
				pair.y0 = rand_f64_range(-90.0, 90.0);
				pair.x1 = rand_f64_range(-180.0, 180.0);
				pair.y1 = rand_f64_range(-90.0, 90.0);
				
				state->cluster.leftover_pairs -= 1;
			}
		} break;
	}
	
	return pair;
}

struct Args {
	Random_Method method;
	i32 seed;
	i32 pair_count;
};

struct Gen_Results {
	f64 agv;
};

static Gen_Results generate_files(Args args) {
	Gen_Results results = {};
	
	free_temp_storage();
	
	srand(args.seed);
	
	Random_Method_State method_state = {};
	init_random_method_state(&method_state, args.method, args.pair_count);
	
	u32 buffer_cap = sizeof(Pair) * args.pair_count + sizeof(f64);
	u8 *buffer = (u8 *) calloc(1, buffer_cap);
	
	f64 sum = 0;
	
	FILE *json = fopen(tsprintf("data_%i_json.json", args.pair_count), "wb");
	if (json) {
		fprintf(json, "{\"pairs\":[\n");
		
		for (int pair_index = 0; pair_index < args.pair_count; pair_index += 1) {
			Pair pair = generate_pair(&method_state);
			
			sum += haversine_of_degrees(pair.x0, pair.y0, pair.x1, pair.y1, EARTH_RADIUS);
			
			*(Pair *) (buffer + sizeof(Pair) * pair_index) = pair;
			
			fprintf(json, "\t{\"x0\":%f,\"y0\":%f,\"x1\":%f,\"y1\":%f}", pair.x0, pair.y0, pair.x1, pair.y1);
			if (pair_index != args.pair_count - 1) {
				fprintf(json, ",");
			}
			fprintf(json, "\n");
		}
		
		fprintf(json, "]}");
		
		f64 avg = sum / (f64) args.pair_count;
		
		*(f64 *) (buffer + sizeof(Pair) * args.pair_count) = avg;
		
		FILE *data = fopen(tsprintf("data_%i_haveranswer.f64", args.pair_count), "wb");
		if (data) {
			fwrite(buffer, 1, buffer_cap, data);
			fclose(data);
		}
		
		fclose(json);
		
		results.agv = avg;
	}
	
	free(buffer);
	
	return results;
}

int main(int argc, char **argv) {
	bool ok = true;
	
	Args args = {};
	bool show_usage = false;
	
	if (argc > 1) {
		if (memcmp(argv[1], sl_expand_pfirst("uniform")) == 0) {
			args.method = Random_Uniform;
		} else if (memcmp(argv[1], sl_expand_pfirst("cluster")) == 0) {
			args.method = Random_Cluster;
		} else {
			show_usage = true;
		}
	} else {
		show_usage = true;
	}
	
	if (argc > 2) {
		args.seed = atoi(argv[2]);
	} else {
		show_usage = true;
	}
	
	if (argc > 3) {
		args.pair_count = atoi(argv[3]);
	} else {
		show_usage = true;
	}
	
	if (show_usage) {
		fprintf(stderr, "Usage:\n\t%s uniform/cluster [seed] [number of coordinate pairs to generate]\n", argv[0]);
		ok = false;
	} else {
		init_temp_storage();
		
		Gen_Results results = generate_files(args);
		switch (args.method) {
			case Random_Uniform: {
				printf("Method: uniform\n");
			} break;
			
			case Random_Cluster: {
				printf("Method: cluster\n");
			} break;
		}
		printf("Random seed: %i\n", args.seed);
		printf("Pair count: %i\n", args.pair_count);
		printf("Expected average: %f\n", results.agv);
	}
	
	return !ok;
}
