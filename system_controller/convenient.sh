#!/bin/bash
docker-compose scale reader=1 writer=1 server=3 controller=1
#docker-compose scale reader=50 writer=50 server=3 controller=1

./system_management setup
./system_management setfile_size 12
./system_management setread_dist const 100
sleep 2
./system_management setwrite_dist const 100
#sleep 2
./system_management start
#sleep 2 
#./system_management stop
