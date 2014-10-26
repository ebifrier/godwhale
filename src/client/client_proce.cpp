#include "precomp.h"
#include "stdinc.h"
#include "commandpacket.h"
#include "syncposition.h"
#include "client.h"

namespace godwhale {
namespace client {

/**
 * @brief 受信した各コマンドの処理を行います。
 */
int Client::proce(bool searching)
{
    int status = PROCE_CONTINUE;

    while (status == PROCE_CONTINUE) {
        shared_ptr<CommandPacket> command = getNextCommand();
        if (command == nullptr) {
            return PROCE_OK;
        }

        int type = command->getType();
        if (searching) {
            if (type == COMMAND_LOGIN ||
                type == COMMAND_SETPOSITION  ||
                type == COMMAND_MAKEROOTMOVE ||
                type == COMMAND_STOP ||
                type == COMMAND_QUIT) {
                return PROCE_ABORT;
            }
        }

        removeCommand(command);
        
        switch (command->getType()) {
        case COMMAND_LOGIN:
            status = proce_Login(command);
            break;
        case COMMAND_SETPOSITION:
            status = proce_SetPosition(command);
            break;
        case COMMAND_MAKEROOTMOVE:
            status = proce_MakeRootMove(command);
            break;
        }
    }

    return (status == PROCE_CONTINUE ? PROCE_OK : status);
}

int Client::proce_Login(shared_ptr<CommandPacket> command)
{
    LOG_NOTIFICATION() << "handle login";

    if (m_logined) {
        LOG_ERROR() << "すでにログインしています。";
        return PROCE_ABORT;
    }

    std::string address = command->getServerAddress();
    std::string port = command->getServerPort();
    connect(address, port);
    login(command->getLoginId());

    return PROCE_ABORT;
}

int Client::proce_SetPosition(shared_ptr<CommandPacket> command)
{
    LOG_NOTIFICATION() << "handle setposition";

    m_positionId = command->getPositionId();
    m_position = command->getPosition();

    // bonaの局面を与えられた局面に設定します。

    return PROCE_CONTINUE;
}

int Client::proce_MakeRootMove(shared_ptr<CommandPacket> command)
{
    LOG_NOTIFICATION() << "handle makerootmove";

    if (m_positionId != command->getOldPositionId()) {
        LOG_ERROR() << "makerootmove: position_idが一致しません。";
        return PROCE_CONTINUE;
    }

    SyncPosition::get()->makeMove(command->getMove());

    m_positionId = command->getPositionId();
    return PROCE_CONTINUE;
}

} // namespace client
} // namespace godwhale
