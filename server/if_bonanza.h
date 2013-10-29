/* $Id: if_bonanza.h,v 1.4 2012/03/26 06:21:19 eikii Exp $ */

/* bonanza‘¤‚©‚çQÆ‚·‚éŠÖ”/•Ï”‚Í‚·‚×‚Ä extern "C" ‚ÅéŒ¾‚µ‚Ü‚·B*/

#ifdef __cplusplus
extern "C" {
#endif

#include "bonanza6/shogi.h"

extern int slave_proc_offset;
extern int master_proc_offset;
extern FILE* slavelogfp;
extern FILE* masterlogfp;

extern int Mproc, Nproc, Ncomm;
extern tree_t* g_ptree;

 // for main()
extern void mpi_init(int argc, char **argv, int *nproc, int *mproc);
extern void mpi_close();
extern void quitHook();
extern void slave();

 // for search()
extern int detectSignalSlave();

 // for bonanza6
extern void iniGameHook(const min_posi_t *posi);
extern void makeMoveRootHook(Move move);
extern void unmakeMoveRootHook();
extern int master(Move* retmvseq);

#ifdef __cplusplus
}
#endif
