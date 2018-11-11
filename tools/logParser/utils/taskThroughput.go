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
	// 自定义时间类型，方便后续计算时间差、判断时间先后
	timeCustom struct {
		Hour   uint64
		Minute uint64
		Second uint64
		Ms     uint64
		Us     uint64
	}
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

func GetTimeInterMs(start, end string) float64 {
	var s, e timeCustom

	st := strings.Split(start, ":")
	s.Hour, _ = strconv.ParseUint(st[0], 10, 64)
	s.Minute, _ = strconv.ParseUint(st[1], 10, 64)
	s.Second, _ = strconv.ParseUint(st[2], 10, 64)
	s.Ms, _ = strconv.ParseUint(st[3], 10, 64)
	s.Us, _ = strconv.ParseUint(st[4], 10, 64)

	et := strings.Split(end, ":")
	e.Hour, _ = strconv.ParseUint(et[0], 10, 64)
	e.Minute, _ = strconv.ParseUint(et[1], 10, 64)
	e.Second, _ = strconv.ParseUint(et[2], 10, 64)
	e.Ms, _ = strconv.ParseUint(et[3], 10, 64)
	e.Us, _ = strconv.ParseUint(et[4], 10, 64)

	var left, right float64
	if timeCompare(s, e) < 0 {
		left = float64(e.Hour*3600000 + e.Minute*60000 + e.Second*1000 + e.Ms + e.Us/1000000.0)
		right = float64(s.Hour*3600000 + s.Minute*60000 + s.Second*1000 + s.Ms + s.Us/1000000.0)
	} else {
		left = float64(s.Hour*3600000 + s.Minute*60000 + s.Second*1000 + s.Ms + s.Us/1000000.0)
		right = float64(e.Hour*3600000 + e.Minute*60000 + e.Second*1000 + e.Ms + e.Us/1000000.0)
	}

	return right - left
}

// lhs < rhs : return 1
// lhs > rhs : return -1
// lhs = rhs : no such situation
func timeCompare(lhs, rhs timeCustom) int {
	if lhs.Hour > rhs.Hour {
		return -1
	} else {
		return 1
	}

	if lhs.Minute > rhs.Minute {
		return -1
	} else {
		return 1
	}

	if lhs.Second > rhs.Second {
		return -1
	} else {
		return 1
	}

	if lhs.Ms > rhs.Ms {
		return -1
	} else {
		return 1
	}

	if lhs.Us > rhs.Us {
		return -1
	} else {
		return 1
	}
}
