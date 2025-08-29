#include "haversine_base.h"
#include "haversine_timing.h"
#include "haversine_formula.h"

#define HAVERSINE_PROFILER 1
#include "haversine_profiler.h"

#include "haversine_base.cpp"
#include "haversine_timing.cpp"
#include "haversine_profiler.cpp"
#include "haversine_formula.cpp"

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
	Prof_Function();
	
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

static Parsed_Pairs parse_json_pairs(string input) {
	Prof_Function();
	
	Parsed_Pairs result = {};
	result.ok = true;
	
	Json_Parse_Ctx parser = {};
	parser.input = input; 
	
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
		
		{
			Prof_Block("Json first pass");
			
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
			
			// End of first profiled loop scope
		}
		
		if (result.ok) {
			result.pairs = (Pair *) malloc(sizeof(Pair) * result.pair_count);
		}
		
		restore_json_token(&parser);
		
		{
			Prof_Block("Json second pass");
			
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
			
			// End of second profiled loop scope
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
	begin_profile();
	
	bool ok = true;
	
	if (argc > 1) {
		init_temp_storage();
		
		string json_input = read_entire_file(argv[1]);
		
		auto parsed = parse_json_pairs(json_input);
		if (parsed.ok) {
			f64 avg = 0;
			
			{
				Prof_Block("sum");
				
				f64 sum = 0;
				
				for (int pair_index = 0; pair_index < parsed.pair_count; pair_index += 1) {
					Pair pair = parsed.pairs[pair_index];
					sum += haversine_of_degrees(pair.x0, pair.y0, pair.x1, pair.y1, EARTH_RADIUS);
				}
				
				if (parsed.pair_count != 0) {
					avg = sum / (f64) parsed.pair_count;
				}
			}
			
			{
				Prof_Block("final output");
				
				printf("Input size: %lli\n", json_input.len);
				printf("Pair count: %i\n", parsed.pair_count);
				printf("Haversine avg: %f\n", avg);
			}
			
			end_and_print_profile();
		} else {
			ok = false;
		}
	} else {
		fprintf(stderr, "Usage:\n\t%s [haversine_input.json]\n", argv[0]);
		ok = false;
	}
	
	return !ok;
}

TU_End_Prof_Static_Assert();
