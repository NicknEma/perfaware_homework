#ifndef MATH_H
#define MATH_H

#if _MSC_VER
#include "intrin.h"
#else
#include "x86intrin.h"
#endif

#define PI64 3.14159265358979323846264338327950288419716939937510582097494459230781640628
#define ONE_OVER_SQRT2 0.707106781186547524400844362104849039284835937688474036588339868995366239231053519425193767163820786367506923115456148512462418027925368606322061

static f64 square_f64(f64 x);
static f64 radians_from_degrees_f64(f64 degrees);

static f64 sqrt_sse(f64 x);
static f64 sin_q(f64 x);
static f64 sin_q_half(f64 x);
static f64 sin_q_quarter(f64 x);
static f64 cos_q_quarter(f64 x);

static f64 sin_taylor_fast(f64 x, u32 max_exp);
static f64 sin_taylor_slow(f64 x, u32 max_exp);
static f64 sin_taylor_casey(f64 x, u32 max_exp);
static f64 sin_taylor_horner(f64 x, u32 max_exp);
static f64 sin_taylor_horner_fmadd(f64 x, u32 max_exp);
static f64 sin_taylor_horner_fma(f64 x, u32 max_exp);

static f64 odd_coefficients_table(f64 x, f64 *values, u32 count);

static f64 sin_ce(f64 x);
static f64 asin_ce(f64 x);
static f64 asin_ce_ext_i(f64 x);
static f64 asin_ce_ext_r(f64 x);

///////////////////////////
// Finalized math functions

static f64 sqrt_hv(f64 x);
static f64 sin_hv(f64 x);
static f64 cos_hv(f64 x);
static f64 asin_hv(f64 x);

#endif
