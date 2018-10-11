#ifndef _ABD_CLIENT
#define _ABD_CLIENT

#include <czmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <algo_utils.h>
#include <base64.h>
#include "../baseprocess/client.h"

typedef struct _log_item {
    unsigned int op_num;
    float latency;
    size_t data_size;
    float inter;
}log_item;

// this is the write tag value phase of ABD
void ABD_write_value_phase(
    char *obj_name,
    unsigned int op_num,
    zsock_t *sock_to_servers,
    unsigned int num_servers,
    RawData *raw_data,
   log_item *log
   // Tag max_tag   // for read it is max and for write it is new
);

#endif
