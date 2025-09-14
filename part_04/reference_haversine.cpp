
static f64 square_f64(f64 x) {
    f64 result = x * x;
    return result;
}

static f64 radians_from_degrees_f64(f64 degrees) {
    f64 result = 0.01745329251994329577 * degrees;
    return result;
}

#define EARTH_RADIUS 6372.8

static f64 reference_haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 r) {
    f64 lat1 = y0;
    f64 lat2 = y1;
    f64 lon1 = x0;
    f64 lon2 = x1;
    
    f64 dLat = radians_from_degrees_f64(lat2 - lat1);
    f64 dLon = radians_from_degrees_f64(lon2 - lon1);
    lat1 = radians_from_degrees_f64(lat1);
    lat2 = radians_from_degrees_f64(lat2);
    
    f64 a = square_f64(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*square_f64(sin(dLon/2));
    f64 c = 2.0*asin(sqrt(a));
    
    f64 result = r * c;
    
    return result;
}
