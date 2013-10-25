/* $Id: initact.h,v 1.15 2012-04-04 22:34:31 eiki Exp $ */

/**
 * called at the beginning of master().
 * determine which to do first, ROOT/FIRST/LIST
 */

//******** extshrRecordC ********

struct extshrRecordUnitC
{
    int call, mvs;

    extshrRecordUnitC() {}
    void clr() { memset(this, 0, sizeof(timeRecordUnitC)); }
    void regist(int nmv) { mvs += nmv; call++; }
};


struct perfMsC
{
    int perfUsefirst, perfUselist, perfUseroot;
    extshrRecordUnitC extshrRec[MAXPROC];

    perfMsC() {}
    void clear() { memset(this, 0, sizeof(perfMsC)); }
    void print() {
        MSTOut("perfMs: root %d first %d list %d\n",
               perfUseroot, perfUsefirst, perfUselist);
        MSTOut("extend/shrink: (call/nmove)");
        forr (pr, 1, Nproc-1) {
            MSTOut("%d/%d ", extshrRec[pr].call, extshrRec[pr].mvs);
        }
        MSTOut("\n");
    }

    void incRoot() { perfUseroot++; }
    void incFirst() { perfUsefirst++; }
    void incList() { perfUselist++; }
    void incExtshr(int pr, int nmv) { extshrRec[pr].regist(nmv); }
};

perfMsC pfGame;
extern int inhFirst;

void planeC::initialAction() {
    int i;
    int usefirst, uselist;
    int firstseqLeng;
    mvC firstseq[GMX_MAX_PV];
    usefirst = -1;
    uselist = -1;
    waitRoot = 0;

    lastDoneItd = shallowItd = deepItd = 0;

    // find the deepest ITD that's completed
#define MIN_ITD 5   // FIXME define elsewhere

    for (i=MAX_ITD-1; i>=MIN_ITD; i--) {
        if (stream[i].seqFromPrev[0] == mv2root &&
            stream[i].row[0].rowdone()) {
            break;
        }
    }
    
    if (i >= MIN_ITD) {
        lastDoneItd = i;
    }

    MSTOut("== iact: rdone passed i=%d\n", i);

    // if str[i]'s bestmv is done in i+1, use firstseq of i+1
    if (i >= MIN_ITD &&
        mv2root == stream[i+1].seqFromPrev[0] &&
        stream[i+1].row[0].bestval > -score_max_eval &&
        stream[i+1].row[0].bestule == ULE_EXACT   &&
        stream[i+1].row[0].bestseqLeng > 2) {
        assert(stream[i+1].row[0].bestseq[0] != NULLMV);

        if (stream[i+1].row[0].mvdone(itdexd2srd(i+1,0),
                                      stream[i].row[0].bestseq[0],
                                      stream[i+1].row[0].alpha,
                                      stream[i+1].row[0].beta)) {
            i++;
            MSTOut("== iact: str[last+1] to be used\n");
        }
    }

    if (i >= MIN_ITD) {
        firstseqLeng = stream[i].row[0].bestseqLeng;
        forr(k, 0, firstseqLeng-1)
            firstseq[k] = stream[i].row[0].bestseq[k];
        MSTOut("== iact: firstseqLeng=%d\n", firstseqLeng);
    }

    do {  // dummy loop to allow cont/brk
        if (i < MIN_ITD) break; // ROOT

        shallowItd = deepItd = lastDoneItd+1;
        streamC& st = stream[shallowItd];
        usefirst = firstseqLeng;

        if (mv2root != st.seqFromPrev[0]) {
            // str[shal] not useable.  use FIRST w/ lastItd
            //usefirst = lastleng;
            break;
        }

        forr (e, 0, st.seqprevLeng-2) {
            // check if last Itd's bestseq and shallow's seqPrev matches.
            // if not, use FIRST

            if (e < firstseqLeng && e>0 &&
                st.seqFromPrev[e] != firstseq[e-1]) {
                MSTOut(")))) initact seq mismch: e %d\n", e);
                break;
            }

            rowC& r = st.row[e];
            if (r.procmvs[1].mvcnt < 0) { // not mvgened - stream unuseable, use FIRST
                //usefirst = lastleng;
                MSTOut(")))) initact mv nogen  : e %d\n", e);
                break;
            }

            if (r.bestule != ULE_EXACT) {
                MSTOut(")))) initact result not exact : e %d\n", e);
                break;
            }

            if (r.bestval == -score_bound)
                continue;  // firstmv not done yet, go one EXD deep

            // now bestval > -INF

            // bring bestmv to first if needed
            assert(r.bestseq[0] != NULLMV);
            if (r.bestseq[0] != r.firstmv.mv) {
                int pr = -1, suf = -1;
                r.findmv(r.bestseq[0], &pr, &suf);
                assert(pr != -1 && suf != -1);
                mvEntryC& me = r.procmvs[pr].mvs[suf];
                if (me.depth >= itdexd2srd(shallowItd, e) &&
                    me.upper == -r.bestval &&
                    me.upper == me.lower) {
                    r.replaceFirst(r.bestseq[0]);
                    MSTOut(")))) initact repl1st   : e %d mv %07x\n",
                           e, readable(r.bestseq[0]));
                }
            }

            // the fact that bestval>-INF must indicate bestmv (now firstmv) has
            // been srched and gives an exact value  (FIXME true?)

            if (!(r.firstmv.depth >= itdexd2srd(shallowItd, e) &&
                  r.firstmv.upper == -r.bestval &&
                  r.firstmv.upper == r.firstmv.lower)) {
                //usefirst = lastleng;
                MSTOut(")))) initact 1st no val: e %d\n", e);
                break;
            }

            // now some completed mv is found.  use it as the FIRST result,
            // start srch from LIST

            MSTOut(")))) initact use list  : e %d\n", e);
            uselist = e;
            usefirst = -1;
            break;
        }
    } while(0);

    // now we know whether the initial action should be ROOT, FIRST or LIST.
    // do whatever is needed

    if (usefirst < 0 && uselist < 0) { // ROOT
        cmd2send.setCmdRoot();
        cmd2send.send(PR1);   // FIXME primitive version for now.  tune later

        cmd2send.setCmdPicked(); // send Root to all proc, pick one w/ deepest result
        cmd2send.send(PR1);
        inhFirst = 1;
        waitRoot = 1;
        MSTOut(":::: useRoot\n");
        pfGame.incRoot();
    }
    else if (usefirst >= 0) {
        assert(usefirst > 0);

        int havelist = rpySetpv(shallowItd, firstseqLeng, firstseq );

        if (firstseqLeng > stream[shallowItd].seqprevLeng - 1)
            firstseqLeng = stream[shallowItd].seqprevLeng - 1;

        cmd2send.setCmdFirst(shallowItd, havelist, firstseqLeng, firstseq );
        cmd2send.send(1);  // FIXME PR1?
        inhFirst = 1;
        MSTOut(":::: useFirst haveList %d\n", havelist);
        pfGame.incFirst();
    }
    else {
        // set up tables, issue SETLIST.  assumes mvlists are already there

        streamC& st = stream[shallowItd];
        int val = st.row[uselist].bestval;
        st.tailExd = uselist;
        st.seqprevLeng = uselist + 1;
        forr(e, uselist+1, MAX_EXPDEP-1) {
            st.row[e].clear();
        }

        forv (e, uselist, 0) {
            rowC& r = st.row[e];
            r.beta = score_bound;
            if (e == uselist) {
                r.gamma = -score_bound;
                r.alpha = val;
                assert(-r.bestval == r.firstmv.lower &&
                       -r.bestval == r.firstmv.upper &&
                       r.bestule == ULE_EXACT         );
            }
            else {
                r.alpha = -score_bound;
                r.gamma = val;
                //assert(-score_bound == r.firstmv.lower &&
                //       - r.bestval == r.firstmv.lower   );
                r.bestval = -score_bound;
                r.bestseqLeng = 0;
                r.bestule = ULE_NA;
            }

            assert(r.procmvs[1].mvcnt >= 0);
            forr (pr, 1, Nproc-1) {
                mvtupleC tuples[600];
                procmvsC& prm = r.procmvs[pr];
                forr (j, 0, prm.mvcnt-1) {
                    tuples[j].mv = prm.mvs[j].mv;
                    // FIXME?  line below copied from serupReorder().  should not copy?
                    tuples[j].bestmv = prm.mvs[j].inherited
                        ? prm.mvs[j].bestmv
                        : NULLMV;
                    tuples[j].depth = prm.mvs[j].depth;
                    tuples[j].upper = prm.mvs[j].upper;
                    tuples[j].lower = prm.mvs[j].lower;

                    prm.mvs[j].numnode =    // FIXME? numnode?
                    prm.mvs[j].inherited =
                    prm.mvs[j].retrying = 0;
                }

                prm.donecnt = 0;

                cmd2send.setCmdList(shallowItd, e, r.procmvs[pr].mvcnt,
                                    val, r.beta, r.firstmv.mv, e,
                                    st.seqFromPrev+1, tuples);
                cmd2send.send(pr);
            }

            val = -val;
        }

        inhFirst = 0;
        MSTOut(":::: useList e %d\n", uselist);
        pfGame.incList();
    }
}
