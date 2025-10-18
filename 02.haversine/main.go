package main

import (
	"os"
	"log"
	"fmt"
	"flag"
	"math/rand"
    "GiGGioSo/pcourse/distance/haversine"
)

var earth_radius float64 = 6372.8

func main() {
	// CLI flags
	p_help := flag.Bool("help", false, "Print command usage")
	p_uniform := flag.Bool("uniform", false, "Use uniform random distribution")
	p_num := flag.Int("n", 10, "Number of points to generate")
	p_outpath := flag.String("out", "data.json", "Number of points to generate")
	flag.Parse()
	help := *p_help
	uniform := *p_uniform
    num := *p_num
    outpath := *p_outpath
    
	if help {
		flag.PrintDefaults()
		return
	}

    log.Println("uniform:", uniform)
    log.Println("num:", num)
    log.Println("outpath:", outpath)

	outfile, err := os.Create(outpath)
	if err != nil {
		log.Fatal(err)
	}
	defer outfile.Close()

	outfile.Write([]byte("{ \"pairs\": [\n"))
    for i := 0; i < num; i++ {
		x0 := rand.Float64() * 360 - 180
		x1 := rand.Float64() * 360 - 180
		y0 := rand.Float64() * 180 - 90
		y1 := rand.Float64() * 180 - 90
        haversine.ReferenceHaversine(x0, y0, x1, y1, earth_radius)

		ending_string := " },\n"
		if i == num - 1 {
			ending_string = " }\n"
		}

		row := fmt.Sprint("    ",
			"{ \"x0\": ", x0, ", \"y0\": ", y0,
			", \"x1\": ", x1, ", \"y1\": ", y1, ending_string)

        log.Println(row)
		outfile.WriteString(row)
    }
	outfile.Write([]byte("]}\n"))
}
