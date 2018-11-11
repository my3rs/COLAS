#ifndef __CLIENT__
#define __CLIENT__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <czmq.h>
#include <zmq.h>
#include <czmq_library.h>
#include <string.h>
#include "algo_utils.h"


#define LOGSIZE 1024

void timer_start();
void timer_stop();
clock_t get_time_inter();

typedef struct _client_Args {
    char client_id[BUFSIZE];
    char *servers_str;
    char port[10];
    char port1[10];
} ClientArgs;

typedef struct _s_log_entry {
	float inter_for_throughput;
	float latency;
	unsigned int data_size;
	unsigned int op_num;
} s_log_entry;

void s_signal_handler(int signal_value);

void s_catch_signals();

void *get_socket_servers(ClientArgs *client_args) ;
//void *get_md_socket_dealer(ClientArgs *client_args) ;

void destroy_server_sockets();

zhash_t *receive_message_frames_at_client(zmsg_t *msg, zlist_t *names) ;
void send_multicast_servers(void *sock_to_servers, int num_servers, char *names[],  int n, ...) ;
void send_multisend_servers(void *sock_to_servers, int num_servers, uint8_t **multipart, int size, char *names[],  int n, ...) ;
#endif
