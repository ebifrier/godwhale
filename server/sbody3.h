/* $Id: sbody3.h,v 1.112 2012-04-25 10:48:24 eiki Exp $ */

//**************** slave ***************

#define MAX_WARM_DEP      MAX_SRCH_DEP // FIXME tune
#define MAX_ROOT_SRCH_DEP 10     // FIXME tune
#define USE_SLV_HISTORY    1     // FIXME tune
//#define USE_FIRST_HASH
//#define EXTEND_LIST_HASH
#define ROOT_SORT_BY_NODES
#define PUZZLE_ON_1MV
#define WEAK_LMR

#if !BNS_COMPAT
  // in compat mode, this must not be defined.  if not, either on or off fine
  // 3/7/2011 on vs off 16-27 -> disable => 3/16/2011 on 47 - off 33, restored
  // 4/21/2012 vs bns2c, on 40-55, off 40-44 => off
//#define USE_LIST_ASP
#endif

//#define PUZZLE_VAL (score_bound - 30) - No! abs must be <score_max_eval
#define PUZZLE_DEP 7
#define PUZZLE_VAL (3)

#define DECISIVE_SRD 100
#define DECISIVE_DEPTH (88)

int inRoot, rootExceeded, inFirst, firstReplied;
uint64_t preNodeCount;
mvC firstMoves[MAX_EXPDEP][GMX_MAX_LEGAL_MVS];
int firstMvcnt[MAX_EXPDEP];

extern int tlp_max_arg;

int MAX_SRCH_DEP = MAX_SRCH_DEP_FIGHT;  // FIXME? here? shared w/ someone else?

//****

int problemMode()
{
    return (game_status & flag_problem);
}

static void histGoodPv(tree_t * restrict ptree, unsigned int mv,
                       int ply, int dep, int turn)
{
    if (UToCap(mv) || (I2IsPromote(mv) && I2PieceMove(mv) != silver)) {
        return;
    }

    unsigned int key = phash(mv, turn);
    hist_tried[key]   = (unsigned short)((int)hist_tried[key]   + dep);
    hist_goodary[key] = (unsigned short)((int)hist_goodary[key] + dep);
    
    unsigned int umv = UToFromToPromo(mv);
    unsigned int uval = 0;
    if (umv == ptree->amove_killer[ply].no1) {
        return;
    }

    if (umv == ptree->amove_killer[ply].no2) {
        uval = ptree->amove_killer[ply].no2_value;
    }

    ptree->amove_killer[ply].no2 = ptree->amove_killer[ply].no1;
    ptree->amove_killer[ply].no2_value = ptree->amove_killer[ply].no1_value;
    ptree->amove_killer[ply].no1 = umv;
    ptree->amove_killer[ply].no1_value = uval;
}

//****

#define INITIAL_ASP_VAL (-score_bound-1)
#define ASP_WINDOW      (300) 

static int srchAsp(tree_t * restrict ptree, int val, int turn,
                   int dep, int ply, int stat)
{
    int A, B, oldA, oldB;
    oldA = root_alpha;
    oldB = root_beta;

    if (BNS_COMPAT || abs(val) > score_max_eval - ASP_WINDOW) {
        A = -score_bound; B = score_bound;
    }
    else {
        //A = max(-score_bound, val - ASP_WINDOW);
        //B = min(+score_bound, val + ASP_WINDOW);
        A = val - ASP_WINDOW;
        B = val + ASP_WINDOW;
    }

    if (USE_SRCHR && ply == 1) {
        forr (i, 0, root_nmove-1) {
            root_move_list[i].status = 0;
        }
    }

    while (1) {
        root_alpha = A;
        root_beta  = B;

        if (USE_SRCHR && ply == 1) {
            val = searchr(ptree, A, B, turn, dep);
        } else {
            val = search(ptree, A, B, turn, dep, ply, stat);
        }

        if (root_abort) break;

        if (-score_bound < A && val <= A) {
            if (USE_SRCHR && ply == 1) {
                forr (i, 0, root_nmove-1) { // FIXME? Bnz does not do this?
                    root_move_list[i].status = 0; // (Crafty either)  why?  investigate.
                }
            }
            A = -score_bound;
            SLTOut("cccc aspiration fail low\n");
        }
        else if (B < score_bound && B <= val) {
            B = score_bound;
            root_move_list[0].status = 0;
            SLTOut("cccc aspiration fail high\n");
        }
        else {
            break;
        }
    }

    root_alpha = oldA;
    root_beta  = oldB;
    return val;
}

 //********

static void doRoot();  // defined below
static void doFirst();  // defined below
static void doList(jobDtorC);  // defined below
static unsigned int state_node =
    node_do_mate | node_do_null | node_do_futile |
    node_do_recap | node_do_recursion | node_do_hashcut;
static unsigned int state_node_dmy = 0;

void slave()
{
    int64_t pre, post, sleepStarted = 0LL;
    if (problemMode())
        MAX_SRCH_DEP = MAX_SRCH_DEP_PROB;
    tlp_max = tlp_max_arg;
    tlp_start();

    // clear tables
    singleCmd.clear();
    splane.clear();

    while (1) {
        proce(0 /*not nested*/);

        jobDtorC srchJob;
        if (singleCmd.valid) { // %1 11/25/2011 single case missing
            srchJob.setSingle();
        } else {
            srchJob = splane.nextjob();
        }

        if (!srchJob.idle()) {
            SLDOut("---- slave while loop after proce: job i %d e %d s %d\n",
                   srchJob.itd, srchJob.exd, srchJob.mvsuf);
        }

        if (srchJob.idle()) {
            if (MEASURE_TIME) {
                pre = worldTimeLl();
                if (sleepStarted == 0LL) {
                    sleepStarted = pre;
                }
                if (DBG_INTERVAL_CHK) {
                    SLTOut("%16lld> before microsleep\n", pre);
                }
            }

            microsleep(SLAVE_INTERVAL);
            if (MEASURE_TIME) {
                post = worldTimeLl();
                if (DBG_INTERVAL_CHK) {
                    SLTOut("%16lld> after  microsleep\n", post);
                }
            }

            continue;
        }

        if (MEASURE_TIME && sleepStarted != 0LL) {
            SLTOut("ssssssss slept %16lld ns\n", post - sleepStarted);
            sleepStarted = 0LL;
        }

        if (DBG_SL) dumpSlState();

        // do search

        detected = expired = 0;
        root_abort = 0;
        running.clear();
        running.valid = 1;  // will do some work anyway (FIXME? true?)

        if (srchJob.onSingle()) {
            running.job.setSingle();

            // ******** ROOT ********
            if (singleCmd.opcode == CMD_OPCODE_ROOT) {
                doRoot();
            }
            // ******** FIRST ********
            else if (singleCmd.opcode == CMD_OPCODE_FIRST) {
                doFirst();
            }
            else NTBR;
        }
        // ******** LIST ********
        else if (srchJob.onList()) {  // pvs
            doList(srchJob);
        }
        else NTBR;

        running.job.setIdle();
        running.valid = 0;

        // return reply, if exists
        if (rpypkt.cnt() > 0) {
            rpypkt.send();
        }
    }
}

//**************** ROOT  ***************

#define NO_FUTIL_DEP  4

static void makeRootMoveList()
{
    hash_probe(g_ptree, 1, NO_FUTIL_DEP * PLY_INC, root_turn,
               -score_bound, score_bound, &state_node_dmy);

    int hashmv = g_ptree->amove_hash[1];
    if (hashmv != MOVE_NA && hashmv != MOVE_PASS &&
        is_move_valid(g_ptree, hashmv, root_turn)) {
        g_ptree->pv[0].a[1] = hashmv;  // make_root_move_list() uses this
    }
    make_root_move_list(g_ptree /*, flag_nopeek*/);
}


static void doRoot()
{
    tree_t* ptree = g_ptree;
    inRoot = rootExceeded = 0;

    // iterative deepening on root, calling search().
    // (use searchr()?  well, may try later...)  stop when:
    // 1) only 0/1 mvs exist 2) +-inf returned 3) #nodes > const

    // FIXMEEE!!!! in det_sig(), abort on #nod>MAX_**. then #nod need cleared
    //g_ptree->node_searched = 0;

    rewindToRoot();
    if (InCheck(Flip(root_turn))) {
        singleCmd.pvleng = singleCmd.itd = 0;
        SLTOut("******** WARNING: selfchk detected at root ********\n");
        return;
    }

    //**** root move generation
    if (USE_SRCHR) {
        makeRootMoveList();
    }

#ifdef PUZZLE_ON_1MV
    if (root_nmove == 1) {
        //**** only one root move.  do 'puzzle' search and
        //     find two moves for next ponder

        int mv1 = root_move_list[0].move;
        MakeMove(root_turn, mv1, 1);
        assert(!InCheck(root_turn));
        hash_probe(g_ptree, 2,
                   PUZZLE_DEP * PLY_INC + PLY_INC/2, Flip(root_turn),
                   -score_bound, score_bound, &state_node_dmy);
        int mv2 = ptree->amove_hash[2];
        if (mv2 == MOVE_NA || !is_move_valid(ptree, mv2, Flip(root_turn))) {
            SLTOut("pppp calling puzzling srch\n");
            ptree->nsuc_check[2] = !InCheck(Flip(root_turn)) ? 0 : 1;
            ptree->save_eval[1] =  ptree->save_eval[2] = INT_MAX;
            g_ptree->move_last[1] = g_ptree->amove;
            perfRecMove.startSrch();
            search(g_ptree, -score_bound, score_bound, Flip(root_turn),
                   PUZZLE_DEP * PLY_INC + PLY_INC/2, 2/*ply*/, state_node);
            perfRecMove.endSrchPuzzle();
            mv2 = !root_abort && ptree->pv[1].length >= 2 ?
                ptree->pv[1].a[2] : MOVE_NA;
           }
           assert(mv2 == MOVE_NA || is_move_valid(ptree, mv2, Flip(root_turn)));
           if (mv2 != MOVE_NA) {
               int bad = 0;

               MakeMove(Flip(root_turn), mv2, 2);
               if (InCheck(Flip(root_turn))) {
                   bad = 1;
               }
               UnMakeMove(Flip(root_turn), mv2, 2);
               if (bad) {
                   mv2 = MOVE_NA;
               }
           }
           UnMakeMove(root_turn, mv1, 1);
           //SLTOut("pppp puzzling done  mv1 %07x mv2 %07x nod %d\n",
           //    readable_c(mv1), readable_c(mv2), ptree->node_searched - pren);
           rpyent.pushRoot(MAX_SRCH_DEP, PUZZLE_VAL, mvC(mv1), mvC(mv2));
           singleCmd.clear();
           return;
       }
#endif

       if (root_nmove == 0) {
           //**** no root move.  lost
           //rpyent.pushSetpv(1 /*itd*/, 0 /*pvleng*/, NULL);
           rpyent.pushRoot(MAX_SRCH_DEP, singleCmd.val, NULLMV, NULLMV);
           rpypkt.send();
           singleCmd.clear();
           return;
       }

        //**** FIXME TBC look for hash results, reply Root/set inRoot.
        //     (refer to rfdl.h?)

        //**** loop start

       int val = INITIAL_ASP_VAL;
       inRoot = 0;  // 11/26/2011 %4 was missing
       rootExceeded = 0;

       forr (dep, NO_FUTIL_DEP, MAX_SRCH_DEP-1) {
           //root_move_gen() gen'ed >1 moves. srchr() wont cut any mv FIXME true?
           assert(root_nmove > 1);
           singleCmd.itd = iteration_depth = dep;
           perfRecMove.startSrch();
           SLTOut("-------- ROOT srch dep=%d val=%d..", dep, val);
           val = USE_ASP
               ? srchAsp(g_ptree, val, root_turn,
                         dep * PLY_INC + PLY_INC/2, 1/*ply*/, state_node)
               : search(g_ptree, -score_bound, score_bound, root_turn,
                        dep * PLY_INC + PLY_INC/2, 1/*ply*/, state_node);

           perfRecMove.endSrchRoot();
           SLTOut(" .. done abt=%d val=%d #nod=%d\n",
                  root_abort, val, g_ptree->node_searched - preNodeCount);

           if (root_abort) {
               if (rootExceeded) {
                   //**** #nod exceeded, now moving to FIRST
                   //set pv&ITDep; set SINGLE to 'FIRST'  turn on fromRoot
                   singleCmd.opcode = CMD_OPCODE_FIRST;
                   singleCmd.fromRoot = 1;
                   singleCmd.haveList = 0;
               }
               else {
                   //**** abort due to STOP
                   singleCmd.clear();
               }

               inRoot = rootExceeded = 0;
               return;
           }

           rootExceeded = 0;
           //** below here, !root_abort, #mv > 1

#ifdef ROOT_SORT_BY_NODES
           /**** shell sort for root moves - copied from iterate.c */
           {
               root_move_t root_move_swap;
               const int n = root_nmove;
               uint64_t sortv;
               int i, j, k, h;
             
               for (k = SHELL_H_LEN - 1; k >= 0; k--)
               {
                   h = ashell_h[k];
                   for (i = n-h-1; i > 0; i--)
                   {
                       root_move_swap = root_move_list[i];
                       sortv          = root_move_list[i].nodes;
                       for (j=i+h; j<n && root_move_list[j].nodes > sortv; j += h)
                       {
                           root_move_list[j-h] = root_move_list[j];
                       }
                       root_move_list[j-h] = root_move_swap;
                   }
               }
           }
#endif

           {  //**** record results in singleCmd
               int leng;
               leng = singleCmd.pvleng = min(ptree->pv[0].length,GMX_MAX_PV-3);

               // 12/22?/2010 #61 was MAXPV-1; allow for setpv+2*setlist
               forr (i, 0, leng-1) {
                   singleCmd.pv[i] = ptree->pv[0].a[i+1];
               }
               SLDOut("pv:");
               forr (i, 1, ptree->pv[0].length) {
                   SLDOut(" %07x", readable(mvC(ptree->pv[0].a[i])));
               }
               SLDOut("\n");

               // FIXME!  if not +-INF & leng<2, look for hash
               singleCmd.ule = ULE_EXACT;
               singleCmd.val = val;  //  12/22?/2010 #62 must not invert here
           }
           
           //**** extend pv by hash  FIXME TBC up to GMX_MAX_PV-2(?)

           if (abs(singleCmd.val) < score_max_eval &&
               singleCmd.pvleng <= 1) {
               assert(singleCmd.pvleng >= 0);
               SLDOut("vvvv leng=0,1 case\n");

               // if leng=0, use hash mv. mv must be valid if leng=0(hash hit)
               if (singleCmd.pvleng == 0) {
                   hash_probe(g_ptree, 1, dep * PLY_INC, root_turn,
                              -score_bound, score_bound, &state_node_dmy);
                   int hashmv = g_ptree->amove_hash[1];

                   assert (hashmv != MOVE_NA && hashmv != MOVE_PASS &&
                           is_move_valid(g_ptree, hashmv, root_turn));
                   singleCmd.pvleng = 1;
                   singleCmd.pv[0].v = hashmv;
               }

               // fwd one mv if leng==1, try to add more by hash
               SLDOut("vvvv fwd one mv(%07x)\n", readable(singleCmd.pv[0]));
               bringPosTo(1, &singleCmd.pv[0], true/*setchks*/);

               // probe hash
               int newdep = singleCmd.itd - 1 + popcnt(curPosPathChecks);
               hash_probe(g_ptree, 2, newdep * PLY_INC, Flip(root_turn),
                          -score_bound, score_bound, &state_node_dmy);
               int hashmv = g_ptree->amove_hash[2];

               // add it to pv if it's valid mv
               if (hashmv != MOVE_NA && hashmv != MOVE_PASS &&
                   is_move_valid(g_ptree, hashmv, Flip(root_turn))) {

                   // make sure not a selfchk
                   MakeMove(Flip(root_turn), hashmv, 2);
                   if (!InCheck(Flip(root_turn))) {
                       singleCmd.pvleng = 2;
                       singleCmd.pv[1].v = hashmv;
                   }
                   UnMakeMove(Flip(root_turn), hashmv, 2);
               }

               rewindToRoot();
           }

           //**** w/ searchr(), leng must be at least 1 unless mated
           assert(singleCmd.pvleng >= 1 || singleCmd.val < -score_max_eval);

           if (abs(singleCmd.val) <= score_max_eval) {  
               //**** reply Root
               mvC mv1 = singleCmd.pvleng >= 1 ? singleCmd.pv[0] : NULLMV;
               mvC mv2 = singleCmd.pvleng >= 2 ? singleCmd.pv[1] : NULLMV;
               rpyent.pushRoot(dep, singleCmd.val, mv1, mv2);
               rpypkt.send();
               
               inRoot = 1;  // one result done. now allow to abort by #nod
               
               // root pv dump
               SLTOut("rrrr root pv leng=%d: ", singleCmd.pvleng);
               forr (k, 0, singleCmd.pvleng-1) {
                   SLTOut("%07x ", readable(singleCmd.pv[k]));
               }
               SLTOut("\n");
           }

           //**** mated
           else if (singleCmd.val < -score_max_eval) {  
               //rpyent.pushSetpv(1 /*itd*/, 0 /*pvleng*/, NULL);
               rpyent.pushRoot(MAX_SRCH_DEP, singleCmd.val, NULLMV, NULLMV);
               rpypkt.send();
               singleCmd.clear();
               break;
           }

           //**** mate
           else {
               assert(singleCmd.val >  score_max_eval);
               //rpyent.pushSetpv(1 /*itd*/, 1 /*pvleng*/, &singleCmd.pv[0]);
               rpyent.pushRoot(MAX_SRCH_DEP, singleCmd.val, singleCmd.pv[0],NULLMV);
               rpypkt.send();
               singleCmd.clear();
               break;
           }
       }
       
       inRoot = 0;
}

//**************** FIRST  ***************

void replyFirst()
{
    if (singleCmd.fromRoot) {  // 11/28/2011 %10 'exd' was 'pvleng'
        rpyent.pushSetpv(singleCmd.itd, singleCmd.exd, singleCmd.pv);
    }

    forr (ply, 0, singleCmd.exd-1) {
        if (!(singleCmd.haveList & (1 << ply))) {
            rpyent.pushSetlist(singleCmd.itd, ply, firstMvcnt[ply],
                               firstMoves[ply]);
        }
    }

     rpyent.pushFirst(singleCmd.itd, singleCmd.exd - 1, -singleCmd.val,
                      0 /*min(GMX_MAX_BESTSEQ-1, singleCmd.bestseqLeng)*/,
                      singleCmd.bestseq);
     // NOTE bestseq is sent in rpyFcomp, not here

     rpypkt.send();   // 11/27/2011 %9 was missing
}

static void doFirst()
{
    tree_t* ptree = g_ptree;

    // if FIRST, loop w/ expdep going up. stop at a certain #node or exd=2
    //     * split at least 2 mvs, to allow parallel execution of tail0/head1
    SLDOut("yyyy single-FIRST entered pvleng %d\n", singleCmd.pvleng);
    singleCmd.pvleng = min(GMX_MAX_PV-3, singleCmd.pvleng);
    assert(singleCmd.pvleng >= 1);
    // 12/18/2010 #45 rpy cnt must be <MAXPV -> subt. setpv&first
    
    //bringPosTo(singleCmd.pvleng, singleCmd.pv);   block A instead
    
    {  // block A
        int leng = singleCmd.pvleng;
        mvC* pv = singleCmd.pv;
        int itd = singleCmd.itd;
        iteration_depth = itd;
        tree_t* restrict ptree = g_ptree;  // referred to in InCheck()
        int turn = root_turn;
        rewindToRoot();
        curPosPathChecks = 0;
        curPosPathLeng = 0;
        inFirst = 0;

        int chked = InCheck(turn) ? 1 : 0;
        ptree->nsuc_check[1] = !chked ? 0 : 1;
        int i;

        //**** go down pv, generate moves and check hash on the way

        for (i=0; i<leng; i++) {
            // handle onerep
            if (!(singleCmd.haveList & (1 << i))) {
                if (i == 0 && root_nmove == -1) {
                    makeRootMoveList();
                    if (root_nmove <= 1) {
                        doRoot();   // will take care rt_nmv=0/1 cases
                        return;
                    }
                }
                setMoves(&firstMvcnt[i], firstMoves[i], pv[i], false/*stop@2*/);
            }

            if (DBG_DUMP_BOARD) {
                SLDOut(",,,, mvgen for setlist ply %d curPosPathLeng %d\n",
                       i, curPosPathLeng);
                out_board(g_ptree, slavelogfp, 0,0);
            }

            // make a move
            assert(is_move_valid(g_ptree, pv[i].v, turn));
            MakeMove(turn, pv[i].v, i+1);
            assert(!InCheck(turn));
            curPosPath[i] = pv[i];
            curPosPathLeng++;
            turn = Flip(turn);

            // handle check
            chked = InCheck(turn) ? 1 : 0;
            if (chked) {
                curPosPathChecks |= 1 << (i+1);
            }
            ptree->nsuc_check[i+2] = !chked ? 0 : ptree->nsuc_check[i]+1;

#ifdef USE_FIRST_HASH
            // check hash
            int ply = i + 1;
            int newdep = itdexd2srd(itd, i) * PLY_INC / 2 ;
            int tv = hash_probe(g_ptree, ply+1, newdep, turn,
                                -score_bound, score_bound, &state_node_dmy);
            if (tv == value_exact) {
                // FIXME? add hashmv to pv? (need valid/selfchk check)
                singleCmd.exd = singleCmd.pvleng = i+1;
                singleCmd.val = HASH_VALUE;
                singleCmd.bestseqLeng = 0;
                inFirst = 1;
                break;
            }
#endif
        }

#define VAL_MVONLY_DMY 4
        if (singleCmd.haveList & HVLST_MVONLY) {
            SLTOut("==== doFirst: mvonly detected e %d len %d\n",
                   singleCmd.exd, leng);
            singleCmd.exd = leng;
            singleCmd.val = VAL_MVONLY_DMY;
            assert(!singleCmd.fromRoot);
            replyFirst();
            rewindToRoot();
            singleCmd.clear();
            return;
        }
    } // block A


    //**** loop w/ expdep going up
    int ply;
    int itd = singleCmd.itd;
    int A = -score_bound;      // FIXME? root_A/B also needed?
    int B =  score_bound;
    int val = INITIAL_ASP_VAL;
    int turn = countFlip(root_turn, singleCmd.pvleng);
    int n;
    uint64_t pren;

    //inFirst = firstReplied = 0;
    firstReplied = 0;
    pren = g_ptree->node_searched;

    // NOTE singlCmd.exd set is needed only after one srch is done (not here)
    // 12/11/2010 #5 was <=1
    for (ply = singleCmd.pvleng; ply >= 1; ply--) {
        if ((unsigned)(pren + MAX_FIRST_NODE) < g_ptree->node_searched)
            break;

        n = (ply == singleCmd.pvleng) ? 2 : firstMvcnt[ply] + 1;
        // NOTE firstMvcnt excludes firstmv
        SLDOut("yyyy single-FIRST loop ply %d n %d\n", ply, n);

        //**** register hash (if any)  FIXME? store value, not store_pv

        if (ply <= singleCmd.pvleng-1) {
            if (DBG_SL && singleCmd.pv[ply] != NULLMV &&
                !is_move_valid(ptree, singleCmd.pv[ply].v, turn)) {
                SLDOut("!!!!!!!! first: writing invalid mv to hash  mv=%07x\n",
                       readable(singleCmd.pv[ply]));
                out_board(ptree, slavelogfp, 0, 0);
                assert(0);
            }
            hash_store_pv(ptree, singleCmd.pv[ply].v, turn);
        }

        SLDOut("bbbbbbbb first: nod_srched %d #mv %d ply %d pvleng %d\n",
               g_ptree->node_searched, n, ply, singleCmd.pvleng);

        if (firstReplied) {
            //firstReplied = 0;
            break;
        }

        if (n==1 && ply != singleCmd.pvleng) {
            assert(ply>0);
            for (int i=singleCmd.bestseqLeng-1; i>=0; i--) {
                singleCmd.bestseq[i+1] = singleCmd.bestseq[i];
            }
            singleCmd.bestseq[0] = singleCmd.pv[ply-1];
            singleCmd.bestseqLeng++;
            singleCmd.exd--;
            singleCmd.val = - singleCmd.val;
            running.val = - running.val;

            // 12/11/2010 #9 was turn/ply/ply+1
            UnMakeMove(Flip(turn), singleCmd.pv[ply-1].v, ply);
            curPosPathChecks &= ~(1 << ply);
            turn = Flip(turn);
            curPosPathLeng--;
            val = -val;
            continue;
        }

        //**** now perform search

        g_ptree->move_last[ply] = g_ptree->amove;

        // NOTE in search(ply), "mvlast[ply]=mvlast[ply-1]" is done.
        //  so setting mvlast[ply-1] is needed (&suffice)
        //  note here search(ply+1) is called, so set [ply]

        g_ptree->save_eval[ply  ] =
        g_ptree->save_eval[ply+1] = INT_MAX;

        // srch each exd.  there s/b >=2 mvs here
        int newdep = itdexd2srd(itd, ply) * PLY_INC / 2;
        MOVE_LAST = singleCmd.pv[ply-1].v;

        SLTOut("-------- FIRST srch dep=%d ply=%d...", newdep, ply);
        perfRecMove.startSrch();
        val = USE_ASP ?
            -srchAsp(g_ptree, -val, turn, newdep, ply+1, state_node) :
            -search(g_ptree, A, B, turn, newdep, ply+1, state_node);
        perfRecMove.endSrchFirst();
        SLTOut(" ..done abt=%d val=%d len=%d #nod=%d mvs %07x %07x..\n",
               root_abort, val, g_ptree->pv[ply].length,
               g_ptree->node_searched - preNodeCount,
               readable_c(g_ptree->pv[ply].a[ply+1]),
               readable_c(g_ptree->pv[ply].a[ply+2]));

        if (root_abort) {
            singleCmd.clear();
            rewindToRoot();
            inFirst = 0;
            return;
        }

        inFirst = 1;  // set when at least one result is available
        // 11/27/2011 %6 setting inFirst was missing

        //**** extend pv by using hashmv if leng insufficient

        if (ply == 1 && g_ptree->pv[ply].length < ply+1) {
            // FIXMEEEE!!!! extend pv more aggressively
            hash_probe(g_ptree, ply+1, newdep, turn,
                       -score_bound, score_bound, &state_node_dmy);
            int hashmv = g_ptree->amove_hash[ply+1];

            // add it to pv if it's valid mv
            if (hashmv != MOVE_NA && hashmv != MOVE_PASS &&
                is_move_valid(g_ptree, hashmv, turn)) {
                MakeMove(turn, hashmv, ply+1);
                if (!InCheck(turn)) {
                    g_ptree->pv[ply].length = ply+1;
                    g_ptree->pv[ply].a[ply+1] = hashmv;
                }
                UnMakeMove(turn, hashmv, ply+1);
            }
        }

        //**** record results

        // if srch stops at ply, exd will be ply-1. bestseq[0] is pv[ply-1]

        int newleng = ptree->pv[ply].length - ply;
        if (newleng == 1 && ply < singleCmd.pvleng &&
            singleCmd.bestseqLeng > 0 &&
            (int)ptree->pv[ply].a[ply+1] == singleCmd.bestseq[0].v) {
            newleng = min(GMX_MAX_BESTSEQ-2, singleCmd.bestseqLeng);
            for (int i = newleng-1; i>=0; i--) {
                singleCmd.bestseq[i+1] = singleCmd.bestseq[i];
            }
        }
        else {
            newleng = min(GMX_MAX_BESTSEQ-2, newleng);
            // 12/15/2010 #41 length*-ply* was missing
            forr (i, 0, newleng-1) {  // FIXME? ply+1?
                singleCmd.bestseq[i+1] = ptree->pv[ply].a[ply+i+1];
            }
        }

        singleCmd.val = running.val = val; // FIXME? running.val not needed?
        singleCmd.exd = ply-1;   // 11/27/2011 %7 was missing
        singleCmd.bestseq[0] = singleCmd.pv[ply-1];
        singleCmd.bestseqLeng = newleng+1;
        // FIXME follow hash in order to extend leng?

        // 12/11/2010 #9 was turn/ply/ply+1
        UnMakeMove( Flip(turn), singleCmd.pv[ply-1].v, ply );
        curPosPathChecks &= ~(1 << ply);
        turn = Flip(turn);
        curPosPathLeng--;
        val = -val;
    } // for ply

    inFirst = 0;

    //singleCmd.exd = ply;   // 12/11/2010 #12 was ply--
    //singleCmd.val = running.val;
    singleCmd.ule = ULE_EXACT;

    // FIXME rewindToRoot()?
    for (; ply >= 0; ply--) {
        if (ply > 0) {  // 12/11/2010 #11 was turn/ply/ply+1  #14 was no if()
            UnMakeMove(Flip(turn), singleCmd.pv[ply-1].v, ply);
            curPosPathChecks &= ~(1 << ply);
            turn = Flip(turn);
            curPosPathLeng--;
        }
    }

    // first(completed expdep,val) create reply

    if (!firstReplied) { // just in case rpyFirst is not done yet..
        singleCmd.exd++;
        singleCmd.val = - singleCmd.val;
        replyFirst();     // 11/27/2011 %8 was missing
        singleCmd.exd--;
        singleCmd.val = - singleCmd.val;
    }

    rpyent.pushFcomp(singleCmd.itd, singleCmd.exd, singleCmd.val,
                     min(GMX_MAX_BESTSEQ-1, singleCmd.bestseqLeng),
                     singleCmd.bestseq);
    rpypkt.send();   // 11/27/2011 %9' was missing

    //splane.cmdNotify(singleCmd.itd, singleCmd.exd, singleCmd.val,
    //                 singleCmd.pv[singleCmd.exd], -1);

    singleCmd.clear();
}

//**************** LIST  ***************

static void doList(jobDtorC srchJob)
{
    int64_t pren, postn;
    tree_t* ptree = g_ptree;
    int itd = srchJob.itd;
    int exd = srchJob.exd;
    int top = srchJob.mvsuf;  // 11/29/2011 %13 not used; was using mvtop

    iteration_depth = itd;

    sstreamC& sstream = splane.stream[itd];
    srowC& srow = splane.stream[itd].row[exd];
    sprocmvsC& sprmv = splane.stream[itd].row[exd].procmvs;

    // if LIST, call search(). if val>A, do PVS
    //           then set pvsretrying. send reply also

    running.job = srchJob;
    running.alpha = effalpha(srow.alpha, srow.gamma);

    // bring to cur pos - (FIXME) dont do this every time!
    bringPosTo(exd, &sstream.seqFromRoot[0], true/*setchk*/);

    int turn= countFlip(root_turn, exd);
    int ply = curPosPathLeng;
    int A = running.alpha;
    int B = srow.beta;  // 12/1/2011 %20 was using infinity instead of beta

    if (USE_SLV_HISTORY && srow.registKillerMv != NULLMV &&
        is_move_valid(g_ptree, srow.registKillerMv.v, turn)) {
        histGoodPv(g_ptree, srow.registKillerMv.v, ply+1,
                   (itd-ply)*PLY_INC, turn); //dep need not be exact
        srow.registKillerMv = NULLMV;
    }

    // MakeMove by a move at top
    smvEntryC& smvent  = sprmv.mvs[top];
    mvC mv = smvent.mv;
    SLDOut("-------- now try mv %07x turn %d ply %d\n",
           readable(mv), turn, ply);
    if (DBG_DUMP_BOARD) {
        out_board(g_ptree, slavelogfp, 0, 0);
    }

    int newply = ply+1;
    { int ply = newply;
        MOVE_CURR = mv.v;
        MakeMove(turn, mv.v, ply);
    }
    SLDOut("-------- mkmv done\n");

    int selfchk = InCheck(turn) ? 1 : 0;

    if (selfchk) {
        running.val = score_bound;       // 12/29(?)/2011 %58 was sc_mate1ply,
        UnMakeMove(turn, mv.v, ply+1);   // caused bestval update
        smvent.depth = DECISIVE_DEPTH;
        // 12/5/2011 %36 retval was -inf, s/b +inf
        smvent.lower = smvent.upper = score_bound;
        rpyent.pushPvs(itd, exd, score_bound,
                       smvent.mv, ULE_EXACT, 0 /*numnode*/,
                       0 /*bestseqLeng */,
                       NULL /*bestseq */);
        return;
    }

    int chk = InCheck(Flip(turn)) ? 1 : 0;
    ptree->nsuc_check[ply+2] = chk ? ptree->nsuc_check[ply] + 1 : 0;

    //nt newdep = (pvsCmd[srchJob].itd-ply-1+ popcnt(curPosPathChecks) + chk)
    // * PLY_INC + (1 + popcnt(pvsCmd[srchJob].onerepMask))*PLY_INC/2;
    int newdep = itdexd2srd(itd, exd) * PLY_INC / 2 - PLY_INC;
    // 11/29/2011 %14  *PLY_INC/2 was missing
    int reduc = 0;
    if (!chk && curPosPathLeng>0 &&
        !(UToCap(mv.v) || (I2IsPromote(mv.v) && I2PieceMove(mv.v) != silver)) &&
        mv.v != (int)ptree->amove_hash[ply+1] &&
        mv.v != (int)ptree->killers[ply+1].no1 &&
        mv.v != (int)ptree->killers[ply+1].no2    ) {
        int depth_reduced = 0;
        unsigned int key     = phash( mv.v, turn );
        unsigned int good    = hist_goodary[key]  + 1;
        unsigned int triedx8 = ( hist_tried[key] + 2 ) * 8U;

#ifndef WEAK_LMR
        if      ( good * 62U < triedx8 ) { depth_reduced = PLY_INC * 4/2; }
        else if ( good * 46U < triedx8 ) { depth_reduced = PLY_INC * 3/2; }
        else if ( good * 34U < triedx8 ) { depth_reduced = PLY_INC * 2/2; }
        else if ( good * 19U < triedx8 ) { depth_reduced = PLY_INC * 1/2; }
#else
        if      ( good *160U < triedx8 ) { depth_reduced = PLY_INC * 3/2; }
        else if ( good * 50U < triedx8 ) { depth_reduced = PLY_INC * 2/2; }
        else if ( good * 19U < triedx8 ) { depth_reduced = PLY_INC * 1/2; }
#endif
 
        reduc = depth_reduced;
    }
    newdep -= reduc;

    // FIXME? need move_last[], stand_pat<_full>[], etc? how about root_A/B?
    g_ptree->save_eval[ply+1] =
        g_ptree->save_eval[ply+2] = INT_MAX;
    g_ptree->move_last[ply+1] = g_ptree->amove;

    // store bestmv to hash
    if (smvent.bestmv != NULLMV) {
        if (!is_move_valid(ptree, smvent.bestmv.v, Flip(turn))) {
            if (DBG_SL && smvent.bestmv != NULLMV) {
                SLDOut("!!!!!!!! pvs: writing invalid mv to hash  mv=%07x\n",
                       readable(smvent.bestmv));
                out_board(ptree, slavelogfp, 0, 0);
                //assert(0);  FIXMEEEE!!!! should not occur!? investigate
            }
        } else
            if (!chk)   // FIXMEEEE!!!! 12/14/2011 %49 when this is done on
            { // checked case, somehow a non-evasion is stored.
                hash_store_pv(ptree, smvent.bestmv.v, Flip(turn));
                SLDOut("<<<< hashmv %07x stored\n", readable(smvent.bestmv));
            }
    }

    //sprmv.mvs[top].perfIncTrycnt();
    pren = g_ptree->node_searched;

    // FIXME?  exam_bb check?  (selfchk is done)
    SLDOut("-------- LIST srch A=%d dep=%d ...", A, newdep);
    perfRecMove.startSrch();
    int val = -search(g_ptree, -A-1, -A, Flip(turn),    //FIXME? see below
                      newdep, ply+2, state_node);
    perfRecMove.endSrchList(ply+1 /*exd*/, sprmv.mvs[top].perfTrycnt++);
    SLDOut(" .. done abt=%d val=%d\n", root_abort, val);

    if (root_abort) {
        UnMakeMove(turn, mv.v, ply+1); //12/5/2011 %35 was missing (srch2 also)
        rewindToRoot();
        return;
    }

    if (val>A) {

        // send retry-reply
        rpyent.pushRetrying(itd, exd, mv);
        rpypkt.send();

        running.retrying = 1;
        if (srow.alpha > A)  // A/B may have changed during the 1st srch
            A = running.alpha = srow.alpha;
        if (srow.beta < B)
            B = srow.beta;
        root_beta = -A;
        newdep += reduc + PLY_INC/2;

        // FIXME?  exam_bb check?  (selfchk is done)
        SLDOut("-------- LIST srch2 A=%d B=%d dep=%d ...", A, B, newdep);
        perfRecMove.startSrch();
#ifdef USE_LIST_ASP
        int oldrta = root_alpha;
        root_alpha = (B == score_bound && abs(A)<score_max_eval ?
                      -A - ASP_WINDOW: -B);
        val = -search(g_ptree, root_alpha, -A, Flip(turn),
                      newdep, ply+2, state_node); //FIXME?  ply ok?
        SLDOut(" .. done abt=%d val=%d\n", root_abort, val);
        if (!root_abort && -score_bound < root_alpha && val>=-root_alpha) {
            root_alpha = -score_bound;
            SLDOut("-------- LIST srch3 A=%d dep=%d ...", A, newdep);
            val = -search(g_ptree, -B, -A, Flip(turn),
                          newdep, ply+2, state_node);
            SLDOut(" .. done abt=%d val=%d\n", root_abort, val);
        }
        root_alpha = oldrta;
#else
        val = -search(g_ptree, -B, -A, Flip(turn),
                      newdep, ply+2, state_node); //FIXME?  ply ok?
        SLDOut(" .. done abt=%d val=%d\n", root_abort, val);
#endif
        perfRecMove.endSrchRetry(ply+1 /*exd*/);

        if (root_abort) {
            UnMakeMove(turn, mv.v, ply+1);
            rewindToRoot();
            return;
        }

        root_beta = score_bound;
        running.retrying = 0;

        //if (val>A) {   12/12/2011 %46 A may be updated during srch
        if (val>max(A,srow.alpha)) {  // 3/6/2012 %xx was val>sr.alpha
            srow.alpha = running.val =  val;  // FIXME? need running?
            // FIXME always?  if (alpha >= gamma)?
            //srow.gamma = - score_bound;
            srow.updated = 1;
            bool lower = (val >= B);  // 12/29(?)/2011 %59 lower case was missing

            // use hashmv if leng insufficient
#ifdef EXTEND_LIST_HASH
            if (!lower && g_ptree->pv[ply+1].length < ply+4) {
#else
                if (!lower && g_ptree->pv[ply+1].length < ply+2) {
#endif

                    int hashmv;
                    if (g_ptree->pv[ply+1].length < ply+2) {
                        hash_probe(g_ptree, ply+2, newdep, Flip(turn),
                                   -score_bound, score_bound, &state_node_dmy);
                        hashmv = g_ptree->amove_hash[ply+2];
                    } else
                        hashmv = g_ptree->pv[ply+1].a[ply+2];

                    // add it to pv if it's valid mv
                    if (hashmv != MOVE_NA && hashmv != MOVE_PASS &&
                        is_move_valid(g_ptree, hashmv, Flip(turn))) {
                        int xturn = Flip(turn);
                        MakeMove(xturn, hashmv, ply+2);
                        if (!InCheck(xturn)) {
                            g_ptree->pv[ply+1].length = ply+2;
                            g_ptree->pv[ply+1].a[ply+2] = hashmv;

#ifdef EXTEND_LIST_HASH
                            int hashmv2;
                            if (g_ptree->pv[ply+1].length < ply+3) {
                                int tv2 = hash_probe(g_ptree, ply+3, newdep-PLY_INC,
                                                     turn, -score_bound, score_bound, &state_node_dmy);
                                hashmv2 = g_ptree->amove_hash[ply+3];
                            } else
                                hashmv2 = g_ptree->pv[ply+1].a[ply+3];

                            if (hashmv2 != MOVE_NA && hashmv2 != MOVE_PASS &&
                                is_move_valid(g_ptree, hashmv2, turn)) {
                                MakeMove(turn, hashmv2, ply+3);
                                if (!InCheck(turn)) {
                                    g_ptree->pv[ply+1].length = ply+3;
                                    g_ptree->pv[ply+1].a[ply+3] = hashmv2;

                                    int tv3 = hash_probe(g_ptree, ply+4, newdep-2*PLY_INC,
                                                         xturn, -score_bound, score_bound, &state_node_dmy);
                                    int hashmv3 = g_ptree->amove_hash[ply+4];
                                    if (hashmv3 != MOVE_NA && hashmv3 != MOVE_PASS &&
                                        is_move_valid(g_ptree, hashmv3, xturn)) {
                                        MakeMove(xturn, hashmv3, ply+4);
                                        if (!InCheck(xturn)) {
                                            g_ptree->pv[ply+1].length = ply+4;
                                            g_ptree->pv[ply+1].a[ply+4] = hashmv3;
                                        }
                                        UnMakeMove(xturn, hashmv3, ply+4);
                                    }

                                }
                                UnMakeMove(turn, hashmv2, ply+3);
                            }
#endif
                        }
                        UnMakeMove(xturn, hashmv, ply+2);
                    }
                }

                // save bestmv/val
                srow.bestseqLeng = lower ? 0 :
                    min(GMX_MAX_BESTSEQ-1, ptree->pv[ply+1].length-ply-1);//FIXME ply+1?
                forr(i, 0, srow.bestseqLeng-1)
                    srow.bestseq[i]  = ptree->pv[ply+1].a[ply+i+2];
                // 12/12/2010 #25 ply+i+1 was i+1
                // 12/12/2010 #27 ply<+i>+*2*, not +1
                //pvsCmd[srchJob].bestseq[0]  = mv;

            } // val>A, srch2
        } // val>A, srch1

        postn = g_ptree->node_searched;

        //srch writes to [ply], not[ply-1] when Bcut
        if (!root_abort && val<=A) { // 3/6/2012 %xx was val<=srow.alpha) {

            //sCmd[srchJob].bestseq[0].v = ptree->pv[ply+2].a[ply+2];
            ////md[srchJob].bestseqLeng =(pvsCmd[srchJob].bestseq[0]==NULLMV)? 0:1;
            //sCmd[srchJob].bestseqLeng = (ptree->pv[ply+2].length >= ply+2) ? 1:0;
            // 1/28/2011 hash is written on bcut, use of hash s/b correct
            hash_probe(g_ptree, ply+2, newdep, Flip(turn),
                       -A-1, -A, &state_node_dmy);
            int hashmv = g_ptree->amove_hash[ply+2];
            // FIXME?  selfchk check?
            bool usemv = (hashmv != MOVE_NA && hashmv != MOVE_PASS &&
                          is_move_valid(g_ptree, hashmv, Flip(turn))) ;
            srow.bestseq[0].v = usemv ? hashmv : MOVE_NA;
            srow.bestseqLeng = usemv ? 1 : 0;
        }        // 12/15/2010 #39  bestseq/Leng in -A<val case was wrong

        //running.val = - val;

        // UnMakeMove by a move at top
        UnMakeMove(turn, mv.v, ply+1);

        //if (!root_abort) {

        //reflectResultsList();
        //val = running.val;      // FIXME? reuse of VAL may be dangerous
        int valChild = - val;  // NOTE VAL is for parent
        assert(running.job.onList());  // pvs
        //if ( !(running.alpha > srow.alpha &&
        //       running.alpha > val) ) {
        {
            //if not (cur srch invalid due to Alpha change),
            // cancel @ NOTIFY.  if pvsretry & fail lo, will come here
            int64_t numnode = postn - pren;
            //int mvloc = sprmv.top;
            int ule = (B<=val) ? ULE_UPPER : (running.alpha < val) ? ULE_EXACT : ULE_LOWER;

#if 0
            //smvent.depth = newdep;
            smvent.depth = itdexd2srd(itd, exd);
            smvent.lower = valChild;   // 11/29/2011 %17 upper/lower inverted
            smvent.upper = (ule == ULE_EXACT) ? valChild : score_bound;
            if (smvent.upper < -score_max_eval || score_max_eval < smvent.lower)
                smvent.depth = DECISIVE_SRD; // 11/29/2011 %15 big dep in mate case
#else
            smvent.update(itdexd2srd(itd, exd), valChild, -B, -A, numnode,
                          srow.bestseq[0]);
#endif
       
            //create reply: pvs(mv,val,bestseq[]) seq[]is: pv[] if upd, else 1mv
            rpyent.pushPvs(itd, exd, valChild,
                           smvent.mv, ule, numnode,
                           srow.bestseqLeng ,
                           srow.bestseq);
        }
    }
