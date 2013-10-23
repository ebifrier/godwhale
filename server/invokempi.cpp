// $Id: invokempi.c,v 1.5 2012-04-20 07:32:31 eikii Exp $

// all MPI funcs should be called in this file
// master : proc# = 0
// slave : proc# =1,2...

//#define DBG_NO_MPI

#ifndef DBG_NO_MPI
#include <mpi.h>
#endif

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "pcommon3.h"

#define MAXPACKETSIZE 19200
//#define NO_PENDING_REQUEST 4100
//#define ENB_SIGHANDLE
#define DBG_MPI 0

 // FIXME bogus
int INCS_PER_USEC, THINK_TIME, BYOYOMI_TIME;
int DBG_MASTER, VMMODE, tlp_max_arg;
int Mproc, Nproc, Ncomm;

#define HASH_DEFAULT_SIZE 20

extern int MAX_ROOT_SRCH_NODE; 
extern int MAX_FIRST_NODE;

 // FIXME TBC set these
int master_proc_offset = 0;
int slave_proc_offset = 0;

FILE* slavelogfp = NULL;
FILE* masterlogfp = NULL;

#define OURTAG 0

static int calcIncsPerUsec();   // defined below 
static void adjustTimeSlave();  // defined below 
static void adjustTimeMaster();  // defined below 

#ifdef ENB_SIGHANDLE
static void sigHandler(int signo)
{
    char s[100];
    sprintf(s, "!!!! ERROR Signal %s !!!!\n",
            signo==SIGSEGV ? "segv" : signo==SIGINT ? "ctrl-c" : "unknown");
    if (!Mproc) MSTOut(s);
    else        SLTOut(s);
    exit(8);
}
#endif

// MPI initialize
void mpi_init(int argc, char **argv, int *nproc, int *mproc)
{
#ifndef DBG_NO_MPI
    int i, pre_10us, post_10us;
    char tmpbuf[100];

    MPI_Init(&argc, &argv);
    INCS_PER_USEC = calcIncsPerUsec(); // dont use microsleep() before here
    VMMODE = 0;
    log2_ntrans_table = HASH_DEFAULT_SIZE;

    // microsleep test
    pre_10us = worldTime();
    microsleep(100000);  // 100ms - must be >15ms due to f!&$#n windows
    post_10us = worldTime();
    sprintf(tmpbuf, "I, pid %d, report %d as 100000us.  incs/us=%d\n",
                 getpid(), (post_10us - pre_10us)*10, INCS_PER_USEC);
    
    // set time offset
    initTime();

    // # procs and ranks
    MPI_Comm_size(MPI_COMM_WORLD, nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, mproc);

    if (*mproc) {
        char slavelogfilename[30];
        sprintf(slavelogfilename, "logslave%02d", Mproc);
        slavelogfp = fopen(slavelogfilename, "w");
        if (slavelogfp == NULL) {
            exit(8);
        }

        adjustTimeSlave();
        SLTOut(tmpbuf);
    }
    else {
        if (DBG_MASTER || DBG_MS_TRACE) {
            masterlogfp = fopen("logmaster.0", "w");
            if (masterlogfp == NULL) {
                exit(8);
            }
        }
        adjustTimeMaster();
        MSTOut(tmpbuf);
        
        // command option analysis: -v = verbose, -q = quick fight, -h = help
        //  -a enable affinity  -m[0-9] master cpu   -s[0-9] slave cpu start
        //   (-[2-9] = thread num ... handled by slave)
        DBG_MASTER = 0;
        THINK_TIME = 900;
        BYOYOMI_TIME = 0;
        use_cpu_affinity = 0;
        for (i=1; i<argc; i++) {    // i=0 is for command itself
            MSTOut("argv[%d]:%s:", i, argv[i]);
            if (!strcmp(argv[i], "-v")) {
                DBG_MASTER = 1;
                MSTOut("master log on. longer probe cycle\n");
            }
            if (!strcmp(argv[i], "-q")) {
                BYOYOMI_TIME = 6;
                THINK_TIME = 0;
                MSTOut("quick fight: use shorter time\n");
            }
            if (!strncmp(argv[i], "-t", 2)) {
                char c = argv[i][2];
                THINK_TIME = (c=='z' ?     0 :
                              c=='q' ?   480 :
                              c=='f' ?   900 :
                              c=='c' ?  1500 :
                              c=='l' ?  1800 :
                              c=='h' ?  3600 :
                              c=='x' ? 10800 : -1);
                if (THINK_TIME == -1) {
                    MSTOut("think time: illegal option, set to 900\n");
                    THINK_TIME = 900;
                } else {
                    MSTOut("think time: %d min (%d sec)\n",
                           THINK_TIME/60, THINK_TIME);
                }
            }
            if (!strncmp(argv[i], "-y", 2)) {
                char c = argv[i][2];
                BYOYOMI_TIME = (c=='0' ?  0 :
                                c=='1' ? 10 :
                                c=='3' ? 30 :
                                c=='6' ? 60 : -1);
                if (BYOYOMI_TIME == -1) {
                    MSTOut("byoyomi time: illegal option, set to 0\n");
                    BYOYOMI_TIME = 0;
                } else {
                    MSTOut("byoyomi time: %d sec\n", BYOYOMI_TIME);
                }
            }
            if (!strcmp(argv[i], "-w")) {
                VMMODE      = 1;
                MSTOut("vm mode set\n");
            }
            if (!strncmp(argv[i], "-m", 2)) {
                master_proc_offset = atoi(&argv[i][2]);
                MSTOut("master proc offset is %d\n", master_proc_offset);
            }
            if (!strcmp(argv[i], "-h")) {
                MSTOut("-v: verbose mode   -q: quick fight mode\n");
                MSTOut("-k: key offset = 1\n");
                exit(4);
            }
            MSTOut("\n");
        }

        //adjustTimeMaster();  // must be after setting keyOffset   - commented 11/9  FIXME ok?
    }

    // command option analysis for slave (see above)
    tlp_max_arg = 1;
    for (i=1; i<argc; i++) {    // i=0 is for command itself
        if (argv[i][0] == '-' &&
            '0' <= argv[i][1] && argv[i][1] <= '9') {
            char s[100];
            int ncore = argv[i][1] - '0';
            if ('0' <= argv[i][2] && argv[i][2] <= '9') {
                ncore = 10 * ncore + argv[i][2] - '0';
            }

            sprintf(s, " tlp num set to %d\n", ncore);
            if (*mproc) SLTOut(s);
            else        MSTOut(s);
            if (Mproc) {
                tlp_max_arg = ncore;
                //tlp_start();
            }
        }
        if (!strcmp(argv[i], "-a")) {
            use_cpu_affinity = 1;
            if (Mproc) {
                SLTOut("thread affinity enabled\n");
            } else {
                MSTOut("thread affinity enabled\n");
            }
        }
        if (!strncmp(argv[i], "-s", 2)) {
            slave_proc_offset = atoi(&argv[i][2]);
            if (Mproc) {
                SLTOut("slave proc offset is %d\n", slave_proc_offset);
            }
        }
        if (!strncmp(argv[i], "-h", 2)) {
            log2_ntrans_table = 20 + argv[i][2] - '0';
            if (Mproc) {
                SLTOut("slave hash size is %d\n", log2_ntrans_table);
            }
        }
        if (!strcmp(argv[i], "-b")) {
            /* threadGroupB = 1; */
            MSTOut("thread group B selected\n");
        }
    }
    
    if (tlp_max_arg >= 3) {
        MAX_ROOT_SRCH_NODE *= (tlp_max_arg-1);
        MAX_FIRST_NODE *= (tlp_max_arg-1);
    }

#ifdef ENB_SIGHANDLE
    signal(SIGSEGV, sigHandler);
    signal(SIGINT , sigHandler);
#endif

    if (!Mproc && use_cpu_affinity) {
        attachCpu(master_proc_offset);
        MSTOut("master cpu set to %d\n", master_proc_offset);
    }
#endif
}

// MPI termination
void mpi_close(void)
{
#ifndef DBG_NO_MPI
    int proc;

    // send end flag (=0) from master to all slaves
    if (!Mproc) {
        for (proc = 1; proc < Nproc; proc++) {
            sendQuit(proc);
        }
#if (USE_SHMEM_HASH)
        shmdt(hashTableAddr);
        shmctl(shmid, IPC_RMID, NULL);
#endif
        microsleep(100000);  // wait 0.1sec for slaves to quit
    }

    // termination processing
    if (DBG_MPI) {
         char str[300];
         sprintf(str, "MPI closing: rank %d\n", Mproc);
         if (!Mproc) MSTOut(str);
         else        SLTOut(str);
    }
    if (Mproc) {
        microsleep(100000);  // wait 0.1sec   slave only (FIXME? right?)
    }
    MPI_Finalize();
#endif
}

//******************* 
void sendPacket(int dst, int* buf, int count)
{
#ifndef DBG_NO_MPI
    //assert(dst != Mproc);

    if (DBG_MPI) {
         char str[300];
         sprintf(str,
                "MPI sending: rank %d, cnt %d, buf0-3 %08x %08x %08x %08x\n",
                 Mproc, count, buf[0], buf[1], buf[2], buf[3]);
         if (!Mproc) MSTOut(str);
         else        SLTOut(str);
    }

    MPI_Send(buf, count, MPI_INT, dst, OURTAG, MPI_COMM_WORLD);

    //if (!Mproc) MSDOut("MPI sending done\n");
    //else        SLDOut("MPI sending done\n");
#endif
}

//******************* 
int recvPacket(int src, int* buf)
{
#ifndef DBG_NO_MPI
    MPI_Status status;

    //printf("MPI receiving: rank %d\n", Mproc);  // FIXME? printf in MPI?

    // NOTE size arg can be bigger than actual size.  cmd has the actual sz
    MPI_Recv(buf, MAXPACKETSIZE, MPI_INT, src, OURTAG,
             MPI_COMM_WORLD, &status);
    if (DBG_MPI) {
        int cnt; // for dbg
        char str[300];
        MPI_Get_count(&status, MPI_UNSIGNED, &cnt);  // FIXME?  MPI_INT?
        sprintf(str,
                "MPI received: rank %d src %d cnt %d buf0-3 %08x %08x %08x %08x\n",
                Mproc, status.MPI_SOURCE, cnt, buf[0], buf[1], buf[2], buf[3]);
        if (!Mproc) MSTOut(str);
        else        SLTOut(str);
    }
#endif
    return 0;
}

//******************* 
int probeProcessor()
{
#ifndef DBG_NO_MPI
    MPI_Status status;
    int flag;

    MPI_Iprobe(MPI_ANY_SOURCE, OURTAG, MPI_COMM_WORLD, &flag, &status);
    if (!flag) {
        return NO_PENDING_REQUEST;  // different from any valid rank
    } else {
        return status.MPI_SOURCE;  // rank of source returned
    }
#else
    return 0;
#endif
}

//******************* 
int probePacketSlave()
{
#ifndef DBG_NO_MPI
    MPI_Status status;
    int flag;

    MPI_Iprobe(MASTERRANK, OURTAG, MPI_COMM_WORLD, &flag, &status);
    return flag;  // nonzero if something has come
#else
    return 0;
#endif
}

int x_dmy_for_calcinc = 0;
#define CALCINC_CONST  1000000000

static int calcIncsPerUsec()
{
    int i, pre, post;

    pre = worldTime();
    for (i=0; i<CALCINC_CONST; i++) {
        x_dmy_for_calcinc &= x_dmy_for_calcinc - 2;
    }
    post = worldTime();

    if (post == pre) {
        printf("Cannot measure time : post=%d pre=%d\n", post, pre);
        exit(8);
    }

    x_dmy_for_calcinc >>= 28;  // keep it close to zero
    return (CALCINC_CONST / (10 * (post - pre)));
}


//******** time related *********** 
static int origin_sec, origin_nsec;
extern int time_offset;

int keyOffset, nodeLeader;
#define KEYOFFSET_BITPOS 2

static void adjustTimeMaster()
{
    int v[100];
    struct timespec ts;
    int proc;
    
    ei_clock_gettime(&ts);
    origin_sec  = ts.tv_sec;
    origin_nsec = ts.tv_nsec;
    MSTOut("origin: sec=%08x, nsec=%08x\n", origin_sec, origin_nsec);

    for (proc = 1; proc <= Nproc-1; proc++) {
        v[0] = origin_sec;
        v[1] = origin_nsec;
        v[2] = 0; //(keyOffset << KEYOFFSET_BITPOS) | isNodeLeader(proc);
        sendPacket(proc, v, 3);  // send origin
        
        recvPacket(proc, v);  // ack

        v[0] = worldTime();
        sendPacket(proc, v, 1);  // send 1st time
        MSTOut("sending %08x as 1st\n", v[0]);
        
        recvPacket(proc, v);
        v[0] = worldTime();
        sendPacket(proc, v, 1);  // send 2nd time
        MSTOut("sending %08x as 2nd\n", v[0]);
        
        microsleep(100 * 1000);  // wait 100ms
    }
}

/*
  M   ...1--->.....2.
  S   .......1'--->..
 */
static void adjustTimeSlave()
{
    int buf[100];
    struct timespec ts;
    int pre, mid, post;

    // for debug purpose, realtime of slave is also displayed
    ei_clock_gettime(&ts);
    SLTOut("slave origin: sec=%08x, nsec=%08x\n", ts.tv_sec, ts.tv_nsec);

    recvPacket(MASTERRANK, buf);  // get origin
    origin_sec  = buf[0];
    origin_nsec = buf[1];
    nodeLeader  = buf[2] & 1;
    keyOffset   = (buf[2] >> KEYOFFSET_BITPOS) & 1;
    
    buf[0] = 0xac1c; // ack
    sendPacket(MASTERRANK, buf, 1);
    
    recvPacket(MASTERRANK, buf);  // get 1st time
    pre = buf[0];
    mid = buf[0] = worldTime();
    sendPacket(MASTERRANK, buf, 1);
    
    recvPacket(MASTERRANK, buf);  // get 2nd time
    post = buf[0];

    time_offset = mid - (post + pre)/2;
    SLTOut("origin: sec=%08x, nsec=%08x\n", origin_sec, origin_nsec);
    SLTOut("offset is %d: pre=%08x, mid=%08x, post=%08x\n",
           time_offset, pre, mid, post);
}