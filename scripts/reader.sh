#!/bin/bash
#~/COLAS/src/process --process-type 0
#!/bin/bash
#gdb -ex run --args ~/COLAS/src/processc --process-type 1  --ip  172.17.0.1 --ip 172.17.0.2 --ip 172.17.0.3  --filesize  4  --wait 100  --algorithm ABD  --code reed_solomon 
#~/COLAS/src/processc --process-type 0  --ip  172.17.0.2 --ip 172.17.0.3 --ip 172.17.0.4  --filesize  4  --wait 100  --algorithm SODAW  --code reed_solomon 
#~/COLAS/src/processc --process-type 0  --ip  172.17.0.2 --ip 172.17.0.3  --filesize  4  --wait 100  --algorithm SODAW  --code reed_solomon 
#~/COLAS/src/processc --process-type 0  --ip  172.17.0.2 --ip  172.17.0.3 --ip  172.17.0.4 --filesize  4  --wait 100  --algorithm SODAW  --code reed_solomon 
#~/COLAS/src/processc --process-type 0  --ip  172.17.0.1 --filesize  4  --wait 100  --algorithm ABD  --code reed_solomon 
#~/COLAS/src/processc --process-type 0  --ip  172.17.0.1 --filesize  4  --wait 100  --algorithm SODAW --code reed_solomon 
#valgrind --leak-check=full --tool=memcheck --show-leak-kinds=all  ~/COLAS/src/processc --process-type 0  --ip  127.0.0.1 --filesize  4  --wait 100  --algorithm ABD  --code reed_solomon 
#--serverid $1


# TEST
#valgrind --leak-check=full --tool=memcheck --show-leak-kinds=all  ~/COLAS/src/processc --process-type 1  --ip  172.17.0.2 --ip 172.17.0.3 --ip 172.17.0.4  --filesize  4  --wait 100  --algorithm SODAW  --code reed_solomon 
#./COLAS/src/processc --process-type 1  --ip  172.17.0.2 --ip 172.17.0.3 --ip 172.17.0.4  --filesize  4  --wait 100  --algorithm SODAW  --code reed_solomon 
#export LD_LIBRARY_PATH=/usr/local/lib:/home/docker/COLAS/src/abd:/home/docker/COLAS/src/soda:/home/docker/COLAS/src/codes:/home/docker/COLAS/src/sodaw
#./src/process --process-type 1  --ip 127.0.0.1   --filesize  4  --wait 100  --algorithm SODAW  --code reed_solomon 
#valgrind --leak-check=full --tool=memcheck --show-leak-kinds=all  ~/COLAS/src/processc --process-type 0  --ip 172.17.0.2  --filesize  4  --wait 100  --algorithm ABD  --code reed_solomon 
./src/processc --process-type 0  --ip 127.0.0.1  --filesize  4  --wait 100  --algorithm ABD --code reed_solomon 
