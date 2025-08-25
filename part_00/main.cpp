#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#define TAU 44.0f/7.0f
float radians(float deg) { return deg * TAU / 360.0f; }

typedef struct Pair Pair;
struct Pair { float x0, y0, x1, y1; };

Pair *json_parse(char *json_data, int json_count, int *count) {
	*count = 0;
	
	int json_index = 0;
	sscanf(json_data + json_index, "{\"pairs\":[\n");
	
	for (; json_index < json_count; json_index += 1) {
		if (json_data[json_index] == '\n') { json_index += 1; break; }
	}
	
	for (; json_index < json_count;) {
		float x0 = 0;
		float y0 = 0;
		float x1 = 0;
		float y1 = 0;
		
		sscanf(json_data + json_index, "\t{\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f},\n",
			   &x0, &y0, &x1, &y1);
		
		for (; json_index < json_count; json_index += 1) {
			if (json_data[json_index] == '\n') { json_index += 1; break; }
		}
		
		*count += 1;
	}
	
	sscanf(json_data + json_index, "]}");
	
	*count = 0;
	
	json_index = 0;
	sscanf(json_data + json_index, "{\"pairs\":[\n");
	
	for (; json_index < json_count; json_index += 1) {
		if (json_data[json_index] == '\n') { json_index += 1; break; }
	}
	
	Pair *pairs = (Pair *) malloc(sizeof(Pair) * (*count));
	for (; json_index < json_count;) {
		sscanf(json_data + json_index, "\t{\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f},\n",
			   &pairs[*count].x0, &pairs[*count].y0, &pairs[*count].x1, &pairs[*count].y1);
		
		for (; json_index < json_count; json_index += 1) {
			if (json_data[json_index] == '\n') { json_index += 1; break; }
		}
		
		*count += 1;
	}
	
	sscanf(json_data + json_index, "]}");
	
	return pairs;
}

void haversine_gen(FILE *stream, int pair_count) {
	fprintf(stream, "{\"pairs\":[\n");
	
	srand((unsigned int)time(0));
	
	for (int pair_index = 0; pair_index < pair_count; pair_index += 1) {
		float x0 = ((float) rand() / (float) RAND_MAX) * 360.0f - 180.0f;
		float y0 = ((float) rand() / (float) RAND_MAX) * 360.0f - 180.0f;
		float x1 = ((float) rand() / (float) RAND_MAX) * 360.0f - 180.0f;
		float y1 = ((float) rand() / (float) RAND_MAX) * 360.0f - 180.0f;
		
		fprintf(stream, "\t{\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f},\n", x0, y0, x1, y1);
	}
	
	fprintf(stream, "]}");
}

float haversine_of_degrees(float x0, float y0, float x1, float y1, float r) {
	float dY = radians(y1 - y0);
	float dX = radians(x1 - x0);
	y0 = radians(y0);
	y1 = radians(y1);
	
	float sindy2 = sin(dY/2);
	float sindx2 = sin(dX/2);
	
	float root_term = (sindy2*sindy2) + cos(y0)*cos(y1)*(sindx2*sindx2);
	float result = 2*r*asin(sqrt(root_term));
	
	return result;
}

int main(int argc, char **argv) {
	if (argc > 1 && memcmp(argv[1], "gen", sizeof("gen")) == 0) {
		FILE *json_dest = fopen("data_10000000_flex.json", "wb");
		haversine_gen(json_dest, 10000000);
		fclose(json_dest);
	} else {
		FILE *json_source = fopen("data_10000000_flex.json", "rb");
		fseek(json_source, 0, SEEK_END);
		size_t json_size = ftell(json_source);
		fseek(json_source, 0, SEEK_SET);
		
		char *json_data = (char *) malloc(sizeof(char) * json_size);
		int json_count = fread(json_data, sizeof(char), json_size, json_source);
		
		int count = 0;
		Pair *pairs = json_parse(json_data, json_count, &count);
		fclose(json_source);
		
		float earth_radius_km = 6371.0f;
		float sum = 0;
		for (int i = 0; i < count; i += 1) {
			sum += haversine_of_degrees(pairs[i].x0, pairs[i].y0, pairs[i].x1, pairs[i].y1, earth_radius_km);
		}
		
		float average = sum / (float) count;
	}
	
	return 0;
}
