#include "precomp.h"
#include "stdinc.h"
#include "commandpacket.h"
#include "servercommand.h"
#include "server.h"

namespace godwhale {
namespace server {

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

/**
 * @brief サーバー用コマンドの処理を行います。
 */
int Server::proce(bool searching)
{
    int status = PROCE_CONTINUE;

    while (status == PROCE_CONTINUE) {
        shared_ptr<ServerCommand> scommand = getNextServerCommand();
        if (scommand == nullptr) {
            return PROCE_OK;
        }

        int type = scommand->getType();
        if (searching) {
            if (type == SERVERCOMMAND_SETPOSITION  ||
                type == SERVERCOMMAND_BEGINGAME    ||
                type == SERVERCOMMAND_ENDGAME      ||
                type == SERVERCOMMAND_MAKEMOVEROOT ||
                type == SERVERCOMMAND_UNMAKEMOVEROOT) {
                return PROCE_ABORT;
            }
        }

        removeServerCommand(scommand);
        
        switch (scommand->getType()) {
        case SERVERCOMMAND_SETPOSITION:
            status = proce_SetPosition(scommand);
            break;
        case SERVERCOMMAND_BEGINGAME:
            status = proce_BeginGame(scommand);
            break;
        case SERVERCOMMAND_ENDGAME:
            status = proce_EndGame(scommand);
            break;
        case SERVERCOMMAND_MAKEMOVEROOT:
            status = proce_MakeMoveRoot(scommand);
            break;
        case SERVERCOMMAND_UNMAKEMOVEROOT:
            status = proce_UnmakeMoveRoot(scommand);
            break;
        default:
            unreachable();
            break;
        }
    }

    return (status == PROCE_CONTINUE ? PROCE_OK : status);
}

/**
 * @brief サーバー用コマンドの処理を行います。
 */
int Server::proce_SetPosition(shared_ptr<ServerCommand> scommand)
{
    LOG_NOTIFICATION() << "set position";

    m_positionId = 0;
    m_position = scommand->getPosition();
    m_currentValue = 0;

    return PROCE_CONTINUE;
}

int Server::proce_BeginGame(shared_ptr<ServerCommand> scommand)
{
    LOG_NOTIFICATION() << "begin game";

    m_currentValue = 0;
    m_turnTimer.start();

    return PROCE_CONTINUE;
}

int Server::proce_EndGame(shared_ptr<ServerCommand> scommand)
{
    LOG_NOTIFICATION() << "end game";

    m_currentValue = 0;

    // 全クライアントを停止します。
    sendCommandAll(CommandPacket::createStop());

    return PROCE_CONTINUE;
}

int Server::proce_MakeMoveRoot(shared_ptr<ServerCommand> scommand)
{
    auto move = scommand->getMove();

    m_position.makeMove(move);
    m_positionId += 10;

    LOG(Notification) << "root move: " << move;
    LOG(Notification) << m_position;

    m_turnTimer.start();

    return PROCE_CONTINUE;
}

int Server::proce_UnmakeMoveRoot(shared_ptr<ServerCommand> scommand)
{
    LOG_NOTIFICATION() << "root unmove";

    throw std::logic_error("unmakeMoveRoot: 実装されていません。");
}

} // namespace godwhale
} // namespace server
