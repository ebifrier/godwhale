/* $Id: pcommon3.h,v 1.4 2012/03/26 06:21:19 eikii Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BNS_COMPAT    0

#define MAX_SRCH_DEP_FIGHT     30
#define MAX_SRCH_DEP_PROB       9
extern int MAX_SRCH_DEP;

#define DBG_INTERVAL_CHK  0

#define areaclr(x)  memset(&x, 0, sizeof(x))
#define forr(i,m,n) for (int i=(m); i<=(n); i++)
#define forv(i,m,n) for (int i=(m); i>=(n); i--)
#define max(m,n)    ((m)>(n) ? (m) : (n))
#define min(m,n)    ((m)<(n) ? (m) : (n))
#define NULLMV      mvC((int)(0))

#define TBC  assert(0)
#define NTBR assert(0)

#define DBG_SL_TRACE 1
#define DBG_MS_TRACE 1
#define DBG_SL       0
#define SLTOut(...) \
    do {if (DBG_SL_TRACE) out_file(slavelogfp, __VA_ARGS__); } while(0)
#define MSTOut(...) \
    do {if (DBG_MS_TRACE) out_file(masterlogfp, __VA_ARGS__); } while(0)
#define SLDOut(...) \
    do {if (DBG_SL      ) out_file(slavelogfp, __VA_ARGS__); } while(0)
#define MSDOut(...) \
    do {if (DBG_MASTER  ) out_file(masterlogfp, __VA_ARGS__); } while(0)
void out_file( FILE *pf, const char *format, ... );

extern int DBG_MASTER, VMMODE;
extern int slave_proc_offset;
extern int master_proc_offset;
extern FILE* slavelogfp;
extern FILE* masterlogfp;

extern int Mproc, Nproc, Ncomm;
extern int x_dmy_for_calcinc, INCS_PER_USEC;

 // for main()
void mpi_init(int argc, char **argv, int *nproc, int *mproc);
void slave();
void mpi_close();
void quitHook();

 // for search()
int detectSignalSlave();

 // for slave()
void ei_clock_gettime(struct timespec* tsp);
int worldTime();
int64_t worldTimeLl();
void microsleep(int);
int readable_c(int mv);

 // for invokempi
void sendQuit(int pr);

#ifndef SHOGI_H
 // copied from shogi.h
#define nsquare 81
typedef struct
{
    unsigned int hand_black, hand_white;
    char turn_to_move;
    signed char asquare[nsquare];
} min_posi_t;
#endif

 // for bonanza6
void iniGameHook(const min_posi_t*);
void makeMoveRootHook(int move);
void unmakeMoveRootHook();
int  master(unsigned int*);

 // s/b declared in callmpi.c
void sendPacket(int dst, int* buf, int count); //FIXME old callmpi uses unsigned
int recvPacket(int rank, int* v); // ditto
int probePacketSlave();
#define MASTERRANK 0
int iammaster();  // true if Mproc==0
//int maxslavesuffix();  // Nproc-1
int probeProcessor();

#define NO_PENDING_REQUEST (-1)

enum
{
    ULE_NA = 0,
    ULE_UPPER = 2,
    ULE_LOWER = 1,
    ULE_EXACT = 3,
};

#ifdef __cplusplus
}  //  extern "C"

 //******** C++ only part ********

// copied from comm2.cc
class mvC {
public:
    int v;
    mvC(int x = 0) : v(x) {}
    bool operator ==(mvC x) { return (v==x.v); }
    bool operator !=(mvC x) { return (v!=x.v); }
};

#endif
