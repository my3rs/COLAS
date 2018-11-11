//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include "czmq.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <zlog.h>
#include <algo_utils.h>
#include "sodaw_client.h"
#include "sodaw_client.h"
#include <base64.h>
#include <rlnc_rs.h>

#define DEBUG_MODE 0

extern int s_interrupted;




Tag *SODAW_write_get_phase(char *obj_name,
	char *client_id,
	unsigned int op_num,
        zsock_t *sock_to_servers,
        unsigned int num_servers) {

    return SODAW_write_get_or_read_get_phase(
            obj_name, client_id, "WRITE_GET",   op_num,
            sock_to_servers,
            num_servers);
}



// this is the write tag value phase of SODAW
void SODAW_write_put_phase (char *obj_name,
        char *writer_id,
        unsigned int op_num,
        zsock_t *sock_to_servers,
        unsigned int num_servers,
        Tag max_tag,    // for read it is max and for write it is new
        EncodeData *encoded_info) {
    // send out the messages to all servers
    char algorithm[BUFSIZE];
    char phase[BUFSIZE];
    char tag_str[BUFSIZE];
    char *value;

    int N = num_servers;
    unsigned int majority =  ceil(((float)num_servers+1)/2);
    int K = majority;
    int symbol_size = SYMBOL_SIZE;

    printf("\t\tencoding data..\n");
    if(encode(encoded_info)==FALSE) {
        printf("Failed to encode data \n");
        exit(EXIT_FAILURE);
    }
    //send encoded data
    printf("\t\tsending encoded data..\n");
    unsigned int round;
    zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };
    tag_to_string(max_tag, tag_str);
    char *types[] = {CLIENTID, OBJECT, ALGORITHM, PHASE, OPNUM, TAG};
    int per_server_payload_size =  encoded_info->num_blocks*encoded_info->encoded_symbol_size;

    send_multisend_servers (sock_to_servers, num_servers,
            encoded_info->encoded_data, per_server_payload_size,
            types,  6, writer_id, obj_name, "SODAW",
            WRITE_PUT, &op_num, tag_str);

    unsigned int responses =0;
    int j =0;
    Tag *tag;
    while (true) {
        //  Tick once per second, pulling in arriving messages
        // zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };
        printf("\t\twaiting for data..\n");
        int rc = zmq_poll(items, 1, -1);
        if(rc < 0 ||  s_interrupted ) {
            printf("Interrupted!\n");
            exit(EXIT_FAILURE);
        }
        printf("\t\treceived data\n");
        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (sock_to_servers);
            //!!
            assert(msg != NULL);
            zlist_t *names = zlist_new();
            //!!
            assert(names);
            assert(zlist_size(names) == 0);

            zhash_t* frames = receive_message_frames_at_client(msg, names);
	        if(DEBUG_MODE) print_out_hash_in_order(frames, names);

            get_string_frame(phase, frames, PHASE);
            round = get_uint_frame(frames, OPNUM);

            if(round==op_num && strcmp(phase, WRITE_PUT)==0) {
                if(DEBUG_MODE) printf("MATCHED MESSAGE!\n");
                responses++;
                if(responses >= majority) {
                    zmsg_destroy (&msg);
                    destroy_frames(frames);
                    zlist_purge(names);
                    zlist_destroy(&names);
                    break;
                }
            } else {
                printf("OLD MESSAGES : (phase%s, opnum%d)\n", phase, op_num);
            }

            zmsg_destroy (&msg);
            destroy_frames(frames);
            zlist_purge(names);
            zlist_destroy(&names);
        }
    }
}



// SODAW write
bool SODAW_write(
        char *obj_name,
        unsigned int op_num ,
        char *payload,
        unsigned int payload_size,
        EncodeData *encoded_data,
        ClientArgs *client_args
        ) 
{
    s_catch_signals();
    int j;
	char s_log[LOGSIZE];
    int num_servers = count_num_servers(client_args->servers_str);


	int rc = zlog_init(NULL);       // $ZLOG_CONF_PATH
	if (rc) {
		printf("zlog init failed\n");
		exit(-1);
	}

	zlog_category_t *category_writer_sodaw = zlog_get_category("writer_sodaw");
	if (!category_writer_sodaw) {
		printf("can not get category_writer_sodaw\n");
		zlog_fini();
		exit(-2);
	}


if(DEBUG_MODE){
    printf("\t\tObj name       : %s\n",obj_name);
    printf("\t\tWriter name    : %s\n",client_args->client_id);
    printf("\t\tOperation num  : %d\n",op_num);
    printf("\t\tSize of data   : %d\n", payload_size);
    printf("\t\tServer string  : %s\n", client_args->servers_str);
    printf("\t\tPort           : %s\n", client_args->port);
    printf("\t\tNum of Servers  : %d\n",num_servers);
}

    void *sock_to_servers= get_socket_servers(client_args);

    printf("\tWRITE_GET (WRITER)\n");
	timer_start();
    Tag *max_tag=  SODAW_write_get_phase(obj_name,
	     client_args->client_id,
            op_num,
            sock_to_servers,
            num_servers
            );
	timer_stop();
	clock_t t_write_get = get_time_inter();

    Tag new_tag;
    new_tag.z = max_tag->z + 1;
    strcpy(new_tag.id, client_args->client_id);
    free(max_tag);

    printf("\tWRITE_PUT (WRITER)\n");
    encoded_data->raw_data = payload;
    encoded_data->raw_data_size = payload_size;

	timer_start();
    SODAW_write_put_phase(
            obj_name,
            client_args->client_id,
            op_num,
            sock_to_servers,
            num_servers,
            new_tag,
            encoded_data
            );
	timer_stop();
	clock_t t_write_put = get_time_inter();


	sprintf(s_log, "{\"client_id\": \"%s\", \"op_num\": %d, \"write_get_time\": %f, \"write_put_time\": %f}", 
		client_args->client_id, op_num, t_write_get, t_write_put);
	zlog_info(category_writer_sodaw, s_log);
	zlog_fini();


    //!! Why was this turned off? We need to destroy the socket or else it becomes a memory leak
    // as we will constantly generate a new one
    /*
       zsocket_destroy(ctx, sock_to_servers);
       zctx_destroy(&ctx);
       destroy_server_names(servers, num_servers);

    */
    //destroy_server_sockets();
	free(payload);
	destroy_EncodeData(encoded_data);
    printf("done\n");
    return true;
}

