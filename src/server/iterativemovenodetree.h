
#ifndef GODWHALE_SERVER_ITERATIVEMOVENODETREE_H
#define GODWHALE_SERVER_ITERATIVEMOVENODETREE_H

#include "movenodetree.h"

namespace godwhale {
namespace server {

/**
 * @brief îΩïúê[âªÇÃê[Ç≥Ç≤Ç∆ÇÃíTçıñÿÇä«óùÇµÇ‹Ç∑ÅB
 */
class IterativeMoveNodeTree
{
public:
    explicit IterativeMoveNodeTree();
    ~IterativeMoveNodeTree();

    void makeMoveRoot();
    void setPV(int itd, std::vector<Move> const & pv);
    void setMoveList(int itd, int pld, std::vector<Move> const & list);

private:
    int m_minIterationDepth;
    int m_maxIterationDepth;
    int m_lastDoneIterationDepth;

    std::vector<MoveNodeTree> m_nodeTrees;
};

} // namespace godwhale
} // namespace server

#endif
