package utils

import (
	"strconv"
	"strings"
)

type (
	LogLine struct {
		ClientID string  `json:"client_id"`
		OpNum    int     `json:"op_num"`
		Latency  float64 `json:"latency"`
		DataSize int     `json:"data_size"`
		Inter    float64 `json:"inter"`
	}

	LogItem struct {
		Latency  float64
		DataSize int
		Inter    float64
	}

	timeCustom struct {
		Hour   uint64
		Minute uint64
		Second uint64
		Ms     uint64
		Us     uint64
	}
)

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
