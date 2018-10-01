#!/bin/bash
#docker-compose down
#docker commit  tiny_snyder kishori82/dev:COLAS3
#docker-compose up #reader=1 writer=1 server=1 controller=1
docker-compose scale reader=2 writer=2 server=7 controller=1

./system_management setup
./system_management setfile_size 10
./system_management setread_dist const 100
./system_management setwrite_dist const 100
#sleep 2
./system_management start
#sleep 2 
#./system_management stop
