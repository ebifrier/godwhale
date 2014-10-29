#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "serverclient.h"
#include "commandpacket.h"

namespace godwhale {
namespace server {

typedef boost::asio::ip::tcp tcp;

shared_ptr<Server> Server::ms_instance;

/**
 * サーバーのシングルトンインスタンスを初期化します。
 */
void Server::initialize(int argc, char * argv[])
{
    ms_instance.reset(new Server());

    ms_instance->startThread();
}

Server::Server()
    : m_acceptor(m_service), m_isAlive(true), m_currentValue(0)
{
    m_acceptor.open(tcp::v4());
    boost::asio::socket_base::reuse_address option(true);
    m_acceptor.set_option(option);

    m_acceptor.bind(tcp::endpoint(tcp::v4(), 4082));
    m_acceptor.listen(100);

    LOG_NOTIFICATION() << "server port=" << 4082;
}

Server::~Server()
{
    m_isAlive = false;

    // 例外が出ないよう、念のためerrorを使っています。
    boost::system::error_code error;
    m_acceptor.cancel(error);
    m_acceptor.close(error);

    if (m_iterateThread != nullptr) {
        m_iterateThread->join();
        m_iterateThread.reset();
    }

    if (m_serviceThread != nullptr) {
        m_serviceThread->join();
        m_serviceThread.reset();
    }
}

/**
 * @brief いくつかのスレッドを開始します。
 */
void Server::startThread()
{
    if (m_serviceThread != nullptr) {
        throw std::logic_error("m_serviceThreadはすでに初期化されています。");
    }

    if (m_iterateThread != nullptr) {
        throw std::logic_error("m_iterateThreadはすでに初期化されています。");
    }

    // IO関係の処理を開始します。
    m_serviceThread.reset(new boost::thread(&Server::serviceThreadMain, this));
    startAccept();

    // 思考用のスレッドを開始します。
    m_iterateThread.reset(new boost::thread(&Server::iterateThreadMain, this));
}

/**
 * @brief IOスレッドのメインメソッドです。
 */
void Server::serviceThreadMain()
{
    while (m_isAlive) {
        try {
            boost::system::error_code error;

            // 処理した量が０であれば、少しウェイトを入れます。
            auto count = m_service.poll_one(error);
            if (count == 0) {
                boost::this_thread::sleep(boost::posix_time::milliseconds(20));
            }

            m_service.reset();
        }
        catch (...) {
            LOG_ERROR() << "ServerのIOスレッドでエラーが発生しました。";
        }
    }
}

/**
 * @brief クライアントの受信処理を開始します。
 */
void Server::startAccept()
{
    if (!m_isAlive) return;

    try
    {
        shared_ptr<tcp::socket> socket(new tcp::socket(m_service));

        m_acceptor.async_accept(*socket,
            bind(&Server::handleAccept, shared_from_this(),
                 socket, boost::asio::placeholders::error));
    }
    catch (std::exception & ex)
    {
        LOG_ERROR() << "acceptの開始処理に失敗しました。(" << ex.what() << ")";
        m_isAlive = false;
    }
    catch (...)
    {
        LOG_ERROR() << "acceptの開始処理に失敗しました。";
        m_isAlive = false;
    }
}

/**
 * @brief クライアントの受信後に呼ばれます。
 */
void Server::handleAccept(shared_ptr<tcp::socket> socket,
                          boost::system::error_code const & error)
{
    if (error) {
        LOG_ERROR() << "acceptに失敗しました。(" << error.message() << ")";
    }
    else {
        shared_ptr<ServerClient> sclient(new ServerClient(shared_from_this()));
        sclient->initialize(socket);

        // クライアントをリストに追加します。
        LOCK(m_guard) {
            m_clientList.push_back(sclient);
        }
        
        LOG_NOTIFICATION() << "クライアントをAcceptしました。";
    }

    startAccept();
}

/**
 * @brief 不要なクライアントを削除し、生きているオブジェクトのみを取り出します。
 */
std::vector<shared_ptr<ServerClient>> Server::getClientList()
{
    ScopedLock locker(m_guard);
    std::vector<shared_ptr<ServerClient>> result;

    // ちょっとした高速化
    result.reserve(m_clientList.size());

    for (auto it = m_clientList.begin(); it != m_clientList.end(); ) {
        auto client = *it;

        if (client != NULL && client->isAlive()) {
            result.push_back(client);
            ++it;
        }
        else {
            it = m_clientList.erase(it);
        }
    }

    return result;
}

/**
 * @brief 指定のIDを持つクライアントにコマンドを送信します。
 */
void Server::sendCommand(int clientId, shared_ptr<CommandPacket> command)
{
    if (command == nullptr) {
        throw std::invalid_argument("command");
    }

    auto & client = m_clientList[clientId];
    client->sendCommand(command, true);
}

/**
 * @brief すべてのクライアントにコマンドを送信します。
 */
void Server::sendCommandAll(shared_ptr<CommandPacket> command)
{
    if (command == nullptr) {
        throw std::invalid_argument("command");
    }

    FOREACH_CLIENT (client) {
        client->sendCommand(command, true);
    }
}

/**
 * @brief 応答コマンドを追加します。
 */
void Server::addReply(shared_ptr<ReplyPacket> reply)
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
void Server::removeReply(shared_ptr<ReplyPacket> reply)
{
    LOCK (m_replyGuard) {
        m_replyList.remove(reply);
    }
}

/**
 * @brief 次に処理する応答コマンドを取得します。
 */
shared_ptr<ReplyPacket> Server::getNextReply() const
{
    LOCK (m_replyGuard) {
        if (m_replyList.empty()) {
            return shared_ptr<ReplyPacket>();
        }

        return m_replyList.front();
    }
}

} // namespace server
} // namespace godwhale
