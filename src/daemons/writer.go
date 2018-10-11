package daemons

import (
	"os"
	//	"encoding/base64"
	"fmt"
	"log"
	//"math/rand"
	"strings"
	"time"
	"unsafe"
)

/*
#cgo CFLAGS: -I../abd -I../sodaw -I../utilities -I../baseprocess -I../ZMQ/include
#cgo LDFLAGS: -L/usr/local/lib -labd  -lsodaw -lbaseprocess -lutilities

#include <../utilities/helpers.h>
#include <algo_utils.h>

#include <abd_client.h>
#include <client.h>
#include <abd_writer.h>
#include <sodaw_writer.h>
*/
import "C"

func writer_daemon(cparameters *C.Parameters, parameters *Parameters) {
	active_chan = make(chan bool, 2)

	//s1 := rand.NewSource(time.Now().UnixNano())
	//ran := rand.New(s1)

	var client_args *C.ClientArgs
	var encoding_info *C.EncodeData

	var payload *C.char
	var opnum int = 1
	var payload_size uint
	for {
		select {
		case active := <-active_chan:
			data.active = active

			if len(data.servers) <= 0 {
				data.active = false
				fmt.Println("please set servers, next startporcess")
				break
			}
			for k, v := range data.servers {
				if v {
					parameters.Ip_list = append(parameters.Ip_list, k)
				}
			}
			parameters.Ipaddresses = strings.Join(parameters.Ip_list, " ")
			parameters.Num_servers = uint(len(parameters.Ip_list))
			copyGoParamToCParam(cparameters, parameters)

			client_args = C.create_ClientArgs(*cparameters)
			encoding_info = C.create_EncodeData(*cparameters)

			ReinitializeParameters()
			LogParameters()
		case active := <-reset_chan:
			data.active = active
			data.write_counter = 0
		default:
			if data.active == true && len(data.servers) > 0 {
				opnum++

				if opnum > 5000 {
					os.Exit(0)
				}

				//rand_wait := rand_wait_time()*int64(time.Millisecond) + int64(time.Millisecond)
				rand_wait := int64(parameters.Wait) * int64(time.Millisecond)
				time.Sleep(time.Duration(rand_wait))

				var abd_data *C.RawData = C.create_RawData()

				fmt.Printf("%s  %d %d %s %s\n", parameters.Server_id, opnum, rand_wait, C.GoString(client_args.servers_str), parameters.port)

				start := time.Now()

				// todo: cyril 2018/10/06
				//payload_size = uint((parameters.Filesize_kb + float64(ran.Intn(100000000)%5)) * 1024)
				payload_size = uint(data.file_size * 1024)

				fmt.Printf("payload %d\n", payload_size)
				payload = C.get_random_data(C.uint(payload_size))

				if data.algorithm == "ABD" {
					abd_data.data = unsafe.Pointer(payload)
					abd_data.data_size = C.ulong(payload_size)
					
					C.ABD_write(C.CString("atomic_object"), C.uint(opnum), abd_data, client_args)
					
					
				}

				

				if data.algorithm == "SODAW" {
					C.SODAW_write(C.CString("atomic_object"), C.uint(opnum), payload, C.uint(payload_size), encoding_info, client_args)
				}


				C.free(unsafe.Pointer(payload))
				C.free(unsafe.Pointer(abd_data))

				elapsed := time.Since(start)

				log.Println(data.run_id, "WRITE", string(data.name), data.write_counter,
					rand_wait/int64(time.Millisecond), elapsed)
				data.write_counter += 1
			} else {
				time.Sleep(5 * 1000 * time.Microsecond)
			}
		}
	}
}

func Writer_process(parameters *Parameters) {
	// This should become part of the standard init function later when we refactor...
	SetupLogging()
	log.Println("INFO", data.name, "Starting")

	data.processType = 1
	//Initialize the parameters
	InitializeParameters(parameters)

	printParameters(parameters)
	// Keep running the server for now
	go HTTP_Server()

	log.Println("INFO\tStarting writer process")
	log.Println("INFO\tTIME in Milliseconds")

	var cparameters C.Parameters
	copyGoParamToCParam(&cparameters, parameters)
	if parameters.Num_servers > 0 {
		data.active = true
		for i := 0; i < int(parameters.Num_servers); i++ {
			data.servers[parameters.Ip_list[i]] = true
		}

	}

	C.printParameters(cparameters)
	writer_daemon(&cparameters, parameters)
}
