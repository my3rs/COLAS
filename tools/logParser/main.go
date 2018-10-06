package main

import (
	"./latency"
	"./utils"
	"bufio"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"strings"
)

type item struct {
	DataSize int
	Inter    float64
}

var (
	mid bool = false
	lat bool = false
)

func main() {
	//latency("ReadLatency.log")
	m := latency.Latency("write_latency.log")
	var newM []item

	fmt.Println()

	file, err := os.Open("/home/cyril/Workspace/logs/write_throughput.log")
	if err != nil {
		log.Fatal(err)
	}

	defer file.Close()

	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)

	for scanner.Scan() {
		line := scanner.Text()
		//fmt.Println(line)
		lr := strings.Split(line, " - ")
		timeLine := strings.Split(lr[0], " ")[1]
		_ = timeLine
		logLine := lr[1]

		var l utils.ThroughputLog
		err = json.Unmarshal([]byte(logLine), &l)
		if err != nil {
			log.Fatal(err)
		}

		if _, exist := m[l.ClientID][l.OpNum]; exist {
			newM = append(newM, item{l.DataSize, m[l.ClientID][l.OpNum].Inter})
		}

		if l.OpNum == 2500 {
			showThroughput(newM)
		} else if l.OpNum == 4500 {
			showThroughput(newM)
		}

	}

	fmt.Println("global")
	sumDataSize := 0
	sumTime := 0.0
	for i := 0; i < len(newM); i++ {
		sumDataSize += newM[i].DataSize
		sumTime += newM[i].Inter
	}
	fmt.Println(float64(sumDataSize) / sumTime)
}

func showThroughput(m []item) {
	sumDataSize := 0
	sumTime := 0.0

	if !mid {
		fmt.Println("50%")
		for i := 0; i < len(m); i++ {
			sumDataSize += m[i].DataSize
			sumTime += m[i].Inter
		}
		fmt.Println(float64(sumDataSize) / sumTime)
		mid = true
	} else if !lat {
		fmt.Println("90%")
		for i := 0; i < len(m); i++ {
			sumDataSize += m[i].DataSize
			sumTime += m[i].Inter
		}
		fmt.Println(float64(sumDataSize) / sumTime)
		lat = true
	}

}
