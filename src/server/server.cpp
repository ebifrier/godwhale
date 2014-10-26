#include "precomp.h"
#include "stdinc.h"

#include "server.h"
#include "serverclient.h"
#include "commandpacket.h"

namespace godwhale {
namespace server {

using namespace boost;
typedef boost::asio::ip::tcp tcp;

shared_ptr<Server> Server::ms_instance;

/**
 * サーバーのシングルトンインスタンスを初期化します。
 */
void Server::initialize()
{
    ms_instance.reset(new Server());

    ms_instance->startThread();
}

Server::Server()
    : m_isAlive(true), m_acceptor(m_service), m_gid(0), m_isPlaying(false)
    , m_currentValue(0)
{
    m_acceptor.open(tcp::v4());
    asio::socket_base::reuse_address option(true);
    m_acceptor.set_option(option);

    LOG_NOTIFICATION() << "server port=" << 4082;

    m_acceptor.bind(tcp::endpoint(tcp::v4(), 4082));
    m_acceptor.listen(100);
}

Server::~Server()
{
    m_isAlive = false;

    // 例外が出ないよう、念のためerrorを使っています。
    system::error_code error;
    m_acceptor.cancel(error);
    m_acceptor.close(error);

    if (m_thread != NULL) {
        m_thread->join();
        m_thread.reset();
    }
}

/**
 * @brief IOスレッドを開始します。
 */
void Server::startThread()
{
    if (m_thread != nullptr) {
        return;
    }

    // IO関係の処理を開始します。
    m_thread.reset(new thread(&Server::serviceThreadMain, this));
    beginAccept();
}

/**
 * @brief IOスレッドのメインメソッドです。
 */
void Server::serviceThreadMain()
{
    while (m_isAlive) {
        try {
            system::error_code error;

            // 処理した量が０であれば、少しウェイトを入れます。
            auto count = m_service.poll_one(error);
            if (count == 0) {
                this_thread::sleep(posix_time::milliseconds(20));
            }

            m_service.reset();
        }
        catch (...) {
            LOG_ERROR() << "ServerのIOスレッドでエラーが発生しました。";
        }
    }
}

void Server::beginAccept()
{
    if (!m_isAlive) return;

    try
    {
        shared_ptr<tcp::socket> socket(new tcp::socket(m_service));

        m_acceptor.async_accept(*socket,
            bind(&Server::handleAccept, shared_from_this(),
                 socket, asio::placeholders::error));
    }
    catch (std::exception ex)
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

void Server::handleAccept(shared_ptr<tcp::socket> socket,
                          const system::error_code &error)
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

    beginAccept();
}

struct Compare
{
    bool operator()(weak_ptr<ServerClient> x, weak_ptr<ServerClient> y) const
    {
        auto px = x.lock();
        auto py = y.lock();

        if (px == NULL) {
            return false;
        }
        else if (py == NULL) {
            return true;
        }
        else {
            return (px->getThreadCount() > py->getThreadCount());
        }
    }
};

/**
 * @brief クライアントがログインしたときに呼ばれます。
 */
void Server::clientLogined(shared_ptr<ServerClient> client)
{
    ScopedLock locker(m_guard);

    m_clientList.push_back(client);

    // 性能順にソートします。
    //std::stable_sort(m_clientList.begin(), m_clientList.end(), Compare());
    m_clientList.sort(Compare());
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

    return std::move(result);
}

} // namespace server
} // namespace godwhale
