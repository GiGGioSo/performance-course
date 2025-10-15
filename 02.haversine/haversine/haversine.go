package haversine

import "math"

func square(n float64) (n2 float64) {
    result := n * n
    return result;
}

func radians(degrees float64) (radians float64) {
    result := 0.01745329251994329577 * degrees
    return result
}

// NOTE(casey): EarthRadius is generally expected to be 6372.8
// x0, x1 in [-180, 180]
// y0, y1 in [-90, 90]
func ReferenceHaversine(x0, y0, x1, y1, earth_radius float64) (result float64) {
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
       Instead, it attempts to follow, as closely as possible, the formula used in the real-world
       question on which these homework exercises are loosely based.
    */
    d_lat := radians(y1 - y0);
    d_lon := radians(x1 - x0);
    lat1 := radians(y0);
    lat2 := radians(y1);
    
    a := square(math.Sin(d_lat/2.0)) + math.Cos(lat1)*math.Cos(lat2)*square(math.Sin(d_lon/2.0));
    c := 2.0 * math.Asin(math.Sqrt(a));
    
    result = earth_radius * c;
    
    return result;
}
