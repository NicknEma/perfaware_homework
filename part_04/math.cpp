
#include "coefficients.inl"

static f64 square_f64(f64 x) {
    f64 y = x * x;
    return y;
}

__declspec(noinline) static f64 cvt_01_to_neg1pos1(f64 b) {
	f64 y = 2*b - 1;
	return y;
}

__declspec(noinline) static f64 sign_f64(f64 x) {
#if 0
	union IEEE_F64 { f64 x; u64 u; };
	IEEE_F64 sign_bit = {};
	sign_bit.x = x;
	sign_bit.u &= (1ULL << 63);
	
	IEEE_F64 value = {};
#if 0
	value.x = x;
	value.x = !!value.x;
	value.u |= sign_bit.u;
#else
	value.x;
#endif
	f64 result = value.x;
#else
	f64 result = (x > 0) - (x < 0);
#endif
	
	return result;
}

static f64 radians_from_degrees_f64(f64 degrees) {
    f64 result = 0.01745329251994329577 * degrees;
    return result;
}

static f64 sqrt_sse(f64 x) {
	// NOTE(ema): Uses intrinsics to generate a sqrt instruction.
	__m128d sse_x = _mm_set_sd(x);
	__m128d sse_y = _mm_sqrt_sd(sse_x, sse_x);
	f64 y = _mm_cvtsd_f64(sse_y);
	return y;
}

static f64 sin_q(f64 x) {
	// NOTE(ema): Approximates sin(x) in the [0, PI] range with a quadratic.
	
	f64 y = (-4/(PI64*PI64))*square_f64(x) + (4/PI64)*x;
	return y;
}

static f64 sin_q_half(f64 x) {
	// NOTE(ema): Approximates sin(x) in the [0, PI] range with a quadratic and
	// then extends the result to the [-PI, PI] range.
	f64 x2 = square_f64(x);
	f64 a = -4/(PI64*PI64);
	f64 b = 4/PI64;
	
	f64 y = a*x2 + b*fabs(x);
	y *= sign_f64(x);
	return y;
}

static f64 sin_q_quarter(f64 x) {
	// NOTE(ema): Approximates sin(x) in the [0, PI/2] range with a quadratic and
	// then extends the result to the [-PI, PI] range.
	f64 x2 = square_f64(x);
	f64 a = -4/(PI64*PI64);
	f64 b = 4/PI64;
	
	f64 y = 0;
	if (x >= 0) {
		if (x >= PI64/2) {
			x = PI64 - x;
			x2 = square_f64(x);
		}
		y = a*x2 + b*x;
	} else {
		if (x <= -PI64/2) {
			x = -PI64 - x;
			x2 = square_f64(x);
		}
		y = -(a*x2 + b*-x);
	}
	return y;
}

static f64 cos_q_quarter(f64 x) {
	// NOTE(ema): Approximates cos(x) in the range [-PI, PI] by conditioning the
	// input to be an input for sin(x).
	if (x > PI64/2) {
		x -= (PI64 + PI64);
	}
	x += PI64/2;
	
	f64 y = sin_q_quarter(x);
	return y;
}

static f64 sin_taylor_fast(f64 x, u32 max_exp) {
	// NOTE(ema): Approximates sin(x) in the range [0, PI/2] using a taylor series.
	f64 sgn = 1;
	f64 num = x;
	f64 den = 1;
	
	f64 y = 0;
	for (u32 exp = 1; exp <= max_exp; exp += 2) {
		y += (sgn * num / den);
		
		num *= x*x;
		den *= (f64)(exp+1) * (f64)(exp+2);
		sgn *= -1;
	}
	
	return y;
}

static f64 factorial(u32 n) {
	f64 result = 1;
	for (u32 i = 1; i <= n; i += 1) {
		result *= (f64)i;
	}
	return result;
}

static f64 sin_taylor_slow(f64 x, u32 max_exp) {
	// NOTE(ema): Approximates sin(x) in the range [0, PI/2] using a taylor series.
	f64 sgn = 1;
	
	f64 y = 0;
	for (u32 exp = 1; exp <= max_exp; exp += 2) {
		y += (sgn * pow(x, (f64)exp) / factorial(exp));
		
		sgn *= -1;
	}
	
	return y;
}

static f64 sin_taylor_coefficient(u32 exp) {
	// NOTE(ema): Computes a coefficient for the sine taylor expansion, given the
	// odd exponent.
	f64 sign = (((exp - 1)/2) % 2) ? -1.0 : 1.0;
    f64 coef = (sign / factorial(exp));
	
    return coef;
}

static f64 sin_taylor_casey(f64 x, u32 max_exp) {
	// NOTE(ema): Approximates sin(x) in the range [0, PI/2] using a taylor series.
	// The loop is structured the way Casey did it, isolating the coefficients.
	f64 y = 0;
	
	f64 x2 = x*x;
	f64 xpow = x;
	for (u32 exp = 1; exp <= max_exp; exp += 2) {
		y += xpow * sin_taylor_coefficient(exp);
		xpow *= x2;
	}
	
	return y;
}

static f64 sin_taylor_horner(f64 x, u32 max_exp) {
	// NOTE(ema): Approximates sin(x) in the range [0, PI/2] using a taylor series.
	// Horner's rule is applied to get more precision.
	f64 y = 0;
	
	f64 x2 = x*x;
	for (u32 inv_exp = 1; inv_exp <= max_exp; inv_exp += 2) {
		u32 exp = max_exp - (inv_exp - 1);
		y = x2*y + sin_taylor_coefficient(exp);
	}
	y *= x;
	
	return y;
}

static f64 sin_taylor_horner_fmadd(f64 x, u32 max_exp) {
	// NOTE(ema): Modification of sin_taylor_horner() that uses intrinsics to
	// generate a fmadd instruction.
	f64 x2 = x*x;
	
	__m128d sse_y  = {};
	__m128d sse_x2 = _mm_set_sd(x2);
	for (u32 inv_exp = 1; inv_exp <= max_exp; inv_exp += 2) {
		u32 exp  = max_exp - (inv_exp - 1);
		f64 coef = sin_taylor_coefficient(exp);
		
		__m128d sse_coef = _mm_set_sd(coef);
#if 0
		sse_y = _mm_fmadd_sd(sse_x2, sse_y, sse_coef);
#else
		sse_y = _mm_add_sd(_mm_mul_sd(sse_x2, sse_y), sse_coef);
#endif
	}
	f64 y = _mm_cvtsd_f64(sse_y);
	y *= x;
	
	return y;
}

static f64 sin_taylor_horner_fma(f64 x, u32 max_exp) {
	// NOTE(ema): Modification of sin_taylor_horner() that uses fma() to
	// generate a fmadd instruction where supported.
	f64 y = 0;
	
	f64 x2 = x*x;
	for (u32 inv_exp = 1; inv_exp <= max_exp; inv_exp += 2) {
		u32 exp = max_exp - (inv_exp - 1);
		y = fma(y, x2, sin_taylor_coefficient(exp));
	}
	y *= x;
	
	return y;
}

static f64 odd_coefficients_table(f64 x, f64 *values, u32 count) {
	// NOTE(ema): Approximate any function given a table of coefficients.
	f64 y = values[count - 1];
	
	f64 x2 = x*x;
	for (u32 index = count - 1; index != 0; index -= 1) {
		f64 c = values[index - 1];
		y = fma(y, x2, c);
	}
	y *= x;
	
	return y;
}

static f64 sin_ce(f64 x) {
	// NOTE(ema): Approximate sin(x) in the range [0, PI/2] using 9 minimax coefficients.
	// The coefficients were donated by Demetri Spanos.
	f64 x2 = x*x;
	
	f64 y = 0x1.883c1c5deffbep-49;
	y = fma(y, x2, -0x1.ae43dc9bf8ba7p-41);
	y = fma(y, x2, 0x1.6123ce513b09fp-33);
	y = fma(y, x2, -0x1.ae6454d960ac4p-26);
	y = fma(y, x2, 0x1.71de3a52aab96p-19);
	y = fma(y, x2, -0x1.a01a01a014eb6p-13);
	y = fma(y, x2, 0x1.11111111110c9p-7);
	y = fma(y, x2, -0x1.5555555555555p-3);
	y = fma(y, x2, 0x1p0);
	y *= x;
	
	return y;
}

static f64 asin_ce(f64 x) {
	// NOTE(ema): Approximate asin(x) in the range [0, 1/sqrt(2)] using 11 minimax coefficients.
	// The coefficients were donated by Demetri Spanos.
	f64 x2 = x*x;
	
	f64 y = 0x1.699a7715830d2p-3;
	y = fma(y, x2, -0x1.2deb335977b56p-2);
	y = fma(y, x2, 0x1.103aa8bb00a4ep-2);
	y = fma(y, x2, -0x1.ba657aa72abeep-4);
	y = fma(y, x2, 0x1.b627b3be92bd4p-5);
	y = fma(y, x2, 0x1.0076fe3314273p-6);
	y = fma(y, x2, 0x1.fe5b240c320ebp-6);
	y = fma(y, x2, 0x1.6d4c8c3659p-5);
	y = fma(y, x2, 0x1.3334fd1dd69f5p-4);
	y = fma(y, x2, 0x1.5555525723f64p-3);
	y = fma(y, x2, 0x1.0000000034db9p0);
	y *= x;
	
	return y;
}

static f64 asin_ce_ext_i(f64 x) {
	// NOTE(ema): Extends asin_ce(x) to the range [0, 1]. It uses to comparisons to pre- and
	// post-condition the values.
	f64 t = x;
	if (x > ONE_OVER_SQRT2) {
		t = sqrt_sse(1 - square_f64(x));
	}
	
	f64 y = asin_ce(t);
	
	if (x > ONE_OVER_SQRT2) {
		y = (PI64/2) - y;
	}
	
	return y;
}

static f64 asin_ce_ext_r(f64 x) {
	// NOTE(ema): Extends asin_ce(x) to the range [0, 1]. It uses "recursion" (kind of) to avoid the
	// second comparison.
	f64 y = 0;
	if (x > ONE_OVER_SQRT2) {
		x = sqrt_sse(1 - square_f64(x));
		y = asin_ce(x);
		y = (PI64/2) - y;
	} else {
		y = asin_ce(x);
	}
	
	return y;
}
