/* $Id: master3.cpp,v 1.52 2012-04-04 22:34:40 eiki Exp $ */

#define MASTER_CC
#include "pcommon3.h"
#include "comm3.h"

int problemMode();  // in sbody3.h  FIXME here?

//******** data decl   FIXME s/b here? ********

//cmdPacketC cmd2send;
static replyPacketC rpypkt;
static replyEntryC  rpyent(&rpypkt);
int exitPending, exitAcked;

#define NOSIDE (-1)
int compTurn = NOSIDE;
extern int root_turn, depth_limit;
#define Flip(turn)          ((turn)^1)

#define PR1 1

  // FIXME bogus
#define CMDCHK_INTERVAL_MS 1000
#define MASTER_INTERVAL 1000

int cmdchkTick, detected, expired, touched, inhFirst, waitRoot;
extern "C" int detect_signals_master();
extern int last_root_value;
int microsleepMaster(int);

 // need compTurn
#include "plane.h"
#include "time.h"

planeC plane;

//******** I/F with Bonanza ********

void iniGameHook(const min_posi_t* posi) {
 MSDOut("---- iniGameHook called\n");
 plane.clear();
 pfGame.clear();

 cmd2send.setCmdSetroot(posi);
 forr(pr, 1, Nproc-1)
   cmd2send.send(pr);

 root_turn = posi->turn_to_move;

} // iniGameHook()


void makeMoveRootHook(int move) {
 MSDOut("---- makeMoveRootHook called\n");
 mvC mv2rt = mvC(move);
 plane.makeMoveRoot(mv2rt);

 cmd2send.setCmdFwd(mv2rt);
 forr(pr, 1, Nproc-1)
   cmd2send.send(pr);

 //root_turn = Flip(root_turn);  12/19/2011 %50 this is done in mkmvroot
} // makeMoveRootHook()


void unmakeMoveRootHook() {
 MSDOut("---- unmakeMoveRootHook called\n");
 plane.unmakeMoveRoot();

 cmd2send.setCmdRwd();
 forr(pr, 1, Nproc-1)
   cmd2send.send(pr);

 //root_turn = Flip(root_turn);
} // unmakeMoveRootHook()


void quitHook() {
 MSTOut("==== perf summary over game ====\n");
 pfGame.print();
}


void sendQuit(int proc) {
 cmd2send.clear();
 cmd2send.setCmdQuit();
 cmd2send.send(proc);
}


//******** master() ******** **********************************************

static void handleReplyMaster(); // defined below

int master(unsigned int* retmvseq) {

  if (problemMode())
    MAX_SRCH_DEP = depth_limit;

   // reset existing results

  plane.lastDoneItd = 0;

   // set compTurn when master() is first called

  if (compTurn == NOSIDE)
    compTurn = root_turn;

  // 1. ROOT/FIRST/LIST issue
 
 plane.initialAction();

#if 0
    // ROOT case - FIXME delete this block
   cmd2send.setCmdRoot();
   cmd2send.send(PR1);     // FIXME primitive version for now.  tune later

   cmd2send.setCmdPicked(); // send Root to all proc, pick one w/ deepest result
   cmd2send.send(PR1);
#endif

  // 2. loop begin
 detected = expired = cmdchkTick = touched = 0;
 exitPending = exitAcked = 0;
 //inhFirst = 1;

 while (1) {

   // detect/expire check
   // if yes, send STOP and wait

   if (cmdchkTick++ > CMDCHK_INTERVAL_MS) {
     if (DBG_INTERVAL_CHK)
        MSTOut("%16lld> cmdchk called\n", worldTimeLl());
     cmdchkTick = 0;

     if (!detected && !expired) {
       detected = detect_signals_master();
       if (!detected)
          expired = timeCheck();

       if (detected || expired) {
         MSTOut("____ sig detected det %d exp %d\n", detected, expired);

         if (!exitPending) {
           exitPending = (1 << Nproc) - 2;  // bits[Nproc-1:1] are set
           touched = 0;      // so reply won't come after FWD
           //cmd2send.setCmdCancel(0, detected ? 1 : 0);  // cancel all cmd
           cmd2send.setCmdStop();
           forr(pr, 1, Nproc-1)
             cmd2send.send(pr);
         }
       }
     }
   }

   forr(iijjkk, 1, 10) {

     // check replies
     rpyent.getReply();
     if (!rpyent.havereply())
         break;
     touched = 1;

     handleReplyMaster();

   } // forr iijjkk

   // termination (deep enuf / mate[d]) check

   if (!detected && !expired && !exitPending && !exitAcked) {
     bool termin = plane.terminating();
     if (termin) {
        exitPending = (1 << Nproc) - 2;  // bits[Nproc-1:1] are set
                          // wait for ack before exit
        touched = 0;      // so reply won't come after FWD
        cmd2send.setCmdStop();  // cancel all cmd
        forr(pr, 1, Nproc-1)
          cmd2send.send(pr);
     }
   }

   // if ending (terminate or exitAck), return

   if (exitAcked /* || srched deep enuf */) {
     int val;
     mvC mvs[2];
     plane.finalAnswer(&val, mvs);
     last_root_value = root_turn ? - val : val;  // flip by turn - v from black
     forr(j, 0, 1)
       retmvseq[j+1] = mvs[j].v;  // 12/3/2011 %28 was [j] = [j]
     return (mvs[0]==NULLMV ? 0 : mvs[1]==NULLMV ? 1 : 2);  // FIXME? right?
   }

   if (!touched || exitPending) {
     microsleepMaster(MASTER_INTERVAL);
     continue;
   }

   plane.catchupCommits();  // commit finished rows

   bool xfer = plane.transferIfNeeded(); // send EXT/SHR if needed

   if (!xfer)
     plane.next1stIfNeeded(); // send next 1st if needed

 } // while 1

} // master()

 //******** handleReplyMaster() ********************

static void dumpRpySetpv();
static void dumpRpySetlist();
static void dumpRpyFirst();
static void dumpRpyRetrying();
static void dumpRpyStopack();
static void dumpRpyRoot();
static void dumpRpyFcomp();
static void dumpRpyPvs();

static void handleReplyMaster() {
 assert(rpyent.havereply());
 int opc = rpyent.opcode();
 int exd = rpyent.exd();
 MSDOut("++++++ handleRpy entered opc %d exd %d\n", opc, exd);

 if (exitPending && (opc != RPY_OPCODE_STOPACK)) {
   // waiting to exit.  do nothing, just ignore reply except ack
 }
  //******** RPY_SETPV **
 else if (opc==RPY_OPCODE_SETPV) {
   dumpRpySetpv();
   mvC pv[GMX_MAX_PV];
   forr(i, 0, rpyent.pvleng()-1)
       pv[i] =  rpyent.pv(i);
   plane.rpySetpv(rpyent.itd(), rpyent.pvleng(), pv);
 }
   //******** RPY_SETLIST **
 else if (opc==RPY_OPCODE_SETLIST) {
   dumpRpySetlist();
   mvC mvs[GMX_MAX_LEGAL_MVS];  // 11/29/2011 %11 was GMX_MAX_PV
   forr(i, 0, rpyent.mvcnt()-1)
       mvs[i] =  rpyent.mv(i);
   plane.rpySetlist(rpyent.itd(), rpyent.exd(), rpyent.mvcnt(), mvs);
 }
  //******** RPY_FIRST **
 else if (opc==RPY_OPCODE_FIRST) {
   dumpRpyFirst();
   mvC bestseq[GMX_MAX_BESTSEQ];
   forr(i, 0, rpyent.seqleng_first()-1)
       bestseq[i] =  rpyent.bestseq_first(i);
   plane.rpyFirst(rpyent.itd(), rpyent.exd(), rpyent.val());
                    // ,rpyent.seqleng_first(), bestseq);
 }
   //******** RPY_RETRYING **
 else if (opc==RPY_OPCODE_RETRYING) {
   dumpRpyRetrying();
   plane.rpyRetrying(rpyent.itd(), rpyent.exd(),
                    rpyent.retrymv(), rpyent.rank());
 }
   //******** RPY_STOPACK **
 else if (opc==RPY_OPCODE_STOPACK) {
   dumpRpyStopack();
   exitPending &= ~(1 << rpyent.rank());
   if (exitPending == 0)
     exitAcked = 1;
 }

   //******** RPY_ROOT  **
 else if (opc==RPY_OPCODE_ROOT) {
   dumpRpyRoot();
   plane.rpyRoot(rpyent.itd(), rpyent.val(),  // 12/25/2011 %57 was exd instead
                    rpyent.rootmv(), rpyent.secondmv());    // of val
 }

   //******** RPY_FCOMP **
 else if (opc==RPY_OPCODE_FCOMP) {
   dumpRpyFcomp();
   int seqleng = rpyent.seqleng_first();
   mvC rpyseq[GMX_MAX_PV];
   forr(i, 0, seqleng-1)
       rpyseq[i] =  rpyent.bestseq_first(i);
   plane.rpyFcomp(rpyent.itd(), rpyent.exd(), rpyent.val(), seqleng, rpyseq);
   inhFirst = 0;
 }

   //******** RPY_PVS **
 else if (opc==RPY_OPCODE_PVS) {
   dumpRpyPvs();
   int seqleng = rpyent.seqleng_pvs();
   mvC rpyseq[GMX_MAX_PV];
   forr(i, 0, seqleng-1)
       rpyseq[i] =  rpyent.bestseq_pvs(i);
   plane.rpyPvs(rpyent.rank(), rpyent.itd(), rpyent.exd(), rpyent.val(),
        rpyent.pvsmv(),rpyent.ule(), rpyent.numnode(), seqleng, rpyseq);
 }
 else
   MSTOut("++++ WARNING: unknown reply opcode\n");

 rpyent.erase();  // 11/25/2011 %2 was missing

} // handleReplyMaster()

 //******** dump routines ****************

static void dumpRpySetpv() {
   if (DBG_DUMP_COMM) {
     MSTOut("%8d>&&&&&&&& RPY_SETPV %d: itd %d leng %d\npv: ",
            worldTime(), rpyent.rank(), rpyent.itd(), rpyent.pvleng());
     forr(i, 0, rpyent.pvleng()-1)
       MSTOut(" %07x", readable(rpyent.pv(i)));
     MSTOut("\n");
   }
}
static void dumpRpySetlist() {
   if (DBG_DUMP_COMM) {
     MSTOut("%8d>&&&&&&&& RPY_SETLIST %d: itd %d exd %d mvcnt %d\nmvs: ",
        worldTime(), rpyent.rank(), rpyent.itd(), rpyent.exd(), rpyent.mvcnt());
     forr(i, 0, rpyent.mvcnt()-1) {
       MSTOut(" %07x", readable(rpyent.mv(i)));  // FIXME bestmv too
       if ((i%6)==5) MSTOut("\n");
     }
     MSTOut("\n");
   }
}
static void dumpRpyFirst() {
   if (DBG_DUMP_COMM) {
 MSTOut("%8d>&&&&&&&& RPY_FIRST %d: itd %d exd %d val %d seqleng %d\nbestseq: ",
           worldTime(),rpyent.rank(), rpyent.itd(), rpyent.exd(), rpyent.val(),
           rpyent.seqleng_first());
     forr(i, 0, rpyent.seqleng_first()-1) {
       MSTOut(" %07x", readable(rpyent.bestseq_first(i)));
       if ((i%6)==5) MSTOut("\n");
     }
     MSTOut("\n");
   }
}
static void dumpRpyRetrying() {
   if (DBG_DUMP_COMM) {
     MSTOut("%8d>&&&&&&&& RPY_RETRYING %d: itd %d exd %d retrymv %07x\n",
        worldTime(), rpyent.rank(), rpyent.itd(), rpyent.exd(),
        readable(rpyent.retrymv()));
   }
}
static void dumpRpyStopack() {
   if (DBG_DUMP_COMM) {
     MSTOut("%8d>&&&&&&&& RPY_STOPACK\n", worldTime());
   }
}
static void dumpRpyRoot() {
   if (DBG_DUMP_COMM) {
     MSTOut("%8d>&&&&&&&& RPY_ROOT %d: itd %d val %d rootmv %07x %07x\n",
        worldTime(), rpyent.rank(), rpyent.itd(), rpyent.val(),
        readable(rpyent.rootmv()), readable(rpyent.secondmv()));
   }
}
static void dumpRpyFcomp() {
   if (DBG_DUMP_COMM) {
     MSTOut("%8d>&&&&&&&& RPY_FCOMP %d: itd %d exd %d val %d seqleng %d\n",
        worldTime(), rpyent.rank(), rpyent.itd(), rpyent.exd(), rpyent.val(),
        rpyent.seqleng_first());
     forr(i, 0, rpyent.seqleng_first()-1) {
       MSTOut(" %07x", readable(rpyent.bestseq_first(i)));
       if ((i%6)==5) MSTOut("\n");
     }
     MSTOut("\n");
   }
}
static void dumpRpyPvs() {
   if (DBG_DUMP_COMM) {
     MSTOut(
"%8d>&&&&RPY_PVS %d: itd %d exd %d val %d mv %07x ule %d num %d seqlen %d\nbestseq: ",
        worldTime(),   rpyent.rank(), rpyent.itd(), rpyent.exd(), rpyent.val(),
        readable(rpyent.pvsmv()),rpyent.ule(),
        rpyent.numnode(), rpyent.seqleng_pvs());
     forr(i, 0, rpyent.seqleng_pvs()-1) {
       MSTOut(" %07x", readable(rpyent.bestseq_pvs(i)));
       if ((i%6)==5) MSTOut("\n");
     }
     MSTOut("\n");
   }
}

