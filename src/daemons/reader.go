package daemons

import (
	"os"
	"fmt"
	"log"
	"strings"
	"time"
	"strconv"
	"unsafe"
)

/*
#cgo CFLAGS: -I../abd -I../sodaw -I../utilities -I../baseprocess -I../ZMQ/include
#cgo LDFLAGS: -L/usr/local/lib -labd  -lsodaw
#include <abd_client.h>
#include <client.h>
#include <sodaw_reader.h>
#include <abd_reader.h>

#include <sodaw_writer.h>
#include <abd_writer.h>

#include <../utilities/helpers.h>
*/
import "C"

func reader_daemon(cparameters *C.Parameters, parameters *Parameters) {
	active_chan = make(chan bool, 2)
	data.active = false

	var client_args *C.ClientArgs
	var encoding_info *C.EncodeData
	var abd_data *C.RawData
	var opnum int = 0


	for {
		select {
		case active := <-active_chan: //start
			data.active = active
			if len(data.servers) <= 0 {
				data.active = false
				fmt.Println("please set servers,next startporcess")
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

			ReinitializeParameters()
			LogParameters()
			write_initial_data(cparameters, parameters)
			opnum = 0
		case active := <-reset_chan:
			data.active = active
			data.write_counter = 0
		default:
			if data.active == true && len(data.servers) > 0 {
				opnum++

				rand_wait := int64(parameters.Wait) * int64(time.Millisecond)
				time.Sleep(time.Duration(rand_wait))

				if opnum > 2000 {
					os.Exit(0)
				}

				if len(data.inter_read_wait_distribution) == 2 {
					read_distribution := data.inter_read_wait_distribution[0]
					read_distance, _ := strconv.Atoi(data.inter_read_wait_distribution[1])
					fmt.Println("[PARAMETERS DEBUG] Preparing to sleep...")

					if read_distribution == "const" {
						time.Sleep(time.Duration(read_distance) * 1000 * time.Microsecond)
						fmt.Println("[PARAMETERS DEBUG] Sleep...")
					}
				}

				fmt.Printf("%s %d %d %s %s\n", parameters.Server_id, opnum, rand_wait, C.GoString(client_args.servers_str), parameters.port)

				// call the ABD algorithm
				start := time.Now()
				if data.algorithm == "ABD" {
					abd_data = C.ABD_read(C.CString("atomic_object"), C.uint(opnum), client_args)
					C.destroy_ABD_Data(abd_data)
				}

				// call the SODAW algorithm
				if data.algorithm == "SODAW" {
					encoding_info = C.create_EncodeData(*cparameters)
					C.SODAW_read(C.CString("atomic_object"), C.uint(opnum), encoding_info, client_args)
					C.destroy_DecodeData(encoding_info)
				}

				elapsed := time.Since(start)
				log.Println(data.run_id, "READ", string(data.name), data.write_counter, rand_wait/int64(time.Millisecond), elapsed)

				data.write_counter += 1

				
			} else {
				time.Sleep(5 * time.Microsecond)
			}
		}
	}
}

func Reader_process(parameters *Parameters) {

	// This should become part of the standard init function later when we refactor...
	data.processType = 0
	SetupLogging()
	fmt.Println("INFO\tStarting reader\n")

	InitializeParameters(parameters)
	printParameters(parameters)

	go HTTP_Server()

	log.Println("INFO\tStarting reader process\n")
	log.Println("INFO\tTIME in Milliseconds\n")

	var cparameters C.Parameters
	copyGoParamToCParam(&cparameters, parameters)

	if parameters.Num_servers > 0 {
		data.active = true
		for i := 0; i < int(parameters.Num_servers); i++ {
			data.servers[parameters.Ip_list[i]] = true
		}
	}
	C.printParameters(cparameters)

	reader_daemon(&cparameters, parameters)
}

func write_initial_data(cparameters *C.Parameters, parameters *Parameters) {

	var opnum int = 1
	var payload_size uint
	var payload *C.char

	// payload_size = uint(parameters.Filesize_kb * 1024)
	payload_size = uint(data.file_size * 1024)
	payload = C.get_random_data(C.uint(payload_size))

	var client_args *C.ClientArgs = C.create_ClientArgs(*cparameters)
	var encoding_info *C.EncodeData = C.create_EncodeData(*cparameters)
	var abd_data *C.RawData = C.create_RawData()

	if data.algorithm == "ABD" {
		abd_data.data = unsafe.Pointer(payload)
		abd_data.data_size = C.uint(payload_size)
		C.ABD_write(C.CString("atomic_object"), C.uint(opnum), abd_data, client_args)
	} else if data.algorithm == "SODAW" {
		C.SODAW_write(C.CString("atomic_object"), C.uint(opnum), payload, C.uint(payload_size), encoding_info, client_args)
	}

	C.free(unsafe.Pointer(payload))
	C.free(unsafe.Pointer(abd_data))
}
