package daemons

import (
	"os"
	"fmt"
	"log"
	"math/rand"
	"strings"
	"time"
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

#include <helpers.h>
*/
import "C"

func reader_daemon(cparameters *C.Parameters, parameters *Parameters) {
	active_chan = make(chan bool, 2)
	//var object_name string = "atomic_object"
	data.active = false

	var client_args *C.ClientArgs
	var encoding_info *C.EncodeData
	var data_read_c *C.char
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
			encoding_info = C.create_EncodeData(*cparameters)

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

				if opnum > 5000 {
					os.Exit(0)
				}

				//rand_wait := rand_wait_time()*int64(time.Millisecond) + int64(time.Millisecond)
				rand_wait := int64(parameters.Wait) * int64(time.Millisecond)
				time.Sleep(time.Duration(rand_wait))

				fmt.Printf("%s %d %d %s %s\n", parameters.Server_id, opnum, rand_wait, C.GoString(client_args.servers_str), parameters.port)

				// call the ABD algorithm

				start := time.Now()
				if data.algorithm == "ABD" {

					abd_data = C.ABD_read(C.CString("atomic_object"), C.uint(opnum), client_args)

					var tmp *C.zframe_t = (*C.zframe_t)(abd_data.data)
					C.zframe_destroy(&tmp)

					C.free(unsafe.Pointer(abd_data.tag))
					C.free(unsafe.Pointer(abd_data))

					/*
						data_read_c = nil
						 C.ABD_read(
						data_read_c = nil /* C.ABD_read(
						C.CString(object_name),
						C.CString(data.name),
						(C.uint)(data.write_counter),
						C.CString(servers_str),
						C.CString(data.port))*/

					//data_read = C.GoString(data_read_c)
					//            C.free(unsafe.Pointer(&data_read_c))
				}

				// call the SODAW algorithm
				if data.algorithm == "SODAW" {
					var payload_read *C.char
					payload_read = C.SODAW_read(C.CString("atomic_object"), C.uint(opnum), encoding_info, client_args)
					_ = payload_read

					/*
						data_read_c = nil  C.SODAW_read(
						C.CString(object_name),
						C.CString(data.name),
						(C.uint)(data.write_counter),
						C.CString(servers_str),
						C.CString(data.port))
					*/

					//	data_read = C.GoString(data_read_c)
				}

				elapsed := time.Since(start)
				//log.Println(data.run_id, "READ", string(data.name), data.write_counter, rand_wait/int64(time.Millisecond), elapsed, len(data_read))
				log.Println(data.run_id, "READ", string(data.name), data.write_counter, rand_wait/int64(time.Millisecond), elapsed)
				time.Sleep(5 * 1000 * time.Microsecond)
				C.free(unsafe.Pointer(data_read_c))

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

	s1 := rand.NewSource(time.Now().UnixNano())
	ran := rand.New(s1)

	payload_size = uint((parameters.Filesize_kb + float64(ran.Intn(100000000)%5)) * 1024)
	payload = C.get_random_data(C.uint(payload_size))

	var client_args *C.ClientArgs = C.create_ClientArgs(*cparameters)
	var encoding_info *C.EncodeData = C.create_EncodeData(*cparameters)
	var abd_data *C.RawData = C.create_RawData(*cparameters)

	if data.algorithm == "ABD" {
		abd_data.data = unsafe.Pointer(payload)
		abd_data.data_size = C.ulong(payload_size)
		C.ABD_write(C.CString("atomic_object"), C.uint(opnum), abd_data, client_args)
	}

	if data.algorithm == "SODAW" {
		C.SODAW_write(C.CString("atomic_object"), C.uint(opnum), payload, C.uint(payload_size), encoding_info, client_args)
	}

	C.free(unsafe.Pointer(payload))
}
