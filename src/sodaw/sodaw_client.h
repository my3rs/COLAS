#ifndef _SODAW_CLIENT
#define _SODAW_CLIENT

#include "czmq.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <algo_utils.h>
#include <base64.h>
#include "../baseprocess/client.h"


//   write_value_phase(obj_name, writer_id,  op_num, sock_to_servers, servers, num_servers, port, payload, size, max_tag);

zhash_t *receive_message_frames_from_server_SODAW(zmsg_t *msg)  ;
char *number_responses_at_least(zhash_t *received_data, unsigned int atleast);
int dump_recvdata(zhash_t *recv_tab);
Tag *SODAW_write_get_or_read_get_phase(
    char *obj_name,
    char *client_id,
    const char *_phase,
    unsigned int op_num,
    zsock_t *sock_to_servers,
    unsigned int num_servers
);

#endif
