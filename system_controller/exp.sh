#!/bin/bash

n_reader=""
n_writer=""
n_server=""
file_size=""
read_dist=""
write_dist=""

usage() {
      echo "$0 -r 3 -w 3 -s 3 -f 1024 --read-dist 100 --write-dist 100"
}

while getopts "r:w:s:f:h-:" arg
do
      case $arg in
            r)
                  n_reader=$OPTARG
                  echo "Reader: $n_reader"
                  ;;
		w)
                  n_writer=$OPTARG
                  echo "Writer: $n_writer"
                  ;;
            s)
                  n_server=$OPTARG
                  echo "Server: $n_server"
                  ;;
            f)
                  file_size=$OPTARG
                  echo "File size: $file_size"
                  ;;
            h)
                  usage
                  ;;
            -)
                  case $OPTARG in
                        read-dist=*)
                              read_dist=${OPTARG#*=}
                              echo "Read dist: $read_dist"
                              ;;
                        write-dist=*)
                              write_dist=${OPTARG#*=}
                              echo "Write dist: $write_dist"
                              ;;
                  esac
                  ;;
            ?)
                  usage
                  exit 1
                  ;;
      esac
done

sleep 2

echo "Waiting..."

docker-compose scale reader=$n_reader writer=$n_writer server=$n_server controller=1
./system_management setup
./system_management setfile_size $file_size
./system_management setread_dist const $read_dist
sleep 2
./system_management setwrite_dist const $write_dist

echo "Starting..."
sleep 2
./system_management start

