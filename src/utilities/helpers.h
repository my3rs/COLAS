#ifndef __HELPERS__
#define __HELPERS__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "../baseprocess/server.h"
#include "algo_utils.h"
#include "../baseprocess/client.h"
#include "../codes/rlnc_rs.h"


typedef struct _Parameters {
    char **ipaddresses;
    unsigned int num_servers;
    char server_id[BUFSIZE];
    char port[10];
    char port1[10];
    enum Algorithm algorithm;
    enum CodingAlgorithm coding_algorithm;
    int wait;
    float filesize_kb;
    enum ProcessType processtype;
} Parameters;


/*
char **get_memory_for_ipaddresses(int num_ips) {
    char **ipaddresses =  (char **)malloc(num_ips *sizeof(char *));
    int i;
    for( i =0; i < num_ips; i++)  {
        ipaddresses[i] = (char *)malloc(16*sizeof(char));
    }
    return ipaddresses;
}
*/
	
void setDefaults(Parameters *parameters);

EncodeData *create_EncodeData(Parameters parameters) ;
void destroy_EncodeData(EncodeData *encoding_info) ;
void destroy_DecodeData(EncodeData *encoding_info) ;

RawData *create_RawData() ;
void destroy_RawData(RawData *raw_data) ;

Server_Args *get_server_args(Parameters parameters) ;
void destroy_server_args(Server_Args *server_args) ;

Server_Status * get_server_status( Parameters parameters) ;
void destroy_server_status(Server_Status * server_status);


ClientArgs *create_ClientArgs(Parameters parameters) ;
void destroy_ClientArgs(ClientArgs *client_args) ;



char *get_servers_str(Parameters parameters) ;
char * get_random_data(unsigned int filesize);
bool is_equal(char *payload1, char*payload2, unsigned int size) ;

void printParameters(Parameters parameters) ;
#endif
