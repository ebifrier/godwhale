/* $Id: comm3.h,v 1.16 2012-04-19 22:25:07 eiki Exp $ */

#define DBG_DUMP_COMM 1

 // FIXME not needed?
#define ROOT_PARALLEL 0

#define MAX_EXTEND_MOVES 10

 // for [s]plane
#define MAX_EXPDEP  12
#define MAX_SEQPREV MAX_EXPDEP
#define MAXPROC    16
#define MAX_ITD    32
#define MAX_XFER_MVS  20

#define HVLST_MVONLY  (1 << 29)

struct tripleC { int alpha, beta, gamma; };

#define effalpha(a,g) ((a) > - score_bound ? (a) : (g))

/*
commands:

- SETROOT       pos(handB, handW, turn, asq[81])
- FWD           fwdmv
- RWD            -
- QUIT           -

- STOP           -
- ROOT           -
- WARM           -

- FIRST         ITD, haveList, pvleng, pv[]
- LIST          ITD, EXD, mvcnt, alpha,
                haveList(one processor only), onerepMask ("),
                pvleng, pv[],
                <mv, bestmv>[] 
- NOTIFY        ITD, EXD, new_alpha
- EXTEND        ITD, EXD, mvcnt, mv[MAX_EXTEND_MOVES], bestmv[]
- SHRINK        ITD, EXD, mv_location
- COMMIT        ITD, EXD
- CANCEL        ITD, EXD
*/

enum
{
    CMD_OPCODE_NULL    = 0,
    CMD_OPCODE_SETROOT = 1,
    CMD_OPCODE_FWD        ,   // 2
    CMD_OPCODE_RWD        ,   // 3
    CMD_OPCODE_QUIT       ,   // 4
    CMD_OPCODE_ROOT       ,   // 5
    CMD_OPCODE_WARM       ,   // 6
    CMD_OPCODE_FIRST      ,   // 7
    CMD_OPCODE_LIST       ,   // 8
    CMD_OPCODE_NOTIFY     ,   // 9
    CMD_OPCODE_EXTEND     ,   // a
    CMD_OPCODE_SHRINK     ,   // b
    CMD_OPCODE_CANCEL     ,   // c
    CMD_OPCODE_COMMIT     ,   // d
    CMD_OPCODE_VERIFY     ,   // e
    CMD_OPCODE_STOP       ,   // f
    CMD_OPCODE_PICKED     ,   //10
    CMD_OPCODE_END,      // 0x11
};


enum
{
    CMD_LOC_MAGIC    = 0,
    CMD_LOC_SIZE     = 1,
    CMD_LOC_OPCODE   = 2,
    CMD_LOC_ITD      = 3,
    CMD_LOC_EXD      = 4,
    CMD_LOC_MVCNT    = 5,
    CMD_LOC_TUPLE_OFS_EXTEND    = 6,
    CMD_LOC_ALPHA    = 6,
    CMD_LOC_HAVELIST_LIST = 7,  // 1/27/2011 not used, replaced by firstmv
    CMD_LOC_FIRSTMV = 7,
    CMD_LOC_ONEREP   = 8,
    CMD_LOC_PVLENG_LIST  = 9,
    CMD_LOC_BETA     = 10,
    CMD_LOC_PV_OFS_LIST  = 11,
    //CMD_LOC_PAIR_OFS = 19,
    CMD_LOC_PAIR_OFS = CMD_LOC_PV_OFS_LIST + MAX_EXPDEP,
    CMD_LOC_HANDB    = 3,
    CMD_LOC_HANDW    = 4,
    CMD_LOC_TURN     = 5,
    CMD_LOC_ASQ_OFS  = 6,
    CMD_LOC_FWDMV    = 3,
    CMD_LOC_HAVELIST_FIRST = 4,
    CMD_LOC_PVLENG   = 5,
    CMD_LOC_PV_OFS   = 6,
    CMD_LOC_NEWALPHA = 5,
    CMD_LOC_NOTIFYMV = 6,
    CMD_LOC_MV_OFS   = 6,
    //CMD_LOC_BESTMV_OFS   = 16,
    CMD_LOC_MVLOC    = 5,
    CMD_LOC_TAIL     = 4,
    CMD_LOC_ABGOFS   = 5 
};



//**** command ****

#define GMX_MAGIC_CMD    (0x1eace394)
#define GMX_MAX_PV       MAX_EXPDEP
#define GMX_MAX_BESTSEQ  MAX_EXPDEP
#define GMX_MAX_LEGAL_MVS  600
#define GMX_MAX_CMD_PACKET  2400

struct mvtupleC
{
    mvC mv, bestmv; 
    int depth;
    short upper, lower;

    mvtupleC() {}
    mvtupleC(mvC m, mvC b, int d, short u, short l)
        : mv(m), bestmv(b), depth(d), upper(u), lower(l) {}
};

static int mergeint(short a, short b)
{
    unsigned int x = a + (1<<15);
    unsigned int y = b + (1<<15);
    return (int)((x<<16) | y);
}

static void splitint(int n, short *a, short *b)
{
    unsigned int z = (unsigned)n;
    unsigned int x = z >> 16;
    unsigned int y = z & 0xffff;

    *a = (short)((int)x - (1<<15));
    *b = (short)((int)y - (1<<15));
}

class cmdPacketC
{
public:
  int v[GMX_MAX_CMD_PACKET];

  cmdPacketC() { v[CMD_LOC_MAGIC] = GMX_MAGIC_CMD; }
  void clear(); // erase all
  void erase() { int sz = v[CMD_LOC_SIZE]; //12/10/2010 #1 was forr(i,1,v[..]-1)
                 forr(i,1,sz-1) v[i] = 0; }  // erase just enough
  bool havecmd() { return (CMD_OPCODE_NULL   < v[CMD_LOC_OPCODE] &&
                           v[CMD_LOC_OPCODE] < CMD_OPCODE_END);  }

   // methods for master sending

  void setCmdRwd()  { v[CMD_LOC_OPCODE] = CMD_OPCODE_RWD;  v[CMD_LOC_SIZE] = 3;
                      if (DBG_DUMP_COMM)  MSTOut("######## CMD_RWD\n");       }
  void setCmdQuit() { v[CMD_LOC_OPCODE] = CMD_OPCODE_QUIT; v[CMD_LOC_SIZE] = 3;
                      if (DBG_DUMP_COMM)  MSTOut("######## CMD_QUIT\n");       }
  void setCmdRoot() { v[CMD_LOC_OPCODE] = CMD_OPCODE_ROOT; v[CMD_LOC_SIZE] = 3;
                      if (DBG_DUMP_COMM)  MSTOut("######## CMD_ROOT\n");       }
  void setCmdWarm() { v[CMD_LOC_OPCODE] = CMD_OPCODE_WARM; v[CMD_LOC_SIZE] = 3;
                      if (DBG_DUMP_COMM)  MSTOut("######## CMD_WARM\n");       }

  void setCmdPicked() { v[CMD_LOC_OPCODE] = CMD_OPCODE_PICKED; v[CMD_LOC_SIZE] = 3;
                      if (DBG_DUMP_COMM)  MSTOut("######## CMD_PICKED\n");       }
  void setCmdStop() { v[CMD_LOC_OPCODE] = CMD_OPCODE_STOP; v[CMD_LOC_SIZE] = 3;
                      if (DBG_DUMP_COMM)  MSTOut("######## CMD_STOP\n");       }

  void setCmdFwd(mvC move) {
    if (DBG_DUMP_COMM)
      MSTOut("%8d>######## CMD_FWD: mv %07x\n", worldTime(), readable(move));
    v[CMD_LOC_OPCODE] = CMD_OPCODE_FWD;
    v[CMD_LOC_FWDMV] = move.v;
    v[CMD_LOC_SIZE] = CMD_LOC_FWDMV+1;
  }

  void setCmdSetroot(const min_posi_t* posp)   {
    if (DBG_DUMP_COMM)  MSTOut("######## CMD_SETROOT\n"); // FIXME pos dump?
    v[CMD_LOC_OPCODE] = CMD_OPCODE_SETROOT;
    v[CMD_LOC_HANDB] = (int)posp->hand_black;
    v[CMD_LOC_HANDW] = (int)posp->hand_white;
    v[CMD_LOC_TURN ] = (int)posp->turn_to_move;
    forr(i,0,80)
      v[CMD_LOC_ASQ_OFS + i] = (int)posp->asquare[i];
    v[CMD_LOC_SIZE] = CMD_LOC_ASQ_OFS + 81;
  }

  void setCmdFirst(int itd, int havelist, int pvleng, mvC* pv) {
    if (DBG_DUMP_COMM) {
      MSTOut("%8d>######## CMD_FIRST: itd %d havelist %04x pvleng %d\npv: ",
             worldTime(), itd, havelist, pvleng);
      forr(i, 0, pvleng-1) {
        MSTOut(" %07x", readable(pv[i]));
        if ((i%6)==5) MSTOut("\n");
      }
      MSTOut("\n");
    }

    v[CMD_LOC_OPCODE] = CMD_OPCODE_FIRST;
    v[CMD_LOC_ITD   ] = itd;
    v[CMD_LOC_HAVELIST_FIRST] = havelist;
    v[CMD_LOC_PVLENG] = pvleng;

    assert(pvleng <= GMX_MAX_PV);
    forr(i,0,pvleng-1) 
      v[CMD_LOC_PV_OFS + i] = pv[i].v;
    v[CMD_LOC_SIZE] = CMD_LOC_PV_OFS + pvleng;
  }

  void setCmdList(int itd, int exd, int mvcnt,int alpha, int beta,mvC firstmv,
                  int pvleng, mvC* pv, mvtupleC* tuples) {
    if (DBG_DUMP_COMM) {
      MSTOut("%8d>######## CMD_LIST: itd %d exd %d mvcnt %d A %d B %d 1stmv %07x pvleng %d\npv: ",
             worldTime(), itd, exd, mvcnt, alpha, beta, readable(firstmv), pvleng);
      forr(i, 0, pvleng-1) {
        MSTOut(" %07x", readable(pv[i]));
        if ((i%6)==5) MSTOut("\n");
      }
      MSTOut("\nmv/bestmv dep up lo:\n");
      forr(i, 0, mvcnt-1) {
        MSTOut(" %07x/%07x %d %d %d  ", readable(tuples[i].mv),
                readable(tuples[i].bestmv), tuples[i].depth,
                tuples[i].upper, tuples[i].lower);
        if ((i%2)==1) MSTOut("\n");
      }
      MSTOut("\n");
    }
    v[CMD_LOC_OPCODE] = CMD_OPCODE_LIST;
    v[CMD_LOC_ITD   ] = itd;
    v[CMD_LOC_EXD   ] = exd;
    v[CMD_LOC_ALPHA ] = alpha;
    v[CMD_LOC_MVCNT ] = mvcnt;
    //v[CMD_LOC_HAVELIST_LIST] = havelist;
    v[CMD_LOC_FIRSTMV] = firstmv.v;
    v[CMD_LOC_ONEREP] = 0;   // FIXME remove
    v[CMD_LOC_BETA ]  = beta;

    v[CMD_LOC_PVLENG_LIST] = pvleng;
    forr(i,0,pvleng-1)
      v[CMD_LOC_PV_OFS_LIST+i] = pv[i].v;

    forr(i,0,mvcnt-1) { 
      v[CMD_LOC_PAIR_OFS + 4*i    ] = tuples[i].mv.v;
      v[CMD_LOC_PAIR_OFS + 4*i + 1] = tuples[i].bestmv.v;
      v[CMD_LOC_PAIR_OFS + 4*i + 2] = tuples[i].depth;
      v[CMD_LOC_PAIR_OFS + 4*i + 3] = mergeint(tuples[i].upper,tuples[i].lower);
    }
    v[CMD_LOC_SIZE] = CMD_LOC_PAIR_OFS + 4*mvcnt;
  }

  void setCmdExtend(int itd, int exd, int mvcnt, mvtupleC* tuples) {
    if (DBG_DUMP_COMM) {
      MSTOut("######## CMD_EXTEND: itd %d exd %d mvcnt %d   mv/bestmv:\n",
             itd, exd, mvcnt);
      forr(i, 0, mvcnt-1) {
        MSTOut(" %07x/%07x %d %d %d", readable(tuples[i].mv),
                                      readable(tuples[i].bestmv),
                                      tuples[i].depth,
                                      tuples[i].upper,
                                      tuples[i].lower );
        if ((i%2)==1) MSTOut("\n");
      }
      MSTOut("\n");
    }
    v[CMD_LOC_OPCODE] = CMD_OPCODE_EXTEND;
    v[CMD_LOC_ITD   ] = itd;
    v[CMD_LOC_EXD   ] = exd;
    v[CMD_LOC_MVCNT ] = mvcnt;
    assert(mvcnt <= MAX_EXTEND_MOVES);

    forr(i,0,mvcnt-1) { 
      v[CMD_LOC_TUPLE_OFS_EXTEND + 4*i    ] = tuples[i].mv.v;
      v[CMD_LOC_TUPLE_OFS_EXTEND + 4*i + 1] = tuples[i].bestmv.v;
      v[CMD_LOC_TUPLE_OFS_EXTEND + 4*i + 2] = tuples[i].depth;
      v[CMD_LOC_TUPLE_OFS_EXTEND + 4*i + 3] =
            mergeint(tuples[i].upper,tuples[i].lower);
    }
    v[CMD_LOC_SIZE] = CMD_LOC_TUPLE_OFS_EXTEND + 4*mvcnt;
  }

  void setCmdShrink(int itd, int exd, int mvloc) {
    if (DBG_DUMP_COMM) 
      MSTOut("######## CMD_SHRINK: itd %d exd %d mvloc %d\n",
             itd, exd, mvloc);
    v[CMD_LOC_OPCODE] = CMD_OPCODE_SHRINK;
    v[CMD_LOC_ITD   ] = itd;
    v[CMD_LOC_EXD   ] = exd;
    v[CMD_LOC_MVLOC ] = mvloc;

    v[CMD_LOC_SIZE] = CMD_LOC_MVLOC+1;
  }

  void setCmdNotify(int itd, int exd, int newalpha, mvC notifymv) {
    if (DBG_DUMP_COMM) 
      MSTOut("######## CMD_NOTIFY: itd %d exd %d newalpha %d mv %07x\n",
             itd, exd, newalpha, readable(notifymv));
    v[CMD_LOC_OPCODE] = CMD_OPCODE_NOTIFY;
    v[CMD_LOC_ITD   ] = itd;
    v[CMD_LOC_EXD   ] = exd;
    v[CMD_LOC_NEWALPHA ] = newalpha;
    v[CMD_LOC_NOTIFYMV ] = notifymv.v;

    v[CMD_LOC_SIZE] = CMD_LOC_NOTIFYMV+1;
  }

  void setCmdCommit(int itd, int exd) {
    if (DBG_DUMP_COMM) 
      MSTOut("######## CMD_COMMIT: itd %d exd %d\n", itd, exd);
    v[CMD_LOC_OPCODE] = CMD_OPCODE_COMMIT;
    v[CMD_LOC_ITD   ] = itd;
    v[CMD_LOC_EXD   ] = exd;

    v[CMD_LOC_SIZE] = CMD_LOC_EXD+1;
  }

  void setCmdCancel(int itd, int exd) {
    if (DBG_DUMP_COMM) 
      MSTOut("######## CMD_CANCEL: itd %d exd %d\n", itd, exd);
    v[CMD_LOC_OPCODE] = CMD_OPCODE_CANCEL;
    v[CMD_LOC_ITD   ] = itd;
    v[CMD_LOC_EXD   ] = exd;

    v[CMD_LOC_SIZE] = CMD_LOC_EXD+1;
  }

  void setCmdVerify(int itd, int tail, tripleC* p) {
    if (DBG_DUMP_COMM) 
      MSTOut("######## CMD_VERIFY: itd %d tail %d\n", itd, tail);
    v[CMD_LOC_OPCODE] = CMD_OPCODE_VERIFY;
    v[CMD_LOC_ITD   ] = itd;
    v[CMD_LOC_TAIL  ] = tail;

    forr(i,0,11) { 
      v[CMD_LOC_ABGOFS+3*i  ] = p[i].alpha;
      v[CMD_LOC_ABGOFS+3*i+1] = p[i].beta;
      v[CMD_LOC_ABGOFS+3*i+2] = p[i].gamma;
    }

    v[CMD_LOC_SIZE] = CMD_LOC_ABGOFS+36;
  }

  void send(int proc) {
    sendPacket(proc, v, v[CMD_LOC_SIZE]);
    if (DBG_DUMP_COMM) 
      MSTOut("########---- send CMD to rank %d\n", proc);
  }

   // methods for slave using/reading

  bool verifyMagic() { return (v[CMD_LOC_MAGIC] == GMX_MAGIC_CMD); }
  int& opcode() { return (v[CMD_LOC_OPCODE]); }
  int size()   { return (v[CMD_LOC_SIZE]); }
  int itd()    { return (v[CMD_LOC_ITD]); }
  int exd()    { return (v[CMD_LOC_EXD]); }
  int alpha()    { return (v[CMD_LOC_ALPHA]); }
  int beta()     { return (v[CMD_LOC_BETA]); }
  int mvcnt()    { return (v[CMD_LOC_MVCNT]); }
  int onerep()   { return (v[CMD_LOC_ONEREP]); }
  //int havelist_list()     { return (v[CMD_LOC_HAVELIST_LIST]); }
  mvC firstmv()    { return mvC(v[CMD_LOC_FIRSTMV]); }
  int havelist_first()    { return (v[CMD_LOC_HAVELIST_FIRST]); }
  //mvC pairmv(int i)       { return mvC(v[CMD_LOC_PAIR_OFS + 2*i]); }
  //mvC pairbestmv(int i)   { return mvC(v[CMD_LOC_PAIR_OFS + 2*i + 1]); }
  mvtupleC tuple(int i) {
    short a, b;
    splitint(v[CMD_LOC_PAIR_OFS + 4*i + 3], &a, &b);
    return mvtupleC(mvC(v[CMD_LOC_PAIR_OFS + 4*i    ]),
                    mvC(v[CMD_LOC_PAIR_OFS + 4*i + 1]),
                        v[CMD_LOC_PAIR_OFS + 4*i + 2], a, b);
  }
  mvtupleC tuple_extend(int i) {
    short a, b;
    splitint(v[CMD_LOC_TUPLE_OFS_EXTEND + 4*i + 3], &a, &b);
    return mvtupleC(mvC(v[CMD_LOC_TUPLE_OFS_EXTEND + 4*i    ]),
                    mvC(v[CMD_LOC_TUPLE_OFS_EXTEND + 4*i + 1]),
                        v[CMD_LOC_TUPLE_OFS_EXTEND + 4*i + 2], a, b);
  }
  int handb()    { return (v[CMD_LOC_HANDB]); }
  int handw()    { return (v[CMD_LOC_HANDW]); }
  int turn()     { return (v[CMD_LOC_TURN]); }
  int asquare(int sq)    { return (v[CMD_LOC_ASQ_OFS + sq]); }
  mvC fwdmv()            { return mvC(v[CMD_LOC_FWDMV]); }
  int pvleng()           { return (v[CMD_LOC_PVLENG]); }
  mvC pv(int i)          { return mvC(v[CMD_LOC_PV_OFS + i]); }
  int pvleng_list()           { return (v[CMD_LOC_PVLENG_LIST]); }
  mvC pv_list(int i)          { return mvC(v[CMD_LOC_PV_OFS_LIST + i]); }
  int newalpha()         { return (v[CMD_LOC_NEWALPHA]); }
  mvC notifymv()         { return mvC(v[CMD_LOC_NOTIFYMV]); }
  int mvloc()            { return (v[CMD_LOC_MVLOC]); }
  mvC mv(int i)          { return mvC(v[CMD_LOC_MV_OFS + i]); }
  //mvC bestmv(int i)          { return mvC(v[CMD_LOC_BESTMV_OFS + i]); }
  int tail()            { return (v[CMD_LOC_TAIL]); }
  int valpha(int i)          { return (v[CMD_LOC_ABGOFS + 3*i    ]); }
  int vbeta(int i)           { return (v[CMD_LOC_ABGOFS + 3*i + 1]); }
  int vgamma(int i)          { return (v[CMD_LOC_ABGOFS + 3*i + 2]); }

  void getCmd() {
      // if cmd already exists, do nothing
    if (havecmd()) return;

      // now we have no cmd.  get new pkt; if none exists, can't help
    if (!probePacketSlave()) return;

      // some cmd has come.  get MPI packet
    assert(Mproc);   // this method is for slave only
    recvPacket(MASTERRANK, v);
  }
};

#ifndef MASTER_CC
void cmdPacketC::clear() {memset(this, 0, sizeof(cmdPacketC));}
#endif

//**************  reply entry ****************

/*
replies:

- setpv        ITDep, pvleng, pv[]
- setList      ITD, EXD, mvcnt, mv[]
- first        ITD, completed EXD, val, seqleng, bestseq[] 
- pvs          ITD, EXD, val, mv, up/lo/ex,
               seqleng, bestseq[] (seq[] if alpha update, else 1mv)
- retrying     ITD, EXD, mv
- stopack      -
- fcomp        ITD,      mv, mv2   (FIXME s/b mv[]?)

*/

enum
{
    RPY_OPCODE_NULL    = 0,
    RPY_OPCODE_SETPV   = 1,
    RPY_OPCODE_SETLIST    ,   // 2
    RPY_OPCODE_FIRST      ,   // 3
    RPY_OPCODE_PVS        ,   // 4
    RPY_OPCODE_RETRYING   ,   // 5
    RPY_OPCODE_ROOT       ,   // 6
    RPY_OPCODE_STOPACK    ,   // 7
    RPY_OPCODE_FCOMP      ,   // 8
    RPY_OPCODE_END        ,   // 9
};

enum
{
    RPY_LOC_SIZE     = 0,
    RPY_LOC_OPCODE   = 1,
    RPY_LOC_ITD      = 2,
    RPY_LOC_EXD      = 3,
    RPY_LOC_VAL      = 4,
    RPY_LOC_PVSMV    = 5,
    RPY_LOC_ULE      = 6,
    RPY_LOC_NUMNODE  = 7,
    RPY_LOC_SEQLENG_PVS  = 8,
    RPY_LOC_BESTSEQ_OFS_PVS  = 9,
    RPY_LOC_PVLENG   = 3,
    RPY_LOC_PV_OFS   = 4,
    RPY_LOC_MVCNT    = 4,
    RPY_LOC_MVOFS    = 5,
    RPY_LOC_SEQLENG_FIRST= 5,
    RPY_LOC_BESTSEQ_OFS_FIRST= 6,
    RPY_LOC_RETRYMV  = 4,
    RPY_LOC_ROOTMV   = 3,
    RPY_LOC_SECONDMV = 5,
};

#define GMX_MAX_RPY_ENTRY   2400

#define RPY_RANK_BITPOS (16)
#define RPY_SIZE_MASK   ((1 << RPY_RANK_BITPOS) - 1)

class replyPacketC;  // defined below

class replyEntryC
{
public:
    replyPacketC* pktp;
    int v[GMX_MAX_RPY_ENTRY];

    replyEntryC(replyPacketC* p = NULL) : pktp(p) {}
    int size() { return (v[RPY_LOC_SIZE] & RPY_SIZE_MASK); }
    int rank() { return (v[RPY_LOC_SIZE] >> RPY_RANK_BITPOS); } // only in master
    void clear() { memset(v, 0, sizeof(v)); }
    void erase() { int sz = size(); forr(i,0,sz-1) v[i] = 0; } // erase just enuf
    bool havereply() { return (RPY_OPCODE_NULL < v[RPY_LOC_OPCODE] &&
                               v[RPY_LOC_OPCODE] < RPY_OPCODE_END); }

    // methods for slave setting
    void pushEntry();  // defined after rpyPacketC

    void pushSetpv(int itd, int pvleng, mvC* pv)
    {
        v[RPY_LOC_OPCODE] = RPY_OPCODE_SETPV;
        v[RPY_LOC_ITD   ] = itd;
        v[RPY_LOC_PVLENG] = pvleng;

        if (DBG_DUMP_COMM) {
            SLTOut("$$$$ rpy setpv  itd %d pvleng %d  pv: ", itd, pvleng);
        }

        forr (i, 0, pvleng-1) {
            v[RPY_LOC_PV_OFS + i] = pv[i].v;
            if (DBG_DUMP_COMM) {
                SLTOut(" %07x", readable(pv[i]));
            }
        }

        if (DBG_DUMP_COMM) SLTOut("\n");
        v[RPY_LOC_SIZE] = RPY_LOC_PV_OFS + pvleng;
        pushEntry();
    }

    void pushSetlist(int itd, int exd, int mvcnt, mvC* mv)
    {
        v[RPY_LOC_OPCODE] = RPY_OPCODE_SETLIST;
        v[RPY_LOC_ITD   ] = itd;
        v[RPY_LOC_EXD   ] = exd;
        v[RPY_LOC_MVCNT ] = mvcnt;

        if (DBG_DUMP_COMM) {
            SLTOut("$$$$ rpy setlist  itd %d exd %d mvcnt %d   mvs:\n",
                   itd, exd, mvcnt);
        }

        forr (i, 0, mvcnt-1) {
            v[RPY_LOC_MVOFS + i] = mv[i].v;
            if (DBG_DUMP_COMM) {
                SLTOut(" %07x", readable(mv[i]));
            }
        }

        if (DBG_DUMP_COMM) SLTOut("\n");
        v[RPY_LOC_SIZE] = RPY_LOC_MVOFS + mvcnt;
        pushEntry();
    }

    void pushFirst(int itd, int exd, int val, int seqleng, mvC* bestseq) {
        v[RPY_LOC_OPCODE] = RPY_OPCODE_FIRST;
        v[RPY_LOC_ITD   ] = itd;
        v[RPY_LOC_EXD   ] = exd;
        v[RPY_LOC_VAL   ] = val;
        v[RPY_LOC_SEQLENG_FIRST] = seqleng;

        if (DBG_DUMP_COMM) {
            SLTOut("%8d>$$$$ rpy first  itd %d exd %d val %d seqleng %d  bestseq:\n",
                   worldTime(), itd, exd, val, seqleng);
        }

        forr (i, 0, seqleng-1) {
            v[RPY_LOC_BESTSEQ_OFS_FIRST + i] = bestseq[i].v;
            if (DBG_DUMP_COMM) {
                SLTOut(" %07x", readable(bestseq[i]));
            }
        }

        if (DBG_DUMP_COMM) SLTOut("\n");
        v[RPY_LOC_SIZE] = RPY_LOC_BESTSEQ_OFS_FIRST + seqleng;
        pushEntry();
    }

    void pushFcomp(int itd, int exd, int val, int seqleng, mvC* bestseq) {
        v[RPY_LOC_OPCODE] = RPY_OPCODE_FCOMP;
        v[RPY_LOC_ITD   ] = itd;
        v[RPY_LOC_EXD   ] = exd;
        v[RPY_LOC_VAL   ] = val;
        v[RPY_LOC_SEQLENG_FIRST] = seqleng;

        if (DBG_DUMP_COMM) {
            SLTOut("%8d>$$$$ rpy fcomp  itd %d exd %d val %d seqleng %d  bestseq:\n",
                   worldTime(), itd, exd, val, seqleng);
        }

        forr (i, 0, seqleng-1) {
            v[RPY_LOC_BESTSEQ_OFS_FIRST + i] = bestseq[i].v;
            if (DBG_DUMP_COMM) {
                SLTOut(" %07x", readable(bestseq[i]));
            }
        }

        if (DBG_DUMP_COMM) SLTOut("\n");
        v[RPY_LOC_SIZE] = RPY_LOC_BESTSEQ_OFS_FIRST + seqleng;
        pushEntry();
    }

    void pushPvs(int itd, int exd, int val, mvC mv, int ule, int numnode,
                 int seqleng, mvC* bestseq)
    {
        v[RPY_LOC_OPCODE] = RPY_OPCODE_PVS;
        v[RPY_LOC_ITD   ] = itd;
        v[RPY_LOC_EXD   ] = exd;
        v[RPY_LOC_VAL   ] = val;
        v[RPY_LOC_PVSMV ] = mv.v;
        v[RPY_LOC_ULE   ] = ule;
        v[RPY_LOC_NUMNODE]= numnode;
        v[RPY_LOC_SEQLENG_PVS] = seqleng;

        if (DBG_DUMP_COMM) {
            SLTOut("%8d>$$$$ rpy pvs itd %d exd %d val %d mv %07x ule %d seqleng %d  bestseq:\n",
                   worldTime(),  itd, exd, val, readable(mv), ule, seqleng);
        }

        forr (i, 0, seqleng-1) {
            v[RPY_LOC_BESTSEQ_OFS_PVS + i] = bestseq[i].v;
            if (DBG_DUMP_COMM) {
                SLTOut(" %07x", readable(bestseq[i]));
            }
        }

        if (DBG_DUMP_COMM) SLTOut("\n");
        v[RPY_LOC_SIZE] = RPY_LOC_BESTSEQ_OFS_PVS + seqleng;
        pushEntry();
    }

    void pushRetrying(int itd, int exd, mvC mv)
    {
        v[RPY_LOC_OPCODE] = RPY_OPCODE_RETRYING;
        v[RPY_LOC_ITD   ] = itd;
        v[RPY_LOC_EXD   ] = exd;
        v[RPY_LOC_RETRYMV]= mv.v;
        v[RPY_LOC_SIZE]   = RPY_LOC_RETRYMV+1;

        if (DBG_DUMP_COMM) {
            SLTOut("%8d>$$$$ rpy retry itd %d exd %d mv %07x\n",
                   worldTime(),  itd, exd, readable(mv));
        }
        
        pushEntry();
    }

    void pushRoot(int itd, int val, mvC mv, mvC mv2)
    {
        v[RPY_LOC_OPCODE] = RPY_OPCODE_ROOT;
        v[RPY_LOC_ITD   ] = itd;
        v[RPY_LOC_ROOTMV] = mv.v;
        v[RPY_LOC_SECONDMV] = mv2.v;
        v[RPY_LOC_VAL   ] = val;
        v[RPY_LOC_SIZE]   = RPY_LOC_SECONDMV+1;

        if (DBG_DUMP_COMM) {
            SLTOut("%8d>$$$$ rpy root itd %d val %d mv %07x %07x\n",
                   worldTime(),  itd, val, readable(mv), readable(mv2));
        }

        pushEntry();
    }

    void pushStopack()
    {
        v[RPY_LOC_OPCODE] = RPY_OPCODE_STOPACK;
        v[RPY_LOC_SIZE] = RPY_LOC_OPCODE+1;

        if (DBG_DUMP_COMM) {
            SLTOut("%8d>$$$$ rpy stopack\n",
                   worldTime());
        }
        
        pushEntry();
    }

    // methods for master using/reading

    int opcode() { return v[RPY_LOC_OPCODE]; }
    int itd()    { return v[RPY_LOC_ITD]; }
    int exd()    { return v[RPY_LOC_EXD]; }
    int val()    { return v[RPY_LOC_VAL]; }
    mvC pvsmv()  { return mvC(v[RPY_LOC_PVSMV]); }
    int ule()    { return v[RPY_LOC_ULE]; }
    int numnode()      { return v[RPY_LOC_NUMNODE]; }
    int seqleng_pvs()      { return v[RPY_LOC_SEQLENG_PVS]; }
    mvC bestseq_pvs(int i) { return mvC(v[RPY_LOC_BESTSEQ_OFS_PVS+i]); }
    int pvleng()  { return v[RPY_LOC_PVLENG]; }
    mvC pv(int i) { return mvC(v[RPY_LOC_PV_OFS+i]); }
    int mvcnt()   { return v[RPY_LOC_MVCNT]; }
    mvC mv(int i) { return mvC(v[RPY_LOC_MVOFS+i]); }
    int seqleng_first()      { return v[RPY_LOC_SEQLENG_FIRST]; }
    mvC bestseq_first(int i) { return mvC(v[RPY_LOC_BESTSEQ_OFS_FIRST+i]); }
    mvC retrymv()  { return mvC(v[RPY_LOC_RETRYMV]); }
    // 12/11/2010 #21 was LOC_ITD
    mvC rootmv()   { return mvC(v[RPY_LOC_ROOTMV]); }
    mvC secondmv() { return mvC(v[RPY_LOC_SECONDMV]); }

    void getReply();  // defined after rpyPacketC
};


//**************  reply packet ****************

#define GMX_RPY_MAXENT MAX_EXPDEP

enum
{
    RPYPKT_LOC_MAGIC    = 0,
    RPYPKT_LOC_CNT      = 1,
    RPYPKT_LOC_PTR_OFS  = 2,
    RPYPKT_LOC_DATA_OFS = RPYPKT_LOC_PTR_OFS + GMX_RPY_MAXENT // 2+8=10
};

#define GMX_MAX_RPY_PACKET  19200


class replyPacketC
{
public:
    static const int GMX_MAGIC_RPY = 0x0ddbeefc;
    int poptop;
    int rank;
    int v[GMX_MAX_RPY_PACKET];

    int& magic()    { return v[RPYPKT_LOC_MAGIC]; }
    int& cnt()      { return v[RPYPKT_LOC_CNT]; }
    int& ptr(int i) { return v[RPYPKT_LOC_PTR_OFS + i]; }
    int size()
    {
        assert(0 <= cnt() && cnt() < GMX_RPY_MAXENT);
        return (cnt()==0 ? RPYPKT_LOC_DATA_OFS
                : (ptr(cnt()-1) + v[ptr(cnt()-1) + RPY_LOC_SIZE]));
    }

    void init();
    void erase()
    {
        int sz = size();
        forr (i, 1, sz-1) v[i] = 0;
    }

    // methods for slave setting

    void send()
    {
        sendPacket(MASTERRANK, v, size());
        if (DBG_DUMP_COMM) SLTOut("$$$$ send rpy\n");
        erase();
    }

    void pushEntry(replyEntryC* entp)
    {
        assert(cnt() < GMX_RPY_MAXENT);
        int ofs = ptr(cnt()) = size();
        int entsz = entp->size();

        forr (i, 0, entsz-1) {
            v[ofs+i] = entp->v[i];
        }
        cnt()++;
    }

    // methods for master using/reading

    void getReply(int i, replyEntryC* entp)
    {
        int sz = v[ptr(i) + RPY_LOC_SIZE];
        forr (j, 0, sz-1) {
            entp->v[j] = v[ptr(i) + j];
        }
        entp->v[RPY_LOC_SIZE] |= rank << RPY_RANK_BITPOS;
    }
};

#ifndef MASTER_CC
void replyPacketC::init()
{
    memset(this, 0, sizeof(replyPacketC));
    v[RPYPKT_LOC_MAGIC] = GMX_MAGIC_RPY;
}

//*************

void replyEntryC::pushEntry()
{
    assert(pktp != NULL);
    pktp->pushEntry(this);
}

void replyEntryC::getReply()
{
    // if reply already exists, do nothing
    if (havereply()) return;

    // now we have no reply in this entry
    // if one exists in packet, use it
    if (pktp->poptop < pktp->cnt()) {
        pktp->getReply(pktp->poptop++, this);
        return;
    }

    // no reply in ent/pkt.  get new pkt; if none exists, can't help
    int rk = probeProcessor();
    if (rk != NO_PENDING_REQUEST) { //#3 12/10/2010 was
        pktp->rank = rk;
        recvPacket(rk, pktp->v);
        assert(pktp->cnt() > 0);
        pktp->getReply(0, this);
        pktp->poptop = 1;
        return;
    }
    
    // no reply in ent/pkt, no pkt has come. no reply
    return;
}
#endif
