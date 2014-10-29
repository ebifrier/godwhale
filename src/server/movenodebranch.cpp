#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "commandpacket.h"
#include "movenodebranch.h"

namespace godwhale {
namespace server {

MoveNodeBranch::MoveNodeBranch()
{
    throw std::logic_error("Not supported.");
}

MoveNodeBranch::MoveNodeBranch(int iterationDepth, int plyDepth)
    : m_iterationDepth(iterationDepth), m_plyDepth(plyDepth)
    , m_alpha(-score_bound), m_beta(score_bound)
    , m_bestValue(-score_bound), m_bestULE(ULE_NONE)
    , m_moveListInited(false)
{
    m_clientNodeLists.resize(CLIENT_SIZE);
}

MoveNodeBranch::~MoveNodeBranch()
{
}

/**
 * @brief 局面IDなどの初期化を行います。
 */
void MoveNodeBranch::initialize(int positionId)
{
    BOOST_FOREACH (auto nodeList, m_clientNodeLists) {
        nodeList.clear();
    }

    m_positionId     = positionId;
    m_alpha          = -score_bound;
    m_beta           =  score_bound;
    m_bestValue      = -score_bound;
    m_bestULE        = ULE_NONE;
    m_moveListInited = false;
}

/**
 * @brief 各クライアントに指し手を分配します。
 */
void MoveNodeBranch::setMoveList(std::vector<Move> const & list)
{
    if (m_moveListInited) {
        return;
    }

    int i = 0;

    // 指し手の分配を行います。
    while (true) {
        for (int ci = 0; ci < (int)m_clientNodeLists.size(); ++ci) {
            auto & nodeList = m_clientNodeLists[ci];

            if (i >= (int)list.size()) {
                break;
            }
            nodeList.addNewMove(list[i++]);
        }
    }

    // 分配された指し手をクライアントに通知します。
    for (int ci = 0; ci < (int)m_clientNodeLists.size(); ++ci) {
        auto & nodeList = m_clientNodeLists[ci];

        auto command = CommandPacket::createSetMoveList(m_positionId,
                                                        m_iterationDepth,
                                                        m_plyDepth,
                                                        nodeList.getMoveList());
        Server::get()->sendCommand(ci, command);
    }

    m_moveListInited = true;
}

/**
 * @brief 評価値の更新を行います。
 */
void MoveNodeBranch::updateValue(int value, ValueType vtype)
{
    switch (vtype) {
    case VALUETYPE_ALPHA: m_alpha = value; break;
    case VALUETYPE_BETA:  m_beta  = value; break;
    default:              unreachable();   break;
    }
}

/**
 * @brief 最善手とその評価値の更新を行います。
 */
void MoveNodeBranch::updateBest(int value, Move move, std::vector<Move> const & pv)
{
    if (move.isEmpty()) {
        LOG_ERROR() << "空の指し手が設定されました。";
        return;
    }

    m_bestValue = value;
    m_bestULE = (value >= m_beta ? ULE_LOWER : ULE_EXACT);

    m_bestPV.clear();
    m_bestPV.push_back(move);
    m_bestPV.insert(m_bestPV.end(), pv.begin(), pv.end());

    // α値の更新も行います。
    updateValue(value, VALUETYPE_ALPHA);
}

} // namespace godwhale
} // namespace server
