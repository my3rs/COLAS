//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include "czmq.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <algo_utils.h>
#include "sodaw_client.h"
#include "sodaw_reader.h"
#include <base64.h>
#include <rlnc_rs.h>

#define DEBUG_MODE 1
#define VERBOSE_MODE 1

extern int s_interrupted;



void destroy_received_data(zhash_t *received_data) {
	zlist_t *keys = zhash_keys(received_data);
	char *key;

	for (key = (char *)zlist_first(keys); key != NULL; key = (char *)zlist_next(keys)) {
		zlist_t *data_list = zhash_lookup(received_data, key);
		zframe_t *data_item;
		for (data_item = (zframe_t *)zlist_first(data_list); data_item != NULL; data_item = (zframe_t *)zlist_next(data_list)) {
			zframe_destroy(&data_item);
		}
		zlist_purge(data_list);
		zlist_destroy(&data_list);
	}
	zlist_purge(keys);
	zlist_destroy(&keys);
	zhash_destroy(&received_data);
	zhash_destroy(&received_data);
	return;
}



Tag *SODAW_read_get_phase(char *obj_name,
	char* client_id,
	unsigned int op_num,
	zsock_t *sock_to_servers,
	unsigned int num_servers) {

	return SODAW_write_get_or_read_get_phase(
		obj_name, client_id, READ_GET, op_num,
		sock_to_servers, num_servers);
}


typedef struct _RECVDATA {
	char recv_tag[BUFSIZE];
	unsigned int   opnum;

} RecvData;


// this fetches the max tag and value
void SODAW_read_value(char *obj_name,
	char *client_id,
	unsigned int op_num,
	zsock_t *sock_to_servers,
	unsigned int num_servers,
	Tag read_tag,
	EncodeData *encoding_info) {

	// send out the messages to all servers
	char recv_phase[BUFSIZE];
	unsigned int recv_opnum;
	char read_tag_str[BUFSIZE], recv_tag_str[BUFSIZE];
	Tag recv_tag;

	zmq_pollitem_t items[] = { { sock_to_servers, 0, ZMQ_POLLIN, 0 } };
	char *types[] = { CLIENTID, OBJECT, ALGORITHM, PHASE, OPNUM, TAG };
	tag_to_string(read_tag, read_tag_str);
	send_multicast_servers(sock_to_servers, num_servers, types, 6, client_id, obj_name, SODAW, READ_VALUE, &op_num, read_tag_str);

	unsigned int majority = ceil(((float)num_servers + 1) / 2);

	zhash_t *recv_dataset = zhash_new();
	zlist_t *data_list = NULL;
	char decodeableKey[BUFSIZE];

	zlist_t *recv_metadata_set = zlist_new();
	int count_recv_msg = 0;

	int done = 0;

	while (true) {
		//  Tick once per second, pulling in arriving messages
		if (VERBOSE_MODE) printf("\t\twaiting for data..\n");
		//int rc = zmq_poll(items, 1, -1);
		int rc = zmq_poll(items, 1, 5 * ZMQ_POLL_MSEC);
		if (rc < 0 || s_interrupted) {
			printf("Interrupted!\n");
			exit(EXIT_FAILURE);
		}
		if (rc == 0) {
			if (VERBOSE_MODE && done == 0) {
				//-------------------
				int i = 0;
				for (RecvData *key = zlist_first(recv_metadata_set); key != NULL; key = zlist_next(recv_metadata_set)) {
					printf("[READ-VALUE] %s--recv tag=%s, op=%d--------i=%d\n",
						client_id, key->recv_tag, key->opnum, ++i);
				}
				//--------------------
			}
			if (done == 1)
				break;
		}
		if (items[0].revents & ZMQ_POLLIN) {
			zmsg_t *msg = zmsg_recv(sock_to_servers);
			assert(msg != NULL);
			zlist_t *names = zlist_new();
			assert(names);
			assert(zlist_size(names) == 0);

			//receive message
			zhash_t* frames = receive_message_frames_at_client(msg, names);
			get_string_frame(recv_phase, frames, PHASE);
			get_string_frame(recv_tag_str, frames, TAG);
			get_tag_frame(frames, &recv_tag);
			recv_opnum = get_uint_frame(frames, OPNUM);


			if (VERBOSE_MODE) {
				//--------------------------------
				if (strcmp(recv_phase, READ_VALUE) == 0) {
					RecvData *md = (RecvData *)malloc(sizeof(RecvData));
					md->opnum = recv_opnum;
					strcpy(md->recv_tag, recv_tag_str);
					zlist_append(recv_metadata_set, md);
				}
				//-------------------------------
			}


			if (done == 0) {
				if (recv_opnum == op_num && strcmp(recv_phase, READ_VALUE) == 0) {
					assert(compare_tags(recv_tag, read_tag) >= 0);
					//data list with the same tag
					if (zhash_lookup(recv_dataset, recv_tag_str) == NULL) {
						data_list = zlist_new();
						zhash_insert(recv_dataset, recv_tag_str, (void *)data_list);
					}
					zframe_t *payload_frame = (zframe_t *)zhash_lookup(frames, PAYLOAD);

					//append the received data
					data_list = (zlist_t *)zhash_lookup(recv_dataset, recv_tag_str);
					assert(data_list != NULL);
					zframe_t *dup_payload_frame = zframe_dup(payload_frame);
					zlist_append(data_list, dup_payload_frame);

					//check number of reponses with the same tag
					if (zlist_size(data_list) >= majority) {
						done = 1;
						strcpy(decodeableKey, recv_tag_str);
					}
				}
			}
			zmsg_destroy(&msg);
			zlist_purge(names);
			zlist_destroy(&names);
			destroy_frames(frames);
		}
	}

	get_encoded_info(recv_dataset, decodeableKey, encoding_info);
	destroy_received_data(recv_dataset);

	if (VERBOSE_MODE) {
		printf("Number of keys to decoded %s\n", decodeableKey);
		//---------------------------------------
		RecvData *md;
		md = zlist_first(recv_metadata_set);
		while (md) {
			zlist_remove(recv_metadata_set, md);
			free(md);
			md = zlist_first(recv_metadata_set);
		}
		zlist_destroy(&recv_metadata_set);
		//---------------------------------------
	}
	if (decode(encoding_info) == 0) {
		printf("Failed to decode for key %s\n", decodeableKey);
		exit(EXIT_FAILURE);
	}
}


int get_encoded_info(zhash_t *received_data, char *decodeableKey, EncodeData *encoding_info) {
	if (DEBUG_MODE) {
		printf("N : %d\n", encoding_info->N);
		printf("K : %d\n", encoding_info->K);
		printf("Symbol size : %d\n", encoding_info->symbol_size);
		printf("actual datasize %d\n", encoding_info->raw_data_size);
		printf("num_blocks %d\n", encoding_info->num_blocks);
	}
	zlist_t *coded_elements = (zlist_t *)zhash_lookup(received_data, decodeableKey);

	zframe_t *data_frame;
	int frame_size = 0, cum_size = 0;;
	for (data_frame = (zframe_t *)zlist_first(coded_elements); data_frame != NULL; data_frame = zlist_next(coded_elements)) {
		if (DEBUG_MODE) printf("Length of data %lu\n", zframe_size(data_frame));
		frame_size = zframe_size(data_frame);
		cum_size += frame_size;
	}

	int i;
	encoding_info->encoded_data = (uint8_t **)malloc(encoding_info->K * sizeof(uint8_t*));
	for (i = 0; i < encoding_info->K; i++) {
		encoding_info->encoded_data[i] = (uint8_t *)malloc(frame_size * sizeof(uint8_t));
	}

	i = 0;
	for (data_frame = (zframe_t *)zlist_first(coded_elements); data_frame != NULL; data_frame = zlist_next(coded_elements)) {
		frame_size = zframe_size(data_frame);
		if (DEBUG_MODE) printf("-%x\n", simple_hash(zframe_data(data_frame), frame_size));
		memcpy(encoding_info->encoded_data[i], zframe_data(data_frame), frame_size);
		if (DEBUG_MODE) printf("+%x\n", simple_hash(encoding_info->encoded_data[i], frame_size));
		i++;
	}

	if (DEBUG_MODE) printf("number of symbols used in decoding %d\n", i);
	if (DEBUG_MODE) printf("encoded symbol size %d\n", cum_size / (encoding_info->num_blocks*encoding_info->K));
	encoding_info->encoded_symbol_size = ceil(cum_size / (encoding_info->num_blocks*encoding_info->K));

}

// this is the write tag value phase of SODAW
void SODAW_read_complete_phase(char *obj_name,
	char *client_id,
	zsock_t *sock_to_servers,
	unsigned int num_servers,
	unsigned int opnum,
	Tag max_tag   /* for read it is max and for write it is new*/) 
{

	// send out the messages to all servers
	char tag_str[BUFSIZE];
	char *types[] = { CLIENTID, OBJECT, ALGORITHM, PHASE, OPNUM, TAG };
	//printf("\tREAD_COMPLETE (READER) \n");
	tag_to_string(max_tag, tag_str);
	if (DEBUG_MODE) printf("[READ_COMPLETE %s] object=%s, opnum=%d, tag=%s\n",
		client_id, obj_name, opnum, tag_str);
	send_multicast_servers(sock_to_servers, num_servers, types, 6, client_id, obj_name, SODAW, READ_COMPLETE, &opnum, tag_str);
}

void SODAW_read(char *obj_name,
	unsigned int op_num,
	EncodeData *encoded_data,
	ClientArgs *client_args) {
	s_catch_signals();

	int j;
	int num_servers = count_num_servers(client_args->servers_str);
	void *sock_to_servers = get_socket_servers(client_args);

#ifdef DEBUG_MODE
	printf("\t\tObj name       : %s\n", obj_name);
	printf("\t\tReader name    : %s\n", client_args->client_id);
	printf("\t\tOperation num  : %d\n", op_num);

	printf("\t\tServer string   : %s\n", client_args->servers_str);
	printf("\t\tPort to Use     : %s\n", client_args->port);
	printf("\t\tNum of Servers  : %d\n", num_servers);
#endif

	if (DEBUG_MODE) printf("\tREAD_GET (READER)\n");
	Tag *read_tag = SODAW_read_get_phase(obj_name,
		client_args->client_id,
		op_num,
		sock_to_servers,
		num_servers);
	if (DEBUG_MODE) printf("\t\tmax tag (%d,%s)\n\n", read_tag->z, read_tag->id);


	if (DEBUG_MODE) printf("\tREAD_VALUE (READER)\n");
	SODAW_read_value(obj_name,
		client_args->client_id,
		op_num,
		sock_to_servers,
		num_servers,
		*read_tag,
		encoded_data);

	if (DEBUG_MODE) printf("\tREAD_COMPLETE (READER)\n");
	SODAW_read_complete_phase(obj_name,
		client_args->client_id,
		sock_to_servers,
		num_servers,
		op_num,
		*read_tag);


	free(read_tag);

	//!! Why was this turned off? Socket generates a memory leak
	  /*
	   zsocket_destroy(ctx, sock_to_servers);
	   zctx_destroy(&ctx);
	   destroy_server_names(servers, num_servers);
	   */
	   //destroy_server_sockets();
}