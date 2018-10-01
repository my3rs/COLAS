#include "helpers.h"



char *get_servers_str(Parameters parameters) {
    int i;
    char *q = (char *) malloc( 16*parameters.num_servers*sizeof(char));

    char *p = q;
    strncpy(p, parameters.ipaddresses[0], strlen(parameters.ipaddresses[0]));
    p += strlen(parameters.ipaddresses[0]);
    for(i=1; i < parameters.num_servers; i++) {
        *p = ' ';
        p++;
        strncpy(p, parameters.ipaddresses[i], strlen(parameters.ipaddresses[i]));
        p += strlen(parameters.ipaddresses[i]);
    }
    *p = '\0';
    return q;
}


EncodeData *create_EncodeData(Parameters parameters) {
    unsigned int filesize = (unsigned int) (parameters.filesize_kb*1024);
    EncodeData *encoding_info  = (EncodeData *)malloc(sizeof(EncodeData));
    encoding_info->N = parameters.num_servers;
    encoding_info->K= ceil((float)parameters.num_servers + 1)/2;
    encoding_info->symbol_size = SYMBOL_SIZE;
    encoding_info->raw_data_size = filesize;
    encoding_info->num_blocks = ceil( (float)filesize/(encoding_info->K*SYMBOL_SIZE));
    encoding_info->algorithm= parameters.coding_algorithm;
    encoding_info->offset_index=0;

    return encoding_info;
}

void printParameters(Parameters parameters) {
    int i;

    printf("Parameters [C]\n");
    printf("\tName  \t\t\t\t: %s\n", parameters.server_id);;

    switch(parameters.processtype) {
    case reader:
        printf("\tprocesstype\t\t\t: %s\n", "reader");
        break;
    case writer:
        printf("\tprocesstype\t\t\t: %s\n", "writer");
        break;
    case server:
        printf("\tprocesstype\t\t\t: %s\n", "server");
        break;
    default:
        break;
    }

    printf("\tnum servers\t\t\t: %d\n", parameters.num_servers);
    for(i=0; i < parameters.num_servers; i++) {
        printf("\t\tserver %d\t\t: %s\n",i, parameters.ipaddresses[i] );
    }

    switch(parameters.algorithm) {
    case sodaw:
        printf("\talgorithm\t\t\t: %s\n", SODAW   );
        break;
    case abd:
        printf("\talgorithm\t\t\t: %s\n", ABD );
        break;
    default:
        break;
    }

    switch(parameters.coding_algorithm) {
    case full_vector:
        printf("\tcoding algorithm\t\t: %s\n", "RLNC"   );
        break;
    case reed_solomon:
        printf("\tcoding algorithm\t\t: %s\n", "REED-SOLOMON" );
        break;
    default:
        break;
    }
    printf("\tinter op wait (ms)\t\t: %d\n", parameters.wait);
    printf("\tfile size (KB)\t\t\t: %.2f\n", parameters.filesize_kb);
}



RawData *create_RawData(Parameters parameters) {
    RawData *raw_data  = (RawData *)malloc(sizeof(RawData));
    return raw_data;
}

ClientArgs *create_ClientArgs(Parameters parameters) {

    char *servers_str = get_servers_str(parameters);

    ClientArgs *client_args  = (ClientArgs *)malloc(sizeof(ClientArgs));

    strcpy(client_args->client_id, parameters.server_id);
    strcpy(client_args->port, parameters.port);

    client_args->servers_str = (char *)malloc( (strlen(servers_str) +1)*sizeof(char));
    strcpy(client_args->servers_str, servers_str);

    return client_args;
}

void setDefaults(Parameters *parameters) {
    parameters->num_servers = 0;
    parameters->ipaddresses = NULL;
    parameters->algorithm = sodaw;
    parameters->coding_algorithm = full_vector;
    parameters->wait = 100;
    parameters->filesize_kb = 1.1;
    parameters->processtype = server;
    strcpy(parameters->port, PORT);
}

void destroy_server_args(Server_Args *server_args) {
    free(server_args->init_data);
    free(server_args);
}

Server_Args * get_server_args( Parameters parameters) {

    Server_Args *server_args = (Server_Args *) malloc(sizeof(Server_Args));
    strcpy(server_args->server_id, parameters.server_id);

    server_args->servers_str =  get_servers_str(parameters);
    printf("servers args %s\n", server_args->servers_str);

    strcpy(server_args->server_id, parameters.server_id);

    strcpy(server_args->port, PORT);
    strcpy(server_args->port1, PORT1);

    unsigned int filesize = (unsigned int) (parameters.filesize_kb*1024);

    server_args->init_data= get_random_data(filesize);

    server_args->init_data_size= filesize;

    server_args->sock_to_servers = NULL;
    server_args->symbol_size = 1400;
    server_args->coding_algorithm = parameters.coding_algorithm;
    server_args->N = parameters.num_servers;
    server_args->K = ceil((float)parameters.num_servers + 1)/2;
    server_args->status = NULL;


    return server_args;
}

Server_Status * get_server_status( Parameters parameters) {
    Server_Status *server_status = (Server_Status *) malloc(sizeof(Server_Status));

    server_status->network_data = 0;
    server_status->metadata_memory = 0;
    server_status->cpu_load = 0;
    server_status->time_point = 0;
    server_status->process_memory = 0;
    return server_status;
}


char * get_random_data(unsigned int size) {
    srand(23);
    int i;
    char *data = (char *)malloc( (size+1)*sizeof(char));

    for( i = 0 ; i < size ; i++ ) {
        data[i] = 65 + rand()%25;
        //data[i] = 65 + i%25;
    }
    data[i]='\0';
    return data;
}
