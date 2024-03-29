//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include "abd_client.h"


int s_interrupted;


#define DEBUG_MODE 1



void  ABD_write_value_phase(
    char *obj_name,
    char *client_id,
    unsigned int op_num,
    zsock_t *sock_to_servers,
    unsigned int num_servers,
    RawData *abd_data
) {
    // send out the messages to all servers
    char phase[100];
    char tag_str[100];

    unsigned int majority =  ceil(((float)num_servers+1)/2);

    unsigned int round;
    zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };
    tag_ptr_to_string(abd_data->tag, tag_str);


    char *types[] = {CLIENTID, OBJECT, ALGORITHM, PHASE, OPNUM, TAG, PAYLOAD};
    send_multicast_servers(sock_to_servers, num_servers, types,  7, client_id, obj_name, ABD, WRITE_VALUE, &op_num, tag_str, abd_data) ;

    unsigned int responses =0; 

    while (true) {
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
            assert(msg!=NULL);

            zlist_t *names = zlist_new();
            assert(names!=NULL);
            assert(zlist_size(names)==0);

            zhash_t* frames = receive_message_frames_at_client(msg, names);

            get_string_frame(phase, frames, PHASE);
            round = get_int_frame(frames, OPNUM);

            if(round==op_num && strcmp(phase, WRITE_VALUE)==0) {
                responses++;
                if(DEBUG_MODE) print_out_hash_in_order(frames, names);
                if(responses >= majority) {
                    zmsg_destroy(&msg);
                    destroy_frames(frames);
                    zlist_purge(names);
                    zlist_destroy(&names);
                    break;
                }
                //if(responses >= num_servers) break;
            } else {
                printf("\tOLD MESSAGES : (%s, %d)\n", phase, op_num);
            }
	     zmsg_destroy(&msg);
            destroy_frames(frames);
            zlist_purge(names);
            zlist_destroy(&names);
        }
    }   
    return;
}
