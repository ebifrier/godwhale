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
                type == COMMAND_MAKEMOVEROOT ||
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
        case COMMAND_MAKEMOVEROOT:
            status = proce_MakeMoveRoot(command);
            break;
        case COMMAND_SETMOVELIST:
            status = proce_SetMoveList(command);
            break;
        default:
            unreachable();
            break;
        }
    }

    return (status == PROCE_CONTINUE ? PROCE_OK : status);
}

/**
 * @brief サーバーに接続に行きます。
 */
void Client::connect(std::string const & address, std::string const & port)
{
    if (m_rsiService->isOpened()) {
        throw std::logic_error("すでに接続しています。");
    }

    tcp::resolver resolver(m_service);
    tcp::resolver::query query(address, port);
    shared_ptr<tcp::socket> socket(new tcp::socket(m_service));
    
    LOG_NOTIFICATION() << "begin to connect server";

    while (true) {
        tcp::resolver::iterator endpointIt = resolver.resolve(query);
        tcp::resolver::iterator end;

        boost::system::error_code error = boost::asio::error::host_not_found;
        while (error && endpointIt != end) {
            socket->close();
            socket->connect(*endpointIt++, error);
            if (!error) break;
        }

        if (!error) break;

        LOG_NOTIFICATION() << "connect retry .";
        boost::this_thread::sleep(boost::posix_time::seconds(10));
    }

    LOG_NOTIFICATION() << "connected !";

    m_rsiService->startReceive(socket);
    m_logined = false;
}

/**
 * @brief サーバーに接続に行きます。
 */
void Client::login(std::string const & loginName)
{
    if (loginName.empty()) {
    }

    // ログイン用のリプライコマンドをサーバーに送ります。
    //shared_ptr<ReplyPacket> reply; // = ReplyPacket::parse("login name 2");
    
    m_loginName = loginName;
    m_rsiService->sendRSI("login test 2");
}

/**
 * @brief loginコマンドを処理します。
 *
 * login <address> <port> <login_name> <nthreads>
 */
int Client::proce_Login(shared_ptr<CommandPacket> command)
{
    LOG_NOTIFICATION() << "handle login";

    if (m_logined) {
        LOG_ERROR() << "すでにログインしています。";
        return PROCE_ABORT;
    }

    // サーバーに接続します。
    std::string address = command->getServerAddress();
    std::string port = command->getServerPort();
    connect(address, port);

    // ログイン処理を行います。
    std::string loginName = command->getLoginName();
    login(loginName);

    return PROCE_ABORT;
}

/**
 * @brief setpositionコマンドを処理します。
 *
 * setposition <position_id> <address> <port> <login_id> <nthreads>
 */
int Client::proce_SetPosition(shared_ptr<CommandPacket> command)
{
    LOG_NOTIFICATION() << "handle setposition";

    Position const &position = command->getPosition();
    int positionId = command->getPositionId();

    // 現局面を設定します。
    SyncPosition::get()->reset(positionId, position);

    // 後処理
    setPositionWithId(position, positionId);
    m_positionId = positionId;

    LOG_NOTIFICATION() << "position_id=" << positionId;
    LOG_NOTIFICATION() << position;

    return PROCE_CONTINUE;
}

/**
 * @brief setmovelistコマンドを処理します。
 *
 * makerootmove <position_id> <old_position_id> <move>
 */
int Client::proce_MakeMoveRoot(shared_ptr<CommandPacket> command)
{
    LOG_NOTIFICATION() << "handle makemoveroot";

    if (m_positionId != command->getOldPositionId()) {
        LOG_ERROR() << "makemoveroot: position_idが一致しません。";
        return PROCE_CONTINUE;
    }

    int positionId = command->getPositionId();
    Move move = command->getMove();

    // ルート局面を一手進めます。
    if (!SyncPosition::get()->makeMoveRoot(positionId, move)) {
        LOG_ERROR() << "makemoveroot: " << move << ": 指し手を指すことができません。";
        return PROCE_CONTINUE;
    }

    // 後処理
    setPositionWithId(command->getPosition(), positionId);
    m_positionId = positionId;

    return PROCE_CONTINUE;
}

/**
 * @brief setmovelistコマンドを処理します。
 *
 * setmovelist <position_id> <itd> <pld> <move1> ... <moven>
 */
int Client::proce_SetMoveList(shared_ptr<CommandPacket> command)
{
    LOG_NOTIFICATION() << "handle setmovelist";

    if (m_positionId != command->getPositionId()) {
        LOG_ERROR() << "setmovelist: position_idが一致しません。";
        return PROCE_CONTINUE;
    }
}

} // namespace client
} // namespace godwhale
