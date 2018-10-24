//  Asynchronous client-to-server (DEALER to ROUTER)
//
//  While this example runs in a single process, that is to make
//  it easier to start and stop the example. Each task has its own
//  context and conceptually acts as a separate process.

#include <czmq.h>
#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sodaw_client.h"
#include "sodaw_server.h"

extern int s_interrupted;

extern Server_Status *status;
extern Server_Args *server_args;

#ifdef ASLIBRARY
static zhash_t *hash_object_SODAW;
static zhash_t *history_table = NULL;
static zhash_t *readed_table = NULL;
static zhash_t *reading_table = NULL;

void dump_historydata(zhash_t *history_tab){
    zlist_t *history_keys = zhash_keys(history_tab);
    void *key;

    for(key= zlist_first(history_keys);  key!= NULL; key=zlist_next(history_keys) ) {
        HistoryData *his  = (HistoryData *)zhash_lookup(history_tab, (const char *)key);
        printf(">>readerid:%s, opnum:%d,  senderid:%s,  read_tag:%s, send_tag:%s\n",
            his->readerid, his->opnum, his->senderid, his->read_tag_str, his->send_tag_str);
    }
    zlist_purge(history_keys);
    zlist_destroy(&history_keys);
}

void dump_regreader(zhash_t *reading_tab, zhash_t *readed_tab){
    zlist_t *tab_keys;
    void *key;
    Tag t;
    char tag_str[BUFSIZE];

int reading_c = zhash_size(reading_tab);
int readed_c = zhash_size(readed_tab);

    printf("Reading table(%d:%d):\n", reading_c, reading_c+readed_c);
    tab_keys = zhash_keys(reading_tab);
    for(key= zlist_first(tab_keys);  key!= NULL; key=zlist_next(tab_keys) ) {
        RegReader* read  = (RegReader*)zhash_lookup(reading_tab, (const char *)key);
        tag_to_string(read->t_r, tag_str);
        printf(">>readerid:%s, opnum:%d,  tag:%s\n", read->reader_id, read->opnum, tag_str);
    }
    zlist_purge(tab_keys);
    zlist_destroy(&tab_keys);

    printf("Readed table(%d:%d):\n", readed_c, reading_c+readed_c);
    tab_keys = zhash_keys(readed_tab);
    for(key= zlist_first(tab_keys);  key!= NULL; key=zlist_next(tab_keys) ) {
        RegReader* read  = (RegReader*)zhash_lookup(readed_tab, (const char *)key);
        tag_to_string(read->t_r, tag_str);
        printf(">>readerid:%s, opnum:%d,  tag:%s\n", read->reader_id, read->opnum, tag_str);
    }
    zlist_purge(tab_keys);
    zlist_destroy(&tab_keys);
}

static int initialized = 0;

void initialize_SODAW() {
    initialized = 1;
    reading_table = zhash_new();
    readed_table = zhash_new();
    history_table = zhash_new();
    hash_object_SODAW= zhash_new();

    create_metadata_sending_sockets() ;
    server_args->K =  ceil(((float)server_args->num_servers+1)/2);
}

HistoryData* HistoryData_create(char* readerid, char* senderid, char *read_tag, char *send_tag, unsigned int opnum) {
    HistoryData *h = (HistoryData*)malloc(sizeof(HistoryData));
    strcpy(h->readerid, readerid);
    strcpy(h->senderid, senderid);
    strcpy(h->read_tag_str, read_tag);
    strcpy(h->send_tag_str, send_tag);
    h->opnum = opnum;
    return h;
}

RegReader *RegReader_create(Tag tag, char *readerid, unsigned int opnum) {
    RegReader *h = (RegReader *)malloc(sizeof(RegReader));
    h->t_r.z= tag.z;
    strcpy(h->t_r.id, tag.id);
    h->done_by_who = 0;
    h->opnum = opnum;
    strcpy(h->reader_id, readerid);
    return h;
}

void  *reader_op_create(char *buf, char *readerid, unsigned int opnum) {
    sprintf(buf, "%s_%d", readerid, opnum);
}


char *HistoryData_keystring(HistoryData *m) {
    char buf[BUFSIZE];
    int size = 0;

    reader_op_create(buf, m->readerid, m->opnum);
    size += strlen(buf);
    buf[size++]='_';

    strncpy(buf+size, m->senderid, strlen(m->senderid));
    size += strlen(m->senderid);
    buf[size++]='_';

    strncpy(buf+size, m->send_tag_str, strlen(m->send_tag_str));
    size += strlen(m->send_tag_str);
    buf[size++]='\0';

    char *newkey = (char *)malloc(size*sizeof(char));
    strcpy(newkey, buf);
    return newkey;
}


static void send_reader_coded_element(void *worker, char *reader, char *server,
        char *object_name,
        char *algorithm,
        char *phase,
        Tag tagw,
        unsigned int *opnum,
        zframe_t *cs) {
    char tag_w_buff[BUFSIZE];

    if(DEBUG_MODE) printf("\t\tSENDER: %s\n", reader);
    zframe_t *reader_frame = zframe_new(reader, strlen(reader));
    zframe_send(&reader_frame, worker, ZFRAME_REUSE + ZFRAME_MORE);

    if(DEBUG_MODE) printf("\t\tSERVER: %s\n", server);
    zframe_t *server_frame = zframe_new(server, strlen(server));
    zframe_send(&server_frame, worker, ZFRAME_REUSE + ZFRAME_MORE);

    if(DEBUG_MODE) printf("\t\tOBJECT : %s\n", object_name);
    zframe_t *object_frame = zframe_new(object_name, strlen(object_name));
    zframe_send(&object_frame, worker, ZFRAME_REUSE + ZFRAME_MORE);

    if(DEBUG_MODE) printf("\t\tALGORITHM : %s\n", algorithm);
    zframe_t *algorithm_frame = zframe_new(algorithm, strlen(algorithm));
    zframe_send(&algorithm_frame, worker, ZFRAME_REUSE + ZFRAME_MORE);

    if(DEBUG_MODE) printf("\t\tPHASE : %s\n", phase);
    zframe_t *phase_frame = zframe_new(phase, strlen(phase));
    zframe_send(&phase_frame, worker, ZFRAME_REUSE + ZFRAME_MORE);

    tag_to_string(tagw, tag_w_buff) ;
    if(DEBUG_MODE) printf("\t\tTAG : %s\n", tag_w_buff);
    zframe_t *tag_frame = zframe_new(tag_w_buff, strlen(tag_w_buff));
    zframe_send(&tag_frame, worker, ZFRAME_REUSE + ZFRAME_MORE);

    if(DEBUG_MODE) printf("\t\tOPNUM : %d\n", *opnum);
    zframe_t *opnum_frame= zframe_new((const void *)opnum, sizeof(unsigned int));
    zframe_send(&opnum_frame, worker, ZFRAME_REUSE + ZFRAME_MORE);


    if(DEBUG_MODE) printf("\t\tPAYLOAD : %lu\n", zframe_size(cs));
    zframe_t *cs_frame = zframe_dup(cs);
    zframe_send(&cs_frame, worker, ZFRAME_REUSE);

    zframe_destroy(&reader_frame);
    zframe_destroy(&server_frame);
    zframe_destroy(&object_frame);
    zframe_destroy(&algorithm_frame);
    zframe_destroy(&phase_frame);
    zframe_destroy(&tag_frame);
    zframe_destroy(&opnum_frame);
    zframe_destroy(&cs_frame);

}

void algorithm_SODAW_WRITE_PUT(zhash_t *frames,  void *worker) {
    char tag_w_str[BUFSIZE];
    char tag_r_str[BUFSIZE];
    char algorithm[BUFSIZE];
    char senderid[BUFSIZE];
    char writerid[BUFSIZE];
    char object_name[BUFSIZE];
    unsigned int opnum;

    //create the tag t_w as a string
    Tag tag_w;
    get_string_frame(tag_w_str, frames, TAG);
    string_to_tag(tag_w_str, &tag_w);

    // read the new coded element  from the message
    // this is the coded element cs
    zframe_t *payload = (zframe_t *)zhash_lookup(frames, PAYLOAD);
    int size = zframe_size(payload);
    get_string_frame(senderid, frames, SENDER);
    get_string_frame(writerid, frames, CLIENTID);
    get_string_frame(algorithm, frames, ALGORITHM);
    get_string_frame(object_name, frames, OBJECT);
    opnum = get_uint_frame(frames, OPNUM);

    //read the local tag
    Tag tag;
    get_object_tag(hash_object_SODAW, object_name, &tag);
    if(compare_tags(tag_w, tag) > 0  ) {
	    // loop through all the existing (r, tr) pairs
	    zlist_t *r_tr_keys = zhash_keys(reading_table);
	    void *key, *newkey;
	    for(key= zlist_first(r_tr_keys);  key!= NULL; key=zlist_next(r_tr_keys) ) {
	        RegReader *reader  = (RegReader *)zhash_lookup(reading_table, (const char *)key);
	        assert(reader);
	        tag_to_string(reader->t_r, tag_r_str);
	        int reader_opnum = reader->opnum;

	        if(compare_tags(tag_w, reader->t_r) >= 0 ) {
		      printf("[READ_VALUE %s] send coded data to %s\n", server_args->server_id, reader->reader_id);
	            send_reader_coded_element(worker, reader->reader_id, server_args->server_id,
	                object_name, algorithm, READ_VALUE, tag_w, &reader_opnum, payload);

	            char *types[] = {SERVERID, OBJECT, ALGORITHM, PHASE, META_SEND_TAG,
	                META_READERID, META_READ_TAG, OPNUM};
	            send_multicast_servers(server_args->sock_to_servers, server_args->num_servers,
	                    types, 8, server_args->server_id,
	                    object_name, "SODAW", READ_DISPERSE,
	                    tag_w_str, reader->reader_id, tag_r_str, &reader_opnum);
	        }
	    }
	    zlist_purge(r_tr_keys);
	    zlist_destroy(&r_tr_keys);


        // get the hash for the object
        zhash_t *temp_hash_hash = zhash_lookup(hash_object_SODAW, object_name);
        zlist_t *keys = zhash_keys(temp_hash_hash);
        key = (char *)zlist_first(keys);
        assert(key!=NULL);
        // get the  objects stored, i.e., the stored local value
        zframe_t *item = (zframe_t *)zhash_lookup(temp_hash_hash, key);
        assert(item!=NULL);
        zframe_destroy(&item);
        zhash_delete(temp_hash_hash, key);

        //insert the new tag and coded value
        zframe_t *dup_payload = zframe_dup(payload);
        zhash_insert(temp_hash_hash, tag_w_str,(void *) dup_payload);

        zlist_purge(keys);
        zlist_destroy(&keys);
    }

    //send ack to writer
    char *serverid = server_args->server_id;
    zframe_t *serverid_frame = zframe_new(serverid, strlen(serverid));
    zhash_insert(frames, SERVERID, (void *) serverid_frame);
    send_frames_at_server(frames, worker, SEND_FINAL, 7,  SENDER, SERVERID, OBJECT, ALGORITHM, PHASE, OPNUM, TAG);

    dump_historydata(history_table);
    return;
}

void algorithm_SODAW_READ_COMPLETE(zhash_t *frames, void *worker) {
    char tag_r_str[BUFSIZE];
    char readerid[BUFSIZE];

    Tag tag_r;
    get_string_frame(tag_r_str, frames, TAG);
    string_to_tag(tag_r_str, &tag_r);
    get_string_frame(readerid, frames, CLIENTID);
    unsigned int opnum =  get_uint_frame(frames, OPNUM);

	printf("[READ_COMPLETE %s] dump registered readers:\n",
		server_args->server_id);
	dump_regreader(reading_table, readed_table);

    RegReader *reading = zhash_lookup((void *)reading_table, (const char *)readerid);
    if(reading && reading->opnum > opnum)
        return;
    RegReader *readed = zhash_lookup((void *)readed_table, (const char *)readerid);
    if(readed && readed->opnum >= opnum)
    	return;

    if(reading) {
        assert(reading->opnum <= opnum);
        free(reading);
        zhash_delete((void *)reading_table, (const char *)readerid);
    }
    //update the readed table
    if(readed != NULL){
        assert(readed->opnum < opnum );
        free(readed);
        zhash_delete((void *)readed_table, (const char *)readerid);
    }

    //update the reading table
    RegReader *read = RegReader_create(tag_r, readerid, opnum);
    zhash_insert(readed_table, (const char*)readerid, (void *)read);

    if(DEBUG_MODE) {
	printf("[WRITE-COMPLETE %s] complete a read\n", server_args->server_id);
	dump_regreader(reading_table, readed_table);
    }
    //clear the history
    historydata_clear_by_readerid_opnum(history_table, readerid, opnum);
}


void algorithm_SODAW_READ_DISPERSE(zhash_t *frames,  void *worker) {
    char tag_send_str[BUFSIZE];
    char tag_read_str[BUFSIZE];
    char senderid[BUFSIZE];
    char readerid[BUFSIZE];
    char object_name[BUFSIZE];

    get_string_frame(senderid, frames, CLIENTID);
    get_string_frame(object_name, frames, OBJECT);

    Tag send_tag, read_tag;
    get_string_frame(tag_send_str, frames, META_SEND_TAG);//the response tag
    get_string_frame(tag_read_str, frames, META_READ_TAG);//the required read tag
    string_to_tag(tag_send_str, &send_tag);
    string_to_tag(tag_read_str, &read_tag);
    get_string_frame(readerid, frames, META_READERID);
    unsigned int opnum =  get_uint_frame(frames, OPNUM);

    //current service is enough
    RegReader *readed= zhash_lookup((void *)readed_table, (const char *)readerid);
    if(readed && readed->opnum >= opnum)
        return;
    RegReader *reading = zhash_lookup((void *)reading_table, (const char *)readerid);
    if(reading && reading->opnum > opnum)
        return;

    //record the read into the history table
     HistoryData *his = HistoryData_create(readerid, senderid, tag_read_str, tag_send_str, opnum);
     char *his_key = HistoryData_keystring(his);
     zhash_insert((void *)history_table, (const char *)his_key, (void *)his);
     free(his_key);
	 
printf("[READ_DISPERSE %s] dump history table after adding to the history table:\n",
	server_args->server_id);
dump_historydata(history_table);

    //the read is finished
     zlist_t *his_set = historydata_lookup(history_table, readerid, opnum, tag_send_str);

//printf("[READ_DISPERSE]++++ %s finds %d matched responses \n", server_args->server_id, zlist_size(his_set));

     if(zlist_size(his_set) >= server_args->K) {
       //put it into the readed table
       if(reading){
		assert(reading->opnum<=opnum);
        	free(reading);
        	zhash_delete((void *)reading_table, (const char *)readerid);
	}
	if(readed){
		assert(readed->opnum<opnum);
		free(readed);
		zhash_delete((void *)readed_table, (const char *)readerid);
	}
        RegReader *read = RegReader_create(read_tag, readerid, opnum);
        zhash_insert((void *)readed_table, (const char *)readerid, (void*)read);

        //clear the history table
        historydata_clear_by_readerid_opnum(history_table,readerid, opnum);
//historydata_dump_by_readerid_opnum(history_table, readerid, opnum);

printf("[READ_DISPERSE %s] dump history table (%d) after clear the history table (clear at least K responses):\n",
	server_args->server_id, zhash_size(history_table));
dump_historydata(history_table);
    }
    zlist_purge(his_set);
    zlist_destroy(&his_set);
}

void algorithm_SODAW_WRITE_GET_OR_READ_GET_TAG(zhash_t *frames,const  char *phase,  void *worker) {
    char object_name[BUFSIZE];
    char tag_buf[BUFSIZE];

    get_string_frame(object_name, frames, OBJECT);
    Tag tag;
    get_object_tag(hash_object_SODAW, object_name, &tag);
    tag_to_string(tag, tag_buf);

    zframe_t *tag_frame= zframe_new(tag_buf, strlen(tag_buf));
    zhash_insert(frames, TAG, (void *)tag_frame);

    char *server_id = server_args->server_id;
    zframe_t *server_frame = zframe_new(server_id, strlen(server_id));
    zhash_insert(frames, SERVERID, (void *)server_frame);

    send_frames_at_server(frames, worker, SEND_FINAL, 7,  SENDER,  SERVERID, OBJECT,  ALGORITHM, PHASE, OPNUM, TAG);
}

void algorithm_SODAW_WRITE_GET(zhash_t *frames,  void *worker) {
    algorithm_SODAW_WRITE_GET_OR_READ_GET_TAG(frames, WRITE_GET,  worker) ;

}

void algorithm_SODAW_READ_GET(zhash_t *frames,  void *worker) {
    algorithm_SODAW_WRITE_GET_OR_READ_GET_TAG(frames, READ_GET,  worker) ;

}

void algorithm_SODAW_READ_VALUE( zhash_t *frames, void *worker ) {
    char sender_id[BUFSIZE];
    char reader_id[BUFSIZE];
    char algorithm_name[BUFSIZE];
    char object_name[BUFSIZE];
    char tag_loc_str[BUFSIZE];
    char tag_r_str[BUFSIZE];

    //the new coming read
    //printf("\tREAD_VALUE\n");
    get_string_frame(sender_id, frames, SENDER);
    get_string_frame(reader_id, frames, CLIENTID);
    unsigned int opnum =  get_uint_frame(frames, OPNUM);
    get_string_frame(object_name, frames, OBJECT);
    get_string_frame(algorithm_name, frames, ALGORITHM);
    Tag tag_r;
    get_tag_frame(frames, &tag_r);

    //the reader register table
    RegReader *readed = (RegReader*)zhash_lookup(readed_table,(const char*)reader_id);
    if(readed && readed->opnum >= opnum){
        //printf("[READ VALUE %s] WARNING!!!!: have been read(readed opnum:%d, coming opnum:%d, readerid:%s)\n ",
        //    server_args->server_id, readed->opnum, opnum, reader_id);
        return;
    }

    RegReader *reading = (RegReader*)zhash_lookup(reading_table,(const char*)reader_id);
    if(reading && reading->opnum > opnum){
        //printf("[READ VALUE %s] WARNING!!!!: reading opnum:%d greater coming read opnum:%d, readerid:%s, to halt!!!\n ",
        //    server_args->server_id, reading->opnum, opnum, reader_id);
        return;
    }

    //clear the previous register reader
    if(readed){
	 assert(readed->opnum < opnum);
        free(readed);
        zhash_delete((void *)readed_table, (const char *)reader_id);
    }
    if(reading){
	assert(reading->opnum <= opnum);
       free(reading);
       zhash_delete((void *)reading_table, (const char *)reader_id);
    }

    //register the new coming reader
    RegReader *read = RegReader_create(tag_r, reader_id,  opnum);
    zhash_insert((void *)reading_table,  (const char *)reader_id, (void *)read);

printf("[READ_VALUE %s] dump registered readers after register:\n",
	server_args->server_id);
dump_regreader(reading_table, readed_table);

    //find object from stores (with object name)
    Tag tag_loc;
    int result = get_object_tag(hash_object_SODAW, object_name, &tag_loc);
    assert(result == 1);

    if(compare_tags(tag_loc, tag_r)>=0 ) {//current stored tag greater than the required
        zframe_t  *payload_frame = (zframe_t *)get_object_frame(hash_object_SODAW, object_name, tag_loc);
        assert(payload_frame);

printf("[READ_VALUE %s] send coded data to %s\n", server_args->server_id, read->reader_id);
        send_reader_coded_element(worker, sender_id,
		server_args->server_id, object_name, algorithm_name, READ_VALUE,
                tag_loc, &opnum, payload_frame);

        tag_to_string(tag_loc, tag_loc_str);
        tag_to_string(tag_r, tag_r_str);
        // now do the READ-DISPERSE to other servers
        char *types[] = {SERVERID, OBJECT, ALGORITHM, PHASE, META_SEND_TAG,
            META_READERID, META_READ_TAG, OPNUM};
        send_multicast_servers( server_args->sock_to_servers, server_args->num_servers,
                types, 8,
                server_args->server_id, object_name, "SODAW", READ_DISPERSE,
                tag_loc_str, reader_id, tag_r_str, &opnum);
    }
}

void historydata_dump_by_readerid_opnum(zhash_t *history_tab, char *readerid, unsigned int opnum){
    zlist_t *history_keys = zhash_keys(history_tab);

    void *key;
    for(key= zlist_first(history_keys);  key!= NULL; key=zlist_next(history_keys) ) {
        HistoryData *his  = (HistoryData *)zhash_lookup(history_tab, (const char *)key);
        assert(his != NULL);

        if(strcmp(readerid, his->readerid) == 0 && opnum >= his->opnum) {
            printf(">>>>history: reader=%s, op=%d, sender=%s, read_tag=%s, send_tag=%s\n",
              his->readerid, his->opnum, his->senderid, his->read_tag_str, his->send_tag_str);
        }
    }
    zlist_purge(history_keys);
    zlist_destroy(&history_keys);
}



void historydata_clear_by_readerid_opnum(zhash_t *history_tab, char *readerid, unsigned int opnum){
    zlist_t *history_keys = zhash_keys(history_tab);

    void *key;
    for(key= zlist_first(history_keys);  key!= NULL; key=zlist_next(history_keys) ) {
        HistoryData *his  = (HistoryData *)zhash_lookup(history_tab, (const char *)key);
        assert(his != NULL);

        if(strcmp(readerid, his->readerid) == 0 && opnum >= his->opnum) {
            free(his);
            zhash_delete((void *)history_tab, (const char *)key);
        }
    }
    zlist_purge(history_keys);
    zlist_destroy(&history_keys);
}

zlist_t* historydata_lookup(zhash_t * history_tab, char * readerid, unsigned int opnum, char *tag_send_str){
    zlist_t *history_keys = zhash_keys(history_tab);
    zlist_t *result = zlist_new();
    void *key;

    for(key= zlist_first(history_keys);  key!= NULL; key=zlist_next(history_keys) ) {
        HistoryData *his  = (HistoryData *)zhash_lookup(history_tab, (const char *)key);
        assert(his != NULL);

        if(strcmp(readerid, his->readerid) == 0 && opnum == his->opnum &&
            strcmp(tag_send_str, his->send_tag_str)==0) {
            zlist_append((void *)result, key);
        }
    }
    zlist_purge(history_keys);
    zlist_destroy(&history_keys);
    return result;
}

int historydata_count(zhash_t* history_tab, char *readerid, unsigned int opnum){
    zlist_t *history_keys = zhash_keys(history_tab);
    int result = 0;
    void *key;

    for(key= zlist_first(history_keys);  key!= NULL; key=zlist_next(history_keys) ) {
        HistoryData *his  = (HistoryData *)zhash_lookup(history_tab, (const char *)key);
        assert(his != NULL);

        if(strcmp(readerid, his->readerid) == 0 && opnum == his->opnum ) {
            result ++;
        }
    }
    zlist_purge(history_keys);
    zlist_destroy(&history_keys);
    return result;
}




void create_metadata_sending_sockets() {
    int num_servers = count_num_servers(server_args->servers_str);
    server_args->num_servers = num_servers;
    char **servers = create_server_names(server_args->servers_str);

    zctx_t *ctx  = zctx_new();
    void *sock_to_servers = zsocket_new(ctx, ZMQ_DEALER);
    zctx_set_linger(ctx, 0);
    assert (sock_to_servers);
    zsocket_set_identity(sock_to_servers,  server_args->server_id);

    int j;
    for(j=0; j < num_servers; j++) {
        char *destination = create_destination(servers[j], server_args->port);
        int rc = zsocket_connect(sock_to_servers, destination);
        assert(rc==0);
        free(destination);
    }

    destroy_server_names(servers, num_servers);
    server_args->sock_to_servers = sock_to_servers;
}

void algorithm_SODAW(zhash_t *frames, void *worker, void *_server_args) {
    char phase[BUFSIZE];
    char object_name[BUFSIZE];

    if(initialized==0) initialize_SODAW();

    get_string_frame(phase, frames, PHASE);
    get_string_frame(object_name, frames, OBJECT);

    if( has_object(hash_object_SODAW, object_name)==0 ) {
        create_object(hash_object_SODAW, object_name, "SODAW", server_args->init_data, status);
        printf("creating \n");
    }

    if(DEBUG_MODE) print_object_hash(hash_object_SODAW);

    if( strcmp(phase, WRITE_GET)==0 )  {
        algorithm_SODAW_WRITE_GET(frames,  worker);
    } else if( strcmp(phase, WRITE_PUT)==0 )  {
        algorithm_SODAW_WRITE_PUT(frames, worker);
    } else if( strcmp(phase, READ_GET)==0 )  {
        algorithm_SODAW_READ_GET(frames, worker);
    } else if( strcmp(phase, READ_VALUE)==0 )  {
        algorithm_SODAW_READ_VALUE(frames, worker);
    } else if( strcmp(phase, READ_COMPLETE)==0 )  {
        algorithm_SODAW_READ_COMPLETE(frames, worker);
    } else if( strcmp(phase, READ_DISPERSE)==0 )  {
        algorithm_SODAW_READ_DISPERSE(frames, worker);
    }
}

#endif

//  The main thread simply starts several clients and a server, and then
//  waits for the server to finish.
#ifdef ASMAIN
int main (void) {
    int i ;
    zthread_new(server_task, NULL);
    zclock_sleep(60*60*1000);
    return 0;

}
#endif

