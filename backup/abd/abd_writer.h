#ifndef _ABD_WRITER
#define _ABD_WRITER

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
#include "abd_client.h"


//   write_value_phase(obj_name, writer_id,  op_num, sock_to_servers, servers, num_servers, port, payload, size, max_tag);

bool ABD_write(char *obj_name,
               unsigned int op_num,
               RawData *raw_data,
               ClientArgs *client_args
              );
#endif
