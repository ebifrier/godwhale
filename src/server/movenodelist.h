#ifndef GODWHALE_SERVER_MOVENODELIST_H
#define GODWHALE_SERVER_MOVENODELIST_H

#include "movenode.h"

namespace godwhale {
namespace server {

/**
 * @brief 指し手とその探索結果をまとめて保持します。
 */
class MoveNodeList
{
public:
    explicit MoveNodeList();
    ~MoveNodeList();

    /**
     * @brief ノードリストを取得します。
     */
    std::vector<MoveNode> const &getList() const
    {
        return m_nodeList;
    }

    /**
     * @brief 指し手のリストを取得します。
     */
    std::vector<Move> getMoveList() const
    {
        std::vector<Move> result;

        BOOST_FOREACH (auto node, m_nodeList) {
            result.push_back(node.getMove());
        }

        return result;
    }

    /**
     * @brief ノード数を取得します。
     */
    int getSize() const
    {
        return m_nodeList.size();
    }

    /**
     * @brief 指定のインデックスのノードを取得します。
     */
    MoveNode &operator[](int index)
    {
        return m_nodeList[index];
    }

    /**
     * @brief 指定のインデックスのノードを取得します。
     */
    MoveNode const &operator[](int index) const
    {
        return m_nodeList[index];
    }

    /**
     * @brief リスト中の指し手の計算がすべて完了しているか調べます。
     */
    bool isDone(int depth, int A, int B) const
    {
        return (getUndoneCount(depth, A, B) == 0);
    }

    /**
     * @brief リスト中の指し手の計算がすべて完了しているか調べます。
     */
    void setRetrying(Move move)
    {
        int index = indexOf(move);
        if (index >= 0) {
            m_nodeList[index].setRetrying();
        }
    }

    /**
     * @brief 計算が完了していないノードの数を取得します。
     */
    int getUndoneCount(int depth, int A, int B) const
    {
        return (m_nodeList.size() - getDoneCount(depth, A, B));
    }

    void clear();
    void addNewMove(Move move);

    int indexOf(Move move) const;
    int indexOfUndone(int depth, int A, int B) const;
    int getDoneCount(int depth, int A, int B) const;

private:
    std::vector<MoveNode> m_nodeList;
};

} // namespace server
} // namespace godwhale

#endif
