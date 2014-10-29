#ifndef GODWHALE_SERVER_MOVENODE_H
#define GODWHALE_SERVER_MOVENODE_H

#include "move.h"

namespace godwhale {
namespace server {

/**
 * @brief 評価値の種類を示します。
 */
enum ULEType
{
    /// 特になし？
    ULE_NONE  = 0,
    /// 数字が評価値の下限値であることを示す。
    ULE_LOWER = 1,
    /// 数字が評価値の上限値であることを示す。
    ULE_UPPER = 2,
    /// 数字が評価値そのものであることを示す。
    ULE_EXACT = 3,
};

/**
 * @brief 値の種類を示します。
 */
enum ValueType
{
    VALUETYPE_ALPHA,
    VALUETYPE_BETA,
};


/**
 * @brief 指し手とその探索結果を保持します。
 */
class MoveNode
{
public:
    explicit MoveNode(Move move);
    ~MoveNode();

    /**
     * @brief このノードに対応する指し手を取得します。
     */
    Move getMove() const
    {
        return m_move;
    }

    /**
     * @brief 探索した次の手の最善手を取得します。
     */
    Move getBestMove() const
    {
        return m_bestMove;
    }

    /**
     * @brief 指し手の探索深さを取得します。
     */
    int getDepth() const
    {
        return m_depth;
    }

    /**
     * @brief 探索したノード数を取得します。
     */
    int getNodes() const
    {
        return m_nodes;
    }

    /**
     * @brief 評価値の上限値を取得します。
     */
    int getUpper() const
    {
        return m_upper;
    }

    /**
     * @brief 評価値の下限値を取得します。
     */
    int getLower() const
    {
        return m_lower;
    }

    /**
     * @brief 探索中に評価値の更新が行われたかどうかを取得します。
     */
    bool isRetrying() const
    {
        return m_retrying;
    }

    /**
     * @brief 探索中に評価値の更新が行われたときに呼んでください。
     */
    void setRetrying()
    {
        m_retrying = true;
    }

    bool isDone(int depth, int A, int B) const;
    bool isExact(int depth) const;

    void update(int depth, int value, ULEType ule, int nodes, Move bestMove);
    void update(int depth, int value, int lower, int upper, int nodes, Move bestMove);

private:
    Move m_move;
    Move m_bestMove;
    int m_depth, m_nodes;
    short m_upper, m_lower;
    bool m_retrying;
};

} // namespace server
} // namespace godwhale

#endif
