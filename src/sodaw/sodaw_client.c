//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include "sodaw_client.h"

#define DEBUG_MODE 1
extern int s_interrupted;
#ifdef ASLIBRARY

// this fethers the max tag
Tag *SODAW_write_get_or_read_get_phase (char *obj_name,
                                        const char *_phase,
                                        unsigned int op_num,
                                        zsock_t *sock_to_servers,
                                        unsigned int num_servers) {
    // send out the messages to all servers
    char algorithm[BUFSIZE];
    char phase[BUFSIZE];
    char tag_str[BUFSIZE];
    unsigned int round;

    zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };

    char *types[] = {OBJECT, ALGORITHM, PHASE, OPNUM};

    send_multicast_servers(sock_to_servers, num_servers, types, 4, obj_name, SODAW, _phase, &op_num) ;

    unsigned int majority =  ceil(((float)num_servers+1)/2);
    unsigned int responses =0;
    zlist_t *tag_list = zlist_new();

    Tag *tag;
    while (true) {

        printf("\t\twaiting for data..\n");
        int rc = zmq_poll(items, 1, -1);
        if(rc < 0 ||  s_interrupted ) {
            printf("Interrupted!\n");
            exit(EXIT_FAILURE);
        }
        printf("\t\treceived data\n");
        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg =   (sock_to_servers);
            assert(msg != NULL);

            zlist_t *names = zlist_new();
            assert(names);
            assert(zlist_size(names) == 0);

            zhash_t* frames = receive_message_frames_at_client(msg, names);

            get_string_frame(phase, frames, PHASE);

            zframe_t *r = zhash_lookup(frames, PHASE);
            round = get_int_frame(frames, OPNUM);

            zframe_t *s = zhash_lookup(frames, TAG);
            printf("CURRENT OPNUM %d - INCOMING OPNUM %d \n", op_num, round);

            get_string_frame(tag_str, frames, TAG);

            if(round==op_num && strcmp(phase, _phase)==0) {
                if(DEBUG_MODE) print_out_hash_in_order(frames, names);

                responses++;
                // add tag to list
                tag = (Tag *)malloc(sizeof(Tag));
                string_to_tag(tag_str, tag);
                zlist_append(tag_list, (void *)tag);

                if(responses >= majority) {
                    zmsg_destroy (&msg);
                    zlist_purge(names);
                    zlist_destroy(&names);
                    destroy_frames(frames);
                    break;
                }
            }
            zmsg_destroy (&msg);
            zlist_purge(names);
            zlist_destroy(&names);
            destroy_frames(frames);
        }
    }

    //compute the max tag now and return
    Tag *max_tag = get_max_tag(tag_list);
    free_items_in_list(tag_list);
    zlist_destroy(&tag_list);

    return  max_tag;
}

// this fetches the max tag and value
char *number_responses_at_least(zhash_t *received_data, unsigned int atleast) {
    zlist_t *keys = zhash_keys(received_data);
    char *key;
    for(  key = (char *)zlist_first(keys);  key != NULL; key = (char *)zlist_next(keys)) {
        int count = zlist_size(zhash_lookup(received_data, key));
        if( count >= atleast) {
            return key;
        }
    }
    return NULL;
}


zhash_t *receive_message_frames_from_server_SODAW(zmsg_t *msg)  {
    char algorithm_name[BUFSIZE];
    char phase_name[BUFSIZE];
    char buf[BUFSIZE];
    zhash_t *frames = zhash_new();

    insertIntoHash(OBJECT, msg, frames);
    get_string_frame(buf, frames, OBJECT);
    printf("\tobject  %s\n", buf);

    insertIntoHash(ALGORITHM, msg, frames);
    get_string_frame(algorithm_name, frames, ALGORITHM);
    printf("\talgorithm  %s\n", algorithm_name);

    insertIntoHash(PHASE, msg, frames);
    get_string_frame(phase_name, frames, PHASE);
    printf("\tphase naum %s\n", phase_name);

    if( strcmp(algorithm_name, SODAW) ==0 ) {
        if( strcmp(phase_name, WRITE_GET) ==0 ) {
            insertIntoHash(OPNUM, msg, frames);
            insertIntoHash(TAG, msg, frames);
            get_string_frame(buf, frames, TAG);
        }

        if( strcmp(phase_name, WRITE_PUT) ==0 ) {
            insertIntoHash(OPNUM, msg, frames);
            insertIntoHash(TAG, msg, frames);
            get_string_frame(buf, frames, TAG);
        }

        if( strcmp(phase_name, READ_VALUE) ==0 ) {
            insertIntoHash(TAG, msg, frames);
            get_string_frame(buf, frames, TAG);
            printf("tag  %s\n", buf);

            insertIntoHash(PAYLOAD, msg, frames);
        }
    }
    return frames;
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
        //SODAW_write(obj_name, writer_id, i,  payload, size, servers, port);
    }

    //   zclock_sleep(50*1000);
    return 0;
}

#endif
