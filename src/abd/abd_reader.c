//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

/**
 * 2018/09/30 : generate json logs. the parser is in COLAS/tools/logParser (current dir name: logDecoder)
 * */

#include <zlog.h>
#include <time.h>
#include "abd_client.h"
#include "abd_reader.h"


#define DEBUG_MSG_PREFIX "\033[31m[DEBUG] | %s | %s |\033[0m ", __FILE__ , __FUNCTION__

extern int s_interrupted;

#ifdef ASLIBRARY
#define DEBUG_MODE 1
#undef DEBUG_MODE


// this fetches the max tag and value

RawData *  ABD_get_max_tag_value_phase(
        char *obj_name,
        unsigned int op_num,
        zsock_t *sock_to_servers,
        unsigned int num_servers,
        log_item* log)
{
    // send out the messages to all servers
    char phase[64];
    unsigned int round;

    Tag *tag = (Tag *)malloc(sizeof(Tag)), *recv_tag = (Tag *)malloc(sizeof(Tag));
    RawData *max_tag_value = (RawData*)malloc(sizeof(RawData));
    tag->z = -1;
    max_tag_value->tag = tag;

    zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };

    char *types[] = {OBJECT, ALGORITHM, PHASE, OPNUM};
    send_multicast_servers(sock_to_servers, num_servers, types,  4, obj_name, "ABD", GET_TAG_VALUE, &op_num) ;

    unsigned int majority = ceil(((float)num_servers+1)/2);
    unsigned int responses = 0;


    clock_t res_start = clock();

    while (true) {
        // Tick once per second, pulling in arriving messages
        printf("\t\twaiting for data...\n");
        int rc = zmq_poll(items, 1, -1);

        if(rc < 0 ||  s_interrupted ) {
            printf("Interrupted!\n");
            exit(EXIT_FAILURE);
        }

        printf("\t\treceived data\n");

        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv(sock_to_servers);
            assert(msg != NULL);

            // TODO: names 是做什么用的？
            zlist_t *names = zlist_new();
            zhash_t *frames = receive_message_frames_at_client(msg, names);



            if (names != NULL) {
                zlist_purge(names);
                zlist_destroy(&names);
                names = NULL;
            }

            //other frames
            get_string_frame(phase, frames, PHASE);
            round = get_int_frame(frames, OPNUM);
            get_tag_frame(frames, recv_tag);


#ifdef DEBUG_MODE
            print_out_hash_in_order(frames, names);
#endif


            if (round == op_num && strcmp(phase, GET_TAG_VALUE) == 0) {
                responses++;

                zframe_t *value_frame = (zframe_t *) zhash_lookup(frames, PAYLOAD);
                assert(value_frame != NULL);


                if (compare_tag_ptrs(tag, recv_tag) < 0) {
                    //free the original frame
                    if (tag->z != -1) {
                        zframe_t *tmp = (zframe_t *) (max_tag_value->data);
                        zframe_destroy(&tmp);
                        max_tag_value->data = NULL;
                    }


                    //duplicate frame
                    zframe_t *dup_value_frame = zframe_dup(value_frame);
                    max_tag_value->data = (void *)dup_value_frame;
                    max_tag_value->data_size = zframe_size(value_frame);

                    // free old data associated with the old tag
//                    if (max_tag_value->data != NULL) {
//                        zframe_t *tmp = (zframe_t *) (max_tag_value->data);
//                        zframe_destroy(&tmp);
//                    }


                    //update max tag
                    memcpy(tag, recv_tag, sizeof(Tag));

                    // update data
//                    max_tag_value->data = (void *) value_frame;
//                    max_tag_value->data_size = zframe_size(dup_value_frame);
//                    max_tag_value->data_size = zframe_size(value_frame);
                }

                log->data_size += max_tag_value->data_size;



                if (responses >= majority) {

                    if (msg != NULL) {
                        zmsg_destroy(&msg);
                        msg = NULL;
                    }

                    if (frames != NULL) {
                        destroy_frames(frames);
                        frames = NULL;
                    }


                    break;
                }




            } else {
                printf("\tOLD MESSAGES : (%s, %d)\n", phase, op_num);
            }


            if (msg != NULL) {
                zmsg_destroy(&msg);
                msg = NULL;
            }
//
//            if (frames != NULL) {
//                destroy_frames(frames);
//                frames = NULL;
//            }

        }
    }

    clock_t res_finish = clock();

    log->inter = (res_finish - res_start) * 1000.0 / CLOCKS_PER_SEC;


    free(recv_tag);
    return max_tag_value;
}


void  ABD_read(
        char *obj_name,
        unsigned int op_num,
        ClientArgs *client_args
) {
    s_catch_signals();

    int num_servers = count_num_servers(client_args->servers_str);
    void *sock_to_servers = get_socket_servers(client_args);
    char s_log[2048];

    log_item log;
    log.inter = .0;
    log.latency = .0;
    log.data_size = 0;
    log.op_num = op_num;

    int rc = zlog_init("/home/cyril/Workspace/config/zlog.conf");
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
    }

    printf("READ %d\n", op_num);

#ifdef DEBUG_MODE
#undef DEBUG_MODE
#endif

#ifdef DEBUG_MODE
    printf("\t\tObj name       : %s\n",obj_name);
    printf("\t\tClient name    : %s\n",client_args->client_id);
    printf("\t\tOperation num  : %d\n",op_num);

    printf("\t\tServer string   : %s\n", client_args->servers_str);
    printf("\t\tPort to Use     : %s\n", client_args->port);
    printf("\t\tNum of Servers  : %d\n",num_servers);
#endif

    printf("\tMAX_TAG_VALUE (READER)\n");


    clock_t read_start = clock();

    RawData *max_tag_value = ABD_get_max_tag_value_phase(
            obj_name,
            op_num,
            sock_to_servers,
            num_servers,
            &log
    );

    clock_t read_finish = clock();
    log.latency = (read_finish - read_start) * 1.0 / CLOCKS_PER_SEC * 1000;

    /** log */
    sprintf(s_log, "{\"client_id\":\"%s\", \"op_num\":%d, \"latency\": %f, \"data_size\":%lu , \"inter\": %f}",
            client_args->client_id, log.op_num, log.data_size, log.latency, log.inter);

    zlog_info(category_reader, s_log);


    printf("\tWRITE_VALUE (READER)\n");

    memset((void*)&log, 0, sizeof(log_item));
    log.op_num = op_num;

    clock_t write_start = clock();
    ABD_write_value_phase(
            obj_name,
            op_num,
            sock_to_servers,
            num_servers,
            max_tag_value,
            &log
    );
    clock_t write_finish = clock();
    log.latency = (write_finish - write_start) * 1.0 / CLOCKS_PER_SEC;

    sprintf(s_log, "{\"client_id\":\"%s\", \"op_num\":%d, \"latency\": %f, \"data_size\":%lu , \"inter\": %f}",
            client_args->client_id, log.op_num, log.data_size, log.latency, log.inter);

    zlog_info(category_writer, s_log);



    zlog_fini();

    zframe_t *tmp = (zframe_t*)(max_tag_value->data);
    zframe_destroy(&tmp);
    free(max_tag_value->tag);
    free(max_tag_value);
}


#endif


//  The main thread simply starts several clients and a server, and then
//  waits for the server to finish.
//#define ASMAIN

#ifdef ASMAIN

int main (void) {
    int i ;

    char *payload = (char *)malloc(100000000*sizeof(char));
    unsigned int size = 100000000*sizeof(char);

    /*
       char *servers[]= {
                         "172.17.0.7", "172.17.0.5",
                         "172.17.0.4", "172.17.0.6",
                         "172.17.0.3"
                       };

    */

    /*
       char *servers[] = {
    "172.17.0.22", "172.17.0.21", "172.17.0.18", "172.17.0.17", "172.17.0.20", "172.17.0.16", "172.17.0.19", "172.17.0.15", "172.17.0.14", "172.17.0.13", "172.17.0.12", "172.17.0.11", "172.17.0.10", "172.17.0.9", "172.17.0.7", "172.17.0.8", "172.17.0.6", "172.17.0.5", "172.17.0.4", "172.17.0.3"
                         };
    */

    char *servers[]= {
        "172.17.0.2"
    };


    unsigned int num_servers = 1;
    char port[]= {PORT};

    char writer_id[] = { "writer_1"};
    char obj_name[] = {OBJECT};

    unsigned int op_num;
    s_catch_signals();

    for( i=0; i < 5; i++) {
        printf("\nWRITE %d\n", i);
        //ABD_write(obj_name, writer_id, i,  payload, size, servers, port);
    }

//   zclock_sleep(50*1000);
    return 0;
}

#endif
