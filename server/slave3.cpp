/* $Id: slave3.cpp,v 1.64 2012-04-14 05:05:58 eiki Exp $ */

/* TODO 10/14/2011:
 - code splane.h - fill proce
 - ROOT: abort on #nod
 - FIRST: abort on #nod, prepare mvs on CMD_FIRST
 - LIST: set up/lo on LIST, notify on alpha update
 */

 // for INT_MAX
#include <limits.h>

#include "stdinc.h"
#include "comm3.h"
#include "splane.h"

#define DBG_DUMP_BOARD 0
#define MEASURE_TIME   1
#define ABORT_ON_1ST   0
#define USE_ADJUST_FMG 1

#define USE_ASP 1

// change to 0 if you want. with USE_ASP==0, this must be 0
#if (USE_ASP == 1)
#define USE_SRCHR 1
#else
#define USE_SRCHR 0
#endif

  // FIXME tune
//#define MAX_ROOT_SRCH_NODE  30000
//#define MAX_FIRST_NODE      30000
#define MAX_ROOT_SRCH_NODE_UP    50000
#define MAX_ROOT_SRCH_NODE_LIMIT 90000
#define MAX_FIRST_NODE_UP        14000
int MAX_ROOT_SRCH_NODE = MAX_ROOT_SRCH_NODE_UP;
int MAX_FIRST_NODE     = MAX_FIRST_NODE_UP;

 // for inRoot/inFirst checks -  FIXME bogus
#define MAX_ROOT_NODES  40000
#define MAX_FIRST_NODES 40000

tree_t *g_ptree;

class singleCmdC
{
public:
    // input
    int valid, issued;
    int haveList; // FIXME? need chkList? - do without it for now 10/30

    // out for ROOT, in for FIRST
    int itd, pvleng, opcode, fromRoot;
    mvC pv[GMX_MAX_PV];

    // output
    int val, exd, ule, bestseqLeng;
    mvC bestseq[GMX_MAX_BESTSEQ];

    void clear()
    {
        memset(this, 0, sizeof(singleCmdC));
    }
};

struct runningC
{
    jobDtorC job;
    int alpha, retrying, val, valid;

    void clear()
    {
        alpha = retrying = val = valid = 0;
        job.itd = RUNNING_NONE;
    }

    bool dropByShrink(int i, int e, int s)
    {
        return (job.itd == i && job.exd == e && job.mvsuf >= s);
    }

    bool dropByCommit(int i, int e)
    {
        return (job.itd == i && job.exd >= e);
    }
} running;

 //**************** data decl ****************

enum
{
    ACCROOT=0, ACCFIRST=1, ACCLIST=2, ACCWARM=3,
    ACCABORT=4, ACCEXPIRE=5, ACCSLEEP=6, ACCRETRY=7,
    ACCPUZL=8, ACCEND=9
};

const char* srchtypestr[ACCEND] =
{
    "Root", "First", "List", "Warm",
    "Abort", "Expire", "Sleep", "Retry",
    "Puzl"
};

#include "perfsl.h"

replyPacketC rpypkt;
replyEntryC rpyent(&rpypkt);

singleCmdC singleCmd;
splaneC    splane;
cmdPacketC pendingCmd;

mvC mpLastMove;

#define MAX_ROOT_PATH 32    // FIXME?
static int curPosPathLeng;
static mvC curPosPath[MAX_ROOT_PATH];
static int curPosPathChecks;

static int detected, expired, rwdLatch = 0;

 //**************** data decl end ****************

static void dumpSlState();  // defined below

  // in usec  FIXME tune - 50us ok?
#define SLAVE_INTERVAL 50

// copied from searchr.c, modified
static int next_root_move(tree_t * restrict ptree)
{
    int n = root_nmove;

    for (int i = 0; i < n; i++ ) {
        if ( root_move_list[i].status & flag_searched ) continue;
        root_move_list[i].status |= flag_searched;
        ptree->current_move[1]    = root_move_list[i].move;
        return 1;
    }

    return 0;
}

static int countFlip(int turn, int i)
{
    return ((i%2) ? Flip(turn) : turn);
}

static void rewindToRoot()
{
    tree_t* restrict ptree = g_ptree; 

    if (curPosPathLeng != 0) {
        assert(curPosPathLeng > 0);

        for (int i=curPosPathLeng-1; i>=0; i--) {
            UnMakeMove(countFlip(root_turn, i), curPosPath[i].v, i+1/*ply*/);
            curPosPathLeng--;
        }
        curPosPathChecks = 0;
    }
    assert(!curPosPathLeng && !(curPosPathChecks & (~1)));
}

static void bringPosTo(int leng, mvC* pv, bool setchks = true)
{
    tree_t* restrict ptree = g_ptree;

    assert(leng < MAX_ROOT_PATH);

    if (leng > 0 && leng == curPosPathLeng) {
        bool mch = true;
        forr (i, 0, leng-1) {
            if (pv[i] != curPosPath[i]) {
                mch = false; break;
            }
        }
        if (mch) {
            return;   // already at this path
        }
    }
    rewindToRoot();

    if (setchks) {
        curPosPathChecks = 0;
    }
    forr (i, 0, leng-1) {
        int turn = countFlip(root_turn, i);
        int chked = InCheck(turn) ? 1 : 0;
        if (setchks && chked) {
            curPosPathChecks |= 1 << i;
        }
        ptree->nsuc_check[i+1] = !chked ? 0 : i<=1 ? 1 : ptree->nsuc_check[i-1]+1;
        assert(is_move_valid(ptree, pv[i].v, turn));
        MakeMove(turn, pv[i].v, i+1);
        assert(!InCheck(turn));
        curPosPath[i] = pv[i];
    }

    int i = leng;
    int chked = InCheck(countFlip(root_turn, i)) ? 1 : 0;
    if (setchks && chked) {
        curPosPathChecks |= 1 << i;
    }
    ptree->nsuc_check[i+1] = !chked ? 0 : i<=1 ? 1 : ptree->nsuc_check[i-1]+1;

    curPosPathLeng = leng;
}

static int setMoves(int* lengp, mvC* mvp, mvC excludeMv, bool stopAt2 = false)
{
    //**** curPos assumed to have been set
    tree_t* restrict ptree = g_ptree; 

    int ply = curPosPathLeng+1;  // 'ply' is referred to in MOVE_CURR
    int turn = countFlip(root_turn, ply-1);
    int i = 0;
    ptree->move_last[ply] =  ptree->move_last[ply-1] = ptree->amove;
    ptree->anext_move[ply].next_phase = next_move_hash;

    if (USE_SRCHR && ply == 1) {
        forr (i, 0, root_nmove-1) {
            root_move_list[i].status = 0;
        }
    }
 
    while (USE_SRCHR && ply == 1 ? next_root_move(ptree) :
           ptree->nsuc_check[ply] ? gen_next_evasion(ptree, ply, turn) :
           gen_next_move(ptree, ply, turn)) {
        // FIXMEEEE!!!!  should not occur?  investigate!
        if (!is_move_valid(g_ptree, MOVE_CURR, turn)) continue; 
        if ((int)MOVE_CURR == excludeMv.v) continue; //dont gen firstmv
        mvp[i++] = MOVE_CURR;
        if (stopAt2 && i >= 2) break; // if stopAt2, set firstmv to NULLMV
    }

    *lengp = i;
    return i;
}

static int popcnt(int x)
{
    /*int a = 0;

    forr (i, 0, 31) {
        a += x & 1;
        x >>= 1;
    }

    return a;*/

    x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x & 0x0f0f0f0f) + ((x >> 4) & 0x0f0f0f0f);
    x = (x & 0x00ff00ff) + ((x >> 8) & 0x00ff00ff);
    return ((x & 0x0000ffff) + ((x >> 16) & 0x0000ffff));
}

//**********

void resetBonaSearch(tree_t * restrict ptree)
{
    ptree->node_searched = 0;
    ptree->nquies_called = 0;
    ptree->current_move[0] = 0;
    ptree->pv[0].a[0] = 0;
    ptree->pv[0].a[1] = 0;
    ptree->pv[0].depth = 0;
    ptree->pv[0].length = 0;

    root_abort = 0;
    root_alpha = -score_bound;
    root_beta =   score_bound;

    game_status &= ~(flag_move_now | flag_quit_ponder | flag_search_error);

    ptree->save_eval[0] =
    ptree->save_eval[1] = INT_MAX;

    forr (ply, 0, PLY_MAX-1) {
        ptree->amove_killer[ply].no1 =
        ptree->amove_killer[ply].no2 = 0;
    }

    ptree->nsuc_check[0] = 0;
    ptree->nsuc_check[1] = InCheck(root_turn) ? 1 : 0;

    ptree->move_last[0] = ptree->amove;
    ptree->move_last[1] = ptree->amove;
}

//******** proce ********

enum
{
    PROCE_OK = 0,
    PROCE_ABORT = 1,
};

int proce(int nested)
{
    // if nested, no cmd should be pending
    if (nested) assert(!pendingCmd.haveCmd());

    // if cmd has come, put it to pendigCmd(pc)
    pendingCmd.getCmd();

    while (pendingCmd.haveCmd()) {
        int pcOpc = pendingCmd.opcode();
        if (DBG_DUMP_COMM) {
            SLTOut("======== proce start: opc=%d ========\n", pcOpc);
        }

        if (nested &&  // pc is either setroot/fwd/rwd/quit/first
            ( pcOpc == CMD_OPCODE_SETROOT  ||
              pcOpc == CMD_OPCODE_MKMOVE   ||
              pcOpc == CMD_OPCODE_UNMKMOVE ||
              pcOpc == CMD_OPCODE_QUIT     ||
              pcOpc == CMD_OPCODE_STOP     ||
              ( pcOpc == CMD_OPCODE_LIST && running.job.onSingle() &&
                // running.valid must be on, if nested
                ( (ROOT_PARALLEL && singleCmd.opcode == CMD_OPCODE_ROOT) ||
                  (singleCmd.opcode == CMD_OPCODE_WARM) )) ||
              ( ABORT_ON_1ST && pcOpc == CMD_OPCODE_FIRST) )) {

            // for these cases, the cmd will be executed after abort and
            // re-execution of proce(0), so pendingCmd must remain.
            // for all the other cases, cmd is handled here and hence erased

            if (pcOpc == CMD_OPCODE_STOP) {
                if (pendingCmd.exd() == 0) // FIXME no exd sent as of 10/10/2011
                    expired = 1;
                else
                    detected = 1;
            }

            if (pcOpc == CMD_OPCODE_FIRST) {
                assert(running.job.onList());  // must be LIST
            }

            //  set retryLater  - not needed?
            return PROCE_ABORT;  // if nonzero, search() will set root_abort on
        }

        assert(!(nested && (pcOpc == CMD_OPCODE_ROOT||pcOpc == CMD_OPCODE_WARM)));
        // if ROOT/WARM, must not be nested (RWD/FWD comes before them)
        // if nested, either LIST/NOTIFY/EXTEND/SHRINK/COMMIT/CANCEL

        switch(pcOpc) {
        case CMD_OPCODE_SETROOT:
        {
            min_posi_t currentRootPos;
            SLTOut("^^^^ setroot called\n");
            assert(!nested);

            // set current pos, then initialize
            currentRootPos.hand_black = pendingCmd.handb();
            currentRootPos.hand_white = pendingCmd.handw();
            currentRootPos.turn_to_move = pendingCmd.turn();
            forr (i, 0, 80) {
                currentRootPos.asquare[i] = pendingCmd.asquare(i);
            }
            ini_game(g_ptree, &currentRootPos, 0, NULL, NULL);

            // clear tables
            singleCmd.clear();
            splane.clear();
            running.clear();  // FIXME?  not needed, but no harm anyway
            mpLastMove = NULLMV;
            curPosPathLeng = curPosPathChecks = 0;
            resetBonaSearch(g_ptree);
            perfSetRootHook();
            if (DBG_DUMP_COMM) {
                out_board(g_ptree, slavelogfp, 0, 0);
            }
        }
        break;

        // ルート局面から指し手を進めます。(make_move_rootと同じ)
        case CMD_OPCODE_MKMOVE:
        {
            assert(!nested);
            // make the move
            rewindToRoot();
            mpLastMove = pendingCmd.mkMoveMove();
            SLTOut("%8d>calling makemove: fwdmv %07x\n",
                   worldTime(), readable(mpLastMove));
            if (g_ptree->node_searched) {
                displayPerf(g_ptree);
                //perfRecMove.print();
                //perfRecGame.add(&perfRecMove);
                //perfRecMove.clear();
            }
            perfMkMoveHook();
            make_move_root(g_ptree, mpLastMove.v, flag_time);

            // clear tables
            singleCmd.clear();
            splane.clear();
            running.clear();  // FIXME?  not needed, but no harm anyway
            {
                int i, ply;
                const int HISTORY_DIVIDE = 256;   // FIXME tune

                for ( i = 0; i < (int)HIST_SIZE; i++ ) {
#if BNS_COMPAT
                    hist_goodary[i]  = 0;
                    hist_tried[i] = 0;
#else
                    hist_goodary[i]  /= HISTORY_DIVIDE;
                    hist_tried[i] /= HISTORY_DIVIDE;
#endif
                }

                for ( ply = 0; ply < PLY_MAX; ply++ ) {
                    g_ptree->amove_killer[ply].no1 = 
                        g_ptree->amove_killer[ply].no1_value = 
                        g_ptree->amove_killer[ply].no2 = 
                        g_ptree->amove_killer[ply].no2_value = 0;
                }

                if (USE_ADJUST_FMG && !rwdLatch) {
                    adjust_fmg();
                }
                if (BNS_COMPAT) {
                    clear_trans_table();
                }
                rwdLatch = 0;
            }

            initPerf(g_ptree);
            resetBonaSearch(g_ptree);
#ifdef INANIWA_SHIFT
            detect_inaniwa(g_ptree);
#endif
        }
        break;

        // ルート局面から指し手を一つ戻します。(unmake_move_rootと同じ)
        case CMD_OPCODE_UNMKMOVE:
            assert(!nested);
            // unmake the move
            rewindToRoot();
            if (mpLastMove == NULLMV) {
                printf("error: rewinding beyond stack\n");
                break;
            }
            SLTOut("%8d>calling unmakemove: lastmv %07x\n",
                   worldTime(), readable(mpLastMove));
            unmake_move_root(g_ptree);
            mpLastMove = NULLMV;
            // clear tables
            singleCmd.clear();
            splane.invalidateAll();
            running.clear();  // FIXME?  not needed, but no harm anyway
            // resetBona.. is - not needed, since FWD will follow (right?)
            rwdLatch = 1;
            break;

        // プロセスを終了します。
        case CMD_OPCODE_QUIT:
            SLTOut("calling quit\n");
            perfQuitHook();
            mpi_close();   // (FIXME)  mpi_close() s/b inside fin()?
            fin();
            exit(0);

        case CMD_OPCODE_ROOT:
        case CMD_OPCODE_WARM:
            if (DBG_DUMP_COMM) {
                SLTOut("^^^^ %s called\n",
                       pcOpc==CMD_OPCODE_ROOT ? "root" : "warm");
            }
            assert(!nested && running.job.idle() && !curPosPathLeng);
            singleCmd.valid = 1;
            singleCmd.opcode = pcOpc;
            singleCmd.issued =
                singleCmd.itd =
                singleCmd.fromRoot =
                singleCmd.haveList =
                singleCmd.pvleng = 0;
            singleCmd.ule = ULE_NA;
            singleCmd.bestseqLeng = 0;
            singleCmd.val = 0;
            forr (i, 0, GMX_MAX_PV-1) {
                singleCmd.pv[i] = NULLMV;
            }
            forr (i, 0, GMX_MAX_BESTSEQ-1) {
                singleCmd.bestseq[i] = NULLMV;
            }
            //singleCmd.picked = (pcOpc==CMD_OPCODE_ROOT ? 0 : 1); //WARM mean picked
            break;

        case CMD_OPCODE_FIRST:
            assert(!(ABORT_ON_1ST && nested));
            singleCmd.valid    = 1;
            singleCmd.opcode   = pcOpc;
            singleCmd.issued   = 0;
            singleCmd.itd      = pendingCmd.itd();
            singleCmd.pvleng   = pendingCmd.pvleng();
            singleCmd.fromRoot = 0;
            singleCmd.haveList = pendingCmd.havelist_first();
            singleCmd.ule      = ULE_NA;
            singleCmd.val      = 0;
            singleCmd.bestseqLeng = 0;
            if (DBG_DUMP_COMM) {
                SLTOut("^^^^ first called itd %d pvleng %d havelist %04x\n"
                       "first pv:",
                       singleCmd.itd, singleCmd.pvleng, singleCmd.haveList);
            }
            forr (i, 0, singleCmd.pvleng-1) {
                singleCmd.pv[i] = pendingCmd.pv(i);
                if (DBG_DUMP_COMM) SLTOut(" %07x", readable(pendingCmd.pv(i)));
            }
            if (DBG_DUMP_COMM) SLTOut("\n");

            forr (i, 0, GMX_MAX_BESTSEQ-1) {
                singleCmd.bestseq[i] = NULLMV;
            }
            if (ABORT_ON_1ST) {
                running.job.setIdle();
            }
            break;

        case CMD_OPCODE_LIST:
        {
            mvC pv[GMX_MAX_PV];
            mvtupleC tuples[GMX_MAX_LEGAL_MVS];
            int pvlen = pendingCmd.pvleng_list();
            int cnt = pendingCmd.mvcnt();

            forr (i, 0, pvlen-1) {
                pv[i] = pendingCmd.pv_list(i);
            }
            forr (i, 0, cnt-1) {
                tuples[i] = pendingCmd.tuple(i);
            }
            splane.cmdList(pendingCmd.itd(), pendingCmd.exd(),
                           pendingCmd.alpha(), pendingCmd.beta(),
                           pendingCmd.firstmv(), pvlen, pv, cnt, tuples);

            if (DBG_DUMP_COMM) {
                SLTOut("^^^^ list called  itd %d exd %d mvcnt %d alpha %d beta %d\n",
                       pendingCmd.itd(), pendingCmd.exd(), pendingCmd.mvcnt(),
                       pendingCmd.alpha(), pendingCmd.beta());
                SLTOut("^^^^ list cmd pvleng %d   pv: ",
                       pendingCmd.pvleng_list());
                forr (i, 0, pendingCmd.pvleng_list() - 1) {
                    SLTOut(" %07x", readable(pendingCmd.pv_list(i)));
                }
                SLTOut("\n");
                forr (i, 0, pendingCmd.mvcnt() - 1) {
                    mvtupleC tuple = pendingCmd.tuple(i);
                    SLTOut(" %07x-%07x d %d u %d l %d", readable(tuple.mv),
                           readable(tuple.bestmv), tuple.depth,
                           tuple.upper, tuple.lower);
                    if ((i%2) == 1) SLTOut("\n");
                }
                SLTOut("\n");
            }
        }
        break;

        case CMD_OPCODE_EXTEND:
        {   // find the relevant slot in pvsCmd[]
            int addcnt = pendingCmd.mvcnt();
            mvtupleC tuples[MAX_XFER_MVS];

            forr (i, 0, addcnt-1) {
                tuples[i] = pendingCmd.tuple_extend(i);
            }
            splane.cmdExtend(pendingCmd.itd(), pendingCmd.exd(),
                             addcnt, tuples);
        }
        break;

        case CMD_OPCODE_SHRINK:
        {   // find the relevant slot in pvsCmd[]
            int itd = pendingCmd.itd();
            int exd = pendingCmd.exd();
            int mvcnt = pendingCmd.mvcnt();

            if (DBG_DUMP_COMM) {
                SLTOut("^^^^ shrink called  itd %d exd %d cnt %d\n", itd, exd,mvcnt);
            }

            bool doAbort = nested && !running.job.onSingle() &&
                running.dropByShrink(itd, exd, mvcnt);

            splane.cmdShrink(itd, exd, mvcnt);

            // abort if shrinking makes current job unnecessary
            if (doAbort) {
                running.job.setIdle();
                pendingCmd.erase();  // clear the pending cmd
                return PROCE_ABORT;
            }
        }
        break;

        case CMD_OPCODE_COMMIT:
        {   // even if master proceeds after sending SHRINK,
            // COMMIT should come after SHRINK
            int itd = pendingCmd.itd();
            int exd = pendingCmd.exd();
            bool doAbort = nested &&
                ((itd == RUNNING_SINGLE && running.job.onSingle()) ||
                 (running.job.onList() && running.dropByCommit(itd, exd)));

            if (DBG_DUMP_COMM) {
                SLTOut("^^^^ commit called  itd %d exd %d \n", itd, exd);
            }
            if (itd != RUNNING_SINGLE) {
                splane.cmdCommit(itd, exd);
            }
            // abort if job currently running
            if (doAbort) {
                running.job.setIdle();
                pendingCmd.erase();  // clear the pending cmd
                return PROCE_ABORT;
            }
        }
        break;

        case CMD_OPCODE_STOP:
        {   // find the relevant slot in pvsCmd[]
            if (DBG_DUMP_COMM) {
                SLTOut("^^^^ stop called\n");
            }
            assert(!nested);
            singleCmd.clear();
            splane.clear();
            rewindToRoot();
            rpyent.pushStopack();
            rpypkt.send();
        }
        break;

        case CMD_OPCODE_NOTIFY:
        {
            mvC mv = pendingCmd.notifymv();
            int itd = pendingCmd.itd();
            int exd = pendingCmd.exd();
            int val = pendingCmd.newalpha();
            if (DBG_DUMP_COMM) {
                SLTOut("^^^^ notify called  itd %d exd %d newalpha %d mv %07x\n",
                       itd, exd, val, readable(mv));
            }

            int runexd = (!nested || itd != running.job.itd) ? 99 : running.job.exd;
            int doAbort = splane.cmdNotify(itd, exd, val, mv, runexd);

            // cancel current job if necessary (only if not retrying)
            if (nested && doAbort) {
                pendingCmd.erase();
                return PROCE_ABORT;
            }
        }
        break;

        case CMD_OPCODE_VERIFY:
        {
            int itd = pendingCmd.itd();
            int tail = pendingCmd.tail();
            SLTOut("'''' Verify: i %d tl %d\n", itd, tail);

            forr (e, 0, tail-1) {
                srowC& sr = splane.stream[itd].row[e];
                if (pendingCmd.valpha(e) != sr.alpha)
                    SLTOut("SLWARNING: A mismatch e %d MS: %d SL: %d\n",
                           e, pendingCmd.valpha(e), sr.alpha);
                if (pendingCmd.vbeta(e) != sr.beta)
                    SLTOut("SLWARNING: B mismatch e %d MS: %d SL: %d\n",
                           e, pendingCmd.vbeta(e), sr.beta);
                if (pendingCmd.vgamma(e) != sr.gamma)
                    SLTOut("SLWARNING: G mismatch e %d MS: %d SL: %d (A %d %s)\n",
                           e, pendingCmd.vgamma(e), sr.gamma, sr.alpha,
                           (pendingCmd.vgamma(e) > sr.alpha ? "WATCH!!" : ""));
            }
        }
        break;

        case CMD_OPCODE_PICKED:
        {   // just set PICKED flag in singleCmd
            if (DBG_DUMP_COMM) 
                SLTOut("^^^^ picked called\n");
            //singleCmd.picked = 1;
        }
        break;


        default: NTBR;

        } // switch pc

        pendingCmd.erase();        // clear pc
        pendingCmd.getCmd();      // read in next cmd, if any
    }

    return PROCE_OK;
}

//**----------------------------
int detectSignalSlave()
{
    // 11/26/2011 %5 inRoot/inFirst chk s/b in detSigSlv, not for master
    if (inRoot &&
        preNodeCount + MAX_ROOT_NODES <= g_ptree->node_searched) {
        rootExceeded = 1;
        return 1;
    }

    if (inFirst && !firstReplied &&
        preNodeCount + MAX_FIRST_NODES <= g_ptree->node_searched) {
        firstReplied = 1;
        replyFirst();
    }

    return proce(1/*nested*/);
}

#include "sbody3.h"
