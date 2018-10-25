//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include <zlog.h>
#include "abd_client.h"
#include "abd_reader.h"


extern int s_interrupted;

#define DEBUG_MODE 1


void destroy_ABD_Data(RawData *abd_data){
    zframe_t *tmp = (zframe_t*)(abd_data->data);
    zframe_destroy(&tmp);
    free(abd_data->tag);
    free(abd_data);
}


// this fetches the max tag and value

RawData *  ABD_get_max_tag_value_phase(
    char *obj_name,
    char *client_id, 
    unsigned int op_num,
    zsock_t *sock_to_servers,
    unsigned int num_servers,
	s_log_entry* log)
{
    // send out the messages to all servers
    char phase[64];
    unsigned int round;

    Tag *tag = (Tag *)malloc(sizeof(Tag)), *recv_tag = (Tag *)malloc(sizeof(Tag));
    RawData *max_tag_value = (RawData*)malloc(sizeof(RawData));
    tag->z = -1;
    max_tag_value->tag = tag;

    zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };

    char *types[] = {CLIENTID, OBJECT, ALGORITHM, PHASE, OPNUM};
    send_multicast_servers(sock_to_servers, num_servers, types,  5, client_id, obj_name, "ABD", GET_TAG_VALUE, &op_num) ;

    unsigned int majority =  ceil(((float)num_servers+1)/2);
    unsigned int responses =0;
    while (true) {
        //  Tick once per second, pulling in arriving messages
        // zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };
        printf("\t\twaiting for data...\n");
        int rc = zmq_poll(items, 1, -1);
        if(rc < 0 ||  s_interrupted ) {
            printf("Interrupted!\n");
            exit(EXIT_FAILURE);
        }
        printf("\t\treceived data\n");
        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv(sock_to_servers);
	    assert(msg!=NULL);

            zlist_t *names = zlist_new();
            zhash_t* frames = receive_message_frames_at_client(msg, names);

	    //other frames
	    get_string_frame(phase, frames, PHASE);
            round = get_int_frame(frames, OPNUM);
            get_tag_frame(frames, recv_tag);

		if(DEBUG_MODE) print_out_hash_in_order(frames, names);

		if(round==op_num && strcmp(phase, GET_TAG_VALUE)==0) {
            responses++;

            zframe_t *value_frame = (zframe_t *)zhash_lookup(frames, PAYLOAD);
            assert(value_frame !=NULL);

            if(compare_tag_ptrs(tag, recv_tag) < 0){
			//free the original frame
                if(tag->z != -1){
                    zframe_t *tmp = (zframe_t*)(max_tag_value->data);
                    zframe_destroy(&tmp);
					max_tag_value->data = NULL;
                }
                //update max tag
                memcpy(tag, recv_tag, sizeof(Tag));
                //duplicate frame
                zframe_t *dup_value_frame = zframe_dup(value_frame);
                max_tag_value->data = (void *)dup_value_frame;
                max_tag_value->data_size = zframe_size(dup_value_frame);
	    }

            if(responses >= majority) {
                zlist_purge(names);
                zlist_destroy(&names);
                zmsg_destroy (&msg);
                destroy_frames(frames);
                break;
            }

        } else {
            printf("\tOLD MESSAGES : (%s, %d)\n", phase, op_num);
        }
            zlist_purge(names);
            zlist_destroy(&names);
            zmsg_destroy (&msg);
            destroy_frames(frames);
        }
    }
    free(recv_tag);
    return max_tag_value;
}


RawData  *ABD_read(
    char *obj_name,
    unsigned int op_num ,
    ClientArgs *client_args
) {
    s_catch_signals();

    int num_servers = count_num_servers(client_args->servers_str);
    void *sock_to_servers= get_socket_servers(client_args);

	char str_log[LOGSIZE];
	s_log_entry log_line;
	log_line.inter_for_throughput = 0;
	log_line.latency = 0;
	log_line.data_size = 0;
	log_line.op_num = op_num;

	/*int rc = zlog_init("/home/cyril/Workspace/config/zlog.conf");
	if (rc) {
		printf("zlog init failed\n");
		exit(-1);
	}

	zlog_category_t *category_reader = zlog_get_category("reader");
	if (!category_reader) {
		printf("can not get reader category\n");
		zlog_fini();
		exit(-2);
	}
	zlog_category_t *category_writer = zlog_get_category("writer");
	if (!category_writer) {
		printf("can not get writer category\n");
		zlog_fini();
		exit(-2);
	}*/

	if (DEBUG_MODE) {
		printf("\t\tObj name       : %s\n", obj_name);
		printf("\t\tClient name    : %s\n", client_args->client_id);
		printf("\t\tOperation num  : %d\n", op_num);

		printf("\t\tServer string   : %s\n", client_args->servers_str);
		printf("\t\tPort to Use     : %s\n", client_args->port);
		printf("\t\tNum of Servers  : %d\n", num_servers);

		printf("READ %d\n", op_num);
		printf("\tMAX_TAG_VALUE (READER)\n");
	}


    

	clock_t read_start = clock();

    RawData *max_tag_value = ABD_get_max_tag_value_phase(obj_name, client_args->client_id, op_num, sock_to_servers, num_servers, &log_line);

	clock_t read_finish = clock();
	log_line.latency = (read_finish - read_start) * 1.0 / CLOCKS_PER_SEC * 1000;

	if (DEBUG_MODE)
		printf("\tWRITE_VALUE (READER)\n");

	memset((void *)&log_line, 0, sizeof(s_log_entry));
	log_line.op_num = op_num;

	clock_t write_start = clock();
    ABD_write_value_phase(
                      obj_name,
                      client_args->client_id,
                      op_num,
                      sock_to_servers,
                      num_servers,
                      max_tag_value
                    );
	clock_t write_finish = clock();


   destroy_server_sockets();
   return max_tag_value;
}


