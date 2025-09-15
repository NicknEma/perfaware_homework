#ifndef MATH_H
#define MATH_H

#if _MSC_VER
#include "intrin.h"
#else
#include "x86intrin.h"
#endif

#define PI64 3.14159265358979323846264338327950288419716939937510582097494459230781640628

static f64 square_f64(f64 x);
static f64 radians_from_degrees_f64(f64 degrees);

static f64 sqrt_sse(f64 x);
static f64 sin_q(f64 x);
static f64 sin_q_half(f64 x);
static f64 sin_q_quarter(f64 x);
static f64 cos_q_quarter(f64 x);

#endif
