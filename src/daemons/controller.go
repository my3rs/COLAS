//package main
package daemons

import (
	"log"
)

func Controller_process(parameters *Parameters) {
	f := SetupLogging()
	defer f.Close()
	log.Println("Starting Controller")
	data.processType = 3

	InitializeParameters(parameters)
	LogParameters()

	HTTP_Server()

}
