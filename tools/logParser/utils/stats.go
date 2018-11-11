package utils

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"
)

type (
	stats struct {
		MemUsage float64 // MiB
		NetIn    float64 // MB
		NetOut   float64 // MB
	}
)

var (
	statsData map[int]map[string]*stats //[count][ClientID]stats
)

func getMemUseFromLine(line string) float64 {
	var mem float64
	var err error

	if len(line) < 3 {
		return 0
	}
	switch line[len(line)-3:] {
	case "KiB":
		tmp := strings.Split(line, "K")
		mem, err = strconv.ParseFloat(tmp[0], 64)
		checkError(err)
		mem /= 1024
	case "MiB":
		tmp := strings.Split(line, "M")
		mem, err = strconv.ParseFloat(tmp[0], 64)
		checkError(err)
	case "GiB":
		tmp := strings.Split(line, "G")
		mem, err = strconv.ParseFloat(tmp[0], 64)
		checkError(err)
		mem *= 1024
	}

	return mem
}

func getNetIFromLine(line string) float64 {
	var neti float64
	var err error

	switch line[len(line)-2:] {
	case "0B":
		neti = 0
	case "kB":
		tmp := strings.Split(line, "k")
		neti, err = strconv.ParseFloat(tmp[0], 64)
		checkError(err)
		neti /= 1000
	case "MB":
		tmp := strings.Split(line, "M")
		neti, err = strconv.ParseFloat(tmp[0], 64)
		checkError(err)
	case "GB":
		tmp := strings.Split(line, "G")
		neti, err = strconv.ParseFloat(tmp[0], 64)
		checkError(err)
		neti *= 1000
	}
	return neti
}

func getNetOFromLine(line string) float64 {
	var neto float64
	var err error

	switch line[len(line)-2:] {
	case "kB":
		tmp := strings.Split(line, "k")
		neto, err = strconv.ParseFloat(tmp[0], 64)
		checkError(err)
		neto /= 1000
	case "MB":
		tmp := strings.Split(line, "M")
		neto, err = strconv.ParseFloat(tmp[0], 64)
		checkError(err)
	case "GB":
		tmp := strings.Split(line, "G")
		neto, err = strconv.ParseFloat(tmp[0], 64)
		checkError(err)
		neto *= 1000
	}
	return neto
}

func parseLine(line string) (clientId string, s *stats) {
	elements := strings.Fields(line)
	return elements[0], &stats{getMemUseFromLine(elements[2]), getNetIFromLine(elements[6]), getNetOFromLine(elements[8])}
}

func ParseStats(filePath, fileName string) {
	var counter, nServers, maxServers int
	file, err := os.Open(filePath + fileName)
	if err != nil {
		log.Fatal(err)
	}

	defer file.Close()

	statsData = make(map[int]map[string]*stats)
	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)

	for scanner.Scan() {
		line := scanner.Text()
		if strings.HasPrefix(line, "systemcontroller_controller") || strings.HasPrefix(line, "webvirtcloud") {
			continue
		}
		if strings.HasPrefix(line, "system_controller_controller") {
			continue
		}
		if strings.Contains(line, "--") {
			continue
		}

		if strings.HasPrefix(line, "NAME") {
			counter++
			nServers = 0

			statsData[counter] = make(map[string]*stats)
		} else if strings.HasPrefix(line, "system") {
			nServers++

			id, e := parseLine(line)
			statsData[counter][id] = e
			if nServers > maxServers {
				maxServers = nServers
			}
		} else {
			continue
		}
	}

	for k, v := range statsData {
		if len(v) < maxServers {
			delete(statsData, k)
		}
	}

	memUsage := make(map[string]float64)
	var netiw, netis, netir, netor, netow, netos float64
	var nw, ns, nr uint
	for i, m := range statsData {
		_ = i
		for id, s := range m {
			if strings.Contains(id, "writer") {
				nw++
				memUsage["w"] += s.MemUsage
				if s.NetIn > netiw {
					netiw = s.NetIn
				}
				if s.NetOut > netow {
					netow = s.NetOut
				}

			}
			if strings.Contains(id, "reader") {
				nr++
				memUsage["r"] += s.MemUsage
				if s.NetIn > netir {
					netir = s.NetIn
				}
				if s.NetOut > netor {
					netor = s.NetOut
				}

			}
			if strings.Contains(id, "server") {
				ns++
				memUsage["s"] += s.MemUsage
				if s.NetIn > netis {
					netis = s.NetIn
				}
				if s.NetOut > netos {
					netos = s.NetOut
				}
			}
		}
	}

	if nw != 0 {
		fmt.Printf("data input w:\t%.3f MB\n", netiw)
		fmt.Printf("data output w:\t%.3f MB\n", netow)
		fmt.Printf("mem writer:\t%.3f MiB\n", memUsage["w"]/float64(nw))
	}
	if nr != 0 {
		fmt.Printf("data input r:\t%.3f MB\n", netir)
		fmt.Printf("data output r:\t%.3f MB\n", netor)
		fmt.Printf("mem reader:\t%.3f MiB\n", memUsage["r"]/float64(nr))
	}
	if ns != 0 {
		fmt.Printf("data input s:\t%.3f MB\n", netis)
		fmt.Printf("data output s:\t%.3f MB\n", netos)
		fmt.Printf("mem server:\t%.3f MiB\n", memUsage["s"]/float64(ns))
	}

}
