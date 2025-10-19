package main

import (
	"os"
	"log"
	"fmt"
	"flag"
	"encoding/binary"
	"math/rand"
    "GiGGioSo/pcourse/distance/haversine"
)

var earth_radius float64 = 6372.8

var generator_width, generator_height float64 =
	rand_range(40, 80), rand_range(20, 40)
var generator1_x, generator1_y float64 =
	rand_range(
		-180 + generator_width/2,
		180 - generator_width/2),
	rand_range(
		-90 + generator_height/2,
		90 - generator_height/2)
var generator2_x, generator2_y float64 =
	rand_range(
		-180 + generator_width/2,
		180 - generator_width/2),
	rand_range(
		-90 + generator_height/2,
		90 - generator_height/2)

func main() {
	// CLI flags
	p_help := flag.Bool("help", false, "Print command usage")
	p_uniform := flag.Bool("uniform", false, "Use uniform random distribution")
	p_num := flag.Int("n", 10, "Number of points to generate")
	p_outpath := flag.String("out", "data.json", "Path of the output file")
	p_debugpath := flag.String("debug", "debug.float64", "Path of the debug file")
	flag.Parse()
	help := *p_help
	uniform := *p_uniform
    num := *p_num
    outpath := *p_outpath
    debugpath := *p_debugpath
    
	if help {
		flag.PrintDefaults()
		return
	}

    log.Println("uniform:", uniform)
    log.Println("num:", num)
    log.Println("outpath:", outpath)
    log.Println("debugpath:", debugpath)

	outfile, err := os.Create(outpath)
	if err != nil { log.Fatal(err) }
	defer outfile.Close()

	debugfile, err := os.Create(debugpath)
	if err != nil { log.Fatal(err) }
	defer debugfile.Close()

	var average_hav float64 = 0.0

	_, err = outfile.Write([]byte("{ \"pairs\": [\n"))
	if err != nil { log.Fatal(err) }
    for i := 0; i < num; i++ {
		x0, y0, x1, y1 := generate_positions(uniform)
		hav := haversine.ReferenceHaversine(x0, y0, x1, y1, earth_radius)

		row := fmt.Sprintf("    { \"x0\": %.6f, \"y0\": %.6f, \"x1\": %.6f, \"y1\": %.6f", x0, y0, x1, y1)
		if i == num - 1 {
			row += " }\n"
		} else {
			row += " },\n"
		}

		average_hav += (hav - average_hav) / float64(i+1)

		err = binary.Write(debugfile, binary.LittleEndian, hav)
		if err != nil { log.Fatal(err) }

		_, err = outfile.WriteString(row)
		if err != nil { log.Fatal(err) }
    }
	_, err = outfile.Write([]byte("]}\n"))
	if err != nil { log.Fatal(err) }

	err = binary.Write(debugfile, binary.LittleEndian, average_hav)
	if err != nil { log.Fatal(err) }

	log.Printf("haverage: %.6f\n", average_hav);
}

func generate_positions(uniform bool) (x0, y0, x1, y1 float64) {
	if uniform {
		x0 = rand_range(-180, 180)
		y0 = rand_range(-90, 90)
		x1 = rand_range(-180, 180)
		y1 = rand_range(-90, 90)
	} else {
		x0 = rand_range(
			generator1_x - generator_width / 2,
			generator1_x + generator_width / 2)
		y0 = rand_range(
			generator1_y - generator_height / 2,
			generator1_y + generator_height / 2)
		x1 = rand_range(
			generator2_x - generator_width / 2,
			generator2_x + generator_width / 2)
		y1 = rand_range(
			generator2_y - generator_height / 2,
			generator2_y + generator_height / 2)
	}
	return x0, y0, x1, y1
}

func rand_range(from, to float64) (value float64) {
	value = rand.Float64() * (to - from) + from
	return value
}

