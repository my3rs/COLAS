package main

import (
	"encoding/json"
	"fmt"
	"log"
)

type (
	latencyLog struct {
		Algorithm string `json:"algorithm"`
		ClientID  string `json:"client_id"`
		OpNum     int    `json:"op_num"`
		DataSize  int    `json:"data_size"`
	}

	throughputLog struct {
		Algorithm string `json:"algorithm"`
		ClientID  string `json:"client_id"`
		OpNum     int    `json:"op_num"`
		DataSize  int    `json:"data_size"`
	}
)

// JSON contains a sample string to unmarshal.
var JSON = `{
	"client_id": "reader-478430216",
	"op_num": 2,
	"data_size": 1126
}`

func main() {
	// Unmarshal the JSON string into our variable.
	var l latencyLog
	err := json.Unmarshal([]byte(JSON), &l)
	if err != nil {
		log.Println("ERROR:", err)
		return
	}
	fmt.Println(l)
}
