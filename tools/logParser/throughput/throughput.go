package throughput

import (
	"../utils"
	"bufio"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"strings"
)

func Throughput(fileName string) {
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
		_ = timeLine
		logLine := lr[1]

		var l utils.ThroughputLog
		err = json.Unmarshal([]byte(logLine), &l)
		if err != nil {
			log.Fatal(err)
		}

		if _, exist := m[l.ClientID]; !exist {
			m[l.ClientID] = make(map[int]*throughputItem)
		}

		if m[l.ClientID][l.OpNum] != nil {
			m[l.ClientID][l.OpNum].Inter = utils.GetTimeInterMs(m[l.ClientID][l.OpNum].Start, timeLine)
			fmt.Println(m[l.ClientID][l.OpNum])
		} else {
			m[l.ClientID][l.OpNum] = &throughputItem{timeLine, 0.0}
			fmt.Println(m[l.ClientID][l.OpNum])

		}
	}

}
