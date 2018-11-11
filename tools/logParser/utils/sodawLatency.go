package utils

import (
	"bufio"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"strings"
)

type (
	// SODAW Reader 日志的 JSON 结构
	SodawReadLogLine struct {
		ClientID         string  `json:"client_id"`
		OperationID      int     `json:"op_num"`
		ReadGetTime      float64 `json:"read_get_time"`
		ReadValueTime    float64 `json:"read_value_time"`
		ReadCompleteTime float64 `json:"read_complete_time"`
	}

	// 每一条 SODAW Reader 日志的结构化
	SodawReadLogItem struct {
		ReadGetTime      float64
		ReadValueTime    float64
		ReadCompleteTime float64
	}

	// SODAW Writer 日志的 JSON 结构
	SodawWriteLogLine struct {
		ClientID     string  `json:"client_id"`
		OperationID  int     `json:"op_num"`
		WriteGetTime float64 `json:"write_get_time"`
		WritePutTime float64 `json:"write_put_time"`
	}

	// 每一条 SODAW WRITER 日志的结构化
	SodawWriteLogItem struct {
		WriteGetTime float64
		WritePutTime float64
	}
)

var (
	display        = false
	storeToFile    = false
	sodawReadData  map[string]map[int]*SodawReadLogItem  // [ClientID][OperationID]SodawReadLogItem
	sodawWriteData map[string]map[int]*SodawWriteLogItem // [ClientID][OperationID]SodawWriteLogItem

)

func ParseSodawWrite(filePath, fileName string) {
	file, err := os.Open(filePath + fileName)
	if err != nil {
		log.Fatal(err)
	}

	defer file.Close()

	errCount := 0

	sodawWriteData = make(map[string]map[int]*SodawWriteLogItem)

	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)

	for scanner.Scan() {
		line := scanner.Text()
		lr := strings.Split(line, " - ")
		timeLine := strings.Split(lr[0], " ")[1]
		_ = timeLine
		logLine := lr[1]

		var l SodawWriteLogLine
		err = json.Unmarshal([]byte(logLine), &l)
		if err != nil {
			log.Fatal(err)
		}

		if strings.HasPrefix(l.ClientID, "reader") {
			continue
		}

		if sodawWriteData[l.ClientID] == nil {
			sodawWriteData[l.ClientID] = make(map[int]*SodawWriteLogItem)
		}
		if sodawWriteData[l.ClientID][l.OperationID] == nil {
			sodawWriteData[l.ClientID][l.OperationID] = &SodawWriteLogItem{l.WriteGetTime, l.WritePutTime}

		} else {
			errCount++
		}

	}
	staticWrite()
}

func ParseSodawRead(filePath, fileName string) {
	file, err := os.Open(filePath + fileName)
	if err != nil {
		log.Fatal(err)
	}

	defer file.Close()

	errCount := 0

	sodawReadData = make(map[string]map[int]*SodawReadLogItem)

	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)

	for scanner.Scan() {
		line := scanner.Text()
		lr := strings.Split(line, " - ")
		timeLine := strings.Split(lr[0], " ")[1]
		_ = timeLine
		logLine := lr[1]

		var l SodawReadLogLine
		err = json.Unmarshal([]byte(logLine), &l)
		if err != nil {
			log.Fatal(err)
		}

		if sodawReadData[l.ClientID] == nil {
			sodawReadData[l.ClientID] = make(map[int]*SodawReadLogItem)
		}
		if sodawReadData[l.ClientID][l.OperationID] == nil {
			sodawReadData[l.ClientID][l.OperationID] = &SodawReadLogItem{l.ReadGetTime, l.ReadValueTime, l.ReadCompleteTime}

		} else {
			errCount++
		}

	}
	staticRead()
}

func staticRead() {

	count := make(map[string]int)
	sumReadGetTime := make(map[string]float64)
	sumReadValueTime := make(map[string]float64)
	sumReadCompleteTime := make(map[string]float64)
	var sumGet, sumValue, sumComplete float64

	for clientID, e := range sodawReadData {
		if len(sodawReadData[clientID]) < 100 {
			continue
		}

		for _, v := range e {
			count[clientID]++
			sumReadGetTime[clientID] += v.ReadGetTime
			sumReadValueTime[clientID] += v.ReadValueTime
			sumReadCompleteTime[clientID] += v.ReadCompleteTime

		}

		sumGet += sumReadGetTime[clientID] / float64(count[clientID])

		sumValue += sumReadValueTime[clientID] / float64(count[clientID])

		sumComplete += sumReadCompleteTime[clientID] / float64(count[clientID])

	}

	fmt.Printf("read get phase: %.3f us\n", sumGet/float64(len(sodawReadData)))
	fmt.Printf("read value phase: %.3f ms\n", sumValue/float64(len(sodawReadData)))
	fmt.Printf("read complete phase: %.3f us\n", sumComplete/float64(len(sodawReadData)))
	fmt.Println()
}

func staticWrite() {

	count := make(map[string]int)
	//stage := make(map[string]int)
	sumWriteGetTime := make(map[string]float64)
	sumWritePutTime := make(map[string]float64)

	var sumG, sumP float64
	for clientID, e := range sodawWriteData {
		if len(sodawWriteData[clientID]) < 100 {
			continue
		}
		for _, v := range e {
			count[clientID]++

			sumWriteGetTime[clientID] += v.WriteGetTime
			sumWritePutTime[clientID] += v.WritePutTime
		}

		sumG += sumWriteGetTime[clientID] / float64(count[clientID])

		sumP += sumWritePutTime[clientID] / float64(count[clientID])
	}

	fmt.Printf("write get phase: %.3f us\n", sumG/float64(len(sodawWriteData)))
	fmt.Printf("write put phase: %.3f ms\n", sumP/float64(len(sodawWriteData)))
	fmt.Println()
}

func checkError(err error) {
	if err != nil {
		panic(err)
	}
}
