/* $Id: plane.h,v 1.202 2012-04-20 02:33:05 eikii Exp $ */

/** TODO 2/28/2012
   - in cmdFirst, the value s.t. "if value is below this, stop"
    s/b passed to slv?  (to avoid duplicate Fcomp execution)
   - rpl1st case: first of all, why does it happen?  review (q3?)
 */

#define SORT_BY_NUMNODE 1
#define USE_FIRST_BCUT 1

#define DBG_DO_VERIFY 0

//******** FIXME duplicated? ********

// copied from shogi.h
#define score_bound 32600
#define score_max_eval 32500

// 評価形式
typedef enum
{
    VALTYPE_ALPHA,
    VALTYPE_BETA,
    VALTYPE_GAMMA
} valtypeE; 

cmdPacketC cmd2send;
extern int inhFirst;

 // FIXME  right?
#define itdexd2srd(itd, exd) (2*(itd) - (exd))

//******** timeRecordC ********

struct timeRecordUnitC
{
    int itd, exd;
    mvC mv;
    int64_t starttime;
    int64_t accum; ///< 経過時間

    void clr() {
        memset(this, 0, sizeof(timeRecordUnitC));
    }

    void start(int i, int e, mvC m)
    {
        itd = i;
        exd = e;
        mv = m;
        starttime = worldTimeLl();
    }

    void regist(int i, int e, mvC m)
    {
        if (itd == i && exd == e && mv == m && starttime != 0LL) {
            // 累積経過時間に経過時間を追加
            accum += worldTimeLl() - starttime;
        }
        
        starttime = 0LL;
    }
};

// FIXME tune
#define ADD_RETRY_BONUS(x) ((x) * 2 / 4)

/**
 * プロセスごとの時間管理を行います。
 */
struct timeRecordC
{
    timeRecordUnitC timerecu[MAXPROC];

    void clr()
    {
        memset(this, 0, sizeof(timeRecordC));
    }

    void start(int pr, int i, int e, mvC m)
    {
        timerecu[pr].start(i, e, m);
    }

    void regist(int pr, int i, int e, mvC m)
    {
        timerecu[pr].regist(i, e, m);
    }
    
    int bonustime()
    {
        // find proc w/ max retry time
        int64_t z = timerecu[1].accum;
        int x;

        forr (i, 2, Nproc-1) {
            if (z < timerecu[i].accum) {
                z = timerecu[i].accum;   // z: max retry time among procs
            }
        }
        
        z /= 1000000;    // convert nanosec to millisec
        x = (int)z;      // convert int64 to int
        x = ADD_RETRY_BONUS(x);   // bonus time for retry time
        return x;
    }
    
    bool retryingAny()
    {
        forr (i, 1, Nproc-1) {
            if (timerecu[i].starttime != 0LL && timerecu[i].exd == 0) {
                return true;
            }
        }

        return false;
    }
};

timeRecordC timerec;

//******** mvEntryC ********

/**
 * @brief 各ノードの評価値や深さなどを保持します。
 */
class mvEntryC
{
public:
    mvC mv;
    mvC bestmv;
    int depth, numnode;
    short upper, lower;
    char inherited, retrying;
    
    mvEntryC()
    {
    }

    void clear()
    {
        memset(this, 0, sizeof(mvEntryC));
    }
    
    void reset4srch()
    {
        numnode = retrying = 0;
    }   // leave mv, bestmv, inherited intact

    int sortkey()
    {
        return mv.v;
    }
    
    int& sortval()
    {
        return numnode;
    }

    bool done(int dep, int A, int B)
    {
        //MSDOut("//// ent done - ent: d %d u %d l %d  arg: d %d A %d B %d\n",
        //       depth, upper, lower, dep, A, B);
        return (depth >= dep && (upper == lower || upper <= -B || -A <= lower));
                              // 12/1/2011 %21 was         A     B
    }

    bool exact(int dep)
    {
        // FIXME? need exact flag?
        return (depth >= dep && upper == lower);
    }

    void pushNewmv(mvC m)
    {
        clear();
        mv = m;
    }

    void update(int dep, int val, int ule, int numnod, mvC bestmove);
};

void mvEntryC::update(int dep, int val, int ule, int numnod, mvC bestmove)
{
    if (ule == ULE_EXACT) {
        upper = lower = val;
    }
    else if (ule == ULE_UPPER) {
        if (depth < dep) {
            upper = val;
            lower = -score_bound;
        }
        else {
            upper = val;
            //lower = min(lower, upper-1); FIXME? no change; allow up<lo
        }
    }
    else {
        assert(ule == ULE_LOWER);
        if (depth < dep) {
            lower = val;
            upper = score_bound;
        }
        else {
            lower = val;
            //upper = max(upper, lower+1); FIXME? no change; allow up<lo
        }
    }

    numnode = (dep == depth ? numnode : 0) + numnod;
    bestmv = (ule == ULE_EXACT || ule == ULE_LOWER ? bestmove : NULLMV);

    depth  = dep;
    inherited = retrying = 0;
    MSDOut(">>>> ent_upd new: dep %d up %d lo %d bstmv %07x\n",
           depth, upper, lower, readable(bestmv));
}

//******** procmvsC ********

/**
 * @brief プロセスごとの指し手を管理します？
 */
class procmvsC
{
public:
    int donecnt;
    int mvcnt;
    mvEntryC mvs[GMX_MAX_LEGAL_MVS];
    
    void sort();

    void invalidate()
    {
        mvcnt = -1;
    }

    /// 事前に設定された計算が完了していないノードの数を取得します。
    int nleftmvs()
    {
        return (mvcnt - donecnt);
    }

    /// 現在、計算が完了していないノードの数を計算します。
    int nnocompmvs(int dep, int A, int B)
    {
        int compcnt = 0;
        forr (i, 0, mvcnt-1) {
            if (mvs[i].done(dep, A, B)) {
                compcnt++;
            }
        }

        return (mvcnt - compcnt);
    }

    // 指し手が一致する配列のインデックスを取得します。
    int findsuf(mvC mv)
    {
        forr (i, 0, mvcnt-1) {
            if (mvs[i].mv == mv) {
                return i;
            }
        }

        return -1;
    }

    /// 計算が完了したノードの数を設定します。
    void setDonecnt(int dep, int A, int B)
    {
        donecnt = 0;
        forr (i, 0, mvcnt-1) {
            if (mvs[i].done(dep, A, B)) {
                donecnt++;
            }
        }
        
        MSDOut(";;;; set donecnt to %d\n", donecnt);
    }

    // もし指し手mvがあれば、再計算フラグを設定します。
    void setRetrying(mvC mv)
    {
        int suf = findsuf(mv);
        if (suf != -1) { // 1/1/2012 %62 was missing; may come after mv shrunk off
            mvs[suf].retrying = 1;
        }
    }
 
    void dumpmvs()
    {
        MSDOut("procmvs dump:\n");
        forr (i, 0, mvcnt-1) {
            MSDOut("%07x ", readable(mvs[i].mv));
            if (i % 10 == 9) MSDOut("\n");
        }
    }
};

#if SORT_BY_NUMNODE
#include "entsort.h"
#endif

//******** rowC ********

class rowC
{
public:
    int alpha, beta, gamma;
    int bestval, bestseqLeng, bestule;
    mvC bestseq[GMX_MAX_BESTSEQ];
    mvEntryC firstmv;
    int firstproc;
    procmvsC procmvs[MAXPROC];
    
    void summary()
    {
        MSTOut("==== rsum st%d rw%d A %d B %d G %d bv %d bul %d bL %d fmv %07x bsq",
               itd(), exd(), alpha, beta, gamma, bestval, bestule, bestseqLeng,
               readable(firstmv.mv));
        forr (i, 0, min(1, bestseqLeng-1)) {
            MSTOut(" %07x", readable(bestseq[i]));
        }
        MSTOut("%s\nproc done/mvcnt", bestseqLeng>2 ? ".." : "");
        
        forr (pr, 1, Nproc-1) {
            MSTOut("%d/%d ", procmvs[pr].donecnt, procmvs[pr].mvcnt);
        }
        MSTOut("\n");
    }

    // NOTE moves for all procmvs[*] are set at the same time, so [1] is enuf
    bool mvgened()
    {
        return (procmvs[1].mvcnt > -1);
    }

    void clear()
    {
        memset(this, 0, sizeof(rowC));
        invalidate();
    }

    void invalidate()
    {
        forr (i, 1, Nproc-1) {
            procmvs[i].invalidate();
        }
    }

    bool betacut()
    {
        return (beta <= bestval);
    }

    // donecnt is just reference for xfer/nxt1st only.
    // COMMIT will use compmvs    FIXME is this a right thing to do?

    void setDonecntAll()
    {
        int srd = itdexd2srd(itd(), exd());
        forr (i, 1, Nproc-1) {
            procmvs[i].setDonecnt(srd, effalpha(alpha, gamma), beta);
        }
    }

    int nleftmvs(int pr)
    {
        int srd = itdexd2srd(itd(), exd());
        procmvs[pr].setDonecnt(srd, effalpha(alpha, gamma), beta);
        return procmvs[pr].nleftmvs();
    }

    bool nonfirstCompleted()
    {
        if (betacut()) return true;
        
        int srd = itdexd2srd(itd(), exd());
        forr (i, 1, Nproc-1) {
            if (procmvs[i].nnocompmvs(srd, alpha, beta)) {
                return false;
            }
        }
        
        // dont care if firstmv is finished - commits are always done from tail
        //  - NO!  firstmv matters
        //if (bestval == -score_bound)
        //if (!firstmv.done(srd, alpha, beta))
        //if (alpha == -score_bound)
        //  return false;
        
        return true;
    }

    bool rowdone()
    {
        int dep = itdexd2srd(itd(), exd());
        int B   = -bestval;

        MSTOut("\\\\ rd: i %d bval %d 1mvdep %d dep %d 1mvup/lo %d/%d bseqlen %d\n",
               itd(), bestval, firstmv.depth, dep, firstmv.upper,
               firstmv.lower, bestseqLeng);

        if (bestval < -score_max_eval || firstmv.depth < dep || firstmv.lower < B
            || bestseqLeng < 2 || bestule != ULE_EXACT) {
            return false;
        }

        bool exactFound =
            (firstmv.upper == firstmv.lower) &&
            (firstmv.upper == B);
        
        forr (pr, 1, Nproc-1) {
            forr (k, 0, procmvs[pr].mvcnt-1) {
                mvEntryC& mvent = procmvs[pr].mvs[k];
                if (mvent.depth < dep || mvent.lower < B) return false;

                if (mvent.upper == mvent.lower && mvent.upper == B) {
                    exactFound = true;
                }
            }
        }
        
        MSTOut("\\\\ rdone: returning %d\n", (exactFound ? 1 : 0));
        return exactFound;
    }
    
    void findmv(mvC mv, int* pr, int* suf)
    {
        int s;
        *pr = *suf = -1;

        forr (p, 1, Nproc-1) {
            if ((s = procmvs[p].findsuf(mv)) > -1) {
                *suf = s; *pr = p; break;
            }
        }
    }

    int replaceFirst(mvC mv)
    {
        int pr, suf;
        findmv(mv, &pr, &suf);
        if (suf == -1) {  // FIXME?  should this be assertion?
            return 1;  // BESTMV not found, maybe mated and thus removed
        }
        
        mvEntryC oldfirst = firstmv;
        firstmv = procmvs[pr].mvs[suf];
        for (int j = suf - 1; j >= 0; j--) {
            procmvs[pr].mvs[j+1] = procmvs[pr].mvs[j];
        }
        procmvs[pr].mvs[0] = oldfirst;
#define HEAVYNODE 9999999
        procmvs[pr].mvs[0].numnode = HEAVYNODE;
        return 0;
    }

    bool mvdone(int dep, mvC mv, int A, int B);
    bool mvdoneExact(int dep, mvC mv);
    
    int setupReorder(int newdep, int pvleng, mvC* pv);
    void refCreate(int exd, int pvleng, mvC *pv, mvC mv2rt);
    void setlist(int mvcnt, mvC *mvs);
    
    void updateValue(int val, valtypeE typ);
    void updateBest(int val, mvC mv, int seqleng, mvC* seq);

    int itd();
    int exd();
};

 //****
 // returns 0 if success; 1 if BESTMV not found

int rowC::setupReorder(int newdep, int pvleng, mvC* pv)
{
  /* if FIRSTMV is not the current best, sort/reorder the list.  BESTMV to top

   1st   proc1      proc2
   +-+ +-------+ +-----+-+---+
   |A| |       | |  C  |B| D |    (B is the pv move i.e. previous best)
   +-+ +-------+ +-----+-+---+
                |
                V
   +-+ +-------+ +-+-----+---+
   |B| |       | |A|  C  | D |
   +-+ +-------+ +-+-----+---+
  */

    int bestsuf = -1, bestpr = -1;
    int d = exd();
    mvC bestmv = pv[d];
    // NOTE don't copy row if bestseqLeng==0
    assert (bestseqLeng > 0 && bestseq[0] != NULLMV && bestmv != NULLMV) ;
    MSTOut("]]]]setupRe:i %d e %d dep %d bseq0 %07x firstmv %07x bestmv %07x\n",
           itd(),exd(), newdep, readable(bestseq[0]),
           readable(firstmv.mv), readable(bestmv) );

#if SORT_BY_NUMNODE
    forr (pr, 1, Nproc-1) {
        procmvs[pr].sort();
    }
#endif

    bool firstmvExact =
        bestval > -score_bound && bestule == ULE_EXACT &&
        !waitRoot &&   // doRoot case cannot change pv
        bestseqLeng>0 &&
        firstmv.mv == bestseq[0] && 
        firstmv.depth >= newdep && 
        firstmv.upper == firstmv.lower;
    bool bestseqExact =
        bestval > -score_max_eval && bestule == ULE_EXACT &&
        !waitRoot &&
        bestseqLeng>0 &&
        bestseq[0] != firstmv.mv &&
        mvdoneExact(newdep, bestseq[0]);

    // FIXME! this cond is wrong
    bool movebest = (bestseq[0] != firstmv.mv && bestseq[0] != bestmv);
    mvC preferredMv = NULLMV;

    if (firstmvExact || bestseqExact) {
        MSTOut("==== suRe: repl1st fex %d bex %d fmv %07x bsq0 %07x bv %d bL %d\n",
               firstmvExact?1:0, bestseqExact?1:0, readable(firstmv.mv),
               readable(bestseq[0]), bestval, bestseqLeng);
        if (bestseqExact) {
            replaceFirst(bestseq[0]);
        }
        alpha = bestval;
        preferredMv = bestmv;
    }
    else
    { // block B

        // put BESTMV into FIRSTMV.MV (if not already so)

        if (firstmv.mv != bestmv) {
            // find the BESTMV first
            replaceFirst(bestmv);
            firstproc = bestpr;
        }

        // see if result for BESTSEQ[0] s/b kept
        // FIXME?  scan all mvs for exact results? well, bestseq not
        //         available except for bestseq[]...
        
        bestseqLeng = 0;
        bestule     = ULE_NA;
        alpha = bestval = -score_bound;
        
        preferredMv = bestseq[0];
    } // block B

    beta  =  score_bound;
    gamma = -score_bound;  // FIXME?  not needed?  set in setlist?

    // put BESTSEQ[0] to the first of its proc

    if (movebest) {
        MSTOut("==== suRe: movebest %07x\n", readable(preferredMv));

        // find PREFERRED_MV
        int suf, bestsuf2, bestpr2;
        forr (pr, 1, Nproc-1) {
            if ((suf = procmvs[pr].findsuf(preferredMv)) > -1) {
                bestsuf2 = suf; bestpr2 = pr; break;
            }
        }
        
        // FIXME?  below 4 lines s/b in "if (bestsuf != -1) { ... }" ?
        //         ... can such a case ever occur?
        mvEntryC oldbest = procmvs[bestpr2].mvs[bestsuf2];
        for (int j = bestsuf2 - 1; j >= 0; j--) {
            procmvs[bestpr2].mvs[j+1] = procmvs[bestpr2].mvs[j];
        }
        procmvs[bestpr2].mvs[0] = oldbest;
    }
    
    forr (pr, 1, Nproc-1) {
#if SORT_BY_NUMNODE
        //procmvs[pr].sort(); - moved to earlier
#endif
        forr (i, 0, procmvs[pr].mvcnt-1)
            procmvs[pr].mvs[i].numnode = 0;
    }
    
    forr (pr, 1, Nproc-1) {
        // FIXME TBC CUT_MATE
        procmvs[pr].donecnt = 0;
        //procmvs[pr].lastrpy = 0;

        forr (i, 0, procmvs[pr].mvcnt-1) {
            procmvs[pr].mvs[i].retrying = 0;
            // FIXMEEEE!!!!  revisit the cond for inherited
            procmvs[pr].mvs[i].inherited = (pr==bestpr && i==bestsuf) ? 1 : 0;
            // FIXME?  how about NUMNODE?  leave intact?
        }
    }

    MSTOut("]]]] setupRe end:i%d e%d len %d bstseq0 %07x 1stmv %07x\n",
           itd(),exd(), bestseqLeng, readable(bestseq[0]),
           readable(firstmv.mv) );
    return ((firstmvExact || bestseqExact) ? 1 : 0);
}

void rowC::refCreate(int exd, int pvleng, mvC *pv, mvC mv2rt)
{
    assert(exd < pvleng);
    clear();

    bestval = -score_bound;
    alpha   = -score_bound;
    beta    =  score_bound;
    
    firstmv.mv = pv[exd];
}

 //****
 // assumes firstmv is not in the arg list

void rowC::setlist(int mvnum, mvC *mvs)
{
    if (mvgened()) return;   // dont set list if already set

    forr (pr, 1, Nproc-1) {
        procmvs[pr].donecnt =
        procmvs[pr].mvcnt = 0;
    }
    int top = 0;

    while (1) {
        int pr;
        // assign like this: 1,2, ... ,N-1, N-1, ... ,2,1, 1,2,...
        
        if (top == mvnum) break;
        for (pr=1; pr<Nproc; pr++) {
            int loc = procmvs[pr].mvcnt++;
            procmvs[pr].mvs[loc].pushNewmv(mvs[top++]);
            if (top == mvnum) break;
        }

        if (top == mvnum) break;
        for (pr=Nproc-1; pr>=1; pr--) {
            int loc = procmvs[pr].mvcnt++;
            procmvs[pr].mvs[loc].pushNewmv(mvs[top++]);
            if (top == mvnum) break;
        }
    }
}

 //****
  // NOTE: returns T if firstmv==mv.  Beware - do not use this except for
  //       planeC::finalAnswer()

bool rowC::mvdone(int dep, mvC mv, int A, int B)
{
    int mvsuf;
    assert (bestseqLeng > 0);

    if (bestseq[0] == NULLMV) {   // firstmv is not done
        return false;
    }
    if (bestseq[0] == mv) {
        return true;
    }
    if (firstmv.mv == mv) {
        return firstmv.done(dep,A,B);
    }
    forr (pr, 1, Nproc-1) {
        if ((mvsuf = procmvs[pr].findsuf(mv)) != -1) {
            return procmvs[pr].mvs[mvsuf].done(dep, A, B);
        }
    }

    NTBR;   // FIXME may not be found if cut-mate
    return false;  // FIXME compilier dummy
}

 //****
  // NOTE: returns T if firstmv==mv.  Beware - do not use this except for
  //       planeC::finalAnswer()

bool rowC::mvdoneExact(int dep, mvC mv)
{
    int mvsuf;

    forr (pr, 1, Nproc-1) {
        if ((mvsuf = procmvs[pr].findsuf(mv)) != -1) {
            return procmvs[pr].mvs[mvsuf].exact(dep);
        }
    }
    NTBR;   // FIXME may not be found if cut-mate
    return false;  // FIXME compiler dummy
}

 //****

void rowC::updateValue(int val, valtypeE type)
{
    if (type == VALTYPE_ALPHA) {
        alpha = val;
    }
    else if (type == VALTYPE_BETA) {
        beta = val;
    }
    else {
        assert(type == VALTYPE_GAMMA);
        //oldG = gamma;
        gamma = val;
    }

    // always set Donecnt - by A up or B down, Donecnt may change
    setDonecntAll();
}

 //****

void rowC::updateBest(int val, mvC mv, int seqleng, mvC* seq)
{
    // 12/3/2011 %29 arg MV was missing, bestseq[] off by one
    int len = (beta<=val ? 0 : min(seqleng, GMX_MAX_PV - 2));
    forr (d, 0, len-1) {
        bestseq[d+1] = seq[d];
    }
    bestseq[0]  = mv;
    bestseqLeng = len + 1;
    bestval     = val;
    bestule     = beta<=val ? ULE_LOWER : ULE_EXACT;
    MSTOut(".... set bestseq: itd %d exd %d len %d val %d ule %d seq -\n",
           itd(), exd(), bestseqLeng, bestval, bestule);
    forr (d, 0, len) {
        MSTOut("%07x ", readable(bestseq[d]));
    }
    MSTOut("\n");
}

//******** streamC ********

class streamC
{
public:
    int tailExd;
    rowC row[MAX_EXPDEP];
    int seqprevLeng;
    mvC seqFromPrev[MAX_SEQPREV];

    streamC() {}
    int itd();  // defined after decl of plane

    void summary()
    {
        MSTOut("== ssum st%d  tl %d seqpr ", itd(), tailExd);
        forr (i, 0, seqprevLeng-1) {
            MSTOut(" %07x", readable(seqFromPrev[i]));
        }
        MSTOut("\n");
        forr (i, 0, tailExd) {
            row[i].summary();
        }
    }
    
    void clear() {
        memset(this, 0, sizeof(streamC));
        invalidate();
    }

    void invalidate() {
        forr (d, 0, MAX_EXPDEP-1) {
            row[d].invalidate();
        }
        tailExd = -1;
    }

    void propagateUp(int exd, int val);
    void propagateDown(int exd, int val, valtypeE valtyp);
    int  mvlistAvailable(int exd, int pvleng, mvC *pv, mvC mv2rt, int srd);

    // rpySetpv/Setlist/Retrying skip streamC
    void rpyFirst(int exd, int val);
    void rpyRoot(int val, mvC mv, mvC mv2);
    void rpyFcomp(int exd, int val, int pvleng, mvC* pv);
    void rpyPvs(int rank, int exd, int val, mvC mv, int ule, int numnode,
                int seqleng, mvC* bestseq);
};

 //****
 // returns 2 if result available.  1 if only moves available

int streamC::mvlistAvailable(int exd, int pvleng, mvC *pv, mvC mv2rt,int srd)
{
    rowC& r = row[exd];
    if (!r.mvgened() || seqFromPrev[0] != mv2rt) return 0;
    //if (pvleng < exd) return false;    3/11/2012 %xx pvleng->seqprevLeng
    if (seqprevLeng - 1 < exd) return 0;
    if (r.bestseqLeng == 0) return 0;
    // FIXME?  A--- || r-[-].bestval < -score_max_eval ?
    forr (d, 0, exd-1) {
        if (pv[d] != seqFromPrev[d+1]) {
            return 0;
        }
    }

    bool firstmvExact =
        r.bestval > -score_bound &&
        r.bestule == ULE_EXACT &&
        r.bestseqLeng>0 &&
        r.firstmv.mv == r.bestseq[0] && 
        r.firstmv.exact(srd);
    bool bestseqExact =
        r.bestval > -score_bound &&
        r.bestule == ULE_EXACT &&
        r.bestseqLeng>0 &&
        r.bestseq[0] != r.firstmv.mv &&
        r.mvdoneExact(srd, r.bestseq[0]);
    if (firstmvExact || bestseqExact) {
        return 2;
    }

    MSDOut("```` mvlsAvail: exd %d pvlen %d mv2r %07x bstsqlen %d pv:\n",
           exd, pvleng, readable(mv2rt), row[exd].bestseqLeng);
    forr (d, 0, exd-1) {
        MSDOut("%07x/%07x ", readable(pv[d]), readable(seqFromPrev[d+1]));
    }
    MSDOut("\n");

    return 1;
}

  //****

void streamC::rpyFirst(int exd, int valAtExd)
{
    // set up row[exd:0]
    // 12/26/2011 %51  w/o clr, old row beyond seqLeng was wrongly reused
    forr (d, exd+1, seqprevLeng-1) {  // FIXME?  Leng"-1" ?
        row[d].clear();
    }
    seqprevLeng = exd + 1;

    tailExd = exd;
    bool mvonly = (row[exd].alpha > -score_bound);
    int val = mvonly ? row[exd].alpha : valAtExd;

    for (int d=exd; d>=0; d--) {   // FIXME?  exd-1?
        rowC& r = row[d];
        r.gamma = (d==exd && row[exd].alpha > -score_bound) ? -score_bound : val;

        // issue LIST
        forr (pr, 1, Nproc-1) {
            mvtupleC tuples[600];
            procmvsC& prm = r.procmvs[pr];

            forr (j, 0, prm.mvcnt-1) {
                tuples[j].mv = prm.mvs[j].mv;
                tuples[j].bestmv = prm.mvs[j].inherited ?
                                   prm.mvs[j].bestmv : NULLMV;
                tuples[j].depth = prm.mvs[j].depth;
                tuples[j].upper = prm.mvs[j].upper;
                tuples[j].lower = prm.mvs[j].lower;
            }
            
            cmd2send.setCmdList(itd(), d, r.procmvs[pr].mvcnt, val, r.beta,
                                r.firstmv.mv , d,
                                seqFromPrev+1, tuples);
            cmd2send.send(pr);
        }

        val = -val;  // flip for negamax
    }

    if (mvonly) {
        assert(row[exd].bestseq[0] == row[exd].firstmv.mv);
        cmd2send.setCmdNotify(itd(), exd, row[exd].alpha, row[exd].bestseq[0]);
        forr (pr, 1, Nproc-1) {
            cmd2send.send(pr);
        }
        inhFirst = 0;
    }
}

  //****

void streamC::rpyRoot(int val, mvC mv, mvC mv2)
{
    clear();  // %XX 12/27/2011
    row[0].bestval = val;
    row[0].bestule = ULE_EXACT;
    row[0].bestseq[0] = mv;
    row[0].bestseq[1] = mv2;
    row[0].bestseqLeng = (mv==NULLMV ? 0 : (mv2==NULLMV ? 1 : 2));
}

  //****

void streamC::rpyFcomp(int exd, int val, int pvleng, mvC* pv)
{ //set row[exd].A
    rowC& r = row[exd];
    MSTOut("**** rpyFcomp d %d l %d ln %d pv %07x %07x frstmv %07x bestmv %07x\n",
           exd, val, pvleng, readable(pv[0]), readable(pv[1]),
           readable(r.firstmv.mv), readable(r.firstmv.bestmv));

    if (exd > tailExd) {
        // Note bcut by upper row may occur
        MSTOut("**** row already canceled, quitting fcomp\n");
        return;    // fcomp result no longer needed due to another mv's result
    }
    assert(pv[0] == r.firstmv.mv);

    r.firstmv.upper =
    r.firstmv.lower = -val;
    r.firstmv.depth = itdexd2srd(itd(), exd);
    r.firstmv.bestmv= pvleng>1 ? pv[1] : NULLMV;
    r.firstmv.numnode = 0; // ?? FIXME? what's the right thing to do?
    r.firstmv.inherited =
    r.firstmv.retrying = 0;

    r.gamma = -score_bound; // 12/3/2011 %30 setting gamma was missing

    if (val > r.alpha) {
        r.updateValue(val, VALTYPE_ALPHA);
        r.updateBest(val, r.firstmv.mv, pvleng-1, pv+1);
    }

    // this looks correct, but this lowers CPU efficiency to 40% or so...
    //if ((val > oldA || oldG > oldA) && val != oldG) {
    if (1) {
        r.setDonecntAll();   // NOTE use r.{alpha, gamma} here
        // FIXME setDonecnt is called inside updateValue(), duplicating
        // FIXME?  how about firstproc?
        propagateUp(exd-1, -val);

        cmd2send.setCmdNotify(itd(), exd, val, pv[0]);   // FIXME? pv[0]?
        forr (pr, 1, Nproc-1) {
            cmd2send.send(pr);
        }
    }

    summary();

    if (DBG_DO_VERIFY) { //send A/B/G of this stream to each slv, let them compare
        tripleC tr[12];
        forr (i, 0, 11) {
            tr[i].alpha = row[i].alpha;
            tr[i].beta  = row[i].beta ;
            tr[i].gamma = row[i].gamma;
        }
        cmd2send.setCmdVerify(itd(), tailExd, tr);
        forr (pr, 1, Nproc-1) {
            cmd2send.send(pr);
        }
    }
}

  //****

void streamC::rpyPvs(int rank, int exd, int valChild, mvC mv, int ule,
                     int numnode, int seqleng, mvC* bestseq)
{
    rowC& r = row[exd];
    int mvsuf = r.procmvs[rank].findsuf(mv);
    if (mvsuf == -1 || tailExd < exd)
        return;
    // 12/25/2011 %52 mvgen chk missing. rpy may come after mv is shrunk off
    // 3/12/2012  %xx exd chkd missing. rpy may come after bcut in upper row

    r.procmvs[rank].mvs[mvsuf].update(itdexd2srd(itd(), exd),
                                      valChild, ule, numnode,
                                      (seqleng ? bestseq[0] : NULLMV));
    int val = -valChild; // 12/1/2011 %23 val inversion was missing

    if ((ule == ULE_EXACT) ||    // 3/11/2012 %xx this if cond missing
        (ule == ULE_UPPER && val >= r.beta) ||
        (ule == ULE_LOWER && val <= effalpha(r.alpha, r.gamma))) {
        r.procmvs[rank].donecnt++;  // 12/1/2011 %22 donecnt increment was missing
    }

    MSTOut(":::: rk %d i %d e %d tl %d A %d B %d G %d done %d/%d bstv %d len %d\n",
           rank, itd(), exd, tailExd, r.alpha, r.beta, r.gamma,
           r.procmvs[rank].donecnt, r.procmvs[rank].mvcnt, r.bestval, r.bestseqLeng);

    // FIXME slave returns w/ seqleng=0 very often.  need hash retrieval?

    if (ule == ULE_UPPER) { //  3/11/2012 %xx upper case was missing
        assert(r.beta <= val);
        r.updateValue(val, VALTYPE_ALPHA);
        r.updateBest(val, mv, seqleng, bestseq);

        cmd2send.setCmdNotify(itd(), exd, val, mv);
        forr(pr, 1, Nproc-1)
            //if (pr != rank)
            cmd2send.send(pr);
    }

    if (r.alpha < val && ule == ULE_EXACT) { // 12/11/2011 %43 ule was missing

        // FIXME?  often slave returns w/ seqleng=0.  need extension by hash?

        int oldA = r.alpha;
        r.updateValue(val, VALTYPE_ALPHA);
        r.updateBest(val, mv, seqleng, bestseq);

        cmd2send.setCmdNotify(itd(), exd, val, mv);
        forr (pr, 1, Nproc-1) {
            cmd2send.send(pr);
        }

        if (USE_FIRST_BCUT &&    // FIXME verify if this makes me stronger
            exd == tailExd && oldA == -score_bound && val > r.gamma) {
            // firstmv bcut
            const int RUNNING_SINGLE = 99;   // NOTE must match def in slave
            cmd2send.setCmdCommit(RUNNING_SINGLE, 0 /*dmy exd*/);
            cmd2send.send(PR1);   // FIXME? PR1?

            r.firstmv.update(itdexd2srd(itd(), exd), -r.gamma, ULE_LOWER,
                             0 /* numnode FIXME 0? */,
                             (seqprevLeng > exd+1 ? seqFromPrev[exd+1] : NULLMV));
        }

        if (ule == ULE_EXACT && val < r.beta) {
            propagateUp(exd-1, -val);
            propagateDown(exd+1, -val, VALTYPE_BETA);
            // NOTE if val>=B,
            //  - uppers are not affected
            //  - lowers will terminate due to bcut
            //  therefore no action needed (FIXME? right?)
        }

        summary();

        if (DBG_DO_VERIFY) {//send A/B/G of this stream to each slv, let them compare
            tripleC tr[12];
            forr(i, 0, 11) {
                tr[i].alpha = row[i].alpha;
                tr[i].beta  = row[i].beta ;
                tr[i].gamma = row[i].gamma;
            }
            cmd2send.setCmdVerify(itd(), tailExd, tr);
            forr(pr, 1, Nproc-1)
                cmd2send.send(pr);
        }
    }
}


//****

void streamC::propagateUp(int exd, int val)
{
    forv (d, exd, 0) {
        rowC& r = row[d];
        if (val == r.gamma)
            break;   // FIXME need this break?  or go upto d=0?
        MSTOut("jjjj propUp i %d e %d oldG %d newG %d\n",
               itd(), d, r.gamma, val);
        r.updateValue(val, VALTYPE_GAMMA);
        val = -max(r.alpha, val);
    }
}

//****

void streamC::propagateDown(int exd, int val, valtypeE valtyp)
{
    valtypeE typ = valtyp;
    assert(typ == VALTYPE_ALPHA || typ == VALTYPE_BETA);

    forr (d, exd, tailExd) {
        bool isA = (typ == VALTYPE_ALPHA);
        rowC& r = row[d];
        if (isA && val <= r.alpha) 
            break;   // FIXME need this break?  or go downto d=tail?

        MSTOut("jjjj propDn i %d e %d isB %d old %d new %d\n",
               itd(), d, typ, (typ ? r.beta : r.alpha), val);
        r.updateValue(val, typ);
        typ = (!isA ? VALTYPE_ALPHA : VALTYPE_BETA);
        val = -val;
    }
}

//******** planeC ********

class planeC
{
public:
    int shallowItd, deepItd, lastDoneItd, rwdLatch, rwdLatch2;
    mvC mv2root;
    streamC stream[MAX_ITD];

    void summary()
    {
        MSTOut("== psum: shal %d deep %d last %d rw12 %d/%d mv2r %07x\n",
               shallowItd, deepItd, lastDoneItd, rwdLatch, rwdLatch2,
               readable(mv2root));
        if (lastDoneItd > 0) {
            stream[lastDoneItd].row[0].summary();
        }

        if (shallowItd > 0) {
            forr (i, shallowItd, deepItd) {
                stream[i].summary();
            }
        }
    }

    void clear()
    {
        memset(this, 0, sizeof(planeC));
        forr (i, 0, MAX_ITD-1) {
            stream[i].invalidate();
        }
    }

    void makeMoveRoot(mvC mv);
    void unmakeMoveRoot();

    bool terminating();    // return True if ending srch (deep enuf or mate[d])
    void catchupCommits(); // issue cmdCommit for finished rows if any
    bool transferIfNeeded();  // issue EXT/SHR if unbalanced
    void next1stIfNeeded();   // issue next-1st if needed (few mvs for some proc)
    void finalAnswer(int *valp, mvC *mvp);  // return VAL, MVs for bnz master

    int  rpySetpv(int itd, int pvleng, mvC* pv);
    void rpySetlist(int itd, int exd, int mvcnt, mvC* mv) {
        stream[itd].row[exd].setlist(mvcnt, mv);
    }
    void rpyFirst(int itd, int exd, int val) {
        stream[itd].rpyFirst(exd, val);
    }
    void rpyRetrying(int itd, int exd, mvC mv, int pr)
    {
        stream[itd].row[exd].procmvs[pr].setRetrying(mv);
        timerec.start(pr, itd, exd, mv);
    }
    void rpyRoot(int itd, int val, mvC mv, mvC mv2)
    {
        if (itd <= lastDoneItd) return;
        stream[itd].rpyRoot(val, mv, mv2);
        lastDoneItd = itd;
        shallowItd =
        deepItd    = itd + 1;
    }
    void rpyFcomp(int itd, int exd, int val, int pvleng, mvC* pv)
    {
        stream[itd].rpyFcomp(exd, val, pvleng, pv);
    }
    void rpyPvs(int rank, int itd, int exd, int val, mvC mv, int ule, int numnode,
                int seqleng, mvC* bestseq)
    {
        MSDOut(":::: dI %d sI %d lstI %d\n", deepItd, shallowItd, lastDoneItd);
        timerec.regist(rank, itd, exd, mv);
        stream[itd].rpyPvs(rank, exd, val, mv, ule, numnode, seqleng, bestseq);
    }

    //void initialAction(int* usefirst, int* uselist);
    void initialAction();
};

#include "initact.h"

//********

void planeC::makeMoveRoot(mvC mv)
{
    lastDoneItd =
    shallowItd  =
    deepItd     = 0;
    forr (i, 1, MAX_ITD-1) {
        stream[i].tailExd = -1;
    }
    timerec.clr();

    rwdLatch2 = rwdLatch;   // rwd2 is on after RWD-FWD

    if (rwdLatch) {
        rwdLatch = 0;
        assert(mv2root == NULLMV);
        mv2root = mv;
        return;    // FIXME?  should erase streams that are no longer valid?
    }
    // below we can assume rwdLch=0
    MSDOut("---- mkmvrt mv2r %07x\n", readable(mv2root));

    // when making two fwd's, 'shift' itd for one mv, no shift for the other
    // thus SRD (srch dep = 2*i-e) for the row remains the same
    int shift = (compTurn == root_turn ? 1 : 0);

    forr (i, 1, MAX_ITD-1-shift) {
        int srci = i + shift;
        MSDOut("---- i%d i+%d:seqFrPr ", i, shift);
        forr (k, 0, stream[srci].seqprevLeng-1) {
            MSDOut("%07x ", readable(stream[srci].seqFromPrev[k]));
        }
        MSDOut("\n");

        if (stream[srci].seqFromPrev[0] != mv2root) {
            MSDOut("---- clr strm i%d\n", i); 
            stream[i].clear();
        }
        else {
            MSTOut("---- reuse strm i%d from i+%d\n", i, shift); 
            forr (j, 0, GMX_MAX_PV-2) {
                stream[i].row[j] = stream[srci].row[j+1];
                stream[i].seqFromPrev[j] = stream[srci].seqFromPrev[j+1];
            }
            stream[i].row[GMX_MAX_PV-1].clear();
            stream[i].seqFromPrev[GMX_MAX_PV-1] = NULLMV;
            stream[i].seqprevLeng = max(stream[srci].seqprevLeng - 1, 0);
        }
    }

    if (shift) stream[MAX_ITD-1].clear();
    mv2root = mv;
}

//********

void planeC::unmakeMoveRoot()
{
    mv2root = NULLMV;
    rwdLatch = 1;
}

//********

int planeC::rpySetpv(int itd, int pvleng, mvC* pv) // set up row[pvleng-1:0]
{
    int haveList = 0;
    MSTOut("%%%%%%%% setpv: i %d len %d mv2r %07x pv:\n", itd, pvleng,
           readable(mv2root));
    forr (e, 0, pvleng-1) {
        MSTOut("%07x ", readable(pv[e]));
    }
    MSTOut("\n");

    // FIXME?  something like shal-3 to shal+1?  (does it make sense?)
    for (int i=MAX_ITD-1; i>=1; i--) {
        MSDOut("%%%%     i%d seqPrev:", i);
        forr (ee, 0, stream[i].seqprevLeng-1) {
            MSDOut("%07x ", readable(stream[i].seqFromPrev[ee]));
        }
        MSDOut("\n");
    }

    forr (e, 0, pvleng-1) {
        // find already available mv list if any, for each exd
        int copyFrom = -1;
        int ret;
        int srd = itdexd2srd(itd, e);
        for (int i=MAX_ITD-1; i>=1; i--) {
            ret = stream[i].mvlistAvailable(e, pvleng, pv, mv2root, srd);
            if (ret) {
                // mvlist found
                if (copyFrom == -1 || ret == 2)
                    copyFrom = i;
                if (ret == 2)
                    break; // (FIXME)  find 'most dominant' mvlist?
            }
        }

        if (copyFrom == -1) {  // mvlist not found
            MSTOut("%%%%%%%% i%d e%d refcre mv2r %07x\n",
                   itd, e, readable(mv2root));
            stream[itd].row[e].refCreate(e, pvleng, pv, mv2root);

        }
        else {  // mvlist found
            MSTOut("%%%%%%%% i%d e%d setupRe cpfr %d\n", itd, e, copyFrom);
            if (copyFrom != itd) {
                stream[itd].row[e] = stream[copyFrom].row[e];
            }
            // FIXME set variables in row[e]
            int bestFound =
                stream[itd].row[e].setupReorder(itdexd2srd(itd, e), pvleng, pv);
            // 12/31/2011 %60 was using copyFrom instead of itd
            haveList |= 1 << e;
            if (bestFound) {
                MSTOut("==== rySpv: bestfound e %d\n", e);
                haveList |= HVLST_MVONLY;
                pvleng = e+1;
                pv[e] = stream[itd].row[e].firstmv.mv;  // NOTE pv is a local var
                break;         // in both initact and rpySetpv
            }
        }
    }

    // 12/25/2011 %53 was inside loop above.  e0 chg caused err in e1+ later
    forr (e, 0, pvleng-1)
        stream[itd].seqFromPrev[e+1] = pv[e];

    forr (e, pvleng, GMX_MAX_PV-1)
        stream[itd].row[e].clear();  // 12/25/2011 %54 old row survived w/
    // wrong mv2root, caused error later
    stream[itd].seqFromPrev[0] = mv2root;
    stream[itd].seqprevLeng = pvleng+1;

    waitRoot = 0;
    return haveList;
}

//********

bool planeC::terminating()
{
    // 12/1/2011 %25 -inf and itd not complete, should not resign

    if (lastDoneItd < MIN_ITD)
        return false;

    if ((stream[lastDoneItd].row[0].bestval <= -score_max_eval &&
         (stream[lastDoneItd].row[0].bestule == ULE_EXACT ||
          stream[lastDoneItd].row[0].bestule == ULE_UPPER) ) ||
        lastDoneItd >= MAX_SRCH_DEP) {  // 12/3/2011 %31 srch dep cond was missing
        return true;
    }

    for (int i = deepItd; i >= shallowItd; i--) {
        if (stream[i].tailExd > -1 &&
            stream[i].row[0].bestval >= score_bound - 2) {
            return true;
        }
    }

    return false;
}

//********

void planeC::finalAnswer(int *valp, mvC *mvp)
{
    rowC *bestrow;
    int i;

    // find last committed stream 
    i = lastDoneItd;
    bestrow = &stream[i].row[0];
    assert(bestrow->bestule == ULE_EXACT);
    MSTOut(".... finalAnswer: lastItd %d len %d val %d mv0 %07x\n",
           lastDoneItd, bestrow->bestseqLeng, bestrow->bestval,
           readable(bestrow->bestseq[0]));

    //assert(i>0);   // this func must be called after one stream is done
    // 3/9/2012 %?? oppn plays unexpected mv immediately, RWD may come
    if (i == 0) {  // assuming RWD, val/mv not to be used
        *valp = -score_bound;
        forr (k, 0, GMX_MAX_PV-1) mvp[k] = NULLMV;
        //mvp[0] = mvp[1] = NULLMV;
        return;
    }

    // current stream result can be used only if previous str's mv has been done
    forr (j, i+1, deepItd) {
        rowC *row = &stream[j].row[0];

        if (row->procmvs[1].mvcnt > -1 &&  // 12/13/2011 %47 was missing
            stream[j].seqFromPrev[0] == mv2root &&
            row->bestseqLeng > 0 &&
            row->bestule == ULE_EXACT &&
            row->bestval > -score_max_eval &&
            row->mvdone(itdexd2srd(j,0), bestrow->bestseq[0],
                        row->alpha, row->beta)) {
            bestrow = row;
            MSTOut(".... finalAnswer: newItd %d len %d val %d mv %07x %07x\n",
                   j, row->bestseqLeng, row->bestval,
                   readable(row->bestseq[0]), readable(row->bestseq[1]));
        }
        else {
            break;
        }
    }

    // return results
    *valp = bestrow->bestval;
    forr (k, 0, GMX_MAX_PV-1) {
        mvp[k] = bestrow->bestseq[k];
    }
}

//********

bool planeC::transferIfNeeded()
{
    // 3/30/2012 included transfer.h here
    // NOTE: return value: T if first is NOT needed

#define XFER_MIN_NMV 16
#define XFER_MAX_NMV 26

    if (stream[shallowItd].tailExd == -1 &&
        stream[shallowItd].row[0].alpha == -score_bound) {
        return true;  // skip both xfer and first
    }

    streamC& st = stream[shallowItd];

    int nleft[MAX_EXPDEP][MAXPROC];
    int procsum[MAXPROC];
    areaclr(nleft);
    areaclr(procsum);

    // count left mvs for each proc

    forr (pr, 1, Nproc-1) {
        forr (e, 0, st.tailExd) {
            int x = st.row[e].nleftmvs(pr);
            nleft[e][pr] = x;
            procsum[pr] += x;
        }
    }

    // find max and min procs
    int maxpr = -1, minpr = -1;
    int nmaxpr = -1;
    int nminpr = 99999;
    forr (pr, 1, Nproc-1) {
        if (procsum[pr] > nmaxpr) {
            nmaxpr = procsum[pr];
            maxpr  = pr;
        }
        if (procsum[pr] < nminpr) {
            nminpr = procsum[pr];
            minpr  = pr;
        }
    }
    //MSTOut("nmin %d min %d nmax %d max %d\n", nminpr, minpr, nmaxpr, maxpr);
    //MSTOut("pr0  %d pr1 %d \n", procsum[1], procsum[2]);
    assert(minpr != -1);
    assert(maxpr != -1);

    // do nothing if every proc has enuf mvs
    // do first   if no    proc has more than few mvs
    if (nminpr > XFER_MIN_NMV)
        return true;   // skip both xfer and first
    if (nmaxpr < XFER_MAX_NMV)
        return false;  // skip xfer, let first go ahead

    // always try to move 4(?) mvs from one row (FIXME tune later)
#define NUM_XFER_MOVE 4
#define NUM_XFER_BUF  4
    int e;
    for (e=st.tailExd; e>=0; e--) {
        int n = (e==st.tailExd ? NUM_XFER_BUF : 0) + NUM_XFER_MOVE;
        if (nleft[e][maxpr] >= n) break;
    }

    if (e < 0) {
        return false; // do first
    }

    // issue EXT/SHR commands: i, e, maxpr, minpr, (nmax-nmin)/3
    rowC& r = st.row[e];
    int n = NUM_XFER_MOVE;
    int mvloc  = r.procmvs[maxpr].mvcnt - n;
    int minloc = r.procmvs[minpr].mvcnt;
    mvtupleC tuples[MAX_XFER_MVS];
    MSTOut(".... xfer: nmx %d nmn %d n %d mxpr %d mnpr %d mnloc %d mvloc %d\n",
           nmaxpr, nminpr, n, maxpr, minpr, minloc, mvloc);
    forr (j, 0, n-1) {
        mvEntryC& me = r.procmvs[minpr].mvs[minloc+j];
        me         = r.procmvs[maxpr].mvs[mvloc+j];  // mv, bestmv, dep, up, lo
        me.numnode = 0;
        me.inherited = (me.bestmv == NULLMV ? 0 : 1);
        //assert(me.retrying == 0);  FIXME?  should not move a mv under retry?
        me.retrying = 0;
        assert(me.mv != NULLMV);

        tuples[j].mv     = me.mv;
        tuples[j].bestmv = me.bestmv;
        tuples[j].depth  = me.depth ;
        tuples[j].upper  = me.upper ;
        tuples[j].lower  = me.lower ;
    }

    cmd2send.setCmdExtend(shallowItd, e, n, tuples);
    cmd2send.send(minpr);
    cmd2send.setCmdShrink(shallowItd, e, mvloc);
    cmd2send.send(maxpr);
    st.row[e].procmvs[maxpr].mvcnt = mvloc;   // FIXME donecnt also?
    st.row[e].procmvs[minpr].mvcnt += n;
    pfGame.incExtshr(minpr, n);
    return true;  // skip first
}

//********

void planeC::next1stIfNeeded()
{
    if (0) MSDOut("@@@@@@@@ in nxt1 dI %d sI %d tl %d cnt %d tlA %d\n",
                  deepItd, shallowItd, stream[shallowItd].tailExd,
                  stream[shallowItd].row[0].procmvs[1].mvcnt,
                  stream[shallowItd].row[stream[shallowItd].tailExd].alpha);
    int i = shallowItd;
    if (deepItd > i) return;  // 9/22/2011 only two streams allowed FIXME relax?
    if (deepItd >= MAX_SRCH_DEP) return; // 12/5/2011 %37 was missing
    // 12/25/2011 %55 needed inhFirst while Root/First is running
    if (inhFirst || shallowItd == 0 ||
        (shallowItd > lastDoneItd && shallowItd == deepItd &&
         ((stream[shallowItd].tailExd == -1 &&
           stream[shallowItd].row[0].alpha == -score_bound) ||
          stream[shallowItd].row[0].procmvs[1].mvcnt == -1 ||
          stream[shallowItd].row[stream[shallowItd].tailExd].alpha == -score_bound)))
        return; // dont do anything at very first  11/27/2011 %6 was missing

#if 0
#define NMV4NEXT1ST 16 // FIXME bogus, tune
#endif

    // set PV
    ++deepItd;
    i = lastDoneItd;  // 12/11/2011 %44 chose stream when best* was not set yet

    //if (stream[i].row[0].bestval < stream[i+1].row[0].bestval
    // FIXME? above, need chk for > -score_max_eval?
    if (stream[i+1].row[0].bestseqLeng > 1
        // 12/25/2011 %56 this line above missing
        && stream[i+1].seqFromPrev[0] == mv2root
        && stream[i+1].row[0].bestule == ULE_EXACT
        && stream[i+1].row[0].mvdone(itdexd2srd(i+1,0),
                                     stream[i].row[0].bestseq[0],
                                     stream[i+1].row[0].alpha,
                                     stream[i+1].row[0].beta))
        i++;   // (FIXME) assumes deepItd <= shallowItd+1

    int len = stream[i].row[0].bestseqLeng;
    mvC seq[GMX_MAX_PV];    // during rpySetpv (copying row by copyFrom)
    forr (k, 0, GMX_MAX_PV-1) {
        seq[k] = stream[i].row[0].bestseq[k];
    }
    MSTOut("@@@@@@@@ issueing nxt1 i %d dI %d sI %d tl %d ldI %d len %d val %d\n",
           i, deepItd, shallowItd, stream[shallowItd].tailExd, lastDoneItd,
           stream[i].row[0].bestseqLeng, stream[i].row[0].bestval);

    assert(len > 0);
    int haveList = rpySetpv(deepItd, len, seq);    // set up PV in stream[i]
    if (len > stream[deepItd].seqprevLeng - 1) {
        len = stream[deepItd].seqprevLeng - 1;
    }

    // issue NEXT1ST command
    cmd2send.setCmdFirst(deepItd, haveList, len, seq);
    cmd2send.send(1);  // FIXME PR1?

    inhFirst = 1;

    summary();
}

//********
// issue cmdCommit for finished rows if any
void planeC::catchupCommits()
{
    if (shallowItd == deepItd &&
        stream[shallowItd].tailExd == -1 &&
        stream[shallowItd].row[0].alpha == -score_bound) {
        return; // dont do anything at very first
    }

    forr (i, shallowItd, deepItd) {
        int e;
        int tail = stream[i].tailExd;
        if (tail == -1)
            continue;   // 12/3/2011 %32 was missing

        // first, see if betacut occured on any depth
        for (e = 0; e <= tail; e++) {
            if (stream[i].row[e].betacut()) break;
        }

        if (e <= tail) {  // betacut found
            cmd2send.setCmdCommit(i, e);
            forr(pr, 1, Nproc-1)
                cmd2send.send(pr);
            stream[i].tailExd = tail = e-1;
        }

        assert(e == tail+1);

        // now, check for completion from bottom(tail) up
        for (e = tail; e >= 0; e--) {
            if (!stream[i].row[e].nonfirstCompleted())
                break;    // make sure all non-first moves are done

            if (e == stream[i].seqprevLeng - 1 &&
                stream[i].row[e].bestval == -score_bound &&
                stream[i].row[e].gamma > stream[i].row[e].alpha)
                break;    // need to wait for FCOMP

            cmd2send.setCmdCommit(i, e);
            forr (pr, 1, Nproc-1) {
                cmd2send.send(pr);
            }

            stream[i].tailExd--;   // note: could be -1

            // set best*/firstmv at the upper row
            if (e > 0) {
                rowC& re  = stream[i].row[e];
                rowC& rup = stream[i].row[e-1];
                if (re.bestseqLeng > 0) {
                    rup.firstmv.bestmv = re.bestseq[0];
                }

                rup.gamma = -score_bound; // 12/5/2011 %38 was missing

                if (- re.bestval > rup.alpha) {
                    rup.bestval = rup.alpha = - re.bestval;
                    rup.bestule = (- re.bestval >= rup.beta) ? ULE_LOWER : ULE_EXACT;
                    // 12/2/2011 %27 min below was missing
                    rup.bestseqLeng = min(MAX_EXPDEP - 1, re.bestseqLeng + 1);
                    rup.bestseq[0] = rup.firstmv.mv;
                    forr(d, 0, re.bestseqLeng-1)
                        rup.bestseq[d+1] = re.bestseq[d];

                    MSTOut("|||| bestX upd by commit:i %d e %d val(A) %d ule %d len %d:\n",
                           i, e-1, rup.bestval, rup.bestule, rup.bestseqLeng);
                    forr(d, 0, rup.bestseqLeng-1)
                        MSTOut("%07x ", readable(rup.bestseq[d]));
                    MSTOut("\n");

                    // FIXME?  notify needed?
                }

                rup.setDonecntAll(); // 12/5/2011 %39 was missing

                // FIXME?  always exact?  say a sibling above updates A,
                //         Bcut here ... is it possible?
                rup.firstmv.upper =
                rup.firstmv.lower = re.bestval;
                rup.firstmv.depth = itdexd2srd(i, e-1);
                rup.firstmv.numnode =  // FIXME right?
                rup.firstmv.retrying =
                rup.firstmv.inherited = 0;
            }

            stream[i].summary();
        }

        // see if a stream is completed
        if (e == -1) {
            rowC& rup = stream[i].row[0];
            MSTOut("|||| bestX at commit0:lasti %d val(A) %d ule %d len %d:\n",
                   i, rup.bestval, rup.bestule, rup.bestseqLeng);
            forr(d, 0, rup.bestseqLeng-1)
                MSTOut("%07x ", readable(rup.bestseq[d]));
            MSTOut("\n");
            lastDoneItd = i;
            shallowItd++;
        }
    }
}

//********

extern planeC plane;

int streamC::itd()
{
    return (int)(this - &(plane.stream[0]));
}

int rowC::itd()
{
    char* here = (char*)this;
    int it = -1;

    assert(here >= (char*)&plane.stream[0]);
    forr (i, 1, MAX_ITD) {  // 11/29/2011 %12 was MAX_EXPDEP
        if (here < (char*)&plane.stream[i]) {
            it = i-1; break;
        }
    }
    assert(it != -1);
    return it;
}

int rowC::exd()
{
    return (int)(this - &(plane.stream[itd()].row[0]));
}

