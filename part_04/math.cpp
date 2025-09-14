#if _MSC_VER
#include "intrin.h"
#else
#include "x86intrin.h"
#endif

#define PI64 3.14159265358979323846264338327950288419716939937510582097494459230781640628

static f64 square_f64(f64 x) {
    f64 result = x * x;
    return result;
}

__declspec(noinline) static f64 cvt_01_to_neg1pos1(f64 b) {
	f64 result = 2*b - 1;
	return result;
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

static f64 hav_sqrt(f64 x) {
	__m128d mx = _mm_set_sd(x);
	__m128d mz = _mm_set_sd(0);
	__m128d my = _mm_sqrt_sd(mz, mx);
	f64 y = _mm_cvtsd_f64(my);
	return y;
}

static f64 hav_asin(f64 x) {
	f64 y = x;
	return y;
}

static f64 sin_q(f64 x) {
	// NOTE(ema): Approximates sin in the [0, PI] range with a quadratic.
	
	f64 y = (-4/(PI64*PI64))*square_f64(x) + (4/PI64)*x;
	return y;
}

static f64 sin_q_half(f64 x) {
	// NOTE(ema): Approximates sin in the [0, PI] range with a quadratic and
	// then extends the result to the [-PI, PI] range.
	f64 x2 = square_f64(x);
	f64 a = -4/(PI64*PI64);
	f64 b = 4/PI64;
	
	f64 y = a*x2 + b*fabs(x);
	y *= sign_f64(x);
	return y;
}

static f64 sin_q_quarter(f64 x) {
	// NOTE(ema): More range reduction
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
	if (x > PI64/2) {
		x -= (PI64 + PI64/2);
	}
	x += PI64/2;
	
	f64 y = sin_q_quarter(x);
	return y;
}
