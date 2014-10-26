#include "precomp.h"
#include "stdinc.h"
#include "commandpacket.h"
#include "syncposition.h"
#include "client.h"

namespace godwhale {
namespace client {

/**
 * @brief 受信した各コマンドの処理を行います。
 */
int Client::mainloop()
{
    while (true) {
        proce(false /* searching */);

        SearchTask stask; //= getNextSearchTask();

        // 仕事がない場合は
        if (stask.isIdle()) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(50));
            continue;
        }

        // 探索タスクの実行
        LOG_DEBUG() << F("next search task: itd=%1%, pld=%2%, move=%3%")
            % stask.iterationDepth % stask.plyDepth % stask.move;
    }

    return 0;
}

static int getDepthReduced(tree_t * restrict ptree, Move move, int ply, int turn)
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

} // namespace client
} // namespace godwhale
