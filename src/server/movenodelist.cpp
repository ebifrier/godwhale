#include "precomp.h"
#include "stdinc.h"
#include "movenodelist.h"

namespace godwhale {
namespace server {

MoveNodeList::MoveNodeList()
{
    // ちょっとした高速化
    m_nodeList.reserve(200);
}

MoveNodeList::~MoveNodeList()
{
}

/**
 * @brief 設定された指し手一覧をすべてクリアします。
 */
void MoveNodeList::clear()
{
    m_nodeList.clear();
}

/**
 * @brief 新たな候補手を一手設定します。
 */
void MoveNodeList::addNewMove(Move move)
{
    m_nodeList.push_back(MoveNode(move));
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
   
} // namespace server
} // namespace godwhale
