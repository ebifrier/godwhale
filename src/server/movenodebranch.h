
#ifndef GODWHALE_SERVER_MOVENODEBRANCH_H
#define GODWHALE_SERVER_MOVENODEBRANCH_H

#include "movenodelist.h"

namespace godwhale {
namespace server {

/**
 * @brief ある局面の探索結果や候補手を管理します。
 */
class MoveNodeBranch
{
public:
    explicit MoveNodeBranch();
    explicit MoveNodeBranch(int iterationDepth, int plyDepth);
    ~MoveNodeBranch();

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
     * @brief ルート局面からの指し手の数を取得します。
     */
    int getPlyDepth() const
    {
        return m_plyDepth;
    }

    /**
     * @brief 探索結果の評価値を取得します。
     */
    int getBestValue() const
    {
        return m_bestValue;
    }

    /**
     * @brief ULEType(exact,lower,upper)を取得します。
     */
    ULEType getBestULEType() const
    {
        return m_bestULE;
    }

    /**
     * @brief PVに含まれる指し手の数を取得します。
     */
    int getBestPVSize() const
    {
        return m_bestPV.size();
    }

    /**
     * @brief PVに含まれる指し手を取得します。
     */
    Move getBestPV(int ply) const
    {
        return m_bestPV[ply];
    }

    /**
     * @brief このオブジェクトが対応するノードがβカットされたか調べます。
     */
    bool isBetaCut() const
    {
        return (m_bestValue >= m_beta);
    }

    void initialize(int positionId);
    void setMoveList(std::vector<Move> const & list);
    //void clear();

    void updateValue(int value, ValueType vtype);
    void updateBest(int value, Move move, std::vector<Move> const & pv);

private:
    int m_positionId;
    int m_iterationDepth;
    int m_plyDepth;

    int m_alpha;
    int m_beta;
    int m_bestValue;
    ULEType m_bestULE;
    std::vector<Move> m_bestPV;

    bool m_moveListInited;
    std::vector<MoveNodeList> m_clientNodeLists;
};

}
}

#endif
