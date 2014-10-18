#include "precomp.h"
#include "stdinc.h"
#include "movenode.h"
#include "csearch.h"

namespace godwhale {
namespace client {

static int getLMR(tree_t * restrict ptree, Move move, int ply, int turn)
{
    int depth_reduced = 0;

    if (!(move.isDrop() || (move.isPromote() && move.getPiece() != silver)) &&
        move.get() != (move_t)ptree->amove_hash[ply]  &&
        move.get() != (move_t)ptree->killers[ply].no1 &&
        move.get() != (move_t)ptree->killers[ply].no2 ) {
        unsigned int key     = phash(move.get(), turn);
        unsigned int good    = (ptree->hist_good[key]  + 1);
        unsigned int triedx8 = (ptree->hist_tried[key] + 2) * 8U;

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
    }
    
    return depth_reduced;
}

// if LIST, call search(). if val>A, do PVS
//           then set pvsretrying. send reply also
//
// 指し手の探索を行い、よい評価であれば
// pvsretryingフラグを立てPV情報を送信します。
static void doList(tree_t * restrict ptree, SearchData srchJob)
{
    int64_t preNodes, postNodes;
    unsigned int state_node_dmy;
    int itd = srchJob.itd;
    int exd = srchJob.exd;
    int top = srchJob.mvsuf;  // 11/29/2011 %13 not used; was using mvtop

    iteration_depth = itd;

    sstreamC& sstream = splane.stream[itd];
    srowC& srow = splane.stream[itd].row[exd];

    // bring to cur pos - (FIXME) dont do this every time!
    bringPosTo(exd, &sstream.seqFromRoot[0], true/*setchk*/);

    Move move;
    MoveNode node(move);
    int turn= countFlip(root_turn, exd);
    int ply = curPosPathLeng + 1;
    int A = srow.alpha;
    int B = srow.beta;

    /*if (USE_SLV_HISTORY && srow.registKillerMv != NULLMV &&
        is_move_valid(g_ptree, srow.registKillerMv.v, turn)) {
        histGoodPv(g_ptree, srow.registKillerMv.v, ply+1,
                   (itd-ply)*PLY_INC, turn); //dep need not be exact
        srow.registKillerMv = NULLMV;
    }*/

    // 探索の前に手を進めます。
    MOVE_CURR = move.get();
    MakeMove(turn, move.get(), ply);

	// 手番側の王に王手がかかっていたら、
	// その局面は不正のため探索を行いません。
    if (InCheck(turn)) {
        //running.val = score_bound;
        UnMakeMove(turn, move.get(), ply);

        //node.update(DECISIVE_DEPTH, score_bound, score_bound);
        //rpyent.pushPvs(itd, exd, score_bound, move.get(), ULE_EXACT,
        //               0 /*numnode*/, 0 /*bestseqLeng */, NULL /*bestseq */);
        return;
    }

    bool chk = (InCheck(Flip(turn)) != 0);
    ptree->nsuc_check[ply+1] = chk ? ptree->nsuc_check[ply-1] + 1 : 0;

    int depth = 10/*itdexd2srd(itd, exd)*/ * PLY_INC / 2 - PLY_INC;
    int depthReduced = (!chk && ply>1 ? getLMR(ptree, move, ply, turn) : 0);
    depth -= depthReduced;

    // FIXME? need move_last[], stand_pat<_full>[], etc? how about root_A/B?
    ptree->save_eval[ply  ] =
    ptree->save_eval[ply+1] = INT_MAX;
    ptree->move_last[ply  ] = ptree->amove;

    // store bestmv to hash
    Move bestMove = node.getBestMove();
    if (!bestMove.isEmpty()) {
        if (!is_move_valid(ptree, bestMove.get(), Flip(turn))) {
            if (DBG_SL && !bestMove.isEmpty()) {
                /*SLDOut("!!!!!!!! pvs: writing invalid mv to hash  mv=%07x\n",
                       readable(smvent.bestmv));
                out_board(ptree, slavelogfp, 0, 0);*/
                //assert(0);  FIXMEEEE!!!! should not occur!? investigate
            }
        }
        else {
            if (!chk) {
                // checked case, somehow a non-evasion is stored.
                hash_store_pv(ptree, bestMove.get(), Flip(turn));
                //SLDOut("<<<< hashmv %07x stored\n", readable(smvent.bestmv));
            }
        }
    }

    //sprmv.mvs[top].perfIncTrycnt();
    preNodes = ptree->node_searched;

    // Null Move Windowによる枝刈りを行います。
    LOG_DEBUG() << F("---- client search A=%1% depth=%2%") % A % depth;
    //perfRecMove.startSrch();
    int value = -search(ptree, -A-1, -A, Flip(turn), depth, ply+1, state_node);
    //perfRecMove.endSrchList(ply /*exd*/, sprmv.mvs[top].perfTrycnt++);
    LOG_DEBUG() << F("---- done abort=%1% value=%2%") % root_abort % value;

    if (root_abort) {
        UnMakeMove(turn, move.get(), ply)
        rewindToRoot();
        return;
    }

    // もし評価値がalpha値を超えていれば、更新の可能性があります。
    if (value > A) {
        // send retry-reply
        node.setRetrying();
        //rpyent.pushRetrying(itd, exd, move.get());
        //rpypkt.send();

        //running.retrying = 1;
        if (srow.alpha > A) A = srow.alpha;
        if (srow.beta < B)  B = srow.beta;
        root_beta = -A;
        depth += depthReduced + PLY_INC/2; // 減った分を元に戻します。

        LOG_DEBUG() << F("---- client search2 A=%1% depth=%2%") % A % depth;
        //perfRecMove.startSrch();
        value = -search(ptree, -B, -A, Flip(turn), depth, ply+1, state_node);
        //perfRecMove.endSrchRetry(ply+1 /*exd*/);
        LOG_DEBUG() << F("---- done abort=%1% value=%2%") % root_abort % value;

        if (root_abort) {
            UnMakeMove(turn, move.get(), ply);
            rewindToRoot();
            return;
        }

        root_beta = score_bound;
        //running.retrying = 0;

        if (value > std::max(A, srow.alpha)) {
            srow.alpha = value;
            srow.updated = 1;
            bool lower = (value >= B);

            // use hashmv if leng insufficient
            if (!lower && ptree->pv[ply].length < ply+2) {
                int hashmv;
                if (ptree->pv[ply].length < ply+1) {
                    hash_probe(ptree, ply+1, depth, Flip(turn),
                                -score_bound, score_bound, &state_node_dmy);
                    hashmv = ptree->amove_hash[ply+1];
                }
                else {
                    hashmv = ptree->pv[ply].a[ply+1];
                }

                // add it to pv if it's valid mv
                if (hashmv != MOVE_NA && hashmv != MOVE_PASS &&
                    is_move_valid(ptree, hashmv, Flip(turn))) {
                    MakeMove(Flip(turn), hashmv, ply+1);
                    if (!InCheck(Flip(turn))) {
                        ptree->pv[ply].length = ply+1;
                        ptree->pv[ply].a[ply+1] = hashmv;
                    }
                    UnMakeMove(Flip(turn), hashmv, ply+1);
                }
            }

            // save bestmv/val
            srow.bestseqLeng = lower ? 0 :
                std::min(GMX_MAX_BESTSEQ-1, ptree->pv[ply].length-ply-2);
            for(int i = 0; i < srow.bestseqLeng; ++i) {
                srow.bestseq[i] = ptree->pv[ply].a[ply+i+1];
            }
        }
    }

    postNodes = ptree->node_searched;

    //srch writes to [ply], not[ply-1] when Bcut
    if (!root_abort && value <= A) {
        hash_probe(ptree, ply+1, depth, Flip(turn), -A-1, -A, &state_node_dmy);
        int hashmv = ptree->amove_hash[ply+1];
        bool usemv = (hashmv != MOVE_NA && hashmv != MOVE_PASS &&
                      is_move_valid(ptree, hashmv, Flip(turn)));
        srow.bestseq[0].v = usemv ? hashmv : MOVE_NA;
        srow.bestseqLeng = usemv ? 1 : 0;
    }

    // UnMakeMove by a move at top
    UnMakeMove(turn, move.get(), ply);

    //assert(running.job.onList());
    {
        //if not (cur srch invalid due to Alpha change),
        // cancel @ NOTIFY.  if pvsretry & fail lo, will come here
        ULEType ule = (B <= value) ? ULE_UPPER :
                      (running.alpha < value) ? ULE_EXACT :
                      ULE_LOWER;

        uint64_t nodes = postNodes - preNodes;
        node.update(0/*itdexd2srd(itd, exd)*/, -value, -B, -A, nodes,
                    srow.bestseq[0]);
       
        //create reply: pvs(mv,val,bestseq[]) seq[]is: pv[] if upd, else 1mv
        /*rpyent.pushPvs(itd, exd, -value,
                        node.getMove(), ule, nodes,
                        srow.bestseqLeng, srow.bestseq);*/
    }
}

} // namespace client
} // namespace godwhale
