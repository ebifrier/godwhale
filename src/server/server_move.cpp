#include "precomp.h"
#include "stdinc.h"
#include "commandpacket.h"
#include "replypacket.h"
#include "searchresult.h"
#include "movenodetree.h"
#include "syncposition.h"
#include "server.h"
#include "serverclient.h"

/*
 * Serverクラスの中でも特に指し手生成に関わる部分のみを扱っています。
 */

namespace godwhale {
namespace server {
    
using namespace boost;

extern bool IsThinkEnd(tree_t *restrict ptree, unsigned int turnTimeMS);

/**
 * @brief サーバー用コマンドの処理を行います。
 */
int Server::setPosition(Position const & position)
{
    LOG_NOTIFICATION() << "set position";

    m_positionId += 1;
    m_position = position;
    m_currentValue = 0;

    // 局面をクライアントに通知します。
    for (int ci = 0; ci < CLIENT_SIZE; ++ci) {
        auto command = CommandPacket::createSetPosition(m_positionId,
                                                        position);
        Server::get()->sendCommand(ci, command);
    }

    return PROCE_CONTINUE;
}

int Server::beginGame()
{
    LOG_NOTIFICATION() << "begin game";

    m_currentValue = 0;
    m_turnTimer.start();

    return PROCE_CONTINUE;
}

int Server::endGame()
{
    LOG_NOTIFICATION() << "end game";

    m_positionId = 0;
    m_currentValue = 0;

    // 全クライアントを停止します。
    sendCommandAll(CommandPacket::createStop());

    return PROCE_CONTINUE;
}

int Server::makeMoveRoot(Move move)
{
    int oldPositionId = m_positionId;
    m_positionId += 1;

    SyncPosition::get()->rewind();
    if (!SyncPosition::get()->makeMoveRoot(m_positionId, move)) {
        LOG_ERROR() << move << ": makeMoveRootに失敗しました。";
        LOG_ERROR() << SyncPosition::get()->getPosition();
        return PROCE_ABORT;
    }

    LOG(Notification) << "root move: " << move;
    LOG(Notification) << SyncPosition::get()->getPosition();

    // 局面をクライアントに通知します。
    for (int ci = 0; ci < CLIENT_SIZE; ++ci) {
        auto command = CommandPacket::createMakeMoveRoot(m_positionId,
                                                         oldPositionId,
                                                         move);
        Server::get()->sendCommand(ci, command);
    }

    m_turnTimer.start();

    return PROCE_CONTINUE;
}

int Server::unmakeMoveRoot()
{
    LOG_NOTIFICATION() << "root unmove";

    throw std::logic_error("unmakeMoveRoot: 実装されていません。");
}

/**
 * @brief クライアントが受信した応答コマンドを処理します。
 */
int Server::proceClientReply()
{
    auto clientList = getClientList();
    int count = 0;
    
    BOOST_FOREACH(auto client, clientList) {
        // すべての応答コマンドを処理します。
        while (client->proce() > 0) {
            count += 1;
        }
    }

    return count;
}

int Server::iterate(tree_t *restrict ptree, int *value, std::vector<move_t> &pvseq)
{
    LOG_NOTIFICATION() << "";
    LOG_NOTIFICATION() << "";
    LOG_NOTIFICATION() << "";
    LOG_NOTIFICATION() << "------------------ Begin Iterate.";
    LOG_NOTIFICATION() << "thinking: " << ((game_status & flag_thinking) != 0);
    LOG_NOTIFICATION() << "puzzling: " << ((game_status & flag_puzzling) != 0);
    LOG_NOTIFICATION() << "pondering: " << ((game_status & flag_pondering) != 0);

    SearchResult result;
    generateRootMove(&result);

    // 最左ノードを設定します。
    m_ntree->initialize(m_positionId, result.getIterationDepth(), result.getPV());

    // 指し手リストを設定します。
    generateFirstMoves(result);

    while (m_isAlive) {
        int count = proceClientReply();
        if (count == 0) {
            sleep(200);
            continue;
        }
    }

    return 0;
}

bool Server::IsEndIterate(tree_t *restrict ptree, timer::cpu_timer &timer)
{
    if (detect_signals(ptree) != 0) {
        return true;
    }

    if (!(game_status & flag_puzzling)) {
        auto ms = (unsigned int)(timer.elapsed().wall / 1000 / 1000);
        return IsThinkEnd(ptree, ms); //(ms > 10*1000);
    }

    return false;
}

} // namespace server
} // namespace godwhale
