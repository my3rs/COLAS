package latency

import (
	"../utils"
	"bufio"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"strings"
)

var (
	lat = false
	mid = false
)

func Latency(fileName string) map[string]map[int]*utils.LatencyItem {
	filePath := "/home/cyril/Workspace/logs/"
	file, err := os.Open(filePath + fileName)
	if err != nil {
		log.Fatal(err)
	}

	defer file.Close()

	m := make(map[string]map[int]*utils.LatencyItem)

	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)

	for scanner.Scan() {
		line := scanner.Text()
		//fmt.Println(line)
		lr := strings.Split(line, " - ")
		timeLine := strings.Split(lr[0], " ")[1]
		logLine := lr[1]

		var l utils.LatencyLog
		err = json.Unmarshal([]byte(logLine), &l)
		if err != nil {
			log.Fatal(err)
		}

		if _, exist := m[l.ClientID]; !exist {
			m[l.ClientID] = make(map[int]*utils.LatencyItem)
		}

		if m[l.ClientID][l.OpNum] != nil {
			m[l.ClientID][l.OpNum].Inter = utils.GetTimeInterMs(m[l.ClientID][l.OpNum].Start, timeLine)
		} else {
			m[l.ClientID][l.OpNum] = &utils.LatencyItem{timeLine, 0.0}
		}

		if l.OpNum == 2500 {
			showLatency(m)
		} else if l.OpNum == 4500 {
			showLatency(m)
		}

	}
	fmt.Println("global")
	fmt.Println("max latency: ", getMaxLatency(m))
	fmt.Println("average latency: ", getAvgLatency(m))

	return m
}

func showLatency(m map[string]map[int]*utils.LatencyItem) {
	if !mid {
		fmt.Println("50%")
		fmt.Println("max latency: ", getMaxLatency(m))
		fmt.Println("average latency: ", getAvgLatency(m))
		mid = true
	} else if !lat {
		fmt.Println("90%")
		fmt.Println("max latency: ", getMaxLatency(m))
		fmt.Println("average latency: ", getAvgLatency(m))
		lat = true
	}
}

func getAvgLatency(m map[string]map[int]*utils.LatencyItem) float64 {
	var rev float64 = 0
	var n uint = 0
	for _, v := range m {
		for _, item := range v {
			n++
			rev = (item.Inter-rev)/float64(n+1) + rev
		}
	}
	return rev
}

func getMaxLatency(m map[string]map[int]*utils.LatencyItem) float64 {
	var res float64 = 0
	for _, v := range m {
		for _, item := range v {
			if item.Inter > res {
				res = item.Inter
			}
		}
	}

	return res
}
