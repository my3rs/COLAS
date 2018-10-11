#!/bin/bash
#docker-compose scale reader=95 writer=5 server=7 controller=1
docker-compose scale reader=50 writer=50 server=7 controller=1

./system_management setup
./system_management setfile_size 10
./system_management setread_dist const 100
./system_management setwrite_dist const 100
#sleep 2
./system_management start
#sleep 2 
#./system_management stop
