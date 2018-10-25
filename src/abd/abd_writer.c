//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include "abd_writer.h"

extern int s_interrupted;


#define DEBUG_MODE 1

// this fethers the max tag
Tag *ABD_get_max_tag_phase(
                       char *obj_name,
                       char *client_id, 
                       unsigned int op_num,
                       zsock_t *sock_to_servers,
                       unsigned int num_servers
                      ) {

    // send out the messages to all servers

    char phase[100];
    char tag_str[100];
    unsigned int round;

    zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };

    char *types[] = {CLIENTID, OBJECT, ALGORITHM, PHASE, OPNUM};
    send_multicast_servers(sock_to_servers, num_servers, types,  5, client_id, obj_name, "ABD", GET_TAG, &op_num) ;

    unsigned int majority =  ceil(((float)num_servers+1)/2);
//     zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };
    unsigned int responses =0;
    zlist_t *tag_list = zlist_new();

    Tag *tag;
    while (true) {
        //  Tick once per second, pulling in arriving messages

        // zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };
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

            if(round==op_num && strcmp(phase, GET_TAG)==0) {
                if(DEBUG_MODE) print_out_hash_in_order(frames, names);

                responses++;
                // add tag to list
                tag = (Tag *)malloc(sizeof(TAG));
                string_to_tag(tag_str, tag);
                zlist_append(tag_list, (void *)tag);

                if(responses >= majority) {
                   zlist_purge(names);
                   zlist_destroy(&names);
                   zmsg_destroy (&msg);
                   destroy_frames(frames);
                   break;

                }
                //if(responses >= num_servers) break;
            } else {
                printf("\tOLD MESSAGES : %s  %d\n", phase, op_num);
            }
            zmsg_destroy (&msg);
            zlist_purge(names);
            zlist_destroy(&names);
            destroy_frames(frames);
        }
    }
    //comute the max tag now and return
    Tag *max_tag = get_max_tag(tag_list);

    free_items_in_list(tag_list);
    zlist_destroy(&tag_list);
    return  max_tag;
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
#ifdef DEBUG_MODE
    printf("\t\tObj name       : %s\n",obj_name);
    printf("\t\tWriter name    : %s\n",client_args->client_id);
    printf("\t\tOperation num  : %d\n",op_num);
    printf("\t\tSize of data   : %d\n",raw_data->data_size);

    printf("\t\tServer string  : %s\n", client_args->servers_str);
    printf("\t\tPort           : %s\n", client_args->port);

    printf("\t\tNum of Servers  : %d\n",num_servers);

    //for(j=0; j < num_servers; j++) {
    //    printf("\t\tServer : %s\n", servers[j]);
    //}
    printf("\n");
#endif

    void *sock_to_servers= get_socket_servers(client_args);

    printf("WRITE %d\n", op_num);
    printf("\tGET_TAG (WRITER)\n");

    Tag *max_tag=  ABD_get_max_tag_phase(
                                       obj_name,
                                       client_args->client_id,
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
                      client_args->client_id,
                      op_num,
                      sock_to_servers,
                      num_servers,
                      raw_data
                    );
    free(max_tag);
    destroy_server_sockets();
    return true;
}


