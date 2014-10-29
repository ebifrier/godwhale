#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "commandpacket.h"
#include "movenodetree.h"

namespace godwhale {
namespace server {

MoveNodeTree::MoveNodeTree(int iterationDepth)
    : m_positionId(-1), m_iterationDepth(iterationDepth), m_lastPlyDepth(-1)
{
}

MoveNodeTree::~MoveNodeTree()
{
}

/**
 * @brief pldまでの深さのPVが、今のPVと同じか比較します。
 */
bool MoveNodeTree::hasSamePV(int pld, std::vector<Move> const & pv)
{
    if (pld >= (int)m_pvFromRoot.size()) {
        return false;
    }

    for (int d = 0; d < pld; ++d) {
        if (pv[d] != m_pvFromRoot[d]) {
            return false;
        }
    }

    return true;
}

/**
 * @brief pldまでの深さのPVが、今のPVと同じか比較します。
 */
bool MoveNodeTree::isDoneExact(int pld, int srd)
{
    return false;
}

/**
 * @brief 局面IDや最左ノードとなるPVを設定します。
 */
void MoveNodeTree::initialize(int positionId, std::vector<Move> const & pv)
{
    m_nodeBranches.resize(pv.size());

    for (int pld = 0; pld < (int)pv.size(); ++pld) {
        // 各深さのブランチを初期化します。
        m_nodeBranches[pld].initialize(positionId);
    }

    m_positionId   = positionId;
    m_pvFromRoot   = pv;
    m_lastPlyDepth = pv.size() - 1;
}

/**
 * @brief 各指し手深さの候補手一覧を設定します。
 */
void MoveNodeTree::setMoveList(int pld, std::vector<Move> const & list)
{
    if (pld >= (int)m_nodeBranches.size()) {
        LOG_ERROR() << "pld=" << pld << ": 対応していない指し手の深さです。";
        return;
    }

    m_nodeBranches[pld].setMoveList(list);
}

/**
 * @brief 指定の深さから、与えられた値を基準に探索を開始します。
 */
void MoveNodeTree::start(int startPld, int alpha, int beta)
{
    if (startPld >= (int)m_nodeBranches.size()) {
        LOG_ERROR() << "pld=" << startPld << ": 対応していない指し手の深さです。";
        return;
    }

    auto command = CommandPacket::createStart(m_positionId, m_iterationDepth,
                                              startPld, alpha, beta);
    Server::get()->sendCommandAll(command);
}

} // namespace godwhale
} // namespace server
