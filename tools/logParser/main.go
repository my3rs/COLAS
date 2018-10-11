package main

import (
	"./utils"
	"bufio"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"strings"
)

var (
	mid bool = false
	lat bool = false
)

func main() {
	fmt.Println("===========READER===========")
	parse("R.log")

	fmt.Println()
	fmt.Println("===========WRITER===========")
	parse("W.log")
}

func parse(fileName string) map[string]map[int]*utils.LogItem {
	filePath := "/home/cyril/Workspace/logs/"
	file, err := os.Open(filePath + fileName)
	if err != nil {
		log.Fatal(err)
	}

	defer file.Close()

	m := make(map[string]map[int]*utils.LogItem)

	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)

	for scanner.Scan() {
		line := scanner.Text()
		//fmt.Println(line)
		lr := strings.Split(line, " - ")
		timeLine := strings.Split(lr[0], " ")[1]
		_ = timeLine
		logLine := lr[1]

		var l utils.LogLine
		err = json.Unmarshal([]byte(logLine), &l)
		if err != nil {
			log.Fatal(err)
		}

		if m[l.ClientID] == nil {
			m[l.ClientID] = make(map[int]*utils.LogItem)
		}
		if m[l.ClientID][l.OpNum] == nil {
			m[l.ClientID][l.OpNum] = &utils.LogItem{l.Latency, l.DataSize, l.Inter}
		} else {
			log.Fatal("ERROR")
			os.Exit(-1)
		}

	}

	showLatency(m)
	showThroughput(m)

	return m
}

func showLatency(m map[string]map[int]*utils.LogItem) {
	var maxLatency float64 = 0
	var sumLatency float64 = 0
	var num int = 0

	for _, v := range m {
		for _, item := range v {
			num++
			sumLatency += item.Latency

			if item.Latency > maxLatency {
				maxLatency = item.Latency
			}
		}
	}

	fmt.Println("avg latency: ", sumLatency/float64(num), "ms")
	fmt.Println("max latency: ", maxLatency, "ms")

}

func showThroughput(m map[string]map[int]*utils.LogItem) {
	sumDataSize := 0
	sumInter := 0.0
	num := 0

	for _, v := range m {
		for _, item := range v {
			num++
			sumDataSize += item.DataSize
			sumInter += item.Inter
		}
	}

	fmt.Println("throughput: ", float64(sumDataSize)*1000/sumInter/1024.0/1024.0, "MB/s")
}
