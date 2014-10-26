#include "precomp.h"
#include "stdinc.h"
#include "syncposition.h"

namespace godwhale {

shared_ptr<SyncPosition> SyncPosition::ms_instance(new SyncPosition);

SyncPosition::SyncPosition()
    : m_moveSize(0)
{
    memset(m_moveList, 0, sizeof(m_moveList));
    memset(m_checksList, 0, sizeof(m_checksList));
}

/**
 * @brief 局面をルート局面に戻し、bonaの初期化を行います。
 */
void SyncPosition::initialize()
{
    tree_t * restrict ptree = g_ptree;

    rewind();

    // 初期局面の王手チェック
    bool checked = (InCheck(root_turn) != 0);
    m_checksList[0] = checked; // インデックスは0で開始

    ptree->nsuc_check[0] = 0;
    ptree->nsuc_check[1] = checked ? 1 : 0;
}

void SyncPosition::initialize(min_posi_t const & posi)
{
}

/**
 * @brief ルートの指し手を１手進めます。
 */
void SyncPosition::makeRootMove(Move move)
{
    tree_t * restrict ptree = g_ptree;

    rewind();

    make_move_root(ptree, move, flag_time);
    initialize();
}

/**
 * @brief 指し手を１手進めます。
 */
void SyncPosition::makeMove(Move move)
{
    tree_t * restrict ptree = g_ptree;
    int i = m_moveSize;
    int turn = countFlip(root_turn, i);

    // 指し手の確認など
    assert(is_move_valid(ptree, move.get(), turn));
    m_moveList[i] = move;
    MakeMove(turn, move.get(), i+1);
    assert(!InCheck(turn));

    // 王手されているかチェック
    bool checked = (InCheck(Flip(turn)) != 0);
    m_checksList[i+1] = checked;
    ptree->nsuc_check[i+2] = checked ? ptree->nsuc_check[i]+1 : 0;

    m_moveSize += 1;
}

/**
 * @brief 指し手を１手戻します。
 */
void SyncPosition::unmakeMove()
{
    tree_t * restrict ptree = g_ptree;
    int i = m_moveSize - 1;
    int turn = countFlip(root_turn, i);

    assert(m_moveSize > 0);

    UnMakeMove(turn, m_moveList[i].get(), i+1);
    m_checksList[i+1] = false;

    m_moveSize -= 1;
}

/**
 * @brief bonanzaの局面を元に戻します。
 */
void SyncPosition::rewind()
{
    while (m_moveSize > 0) {
        unmakeMove();
    }
}

/**
 * @brief ルート局面から指し手を設定しなおします。
 */
void SyncPosition::resetFromRoot(const std::vector<Move> & pv)
{
    tree_t * restrict ptree = g_ptree;
    int len = pv.size();

    // 今と同じ局面の場合は、何もしません。
    if (pv.size() == m_moveSize &&
        std::equal(pv.begin(), pv.end(), &m_moveList[0])) {
        return;
    }

    // とりあえず局面を戻します。
    rewind();

    BOOST_FOREACH(auto move, pv) {
        makeMove(move);
    }
}

void SyncPosition::extendPV()
{
}

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

/**
 * @brief ある局面で着手可能な指し手をリストアップします。
 */
void SyncPosition::getMoveList(Move exclude, bool firstMoveOnly,
                               std::vector<Move> * result)
{
    tree_t * restrict ptree = g_ptree; 
    
    int ply = m_moveSize + 1;
    int turn = countFlip(root_turn, ply-1);
    int i = 0;

    ptree->move_last[ply  ] =
    ptree->move_last[ply-1] = ptree->amove;
    ptree->anext_move[ply].next_phase = next_move_hash;

    if (ply == 1) {
        for (int i = 0; i < root_nmove; ++i) {
            root_move_list[i].status = 0;
        }
    }
 
    while (ply == 1 ? next_root_move(ptree) :
           ptree->nsuc_check[ply] ?
               gen_next_evasion(ptree, ply, turn) :
               gen_next_move(ptree, ply, turn)) {
        assert(is_move_valid(g_ptree, MOVE_CURR, turn));

        if (MOVE_CURR == exclude) continue;
        result->push_back(MOVE_CURR);
        //if (firstMoveOnly && result.size() >= 2) break;
    }
}

} // namespace godwhale
