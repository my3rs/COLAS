package daemons

import (
	"encoding/base64"
	"fmt"
	"log"
	"time"
)

/*
#cgo CFLAGS: -I../abd  -I../sodaw
#cgo LDFLAGS: -L/usr/local/lib -labd -lsodaw -lzmq -lczmq 
#include <abd_server.h>
#include <stdio.h>

void strcopy(char *a, char *b)
 {


 }
*/
import "C"

func server_logger(status *C.Server_Status) {
	var cpu_use float64

	cpu_use = CpuUsage()
	for true {
		if data.active == true {
			log.Printf("INFO\t%.2f\t%d\t%d\t%d\n",
				cpu_use, int(status.metadata_memory), int(status.data_memory), int(status.network_data))
		}
		time.Sleep(2 * 1000 * time.Millisecond)
		cpu_use = CpuUsage()
	}

}

func server_daemon() {
	active_chan = make(chan bool, 2)

	//	data.active = true
	//	fmt.Println("init file ", 1024*data.init_file_size)

	var status C.Server_Status
	var server_args C.Server_Args

	go server_logger(&status)

	time.Sleep(time.Second)
	//C.server_process(C.CString(data.name),  C.CString(servers_str), C.CString(data.port), init_data, &status)

	for {
		select {
		case active := <-active_chan:
			data.active = active
			InitializeServerParameters(&server_args, &status)
			LogParameters()
			go C.server_process(&server_args, &status)
		case _ = <-reset_chan:
			data.active = false
			data.write_counter = 0
		default:
			if data.active == true && len(data.servers) > 0 {
				time.Sleep(5 * 1000 * time.Microsecond)
			} else {
				time.Sleep(5 * 1000 * time.Microsecond)
			}
		}
	}
}

func InitializeServerParameters(server_args *C.Server_Args, status *C.Server_Status) {
	rand_data := make([]byte, (uint64)(1024*data.init_file_size))
	_ = Generate_random_data(rand_data, int64(1024*data.init_file_size))
	encoded := base64.StdEncoding.EncodeToString(rand_data)
	servers_str := create_server_string_to_C()

	status.network_data = 0
	status.metadata_memory = 0
	status.data_memory = 0
	status.cpu_load = 0
	status.time_point = 0

	server_args.init_data = C.CString(encoded)

	C.strcpy(&server_args.server_id[0], C.CString(data.name))

	server_args.servers_str = C.CString(servers_str)

	C.strcpy(&server_args.port[0], C.CString(data.port))
	server_args.symbol_size = C.int(data.symbol_size)
	server_args.coding_algorithm = C.uint(data.coding_algorithm)
	server_args.N = C.uint(data.N)
	server_args.K = C.uint(data.K)
}

func Server_process(parameters *Parameters) {
	fmt.Println("Starting server\n")
	data.processType = 2
	data.init_file_size = parameters.Filesize_kb

	f := SetupLogging()
	defer f.Close()
	// Run the server for now
	InitializeParameters(parameters)
	printParameters(parameters)

	go HTTP_Server()
	server_daemon()
}
