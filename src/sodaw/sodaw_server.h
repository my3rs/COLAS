#ifndef _SODAW_SERVER
#define _SODAW_SERVER

#include "../baseprocess/server.h"
#include <algo_utils.h>

typedef struct _R_C {
    Tag t_r;
    int done_by_who;
    unsigned int opnum;
    char reader_id[BUFSIZE];
} RegReader;

typedef struct _HISTORYDATA {
    char readerid[BUFSIZE];
    char senderid[BUFSIZE];
    char read_tag_str[BUFSIZE];
    char send_tag_str[BUFSIZE];
    unsigned int   opnum;
} HistoryData;

void  destroy_regreader(RegReader *r_tr);
void algorithm_SODAW(zhash_t *frames, void *worker, void *server_args) ;
void SODAW_initialize();
void historydata_clear_by_readerid_opnum(zhash_t *history_tab, char *readerid, unsigned int opnum);
void historydata_dump_by_readerid_opnum(zhash_t *history_tab, char *readerid, unsigned int opnum);
zlist_t *historydata_lookup(zhash_t *history_tab, char *readerid, unsigned int opnum, char *tag_send_str);
int historydata_count(zhash_t* history_tab, char *readerid, unsigned int opnum);
void create_metadata_sending_sockets() ;
#endif
