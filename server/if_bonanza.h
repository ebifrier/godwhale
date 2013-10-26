/* $Id: if_bonanza.h,v 1.4 2012/03/26 06:21:19 eikii Exp $ */

#ifdef __cplusplus
extern "C" {
#endif

#include "bonanza6/shogi.h"

extern int DBG_MASTER, VMMODE;
extern int slave_proc_offset;
extern int master_proc_offset;
extern FILE* slavelogfp;
extern FILE* masterlogfp;

extern int Mproc, Nproc, Ncomm;
extern tree_t* g_ptree;

 // for main()
void mpi_init(int argc, char **argv, int *nproc, int *mproc);
void mpi_close();
void quitHook();
void slave();

 // for search()
int detectSignalSlave();

 // for bonanza6
void iniGameHook(const min_posi_t*);
void makeMoveRootHook(int move);
void unmakeMoveRootHook();
int  master(unsigned int*);

#ifdef __cplusplus
}
#endif
