#ifndef __ABDPROCESSC__
#define __ABDPROCESSC__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "utilities/algo_utils.h"
#include "utilities/client.h"
#include "codes/rlnc_rs.h"

#include "sodaw/sodaw_client.h"
#include "sodaw/sodaw_reader.h"
#include "sodaw/sodaw_writer.h"
#include "sodaw/sodaw_server.h"

#include "soda/soda_reader.h"

#include "abd/abd_client.h"
#include "abd/abd_writer.h"
#include "abd/abd_reader.h"


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


void setDefaults(Parameters *parameters);

unsigned int readParameters(int argc, char *argv[], Parameters *parameters);

void printParameters(Parameters parameters);

Server_Args *get_server_args(Parameters parameters) ;

Server_Status * get_server_status( Parameters parameters) ;
char *get_servers_str(Parameters parameters) ;

void reader_process(Parameters parameters) ;
void writer_process(Parameters parameters) ;
void write_initial_data(Parameters parameters);
char * get_random_data(unsigned int filesize);

bool is_equal(char *payload1, char*payload2, unsigned int size) ;

void destroy_server_args(Server_Args *server_args) ;
#endif