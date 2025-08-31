
static int clocks_from_effective_address_expression(effective_address_expression *expr) {
	int register_count = 0;
	if (expr->Terms[0].Register.Index != Register_None)
		register_count += 1;
	if (expr->Terms[1].Register.Index != Register_None)
		register_count += 1;
	
	int displacement = expr->Displacement;
	
	int clocks = 0;
	if (register_count == 0) {
		clocks = 6;
	} else if (register_count == 1) {
		if (displacement == 0)
			clocks = 5;
		else
			clocks = 9;
	} else {
		int big = 0;
		if (expr->Terms[0].Register.Index == Register_bp) {
			if (expr->Terms[1].Register.Index == Register_di)
				big = 0;
			else
				big = 1;
		} else {
			if (expr->Terms[1].Register.Index == Register_si)
				big = 0;
			else
				big = 1;
		}
		
		if (displacement == 0) {
			if (!big)
				clocks = 7;
			else
				clocks = 8;
		} else {
			if (!big)
				clocks = 11;
			else
				clocks = 12;
		}
	}
	
	if (expr->Flags & Address_ExplicitSegment)
		clocks += 2;
	
	return clocks;
}
