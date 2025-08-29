#ifndef HAVERSINE_FORMULA_H
#define HAVERSINE_FORMULA_H

#define EARTH_RADIUS 6372.8

static f64 radians_from_degrees(f64 deg);
static f64 haversine_of_degrees(f64 x0, f64 y0, f64 x1, f64 y1, f64 r);

#endif
