package daemons

/*
#cgo CFLAGS: -I../abd -I../sodaw -I../utilities -I..
#cgo LDFLAGS: -L../abd  -labd  -L../sodaw -lsodaw -lm
#include <abd_client.h>
#include <sodaw_reader.h>
#include <helpers.h>
*/
import "C"

import (
	"fmt"
	"io/ioutil"
	"math/rand"
	"strconv"
	"strings"
	"time"
)

func Set_random_seed(seed int64) {
	rand.Seed(seed)
}

func Generate_random_data(rand_bytes []byte, size int64) error {

	for i := 0; i < (int)(size); i++ {
		v := rand.Uint32()
		rand_bytes[i] = (byte)(v)
	}

	return nil
}

func getCPUSample() (idle, total uint64) {
	contents, err := ioutil.ReadFile("/proc/stat")
	if err != nil {
		return
	}
	lines := strings.Split(string(contents), "\n")
	for _, line := range lines {
		fields := strings.Fields(line)
		if fields[0] == "cpu" {
			numFields := len(fields)
			for i := 1; i < numFields; i++ {
				val, err := strconv.ParseUint(fields[i], 10, 64)
				if err != nil {
					fmt.Println("Error: ", i, fields[i], err)
				}
				total += val // tally up all the numbers to get total ticks
				if i == 4 {  // idle is the 5th field in the cpu line
					idle = val
				}
			}
			return
		}
	}
	return
}

func CpuUsage() float64 {
	idle0, total0 := getCPUSample()
	time.Sleep(3 * time.Second)
	idle1, total1 := getCPUSample()

	idleTicks := float64(idle1 - idle0)
	totalTicks := float64(total1 - total0)
	cpuUsage := 100 * (totalTicks - idleTicks) / totalTicks

	//fmt.Printf("CPU usage is %f%% [busy: %f, total: %f]\n", cpuUsage, totalTicks-idleTicks, totalTicks)
	return cpuUsage
}
