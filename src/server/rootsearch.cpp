#include "precomp.h"
#include "stdinc.h"
#include "syncposition.h"
#include "rootsearch.h"

const int INITIAL_SEARCH_VALUE = (-score_bound-1);

const int ROOT_SEARCH_DEPTH_MIN = 4;
const int ROOT_SEARCH_DEPTH_MAX = 32;

const int MAX_ROOT_NODES  = 40000;
const int MAX_FIRST_NODES = 40000;

namespace godwhale {
namespace server {

static uint64_t s_preNodeCount = 0;

static bool s_isRootCancelable = false;
static bool s_isRootExceeded = false;

static bool s_isFirstCancelable = false;

int detectSignalServer()
{
    if (s_isRootCancelable &&
        g_ptree->node_searched >= s_preNodeCount + MAX_ROOT_NODES) {
        s_isRootExceeded = 1;
        return 1;
    }

    /*if (inFirst && !firstReplied &&
        preNodeCount + MAX_FIRST_NODES <= g_ptree->node_searched) {
        firstReplied = 1;
        replyFirst();
    }*/

    return 0; //proce(1/*nested*/);
}

SearchData::SearchData()
{
}

void SearchData::reset(int iterationDepth, int value)
{
    int len = g_ptree->pv[0].length;

    // PV情報を保存します。
    m_pv.resize(len);
    for (int i = 0; i < len; ++i) {
        m_pv[i] = g_ptree->pv[0].a[i+1];
    }

    // 評価値などを保存します。
    m_iterationDepth = iterationDepth;
    m_ule = ULE_EXACT;
    m_value = value;

    assert(m_pv.size() >= 1 || m_value < -score_max_eval);

    // ログ出力
    std::list<std::string> list;
    int turn = root_turn;
    for (int i = 0; i < len; ++i) {
        list.push_back(m_pv[i].str(turn));
        turn = Flip(turn);
    }
    LOG_NOTIFICATION() << "length=" << len << " pv=" << boost::join(list, "");
}

/**
 * @brief 探索のための初期化処理を行います。
 */
static void initRootSearch(tree_t * restrict ptree)
{
    ptree->node_searched         =  0;
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
    ptree->save_eval[0]          =  INT_MAX;
    ptree->save_eval[1]          =  INT_MAX;
    ptree->pv[0].a[0]            =  0;
    ptree->pv[0].a[1]            =  0;
    ptree->pv[0].depth           =  0;
    ptree->pv[0].length          =  0;
    ptree->nsuc_check[0]         =  0;
    ptree->nsuc_check[1]         =  InCheck(root_turn) ? 1 : 0;
    ptree->move_last[0]          =  ptree->amove;
    ptree->move_last[1]          =  ptree->amove;
    iteration_depth              =  0;
    easy_value                   =  0;
    easy_abs                     =  0;
    root_abort                   =  0;
    root_nmove                   =  0;
    root_value                   = -score_bound;
    root_alpha                   = -score_bound;
    root_beta                    =  score_bound;
    root_index                   =  0;
    root_move_list[0].status     = flag_first;
    node_last_check              =  0;
    time_last_check              =  time_start;
    game_status                 &= ~( flag_move_now | flag_suspend
                                    | flag_quit_ponder | flag_search_error );

#if defined(TLP)
    ptree->tlp_abort             = 0;
    tlp_nsplit                   = 0;
    tlp_nabort                   = 0;
    tlp_nslot                    = 0;
#endif

    for (int ply = 0; ply < PLY_MAX; ply++) {
        ptree->amove_killer[ply].no1 =
        ptree->amove_killer[ply].no2 = 0U;
        ptree->killers[ply].no1 =
        ptree->killers[ply].no2 = 0x0U;
    }
}

/**
 * @brief ルートムーブをソートします。
 */
static void sortRootMoveList(tree_t * restrict ptree)
{
    root_move_t root_move_swap;
    int n = root_nmove;
    uint64_t sortv;
    int i, j, k, h;
      
    for ( k = SHELL_H_LEN - 1; k >= 0; k-- ) {
        h = ashell_h[k];
        for ( i = n-h-1; i > 0; i-- ) {
            root_move_swap = root_move_list[i];
            sortv          = root_move_list[i].nodes;
            for ( j = i+h; j < n && root_move_list[j].nodes > sortv; j += h ) {
                root_move_list[j-h] = root_move_list[j];
            }
            root_move_list[j-h] = root_move_swap;
        }
    }
}

/**
 * @brief ルートからある程度の深さまで探索し、PVを取得します。
 */
int generateRootMove(SearchData * result)
{
    tree_t * restrict ptree = g_ptree;

    SyncPosition::get()->rewind();
    s_isRootCancelable = false;
    s_isRootExceeded = false;

    if (InCheck(Flip(root_turn))) {
        LOG_NOTIFICATION() << "王手放置の局面が渡されました。";
        return -1;
    }
    
    LOG_NOTIFICATION() << "begom generate root moves";
    int value = make_root_move_list(ptree);

    // 次の手がない場合は対局を終了させます。
    if (root_nmove == 0) {
        LOG_NOTIFICATION() << "root_moveが生成できませんでした。対局を終了します。";
        return 0;
    }

    // マルチスレッド探索を行う場合
#if defined(TLP)
    int iret = tlp_start();
    if (iret < 0) { return iret; }
#endif

    // iterative deepening search
    int iterationDepth;
    for (iterationDepth = ROOT_SEARCH_DEPTH_MIN;
         iterationDepth < ROOT_SEARCH_DEPTH_MAX;
         ++iterationDepth) {
        s_preNodeCount = ptree->node_searched;

        LOG_NOTIFICATION() << F("begin root search depth=%1% ...") % iterationDepth;
        int value = search(ptree, -score_bound, score_bound, root_turn,
                           iterationDepth*PLY_INC + PLY_INC/2, 1/*ply*/, state_node);
        LOG_NOTIFICATION() << F("... done abort=%1% value=%2% node=%3%")
            % root_abort % value % (g_ptree->node_searched - s_preNodeCount);

        if (root_abort) {
            bool exceeded = s_isRootExceeded;
            s_isRootCancelable = false;
            s_isRootExceeded = false;

            // ノード数制限にかかった場合は、最左ノードの探索に入ります。
            // そうでない場合は通常の終了なので、思考を終了させます。
            return (exceeded ? 1 : -1);
        }

        sortRootMoveList(ptree);

        // bonanza側から探索結果を取り出します。
        result->reset(iterationDepth, value);

        // 詰まされたor詰ました場合
        if (abs(value) > score_max_eval) {
            return 0;
        }
        
        // 詰みでも詰まされてもいない場合
        s_isRootCancelable = true;
    }

  /*{
    int i;
    
    for ( i = 0; i < (int)HIST_SIZE; i++ )
      {
#ifndef SHARED_HIST
                  ptree->hist_good[i]  /= 256U;
                  ptree->hist_tried[i] /= 256U;
#else
                  hist_goodary[i]  /= 256U;
                  hist_tried[i] /= 256U;
#endif
      }
  }*/

    return 0;
}

/**
 * @brief 最左ノードを求めます。
 *
 * 与えられたPVを末端から探索しつつ、最左ノードを更新していきます。
 * 探索打ち切りはノード数が一定値を超えた場合に発生します。
 */
static void doFirst2(const SearchData & data)
{
    tree_t * restrict ptree = g_ptree;
    int itd = data.getIterationDepth();
    int pvSize = data.getPVSize();

    LOG_NOTIFICATION() << "begin first";

    s_isFirstCancelable = 0;

    // 内部局面を初期化します。
    SyncPosition::get()->initialize();

    // 指定のPV通りに指し手を進め、ハッシュなどのチェックを行います。
    for (int i = 0; i < pvSize; i++) {
        /*if ((singleCmd.haveList & (1 << i)) == 0) {
            // 
            if (i == 0 && root_nmove == -1) {
                makeRootMoveList();
                if (root_nmove <= 1) {
                    doRoot();   // will take care rt_nmv=0/1 cases
                    return;
                }
            }

            // 今の局面で指せる手の一覧を取得します。
            //firstMoves[i] = SyncPosition::getRootMoves(pv[i], false);
            //setMoves(&firstMvcnt[i], firstMoves[i], pv[i], false);
        }*/

        /*if (DBG_DUMP_BOARD) {
            SLDOut(",,,, mvgen for setlist ply %d curPosPathLeng %d\n",
                    i, curPosPathLeng);
            out_board(g_ptree, slavelogfp, 0,0);
        }*/

        // make a move
        SyncPosition::get()->makeMove(data.getPV(i));
    }

    // 最善手が見つかった場合はHVLST_MVONLYフラグが立ちます。
    // この場合は、すぐにマスタに返信してから処理を終了します。
    /*if (singleCmd.haveList & HVLST_MVONLY) {
        singleCmd.exd = leng;
        singleCmd.val = 0; // dummy
        assert(!singleCmd.fromRoot);

        //replyFirst();
        SyncPosition::get()->rewind();
        return;
    }*/


    //**** loop w/ expdep going up
    int ply;
    int A = -score_bound;
    int B =  score_bound;
    int turn = countFlip(root_turn, pvSize);

    //inFirst = firstReplied = 0;
    firstReplied = 0;
    uint64_t pren = g_ptree->node_searched;

    // NOTE singlCmd.exd set is needed only after one srch is done (not here)
    // 12/11/2010 #5 was <=1
    for (ply = pvSize; ply >= 1; ply--) {
        if ((unsigned)(pren + MAX_FIRST_NODES) < g_ptree->node_searched)
            break;

        // 着手可能な指し手の数
        int n = (ply == pvSize) ? 2 : firstMvcnt[ply] + 1;

        //**** register hash (if any)  FIXME? store value, not store_pv

        /*if (ply <= pvSize - 1) {
            if (DBG_SL && !data.getPV(ply).isEmpty() &&
                !is_move_valid(ptree, data.getPV(ply).get(), turn)) {
                SLDOut("!!!!!!!! first: writing invalid mv to hash  mv=%07x\n",
                       readable(singleCmd.pv[ply]));
                out_board(ptree, slavelogfp, 0, 0);
                assert(0);
            }
            hash_store_pv(ptree, data.getPV(ply).get(), turn);
        }*/

        if (firstReplied) {
            //firstReplied = 0;
            break;
        }

        if (n == 1 && ply != pvSize) {
            assert(ply>0);
            for (int i=singleCmd.bestseqLeng-1; i>=0; i--) {
                singleCmd.bestseq[i+1] = singleCmd.bestseq[i];
            }
            singleCmd.bestseq[0] = singleCmd.pv[ply-1];
            singleCmd.bestseqLeng++;
            singleCmd.exd--;
            singleCmd.val = -singleCmd.val;
            //running.val = -running.val;

            SyncPosition::get()->unmakeMove();
            continue;
        }

        //**** now perform search

        g_ptree->move_last[ply] = g_ptree->amove;
        g_ptree->save_eval[ply  ] =
        g_ptree->save_eval[ply+1] = INT_MAX;

        // srch each exd.  there s/b >=2 mvs here
        int depth = (itd + ply) * PLY_INC / 2;
        MOVE_LAST = singleCmd.pv[ply-1].v;

        SLTOut("-------- FIRST srch dep=%d ply=%d...", depth, ply);

        iteration_depth = itd;
        int value = -search(g_ptree, A, B, turn, newdep, ply+1, state_node);
            
        SLTOut(" ..done abt=%d val=%d len=%d #nod=%d mvs %07x %07x..\n",
               root_abort, value, g_ptree->pv[ply].length,
               g_ptree->node_searched - preNodeCount,
               readable_c(g_ptree->pv[ply].a[ply+1]),
               readable_c(g_ptree->pv[ply].a[ply+2]));

        if (root_abort) {
            SyncPosition::get()->rewind();
            s_isFirstCancelable = 0;
            return;
        }

        s_isFirstCancelable = true;

        //**** extend pv by using hashmv if leng insufficient

        /*if (ply == 1 && g_ptree->pv[ply].length < ply+1) {
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
        }*/

        // if srch stops at ply, exd will be ply-1. bestseq[0] is pv[ply-1]
        int newleng = ptree->pv[ply].length - ply;
        if (newleng == 1 &&
            singleCmd.pvleng > ply &&
            singleCmd.bestseqLeng > 0 &&
            singleCmd.bestseq[0].v == (int)ptree->pv[ply].a[ply+1]) {
            newleng = singleCmd.bestseqLeng;
            for (int i = newleng-1; i>=0; i--) {
                singleCmd.bestseq[i+1] = singleCmd.bestseq[i];
            }
        }
        else {
            // 12/15/2010 #41 length*-ply* was missing
            forr (i, 0, newleng-1) {  // FIXME? ply+1?
                singleCmd.bestseq[i+1] = ptree->pv[ply].a[ply+i+1];
            }
        }

        singleCmd.val = running.val = val; // FIXME? running.val not needed?
        singleCmd.exd = ply-1;
        singleCmd.bestseq[0] = singleCmd.pv[ply-1];
        singleCmd.bestseqLeng = newleng+1;
        // FIXME follow hash in order to extend leng?

        // 12/11/2010 #9 was turn/ply/ply+1
        SyncPosition::get()->unmakeMove();
        val = -val;
    }

    inFirst = 0;

    //singleCmd.exd = ply;   // 12/11/2010 #12 was ply--
    //singleCmd.val = running.val;
    singleCmd.ule = ULE_EXACT;

    SyncPosition::get()->rewind();

    if (!firstReplied) {
        singleCmd.exd++;
        singleCmd.val = -singleCmd.val;
        //replyFirst();
    }
}

} // namespace godwhale
} // namespace server
