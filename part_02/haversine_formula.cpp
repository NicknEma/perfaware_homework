#define EARTH_RADIUS 6372.8

static f64 radians_from_degrees(f64 deg) {
	return deg * 0.01745329251994329577;
}

static f64 haversine_of_degrees(f64 x0, f64 y0, f64 x1, f64 y1, f64 r) {
	f64 dY = radians_from_degrees(y1 - y0);
	f64 dX = radians_from_degrees(x1 - x0);
	y0 = radians_from_degrees(y0);
	y1 = radians_from_degrees(y1);
	
	f64 sindy2 = sin(dY / 2.0);
	f64 sindx2 = sin(dX / 2.0);
	
	f64 root_term = (sindy2*sindy2) + cos(y0)*cos(y1)*(sindx2*sindx2);
	f64 result = 2.0 * r * asin(sqrt(root_term));
	
	return result;
}
