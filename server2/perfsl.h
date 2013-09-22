/* $Id: perfsl.h,v 1.13 2012-04-06 08:49:12 eikii Exp $ */


//**************** slave dump ***************

static void dumpSlState() {
#if ww
 SLTOut("******** slave dump ********\n");
 SLTOut("mpLastMove %07x  curPosPath: leng %d  chk %04x path:\n",
         readable(mpLastMove), curPosPathLeng, curPosPathChecks);
 forr(i, 0, curPosPathLeng-1)
   SLTOut(" %07x", readable(curPosPath[i]));
 SLTOut("\nlistFifo[0:2] %d %d %d\n", listFifo[0],listFifo[1],listFifo[2]);

 SLTOut("\nsingleCmd: valid %d issued %d havelist %04x onerep %04x\n",
  singleCmd.valid, singleCmd.issued, singleCmd.haveList, singleCmd.onerepMask);
 SLTOut("  itd %d pvleng %d opcode %d fromroot %d   pv:\n",
  singleCmd.itd, singleCmd.pvleng, singleCmd.opcode, singleCmd.fromRoot);
 forr(i, 0, singleCmd.pvleng-1)
   SLTOut(" %07x", readable(singleCmd.pv[i]));
 SLTOut("  val %d exd %d ule %d bestseqLeng %d   bestseq:\n",
  singleCmd.val, singleCmd.exd, singleCmd.ule, singleCmd.bestseqLeng);
 forr(i, 0, singleCmd.bestseqLeng-1)
   SLTOut(" %07x", readable(singleCmd.bestseq[i]));

 forr(k, 0, 2) {
   //SLTOut("\npvsCmd[%d]: valid %d itd %d havelist %04x onerep %04x\n",
   //k,pvsCmd[k].valid, pvsCmd[k].itd,pvsCmd[k].haveList, pvsCmd[k].onerepMask);
   SLTOut("\npvsCmd[%d]: valid %d itd %d firstmv %07x onerep %04x\n",
    k,pvsCmd[k].valid, pvsCmd[k].itd,readable(pvsCmd[k].firstmv), pvsCmd[k].onerepMask);
   SLTOut("  exd %d pvleng %d alpha %d mvcnt %d top %d updated %d  bestmv:\n",
    pvsCmd[k].exd, pvsCmd[k].pvleng, pvsCmd[k].alpha, pvsCmd[k].mvcnt,
    pvsCmd[k].top, pvsCmd[k].updated);
   forr(i, 0, pvsCmd[k].pvleng-1)
     SLTOut(" %07x", readable(pvsCmd[k].pv[i]));
   SLTOut("\n");
   forr(i, 0, pvsCmd[k].mvcnt-1) {
     SLTOut(" %07x-%07x v:%d ule:%d", readable(pvsCmd[k].mvs[i]),
                          readable(pvsCmd[k].bestmv[i]),
                          pvsCmd[k].val[i],
                          pvsCmd[k].ule[i]);
     if ((i%2)==1)  SLTOut("\n");
   }
   SLTOut("\n");
   SLTOut("  bestseqLeng %d   bestseq:\n", pvsCmd[k].bestseqLeng);
   forr(i, 0, pvsCmd[k].bestseqLeng-1)
     SLTOut(" %07x", readable(pvsCmd[k].bestseq[i]));
   SLTOut("\n");
 }
 SLTOut("******** slave dump end ********\n");
#endif
}


// copied from iterate.c

#ifndef PRIu64
#define PRIu64 "llu"
#endif

static unsigned int cpu_start;

int
initPerf( tree_t * restrict ptree )
{
        
  /* initialize variables */
  if ( get_cputime( &cpu_start ) < 0 ) { return -1; }
  time_start = time_turn_start;

  ptree->node_searched         =  0;
  //ptree->nreject_done          =  0;
  //ptree->nreject_tried         =  0;
  ptree->null_pruning_done     =  0;
  ptree->null_pruning_tried    =  0;
  ptree->check_extension_done  =  0;
  ptree->recap_extension_done  =  0;
  ptree->onerp_extension_done  =  0;
  ptree->nfour_fold_rep        =  0;
  ptree->nperpetual_check      =  0;
  ptree->nsuperior_rep         =  0;
  ptree->nrep_tried            =  0;
  ptree->neval_called          =  0;
  ptree->nquies_called         =  0;
  ptree->ntrans_always_hit     =  0;
  ptree->ntrans_prefer_hit     =  0;
  ptree->ntrans_probe          =  0;
  ptree->ntrans_exact          =  0;
  ptree->ntrans_lower          =  0;
  ptree->ntrans_upper          =  0;
  ptree->ntrans_superior_hit   =  0;
  ptree->ntrans_inferior_hit   =  0;
  ptree->fail_high             =  0;
  ptree->fail_high_first       =  0;
  ptree->current_move[0]       =  0;
  ptree->pv[0].a[0]            =  0;
  ptree->pv[0].a[1]            =  0;
  ptree->pv[0].depth           =  0;
  ptree->pv[0].length          =  0;
  iteration_depth              =  0;
  easy_value                   =  0;
  easy_abs                     =  0;
  root_abort                   =  0;
  root_nmove                   = -1;
  root_value                   = -score_bound;
  root_alpha                   = -score_bound;
  root_beta                    =  score_bound;
  //root_move_cap                =  0;
  node_last_check              =  0;
  //time_last_eff_search         =  time_start;
  time_last_check              =  time_start;


#if defined(TLP)
  ptree->tlp_abort             = 0;
  tlp_nsplit                   = 0;
  tlp_nabort                   = 0;
  tlp_nslot                    = 0;
#endif
  return 0;  // dummy for compiler
}


int
displayPerf( tree_t * restrict ptree )
{
  /* prunings and extentions-statistics */
  {
    double drep, dreject, dhash, dnull, dfh1st;

    drep    = (double)ptree->nperpetual_check;
    drep   += (double)ptree->nfour_fold_rep;
    drep   += (double)ptree->nsuperior_rep;
    drep   *= 100.0 / (double)( ptree->nrep_tried + 1 );

    //dreject  = 100.0 * (double)ptree->nreject_done;
    //dreject /= (double)( ptree->nreject_tried + 1 );

    dhash   = (double)ptree->ntrans_exact;
    dhash  += (double)ptree->ntrans_inferior_hit;
    dhash  += (double)ptree->ntrans_superior_hit;
    dhash  += (double)ptree->ntrans_upper;
    dhash  += (double)ptree->ntrans_lower;
    dhash  *= 100.0 / (double)( ptree->ntrans_probe + 1 );

    dnull   = 100.0 * (double)ptree->null_pruning_done;
    dnull  /= (double)( ptree->null_pruning_tried + 1 );

    dfh1st  = 100.0 * (double)ptree->fail_high_first;
    dfh1st /= (double)( ptree->fail_high + 1 );

    SLTOut( "    pruning  -> rep=%4.2f%%  reject=%4.2f%%\n", drep, dreject );
    
    SLTOut( "    pruning  -> hash=%2.0f%%  null=%2.0f%%  fh1st=%4.1f%%\n",
         dhash, dnull, dfh1st );
    
    SLTOut( "    extension-> chk=%u recap=%u 1rep=%u\n",
         ptree->check_extension_done, ptree->recap_extension_done,
         ptree->onerp_extension_done );
  }

  /* futility threashold */
#if ! ( defined(NO_STDOUT) && defined(NO_LOGGING) )
  {
    int misc   = fmg_misc;
    int drop   = fmg_drop;
    int cap    = fmg_cap;
    int mt     = fmg_mt;
    int misc_k = fmg_misc_king;
    int cap_k  = fmg_cap_king;
    SLTOut( "    futility -> misc=%d drop=%d cap=%d mt=%d misc(k)=%d cap(k)=%d\n",
         misc, drop, cap, mt, misc_k, cap_k );
  }
#endif

  /* hashing-statistics */
  {
    double dalways, dprefer, dsupe, dinfe;
    double dlower, dupper, dsat;
    uint64_t word2;
    int ntrans_table, i, n;

    ntrans_table = 1 << log2_ntrans_table;
    if ( ntrans_table > 8192 ) { ntrans_table = 8192; }
    
    for ( i = 0, n = 0; i < ntrans_table; i++ )
      {
        word2 = ptrans_table[i].prefer.word2;
        SignKey( word2, ptrans_table[i].prefer.word1 );
        if ( trans_table_age == ( 7 & (int)word2 ) ) { n++; }

        word2 = ptrans_table[i].always[0].word2;
        SignKey( word2, ptrans_table[i].always[0].word1 );
        if ( trans_table_age == ( 7 & (int)word2 ) ) { n++; }

        word2 = ptrans_table[i].always[1].word2;
        SignKey( word2, ptrans_table[i].always[1].word1 );
        if ( trans_table_age == ( 7 & (int)word2 ) ) { n++; }
      }

    dalways  = 100.0 * (double)ptree->ntrans_always_hit;
    dalways /= (double)( ptree->ntrans_probe + 1 );

    dprefer  = 100.0 * (double)ptree->ntrans_prefer_hit;
    dprefer /= (double)( ptree->ntrans_probe + 1 );

    dsupe    = 100.0 * (double)ptree->ntrans_superior_hit;
    dsupe   /= (double)( ptree->ntrans_probe + 1 );

    dinfe    = 100.0 * (double)ptree->ntrans_inferior_hit;
    dinfe   /= (double)( ptree->ntrans_probe + 1 );

    SLTOut( "    hashing  -> always=%2.0f%% prefer=%2.0f%% supe=%4.2f%% "
         "infe=%4.2f%%\n", dalways, dprefer, dsupe, dinfe );

    dlower  = 100.0 * (double)ptree->ntrans_lower;
    dlower /= (double)( ptree->ntrans_probe + 1 );

    dupper  = 100.0 * (double)ptree->ntrans_upper;
    dupper /= (double)( ptree->ntrans_probe + 1 );

    dsat    = 100.0 * (double)n;
    dsat   /= (double)( 3 * ntrans_table );

    //OutCsaShogi( "statsatu=%.0f", dsat );
    SLTOut( "    hashing  -> "
         "exact=%d lower=%2.0f%% upper=%4.2f%% sat=%2.0f%% age=%d\n",
         ptree->ntrans_exact, dlower, dupper, dsat, trans_table_age );
    if ( dsat > 9.0 ) { trans_table_age  = ( trans_table_age + 1 ) & 0x7; }
  }

#if defined(TLP)
  if ( tlp_max > 1 )
    {
      SLTOut( "    threading-> split=%d abort=%d slot=%d\n",
           tlp_nsplit, tlp_nabort, tlp_nslot+1 );
      if ( tlp_nslot+1 == TLP_NUM_WORK )
        {
          out_warning( "THREAD WORK AREA IS USED UP!!!" );
        }
    }
#endif

  {
    double dcpu_percent, dnps, dmat;
    unsigned int cpu, elapsed;

    SLTOut( "    n=%" PRIu64 " quies=%u eval=%u rep=%u %u(chk) %u(supe)\n",
         ptree->node_searched, ptree->nquies_called, ptree->neval_called,
         ptree->nfour_fold_rep, ptree->nperpetual_check,
         ptree->nsuperior_rep );

    if ( get_cputime( &cpu )     < 0 ) { return -1; }
    if ( get_elapsed( &elapsed ) < 0 ) { return -1; }

    cpu             -= cpu_start;
    elapsed         -= time_start;

    dcpu_percent     = 100.0 * (double)cpu;
    dcpu_percent    /= (double)( elapsed + 1U );

    dnps             = 1000.0 * (double)ptree->node_searched;
    dnps            /= (double)( elapsed + 1U );

#if defined(TLP)
    {
      double n = (double)tlp_max;
      node_per_second  = (unsigned int)( ( dnps + 0.5 ) / n );
    }
#else
    node_per_second  = (unsigned int)( dnps + 0.5 );
#endif

    dmat             = (double)MATERIAL;
    dmat            /= (double)MT_CAP_PAWN;

    //OutCsaShogi( " cpu=%.0f nps=%.2f\n", dcpu_percent, dnps / 1e3 );
    SLTOut( "    time=%s  ", str_time_symple( elapsed ) );
   SLTOut( "cpu=%3.0f%%  mat=%.1f  nps=%.2fK", dcpu_percent, dmat, dnps / 1e3 );
    //SLTOut( "  time_eff=%s\n\n",
        // str_time_symple( time_last_eff_search - time_start ) );
  }

  return 0;  // dummy for compiler
}

 //**** added 12/28/2011

struct perfTimeNodeC {
 perfTimeNodeC() {}
 perfTimeNodeC(int64_t t, int64_t n) : time(t), node(n) {} // FIXME? not needed?
 int call;
 int64_t time, node;
 void clear() { time = node = 0LL; call = 0; }
 void add(int64_t t, int64_t n) { time += t; node += n; call++; }
 void addall(perfTimeNodeC* p) {
  time += p->time; node += p->node; call += p->call;
 }
 void print(const char* s) { SLTOut("perf %s: tm %12lld nd %12lld cl %8d\n",
                             s, time, node, call); }
};

struct perfRecordC {
  //#define MAXTRYCNT 32
 perfTimeNodeC perfAccumRoot, perfAccumFirst, perfAccumAbort, perfAccumPuzzle;
 perfTimeNodeC perfAccumRetry[MAX_EXPDEP];
 perfTimeNodeC perfAccumList[MAX_EXPDEP];
 perfTimeNodeC perfAccumList2[MAXTRYCNT];

 int64_t pren, pre, lastThinkTime;

 int64_t listsum() {
   int64_t x = 0LL;
   forr(e, 0, MAX_EXPDEP-1)
     x += perfAccumList[e].time;
   return x;
 }

 int64_t retrysum() {
   int64_t x = 0LL;
   forr(e, 0, MAX_EXPDEP-1)
     x += perfAccumRetry[e].time;
   return x;
 }

 int64_t listnodesum() {
   int64_t x = 0LL;
   forr(e, 0, MAX_EXPDEP-1)
     x += perfAccumList[e].node;
   return x;
 }

 int64_t retrynodesum() {
   int64_t x = 0LL;
   forr(e, 0, MAX_EXPDEP-1)
     x += perfAccumRetry[e].node;
   return x;
 }

 perfRecordC() {}

 void print(int64_t span) {
   perfAccumRoot.print("Root");
   perfAccumFirst.print("First");
   perfAccumAbort.print("Abort");
   perfAccumPuzzle.print("Puzzle");

   SLTOut("List:\n");
   forr(e, 0, MAX_EXPDEP-1) {
     if (!perfAccumList[e].call) continue;
     SLTOut("exd %d: ", e);
     perfAccumList[e].print("");
   }

   SLTOut("Retry:\n");
   forr(e, 0, MAX_EXPDEP-1) {
     if (!perfAccumRetry[e].call) continue;
     SLTOut("exd %d: ", e);
     perfAccumRetry[e].print("");
   }

   SLTOut("List2:\n");
   forr(j, 0, MAXTRYCNT-1) {
     if (!perfAccumList2[j].call) continue;
     SLTOut("trycnt %2d: ", j);
     perfAccumList2[j].print("");
   }

   int64_t ttltime = perfAccumAbort.time +
                     perfAccumRoot.time +
                     perfAccumFirst.time +
                     perfAccumPuzzle.time +
                     listsum() + 
                     retrysum();

   int64_t ttlnode = perfAccumAbort.node +
                     perfAccumRoot.node +
                     perfAccumFirst.node +
                     perfAccumPuzzle.node +
                     listnodesum() + 
                     retrynodesum();

   SLTOut("---- ratio ----\n");
   SLTOut("Abort: %.2f%%\n", perfAccumAbort.time * 100.0 / span);
   SLTOut("Root : %.2f%%\n", perfAccumRoot.time * 100.0 / span);
   SLTOut("First: %.2f%%\n", perfAccumFirst.time * 100.0 / span);
   SLTOut("Puzzl: %.2f%%\n", perfAccumPuzzle.time * 100.0 / span);
   SLTOut("List : %.2f%%\n", listsum() * 100.0 / span);
   SLTOut("Retry: %.2f%%\n", retrysum() * 100.0 / span);

   SLTOut("\nTotal: %.2f%%\n", ttltime * 100.0 / span);

   SLTOut("\nNPS  : %7d\n", ttlnode * 100000 / (1 + ttltime/10000));
 }

 void add(perfRecordC* p) {
   perfAccumRoot.addall(&(p->perfAccumRoot));
   perfAccumFirst.addall(&(p->perfAccumFirst));
   perfAccumAbort.addall(&(p->perfAccumAbort));
   perfAccumPuzzle.addall(&(p->perfAccumPuzzle));
   forr(e, 0, MAX_EXPDEP-1) {
     perfAccumRetry[e].addall(&(p->perfAccumRetry[e]));
     perfAccumList[e].addall(&(p->perfAccumList[e]));
   }
   forr(j, 0, MAXTRYCNT-1)
     perfAccumList2[j].addall(&(p->perfAccumList2[j]));
 }

 void clear() { memset(this, 0, sizeof(perfRecordC)); }

 void startSrch() {
   pre = worldTimeLl();
   pren = preNodeCount = g_ptree->node_searched;
   int64_t idle = (pre - lastThinkTime);
   idle /= 1000000;  // millisecond
 // FIXME? tune
#define LONG_IDLE 10
   if (lastThinkTime != 0LL && idle > LONG_IDLE)   // omit very first
     SLTOut("++++ long idle: %10lld ms\n", idle);
 }

 void endSrchRoot() { endSrch(ACCROOT); }
 void endSrchFirst() { endSrch(ACCFIRST); }
 void endSrchPuzzle() { endSrch(ACCPUZL); }
 void endSrchRetry(int exd) { endSrch(ACCRETRY, exd); }
 void endSrchList(int exd, int trycnt) { endSrch(ACCLIST, exd, trycnt); }

 void endSrch(int srchtyp, int exd = 0, int trycnt = 0) {
   int64_t post  = worldTimeLl();
   int64_t postn = g_ptree->node_searched;
   int64_t time  = post  - pre;
   int64_t node  = postn - pren;
   lastThinkTime = post;
   if (root_abort)
     perfAccumAbort.add(time, node);
   else if (srchtyp == ACCROOT)
     perfAccumRoot.add(time, node);
   else if (srchtyp == ACCFIRST)
     perfAccumFirst.add(time, node);
   else if (srchtyp == ACCPUZL)
     perfAccumPuzzle.add(time, node);
   else if (srchtyp == ACCRETRY) {
     assert(0 <= exd && exd < MAX_EXPDEP);
     perfAccumRetry[exd].add(time, node);
   } else {
     assert(srchtyp == ACCLIST && 0 <= exd && exd < MAX_EXPDEP &&
            0 <= trycnt && trycnt < MAXTRYCNT);
     perfAccumList[exd].add(time, node);
     perfAccumList2[trycnt].add(time, node);
   }

   SLTOut("******** srched suf %s exd %d cn %d node %lld time %lld nps %lld\n",
                  srchtypestr[srchtyp], exd, trycnt, node, time, 
                  (node * 100000) / (1 + time / 10000));

 }
};


perfRecordC perfRecMove, perfRecGame;
int64_t gameStartTime, lastFwdTime, noThinkTime;

static void perfSetrootHook() {
 perfRecGame.clear();
 perfRecMove.clear();
 gameStartTime = lastFwdTime = worldTimeLl();
 noThinkTime = 0LL;
}

static void perfFwdHook() {
 int64_t post = worldTimeLl();
 int64_t span = post - lastFwdTime;
 if (g_ptree->node_searched) {
   perfRecMove.print(span);
   perfRecGame.add(&perfRecMove);
   perfRecMove.clear();
 } else
   noThinkTime += span;

 lastFwdTime = post;
}

static void perfQuitHook() {
 int64_t post = worldTimeLl();
 int64_t gamespan = post - gameStartTime;
 int64_t thinkspan = gamespan - noThinkTime;

 perfFwdHook();
 SLTOut("------------ Game Total -------------\n");
 perfRecGame.print(thinkspan);
 SLTOut("------------ Perf Summary -----------\n");
 SLTOut("gameStartTime: %12lld \n", gameStartTime); //FIXME? gameEnd(post) also?
 SLTOut("lastFwdTime:   %12lld \n", lastFwdTime);
 SLTOut("noThinkTime:   %12lld \n", noThinkTime);

 SLTOut("\ngame Span:   %12lld \n", gamespan);
 SLTOut("\nthink Span:  %12lld \n", thinkspan);
}

