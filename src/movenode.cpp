#include "precomp.h"
#include "stdinc.h"
#include "movenode.h"

namespace godwhale {

MoveNode::MoveNode(Move move)
    : m_move(move), m_depth(0), m_nodes(0)
    , m_upper(0), m_lower(0), m_retrying(false)
{
}

MoveNode::~MoveNode()
{
}

/**
 * @brief ノードの計算が完了したかどうかを取得します。
 */
bool MoveNode::isDone(int depth, int A, int B) const
{
    return (m_depth >= depth &&
            (m_upper == m_lower || m_upper <= -B || -A <= m_lower));
}

/**
 * @brief 評価値の種類がEXACTかどうかを取得します。
 */
bool MoveNode::isExact(int depth) const
{
    return (m_depth >= depth && m_upper == m_lower);
}

/**
 * @brief 評価値の更新を行います。
 */
void MoveNode::update(int depth, int value, ULEType ule, int nodes, Move bestMove)
{
    if (ule == ULE_EXACT) {
        m_upper = m_lower = value;
    }
    else if (ule == ULE_UPPER) {
        if (depth > m_depth) {
            m_upper = value;
            m_lower = -score_bound;
        }
        else {
            m_upper = value;
        }
    }
    else {
        assert(ule == ULE_LOWER);
        if (depth > m_depth) {
            m_upper = score_bound;
            m_lower = value;
        }
        else {
            m_lower = value;
        }
    }

    m_bestMove = (ule == ULE_EXACT || ule == ULE_LOWER ? bestMove : Move());
    m_nodes = (depth == m_depth ? m_nodes : 0) + nodes;
    m_depth  = depth;
    /*inherited =*/ m_retrying = false;

    /*MSDOut(">>>> ent_upd new: dep %d up %d lo %d bstmv %07x\n",
           m_depth, m_upper, m_lower, readable(bestmv));*/
}

/**
 * @brief 評価値の更新を行います。
 */
void MoveNode::update(int depth, int value, int lower, int upper,
                      int nodes, Move bestMove)
{
    if (value <= lower) {
        if (depth > m_depth) {
            m_upper = lower;
            m_lower = -score_bound;
        }
        else {
            m_upper = lower;
        }
    }
    else if (value >= upper) {
        if (depth > m_depth) {
            m_upper = score_bound;
            m_lower = value;
        }
        else {
            m_lower = value;
        }
    }
    else { // lower < value && value < upper
        m_upper = m_lower = value;
    }

    if (value > lower) m_bestMove = bestMove;
    m_nodes = nodes;
    m_depth = depth;
}


/////////////////////////////////////////////////////////////////////
MoveNodeList::MoveNodeList()
{
}

MoveNodeList::~MoveNodeList()
{
}

/**
 * @brief 与えられた指し手を持つノードのインデックスを取得します。
 */
int MoveNodeList::indexOf(Move move) const
{
    int size = m_nodeList.size();

    for (int i = 0; i < size; ++i) {
        if (m_nodeList[i].getMove() == move) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief 計算が完了していない指し手のインデックスを取得します。
 */
int MoveNodeList::indexOfUndone(int depth, int A, int B) const
{
    int size = m_nodeList.size();

    for (int i = 0; i < size; ++i) {
        if (!m_nodeList[i].isDone(depth, A, B)) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief 計算が完了しているノードの数を取得します。
 */
int MoveNodeList::getDoneCount(int depth, int A, int B) const
{
    int size = m_nodeList.size();
    int count = 0;

    for (int i = 0; i < size; ++i) {
        if (m_nodeList[i].isDone(depth, A, B)) {
            ++count;
        }
    }

    return count;
}
    
} // namespace godwhale
