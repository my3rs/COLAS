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

typedef struct _client_Args {
    char client_id[BUFSIZE];
    char *servers_str;
    char port[10];
    char port1[10];
} ClientArgs;



void s_signal_handler(int signal_value);

void s_catch_signals();

void *get_socket_servers(ClientArgs *client_args) ;
void *get_md_socket_dealer(ClientArgs *client_args) ;


zhash_t *receive_message_frames_at_client(zmsg_t *msg, zlist_t *names) ;
void send_multicast_servers(void *sock_to_servers, int num_servers, char *names[],  int n, ...) ;
void send_multisend_servers(void *sock_to_servers, int num_servers, uint8_t **multipart, int size, char *names[],  int n, ...) ;
#endif
