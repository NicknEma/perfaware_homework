#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

//
// Base
//

typedef  uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef   int8_t  i8;
typedef  int16_t i16;
typedef  int32_t i32;
typedef  int64_t i64;

typedef    float f32;
typedef   double f64;

#define array_count(a) (sizeof(a)/sizeof((a)[0]))

#define sl_expand_pfirst(s) (s), sizeof(s)
#define sl_expand_sfirst(s) sizeof(s), (s)
#define sl(s) make_string(sizeof(s)-1, (u8 *) (s))
#define slct(s) (string){sizeof(s)-1, (u8 *) (s)}
#define slc(s) {sizeof(s)-1, (u8 *) (s)}

#define ss_expand_pfirst(s) (s).data, (s).len
#define ss_expand_sfirst(s) (s).len, (s).data

struct string {
	i64 len;
	u8 *data;
};

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

struct Temp_Storage {
	u8 *ptr;
	i64 cap;
	i64 pos;
};

static Temp_Storage temp_storage = {};

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
	f64 v[4];
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
// Json utils
//

enum Json_Token_Kind : u32 {
	Json_Token_None,
	Json_Token_Invalid,
	
	Json_Token_Comma,
	Json_Token_Colon,
	Json_Token_Lbrack,
	Json_Token_Rbrack,
	Json_Token_Lbrace,
	Json_Token_Rbrace,
	Json_Token_String,
	Json_Token_Number,
	
	Json_Token_EOI,
};

struct Json_Token {
	Json_Token_Kind kind;
	string str;
};

struct Json_Parse_Ctx {
	string input;
	i32 input_index;
	
	Json_Token curr_token;
	
	i32 save_index;
};

static Json_Token make_json_token(Json_Parse_Ctx *parser);

static Json_Token peek_json_token(Json_Parse_Ctx *parser) {
	if (parser->curr_token.kind == Json_Token_None) {
		parser->curr_token = make_json_token(parser);
	}
	return parser->curr_token;
}

static void consume_json_token(Json_Parse_Ctx *parser) {
	parser->curr_token = make_json_token(parser);
}

static Json_Token make_json_token(Json_Parse_Ctx *parser) {
	Json_Token token = {};
	
	string input = parser->input;
	i32 index = parser->input_index;
	
	for (int i = index; i < input.len && isspace(input.data[i]); i += 1) {
		index += 1;
	}
	
	if (index < input.len) {
		if (token.kind == Json_Token_None && input.data[index] == '"') {
			i32 start = index;
			i32 opl = index;
			
			index += 1;
			opl += 1;
			for (; index < input.len && input.data[index] != '"'; index += 1) {
				opl += 1;
			}
			index += 1;
			opl += 1;
			
			token.kind = Json_Token_String;
			token.str = {opl - start, input.data + start};
		}
		
		if (token.kind == Json_Token_None && (isdigit(input.data[index]) || input.data[index] == '-')) {
			i32 start = index;
			i32 opl = index;
			
			for (; index < input.len && (isdigit(input.data[index]) || input.data[index] == '.' || input.data[index] == '-'); index += 1) {
				opl += 1;
			}
			
			token.kind = Json_Token_Number;
			token.str = {opl - start, input.data + start};
		}
		
		if (token.kind == Json_Token_None) {
			struct Kind_Char_Pair { Json_Token_Kind kind; char c; };
			static Kind_Char_Pair token_map[] = {
				{Json_Token_None, '\0'},
				{Json_Token_Comma, ','},
				{Json_Token_Colon, ':'},
				{Json_Token_Lbrack, '['},
				{Json_Token_Rbrack, ']'},
				{Json_Token_Lbrace, '{'},
				{Json_Token_Rbrace, '}'},
			};
			
			Json_Token_Kind best_match = Json_Token_None;
			for (int i = 0; i < array_count(token_map); i += 1) {
				if (input.data[index] == token_map[i].c) {
					best_match = token_map[i].kind;
				}
			}
			
			if (best_match != Json_Token_None) {
				token.kind = best_match;
				token.str = {1, input.data + index};
				
				index += 1;
			}
		}
	} else {
		token.kind = Json_Token_EOI;
	}
	
	if (token.kind == Json_Token_None) {
		token.kind = Json_Token_Invalid;
	}
	
	parser->input_index = index;
	parser->curr_token = token;
	
	return token;
}

static void save_json_token(Json_Parse_Ctx *parser) {
	parser->save_index = parser->input_index - (i32) parser->curr_token.str.len;
}

static void restore_json_token(Json_Parse_Ctx *parser) {
	parser->input_index = parser->save_index;
	consume_json_token(parser);
}

//
// Assuming Json parser
//

static f64 parse_f64(string str, bool *ok) {
	char *cstr = (char *) temp_push(str.len + 1);
	memcpy(cstr, str.data, str.len);
	cstr[str.len] = '\0';
	
	char *endptr = 0;
	
	errno = 0;
	f64 val = strtod(cstr, &endptr);
	
	if (errno != 0 || endptr[0] != '\0') {
		if (ok) {
			*ok = false;
		}
	}
	
	return val;
}

struct Parsed_Pairs {
	Pair *pairs;
	i32 pair_count;
	
	bool ok;
};

static Parsed_Pairs parse_json_pairs(char *name) {
	Parsed_Pairs result = {};
	result.ok = true;
	
	Json_Parse_Ctx parser = {};
	parser.input = read_entire_file(name);
	
	// {
	Json_Token token = peek_json_token(&parser);
	if (token.kind == Json_Token_Lbrace) {
		consume_json_token(&parser);
	} else {
		result.ok = false;
	}
	
	// "pairs"
	if (result.ok) {
		token = peek_json_token(&parser);
		if (token.kind == Json_Token_String) {
			string contents = {token.str.len - 2, token.str.data + 1};
			if (string_equals(contents, sl("pairs"))) {
				consume_json_token(&parser);
			} else {
				result.ok = false;
			}
		} else {
			result.ok = false;
		}
	}
	
	// :
	if (result.ok) {
		token = peek_json_token(&parser);
		if (token.kind == Json_Token_Colon) {
			consume_json_token(&parser);
		} else {
			result.ok = false;
		}
	}
	
	// [
	if (result.ok) {
		token = peek_json_token(&parser);
		if (token.kind == Json_Token_Lbrack) {
			consume_json_token(&parser);
		} else {
			result.ok = false;
		}
	}
	
	if (result.ok) {
		static string fields[] = {
			slc("x0"), slc("y0"), slc("x1"), slc("y1"),
		};
		
		save_json_token(&parser);
		
		for (;result.ok;) {
			// {
			token = peek_json_token(&parser);
			if (token.kind == Json_Token_Lbrace) {
				consume_json_token(&parser);
			} else {
				result.ok = false;
			}
			
			for (int field_index = 0; field_index < array_count(fields); field_index += 1) {
				// field name
				if (result.ok) {
					token = peek_json_token(&parser);
					if (token.kind == Json_Token_String) {
						string contents = {token.str.len - 2, token.str.data + 1};
						if (string_equals(contents, fields[field_index])) {
							consume_json_token(&parser);
						} else {
							result.ok = false;
						}
					} else {
						result.ok = false;
					}
				}
				
				// :
				token = peek_json_token(&parser);
				if (token.kind == Json_Token_Colon) {
					consume_json_token(&parser);
				} else {
					result.ok = false;
				}
				
				// field value
				token = peek_json_token(&parser);
				if (token.kind == Json_Token_Number) {
					consume_json_token(&parser);
				} else {
					result.ok = false;
				}
				
				if (field_index != array_count(fields) - 1) {
					// ,
					token = peek_json_token(&parser);
					if (token.kind == Json_Token_Comma) {
						consume_json_token(&parser);
					} else {
						result.ok = false;
					}
				}
			}
			
			// }
			token = peek_json_token(&parser);
			if (token.kind == Json_Token_Rbrace) {
				if (result.pair_count == 999999)
				{int xx = 0; (void) xx;}
				consume_json_token(&parser);
			} else {
				result.ok = false;
			}
			
			result.pair_count += 1;
			
			if (result.ok) {
				// , or ]
				token = peek_json_token(&parser);
				if (token.kind == Json_Token_Comma) {
					consume_json_token(&parser);
				} else if (token.kind == Json_Token_Rbrack) {
					break;
				} else {
					result.ok = false;
				}
			}
		}
		
		if (result.ok) {
			result.pairs = (Pair *) malloc(sizeof(Pair) * result.pair_count);
		}
		
		restore_json_token(&parser);
		
		for (int pair_index = 0; pair_index < result.pair_count && result.ok; pair_index += 1) {
			// {
			consume_json_token(&parser);
			
			for (int field_index = 0; field_index < array_count(fields); field_index += 1) {
				// field name
				consume_json_token(&parser);
				
				// :
				consume_json_token(&parser);
				
				// field value
				token = peek_json_token(&parser);
				consume_json_token(&parser);
				result.pairs[pair_index].v[field_index] = parse_f64(token.str, &result.ok);
				
				if (field_index != array_count(fields) - 1) {
					// ,
					consume_json_token(&parser);
				}
			}
			// }
			consume_json_token(&parser);
			
			// , or ]
			consume_json_token(&parser);
		}
	}
	
	// }
	if (result.ok) {
		token = peek_json_token(&parser);
		if (token.kind == Json_Token_Rbrace) {
			consume_json_token(&parser);
		} else {
			result.ok = false;
		}
	}
	
	if (!result.ok) {
		fprintf(stderr, "Invalid json\n");
	}
	
	return result;
}

//
// Main program
//

int main(int argc, char **argv) {
	bool ok = true;
	
	char *json_name = 0;
	bool show_usage = false;
	
	if (argc > 1) {
		json_name = argv[1];
	} else {
		show_usage = true;
	}
	
	if (show_usage) {
		fprintf(stderr, "Usage:\n\t%s [haversine_input.json]\n", argv[0]);
		ok = false;
	} else {
		init_temp_storage();
		
		auto parsed = parse_json_pairs(json_name);
		if (parsed.ok) {
			printf("Pair count: %i\n", parsed.pair_count);
			
			f64 sum = 0;
			
			for (int pair_index = 0; pair_index < parsed.pair_count; pair_index += 1) {
				Pair pair = parsed.pairs[pair_index];
				sum += haversine_of_degrees(pair.x0, pair.y0, pair.x1, pair.y1, EARTH_RADIUS);
			}
			
			f64 avg = sum / (f64) parsed.pair_count;
			
			printf("Haversine avg: %f\n", avg);
		} else {
			ok = false;
		}
	}
	
	return !ok;
}
