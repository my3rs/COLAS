//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include "abd_client.h"
#include "abd_reader.h"


extern int s_interrupted;

#ifdef ASLIBRARY
#define DEBUG_MODE 1
#define DEBUG_MSG_PREFIX "\033[31m[DEBUG] | %s | %s |\033[0m ", __FILE__ , __FUNCTION__


// this fetches the max tag and value

void  ABD_get_max_tag_value_phase(
    char *obj_name,
    unsigned int op_num,
    zsock_t *sock_to_servers,
    unsigned int num_servers,
    RawData *max_tag_value      // Don't free this. The caller will do that.
) {

    // send out the messages to all servers

    char phase[64];
    char tag_str[64];
    unsigned int round;

    Tag *tag;

    zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };

    char *types[] = {OBJECT, ALGORITHM, PHASE, OPNUM};
    send_multicast_servers(sock_to_servers, num_servers, types,  4, obj_name, "ABD", GET_TAG_VALUE, &op_num) ;


    unsigned int majority =  ceil(((float)num_servers+1)/2);
    unsigned int responses =0;
    zlist_t *tag_list = zlist_new();

    while (true) {
        //  Tick once per second, pulling in arriving messages

        // zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };
        printf("\t\twaiting for  data...\n");
        int rc = zmq_poll(items, 1, -1);
        if(rc < 0 ||  s_interrupted ) {
            printf("Interrupted!\n");
            exit(EXIT_FAILURE);
        }
        printf("\t\treceived data\n");

        if (items [0].revents & ZMQ_POLLIN) {

            zmsg_t *msg = zmsg_recv(sock_to_servers);

            zlist_t *names = zlist_new();
            zhash_t* frames = receive_message_frames_at_client(msg, names);

            // payload frame
//            zframe_t *value_frame = (zframe_t *)zhash_lookup(frames, PAYLOAD);
//            assert(value_frame !=NULL);

            // other frames
            get_string_frame(phase, frames, PHASE);
            round = get_int_frame(frames, OPNUM);
            get_string_frame(tag_str, frames, TAG);


#ifdef DEBUG_MODE
printf(DEBUG_MSG_PREFIX);
printf("PHASE: \033[32m%s\033[0m OPNUM: %d TAG: %s\n",
        phase,
        round,
        tag_str);
#endif



            if(round==op_num && strcmp(phase, GET_TAG_VALUE)==0) {
                responses++;

                zframe_t *value_frame = (zframe_t *)zhash_lookup(frames, PAYLOAD);
                assert(value_frame != NULL);

                // add tag to list
                tag = (Tag *)malloc(sizeof(Tag));
                string_to_tag(tag_str, tag);
                zlist_append(tag_list, (void *)tag);

                max_tag_value->data = (void *)value_frame;
                max_tag_value->data_size = zframe_size(value_frame);

                if(responses >= majority) {
                   zlist_purge(names);
                   zlist_destroy(&names);
                   zmsg_destroy (&msg);
                   destroy_frames(frames);
                   break;
                }
                //if(responses >= num_servers) break;
            } else {
                printf("\tOLD MESSAGES : (%s, %d)\n", phase, op_num);

            }
            zmsg_destroy (&msg);
        }
    }
    //comute the max tag now and return
    max_tag_value->tag = get_max_tag(tag_list);

    free_items_in_list(tag_list);
    zlist_destroy(&tag_list);
}


RawData  *ABD_read(
    char *obj_name,
    unsigned int op_num ,
    ClientArgs *client_args
) {
    s_catch_signals();

    int num_servers = count_num_servers(client_args->servers_str);
    void *sock_to_servers= get_socket_servers(client_args);

#ifdef DEBUG_MODE
    printf("\t\tObj name       : %s\n",obj_name);
    printf("\t\tWriter name    : %s\n",client_args->client_id);
    printf("\t\tOperation num  : %d\n",op_num);

    printf("\t\tServer string   : %s\n", client_args->servers_str);
    printf("\t\tPort to Use     : %s\n", client_args->port);
    printf("\t\tNum of Servers  : %d\n",num_servers);
#endif


    printf("READ %d\n", op_num);
    printf("\tMAX_TAG_VALUE (READER)\n");

    RawData *max_tag_value = malloc(sizeof(RawData));   // max_tag_value will be freed in process.c

    ABD_get_max_tag_value_phase(obj_name,  
                            op_num, 
                            sock_to_servers,       
                            num_servers, 
                            max_tag_value
                           );

//
//#ifdef DEBUG_MODE
//    printf(DEBUG_MSG_PREFIX);
//    printf("\033[1mmax_tag\033[0m={data=%p, data_size=%d, \033[1mtag\033[0m={z=%d, id=%s}}\n",
//            max_tag_value->data,
//            max_tag_value->data_size,
//            max_tag_value->tag->z,
//            max_tag_value->tag->id);
//#endif


    printf("\tWRITE_VALUE (READER)\n");
    ABD_write_value_phase(
                      obj_name, 
                      op_num, 
                      sock_to_servers,\
                      num_servers, 
                      max_tag_value,
                      *(max_tag_value->tag)
                    );

   return max_tag_value;
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
