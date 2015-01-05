
#ifndef GODWHALE_SERVER_SEARCHRESULT_H
#define GODWHALE_SERVER_SEARCHRESULT_H

#include "movenode.h"

namespace godwhale {
namespace server {

/**
 * @brief ルートからの探索を行い、その結果を保持するためのクラスです。
 */
class SearchResult
{
public:
    explicit SearchResult();

    /**
     * @brief 反復深化の探索深さを取得します。
     */
    int getIterationDepth() const
    {
        return m_iterationDepth;
    }

    /**
     * @brief 探索結果の評価値を取得します。
     */
    int getValue() const
    {
        return m_value;
    }

    /**
     * @brief ULEType(exact,lower,upper)を取得します。
     */
    ULEType getULEType() const
    {
        return m_ule;
    }

    /**
     * @brief PVに含まれる指し手の数を取得します。
     */
    int getPVSize() const
    {
        return m_pv.size();
    }

    /**
     * @brief PVに含まれる指し手を取得します。
     */
    Move getPV(int ply) const
    {
        return m_pv[ply];
    }

    /**
     * @brief PVを取得します。
     */
    std::vector<Move> const & getPV() const
    {
        return m_pv;
    }

    void initialize(tree_t * restrict ptree, int iterationDepth, int value);

private:
    int m_iterationDepth;
    std::vector<Move> m_pv;

    int m_value;
    ULEType m_ule;
};

}
}

#endif
