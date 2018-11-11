//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include "sodaw_client.h"

#define DEBUG_MODE 1
extern int s_interrupted;

// this fethers the max tag
Tag *SODAW_write_get_or_read_get_phase (char *obj_name,
					   char *client_id,
                      const char *_phase,
                                        unsigned int op_num,
                                        zsock_t *sock_to_servers,
                                        unsigned int num_servers) {
    // send out the messages to all servers
    char algorithm[BUFSIZE];
    char phase[BUFSIZE];
    char tag_str[BUFSIZE];
    unsigned int round;
    int done = 0;

    zmq_pollitem_t items [] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };
    char *types[] = {CLIENTID, OBJECT, ALGORITHM, PHASE, OPNUM};
    send_multicast_servers(sock_to_servers, num_servers, types, 5, client_id, obj_name, SODAW, _phase, &op_num) ;

    unsigned int majority =  ceil(((float)num_servers+1)/2);
    unsigned int responses =0;
    zlist_t *tag_list = zlist_new();
    Tag *tag;
    while (true) {
        //printf("\t\twaiting for data.. [SODAW_write_get_or_read_get_phase]\n");
        int rc = zmq_poll(items, 1, 5*ZMQ_POLL_MSEC);
        if(rc < 0 ||  s_interrupted ) {
            printf("Interrupted!\n");
            exit(EXIT_FAILURE);
        }
        if(rc == 0){
	     if(done == 1)
		 	break;
        }
        //printf("\t\treceived data [SODAW_write_get_or_read_get_phase]\n");
        if (items [0].revents & ZMQ_POLLIN) {
			//printf("\t\t [SODAW_write_get_or_read_get_phase]\n");

            zmsg_t *msg = zmsg_recv (sock_to_servers);
            assert(msg != NULL);

            zlist_t *names = zlist_new();
            assert(names);
            assert(zlist_size(names) == 0);
            zhash_t* frames = receive_message_frames_at_client(msg, names);
            get_string_frame(phase, frames, PHASE);
            round = get_uint_frame(frames, OPNUM);
            get_string_frame(tag_str, frames, TAG);

            if(round==op_num && strcmp(phase, _phase)==0) {
		    if(DEBUG_MODE) 	printf("MATCHTED MESSAGE!\n\n");
                responses++;
                // add tag to list
                tag = (Tag *)malloc(sizeof(Tag));
                string_to_tag(tag_str, tag);
                zlist_append(tag_list, (void *)tag);

                if(responses >= majority) {
                    done = 1;
                }
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

// this fetches the max tag and value
char *number_responses_at_least(zhash_t *received_data, unsigned int atleast) {
    zlist_t *keys = zhash_keys(received_data);
    char *key;
    for(key = (char *)zlist_first(keys);  key != NULL; key = (char *)zlist_next(keys)) {
	  zlist_t *data_list = zhash_lookup(received_data, key);
	  if(data_list){
	  	int count = zlist_size(data_list);
		if( count >= atleast) {
		       zlist_purge(keys);
		       zlist_destroy(&keys);
			return key;
		}
	  }
    }
    zlist_purge(keys);
    zlist_destroy(&keys);
    return NULL;
}

int dump_recvdata(zhash_t *received_data){
    zlist_t *keys = zhash_keys(received_data);
    char *key;
    int total = 0;
    for(  key = (char *)zlist_first(keys);  key != NULL; key = (char *)zlist_next(keys)) {
        total += zlist_size(zhash_lookup(received_data, key));
    }

    return total;
}

zhash_t *receive_message_frames_from_server_SODAW(zmsg_t *msg)  {
    char algorithm_name[BUFSIZE];
    char phase_name[BUFSIZE];
    char buf[BUFSIZE];
    zhash_t *frames = zhash_new();

    insertIntoHash(SERVERID, msg, frames);
    get_string_frame(buf, frames, SERVERID);
    if(DEBUG_MODE) printf("\tget msg from server  %s\n", buf);

    insertIntoHash(OBJECT, msg, frames);
    get_string_frame(buf, frames, OBJECT);
    if(DEBUG_MODE) printf("\tobject  %s\n", buf);

    insertIntoHash(ALGORITHM, msg, frames);
    get_string_frame(algorithm_name, frames, ALGORITHM);
    if(DEBUG_MODE) printf("\talgorithm  %s\n", algorithm_name);

    insertIntoHash(PHASE, msg, frames);
    get_string_frame(phase_name, frames, PHASE);
    if(DEBUG_MODE) printf("\tphase num %s\n", phase_name);

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
            if(DEBUG_MODE)  printf("tag  %s\n", buf);
            insertIntoHash(OPNUM, msg, frames);
            insertIntoHash(PAYLOAD, msg, frames);
        }
    }
    return frames;
}
