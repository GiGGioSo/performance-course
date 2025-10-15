package main

import (
	"fmt"
	"flag"
    "GiGGioSo/pcourse/distance/haversine"
)

func main() {
	// CLI flags
	p_help := flag.Bool("help", false, "Print command usage")
	p_uniform := flag.Bool("uniform", false, "Use uniform random distribution")
	p_num := flag.Int("n", 10, "Number of points to generate")
	p_outpath := flag.String("out", "data", "Number of points to generate")
	flag.Parse()
	help := *p_help
	uniform := *p_uniform
    num := *p_num
    outpath := *p_outpath
    
	if help {
		flag.PrintDefaults()
		return
	}

    fmt.Println("uniform:", uniform)
    fmt.Println("num:", num)
    fmt.Println("outpath:", outpath)

    // earth_radius := 6372.8
    earth_radius := 6372.8

    for i := 0; i < num; i++ {
        hav := haversine.ReferenceHaversine(-100, -100, 100, 100, earth_radius)
        fmt.Println("result:", hav)
    }
}
