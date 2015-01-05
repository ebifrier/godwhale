#include "precomp.h"
#include "stdinc.h"
#include "commandpacket.h"
#include "replypacket.h"
#include "server.h"
#include "serverclient.h"

namespace godwhale {
namespace server {

ServerClient::ServerClient(shared_ptr<Server> server)
    : m_server(server), m_isAlive(false), m_logined(false)
    , m_threadCount(0), m_sendPV(false)
{
}

ServerClient::~ServerClient()
{
    close();

    LOG_DEBUG() << "ServerClient[" << m_loginName << "]は閉じられました。";
}

/**
 * @brief 返答コメントの受信などを開始します。
 */
void ServerClient::initialize(shared_ptr<tcp::socket> socket)
{
    if (m_rsiService != nullptr) {
        LOG_ERROR() << "RSIServiceはすでに初期化されています。";
        return;
    }

    // コマンドの送受信サービスを開始します。
    m_rsiService.reset(new RSIService(shared_from_this()));
    m_rsiService->startReceive(socket);

    m_isAlive = true;
}

/**
 * @brief コネクションを切断します。
 */
void ServerClient::close()
{
    if (m_rsiService != nullptr) {
        m_rsiService->close();
        m_rsiService = nullptr;
    }

    m_isAlive = false;
}

/**
 * @brief コネクションの切断時に呼ばれます。
 */
void ServerClient::onDisconnected()
{
    LOG_NOTIFICATION() << "ServerClient[" << m_loginName << "] is disconnected.";

    m_isAlive = false;
}

/**
 * @brief コマンドを送信します。
 */
void ServerClient::sendCommand(shared_ptr<CommandPacket> command, bool isOutLog/*= true*/)
{
    if (command == nullptr) {
        throw std::invalid_argument("command");
    }

    std::string rsi = command->toRSI();
    if (rsi.empty()) {
        LOG_ERROR() << F("command type %1%: toRSIに失敗しました。")
            % command->getType();
    }

    m_rsiService->sendRSI(rsi, isOutLog);
}

/**
 * @brief 応答コマンドのRSI受信時に呼ばれます。
 */
void ServerClient::onRSIReceived(std::string const & rsi)
{
    auto reply = ReplyPacket::parse(rsi);
    if (reply == nullptr) {
        LOG_ERROR() << rsi << ": RSIコマンドの解釈に失敗しました。";
        return;
    }

    // IOとその処理を分離するため、一度リストに入れます。
    addReply(reply);
}

/**
 * @brief 応答コマンドを追加します。
 */
void ServerClient::addReply(shared_ptr<ReplyPacket> reply)
{
    if (reply == nullptr) {
        throw std::invalid_argument("reply");
    }

    LOCK (m_replyGuard) {
        m_replyList.push_back(reply);
    }
}

/**
 * @brief 応答コマンドを削除します。
 */
void ServerClient::removeReply(shared_ptr<ReplyPacket> reply)
{
    LOCK (m_replyGuard) {
        m_replyList.remove(reply);
    }
}

/**
 * @brief 次に処理する応答コマンドを取得します。
 */
shared_ptr<ReplyPacket> ServerClient::getNextReply() const
{
    LOCK (m_replyGuard) {
        if (m_replyList.empty()) {
            return shared_ptr<ReplyPacket>();
        }

        return m_replyList.front();
    }
}

/**
 * @brief 応答コマンドを処理します。
 */
int ServerClient::proce()
{
    auto reply = getNextReply();
    if (reply == nullptr) {
        return 0;
    }

    removeReply(reply);

    switch (reply->getType()) {
    case REPLY_LOGIN:
        proce_Login(reply);
        break;
    }

    return 1;
}

/**
 * @brief login応答を処理します。
 *
 * login <name> <thread-num>
 */
int ServerClient::proce_Login(shared_ptr<ReplyPacket> reply)
{
    LOG_NOTIFICATION() << "handle login";

    m_loginName = reply->getLoginName();
    m_threadCount = reply->getThreadSize();
    return 0;
}

} // namespace server
} // namespace godwhale
