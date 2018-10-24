//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include <time.h>
#include <zlog.h>
#include "abd_writer.h"

extern int s_interrupted;


#define DEBUG_MODE 1

#undef DEBUG_MODE

// this fetchers the max tag
Tag *ABD_get_max_tag_phase(
                       char *obj_name, 
                       unsigned int op_num,
                       zsock_t *sock_to_servers,
                       unsigned int num_servers
                      ) {

    // send out the messages to all servers

    char phase[100];
    char tag_str[100];
    unsigned int round;

    zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };

    char *types[] = {OBJECT, ALGORITHM, PHASE, OPNUM};
    send_multicast_servers(sock_to_servers, num_servers, types,  4, obj_name, "ABD", GET_TAG, &op_num) ;

    unsigned int majority =  ceil(((float)num_servers+1)/2);
    unsigned int responses = 0;
    zlist_t *tag_list = zlist_new();

    Tag *tag;

    while (true) {
        //  Tick once per second, pulling in arriving messages
        printf("\t\twaiting for data....\n");
        int rc = zmq_poll(items, 1, -1);
        if(rc < 0 ||  s_interrupted ) {
            printf("Interrupted!\n");
            exit(EXIT_FAILURE);
        }
        printf("\t\treceived data\n");

        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (sock_to_servers);

            zlist_t *names = zlist_new();
            zhash_t* frames = receive_message_frames_at_client(msg, names);


            get_string_frame(phase, frames, PHASE);
            round = get_int_frame(frames, OPNUM);
            get_string_frame(tag_str, frames, TAG);

#ifdef debug_mode
            print_out_hash_in_order(frames, names);
#endif


            if(round == op_num && strcmp(phase, GET_TAG) == 0) {
                responses ++;

                // add tag to list
                tag = (Tag *)malloc(sizeof(TAG));
                string_to_tag(tag_str, tag);
                zlist_append(tag_list, (void *)tag);

                if(responses >= majority) {
					gc_msg_frames_names(msg, frames, names);
                    
                    break;
                }
            } else {
                printf("\tOLD MESSAGES : %s  %d\n", phase, op_num);
            }

			gc_msg_frames_names(msg, frames, names);
           
        }
    }


    //compute the max tag now and return
    // the return value of get_max_tag is duplicated
    Tag *max_tag = get_max_tag(tag_list);

    free_items_in_list(tag_list);
    zlist_destroy(&tag_list);

    return  max_tag;
}

void gc_msg_frames_names(zmsg_t *msg, zhash_t *frames, zlist_t *names) {
    zmsg_destroy(&msg);
    zlist_purge(names);
    zlist_destroy(&names);
	destroy_frames(frames);

    return;
}

// ABD write
bool ABD_write(
    char *obj_name,
    unsigned int op_num ,
    RawData *raw_data,
    ClientArgs *client_args
) {
    s_catch_signals();
    int num_servers = count_num_servers(client_args->servers_str);
    char s_log[2048];
    log_item log;
    void *sock_to_servers = get_socket_servers(client_args);

    log.data_size = 0;
    log.inter = 0;
    log.latency = 0;
    log.op_num = op_num;



    int rc = zlog_init("/home/cyril/Workspace/config/zlog.conf");
    if (rc) {
        printf("zlog init failed\n");
        exit(-1);
    }

    zlog_category_t *category_writer = zlog_get_category("writer");
    if (!category_writer) {
        printf("can not get write_latency category\n");
        zlog_fini();
        exit(-2);
    }



    printf("WRITE %d\n", op_num);
    printf("\tGET_TAG (WRITER)\n");

    clock_t write_start = clock();

    Tag *max_tag =  ABD_get_max_tag_phase(
                                       obj_name,
                                       op_num,
                                       sock_to_servers, 
                                       num_servers
                                       );


    max_tag->z += 1;
    strcpy(max_tag->id, client_args->client_id);
    raw_data->tag = max_tag;
    printf("\tWRITE_VALUE (WRITER)\n");

    ABD_write_value_phase(
                      obj_name, 
                      op_num, 
                      sock_to_servers, 
                      num_servers, 
                      raw_data,
                      &log
                    );

    clock_t write_finish = clock();
    log.latency = (write_finish - write_start) * 1000.0 / CLOCKS_PER_SEC;

    free(max_tag);

    sprintf(s_log, "{\"client_id\":\"%s\", \"op_num\":%d, \"latency\": %f, \"data_size\":%lu , \"inter\": %f}",
            client_args->client_id, log.op_num, log.data_size, log.latency, log.inter);

    zlog_info(category_writer, s_log);

    zlog_fini();
    return true;
}
