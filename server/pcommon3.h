/* $Id: pcommon3.h,v 1.4 2012/03/26 06:21:19 eikii Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "if_bonanza.h"

#define BNS_COMPAT    0
#define MASTERRANK    0

#define MAX_SRCH_DEP_FIGHT     30
#define MAX_SRCH_DEP_PROB       9
extern int MAX_SRCH_DEP;

#define DBG_INTERVAL_CHK  0

#undef max
#undef min

#define areaclr(x)  memset(&(x), 0, sizeof(x))
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

#define NO_PENDING_REQUEST (-1)

enum
{
    ULE_NA = 0,
    ULE_UPPER = 2,
    ULE_LOWER = 1,
    ULE_EXACT = 3,
};

// copied from comm2.cc
class mvC {
public:
    int v;
    mvC(int x = 0) : v(x) {}
    bool operator ==(mvC x) { return (v==x.v); }
    bool operator !=(mvC x) { return (v!=x.v); }
};

#if defined(_WIN32)
struct timespec
{
    int tv_sec;
    int tv_nsec;
};
#endif

#define NOSIDE (-1)
extern int compTurn;

class planeC;
extern planeC plane;

 // defined in putils.cpp
extern void ei_clock_gettime(struct timespec* tsp);
extern int worldTime();
extern int64_t worldTimeLl();
extern void initTime();
extern void microsleep(int);
extern void microsleepMaster(int);
extern int readable_c(int mv);
extern int readable(mvC);

 // for invokempi.cpp
extern void sendQuit(int pr);

 // for 
extern void sendPacket(int dst, int* buf, int count); //FIXME unsigned
extern int recvPacket(int rank, int* v); // ditto
extern int probePacketSlave();
extern int probeProcessor();

 // for perfsl.h
extern int initPerf(tree_t * restrict ptree);
extern int displayPerf(tree_t * restrict ptree);

 // for slave
extern int inRoot, rootExceeded, inFirst, firstReplied;
extern uint64_t preNodeCount;

extern void replyFirst();

 // sbody.h
extern int problemMode();
