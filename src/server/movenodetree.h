
#ifndef GODWHALE_SERVER_MOVENODETREE_H
#define GODWHALE_SERVER_MOVENODETREE_H

#include "movenodebranch.h"

namespace godwhale {
namespace server {

/**
 * @brief ある局面の探索結果や候補手を管理します。
 */
class MoveNodeTree
{
public:
    explicit MoveNodeTree(int iterationDepth);
    ~MoveNodeTree();

    /**
     * @brief 局面IDを取得します。
     */
    int getPositionId() const
    {
        return m_positionId;
    }

    /**
     * @brief 反復深化の探索深さを取得します。
     */
    int getIterationDepth() const
    {
        return m_iterationDepth;
    }

    /**
     * @brief 
     */
    int getLastPlyDepth() const
    {
        return m_lastPlyDepth;
    }

    /**
     * @brief 指定の深さのノードブランチを取得します。
     */
    MoveNodeBranch &getBranch(int pld)
    {
        return m_nodeBranches[pld];
    }

    /**
     * @brief 指定の深さのノードブランチを取得します。
     */
    MoveNodeBranch const &getBranch(int pld) const
    {
        return m_nodeBranches[pld];
    }

    bool hasSamePV(int pld, std::vector<Move> const & pv);
    bool isDoneExact(int pld, int srd);

    void initialize(int positionId, std::vector<Move> const & pv);
    void setMoveList(int pld, std::vector<Move> const & list);
    void start(int startPld, int alpha, int beta);

private:
    int m_positionId;
    int m_iterationDepth;
    int m_lastPlyDepth;

    std::vector<Move> m_pvFromRoot;
    std::vector<MoveNodeBranch> m_nodeBranches;
};

} // namespace server
} // namespace godwhale

#endif
