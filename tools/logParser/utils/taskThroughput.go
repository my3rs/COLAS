package utils

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"
)

func TaskThroughput(filePath, fileName string) {

	var baseTime timeCustom
	var nTasks uint
	firstLine := true

	file, err := os.Open(filePath + fileName)
	if err != nil {
		log.Fatal(err)
	}

	defer file.Close()

	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)

	for scanner.Scan() {
		line := scanner.Text()
		lr := strings.Split(line, " - ")

		timeLine := strings.Split(lr[0], " ")[1]
		_ = timeLine
		logLine := lr[1]
		_ = logLine

		t := parseTimeFromLog(timeLine)
		if firstLine {
			baseTime = t
			firstLine = false
		}
		if GetTimeInterS(baseTime, t) > 60 {
			fmt.Printf("%d\n", nTasks)

			baseTime = t
			nTasks = 0
		} else {
			nTasks++
		}
	}
}

func parseTimeFromLog(t string) timeCustom {
	a := strings.Split(t, ".")
	b := strings.Split(a[0], ":")
	hour, err := strconv.ParseUint(b[0], 10, 64)
	checkError(err)
	min, err := strconv.ParseUint(b[1], 10, 64)
	checkError(err)
	sec, err := strconv.ParseUint(b[2], 10, 64)
	checkError(err)

	return timeCustom{Hour: hour, Minute: min, Second: sec, Ms: 0, Us: 0}
}

func GetTimeInterS(s, e timeCustom) uint64 {
	var left, right uint64
	if timeCompare(s, e) < 0 {
		left = e.Hour*3600 + e.Minute*60 + e.Second
		right = s.Hour*3600 + s.Minute*60 + s.Second
	} else {
		left = s.Hour*3600 + s.Minute*60 + s.Second
		right = e.Hour*3600 + e.Minute*60 + e.Second
	}

	return right - left
}
