package main

import (
	"fmt"
	"flag"
)

func main() {
	// CLI flags
	p_help := flag.Bool("help", false, "Print command usage")
	p_uniform := flag.Bool("uniform", false, "Use uniform random distribution")
	flag.Parse()
	help := *p_help
	uniform := *p_uniform

	if help {
		flag.PrintDefaults()
		return
	}

	fmt.Println("uniform?", uniform)
}
