#include "precomp.h"
#include "stdinc.h"
#include "iterativemovenodetree.h"

namespace godwhale {
namespace server {

IterativeMoveNodeTree::IterativeMoveNodeTree()
    : m_minIterationDepth(-1), m_maxIterationDepth(-1)
    , m_lastDoneIterationDepth(-1)
{
}

IterativeMoveNodeTree::~IterativeMoveNodeTree()
{
}

/**
 * @brief 
 */
void IterativeMoveNodeTree::makeMoveRoot()
{
}

/**
 * @brief 
 */
void IterativeMoveNodeTree::setPV(int itd, std::vector<Move> const & pv)
{
    //m_nodeTrees[itd].setPV(pv);
}

/**
 * @brief 
 */
void IterativeMoveNodeTree::setMoveList(int itd, int pld, std::vector<Move> const & list)
{
    MoveNodeTree & ntree = m_nodeTrees[itd];
    MoveNodeBranch & branch = ntree.getBranch(pld);

    branch.setMoveList(list);
}

} // namespace godwhale
} // namespace server
