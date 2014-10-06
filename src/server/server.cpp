#include "precomp.h"
#include "stdinc.h"

#include "server.h"
#include "client.h"
#include "commandpacket.h"

namespace godwhale {
namespace server {

using namespace boost;
typedef boost::asio::ip::tcp tcp;

shared_ptr<Server> Server::ms_instance;

/**
 * サーバーのシングルトンインスタンスを初期化します。
 */
void Server::Initialize()
{
    ms_instance.reset(new Server());

    ms_instance->StartThread();
}

Server::Server()
    : m_isAlive(true), m_acceptor(m_service), m_gid(0), m_isPlaying(false)
    , m_currentValue(0)
{
    m_acceptor.open(tcp::v4());
    asio::socket_base::reuse_address option(true);
    m_acceptor.set_option(option);

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
void Server::StartThread()
{
    // IO関係の処理を開始します。
    m_thread.reset(new thread(&Server::ServiceThreadMain, this));
    BeginAccept();
}

/**
 * @brief IOスレッドのメインメソッドです。
 */
void Server::ServiceThreadMain()
{
    while (m_isAlive) {
        try
        {
            system::error_code error;

            // 処理した量が０であれば、少しウェイトを入れます。
            auto count = m_service.poll_one(error);
            if (count == 0) {
                this_thread::sleep(posix_time::milliseconds(20));
            }

            UpdateInfo();
            m_service.reset();
        }
        catch (...) {
            LOG(Error) << "ServerのIOスレッドでエラーが発生しました。";
        }
    }
}

/**
 * @brief マシン台数やNPSなどの情報を更新します。
 */
void Server::UpdateInfo()
{
    // マシン台数などの情報は５秒に一回送ります。
    if (m_sendTimer.elapsed().wall > 5LL*1000*1000*1000) {
        auto clientList = GetClientList();
        long totalNodes = 0;

        // 対局中以外では、NPSを0に設定します。
        if (m_isPlaying) {
            BOOST_FOREACH(auto client, clientList) {
                if (client->HasMove()) {
                    totalNodes += client->GetNodeCount();
                }
            }
        }

        auto ns = m_turnTimer.elapsed().wall;
        auto nps = (long)(totalNodes / ((double)ns/1000/1000/1000));
        SendCurrentInfo(clientList, nps);

        // これで時間がリセットされます。
        m_sendTimer.start();
    }
}

struct Compare
{
    bool operator()(weak_ptr<Client> x, weak_ptr<Client> y) const
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
            return (px->GetThreadCount() > py->GetThreadCount());
        }
    }
};

/**
 * @brief クライアントがログインしたときに呼ばれます。
 */
void Server::ClientLogined(shared_ptr<Client> client)
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
std::vector<shared_ptr<Client> > Server::GetClientList()
{
    ScopedLock locker(m_guard);
    std::vector<shared_ptr<Client> > result;

    // ちょっとした高速化
    result.reserve(m_clientList.size());

    for (auto it = m_clientList.begin(); it != m_clientList.end(); ) {
        auto client = (*it).lock();

        if (client != NULL) {
            result.push_back(client);
            ++it;
        }
        else {
            it = m_clientList.erase(it);
        }
    }

    return std::move(result);
}

void Server::BeginAccept()
{
    if (!m_isAlive) return;

    try
    {
        shared_ptr<tcp::socket> socket(new tcp::socket(m_service));

        m_acceptor.async_accept(*socket,
            bind(&Server::HandleAccept, shared_from_this(),
                 socket, asio::placeholders::error));
    }
    catch (std::exception ex)
    {
        LOG(Error) << "acceptの開始処理に失敗しました。(" << ex.what() << ")";
        m_isAlive = false;
    }
    catch (...)
    {
        LOG(Error) << "acceptの開始処理に失敗しました。";
        m_isAlive = false;
    }
}

void Server::HandleAccept(shared_ptr<tcp::socket> socket,
                          const system::error_code &error)
{
    if (error) {
        LOG(Error) << "acceptに失敗しました。(" << error.message() << ")";
    }
    else {
        shared_ptr<Client> client(new Client(shared_from_this(), socket));

        // クライアントをリストに追加します。
        /*LOCK(m_guard) {
            m_clientList.push_back(client);
        }*/

        client->BeginAsyncReceive();
        LOG(Notification) << "クライアントを受理しました。";
    }

    BeginAccept();
}

/**
 * @brief 評価値やマシン台数などを送信します。
 */
void Server::SendCurrentInfo(std::vector<shared_ptr<Client> > &clientList,
                             long nps)
{
    // 評価値はクジラちゃんからみた数字を送ります。
    auto fmt = format("info current %1% %2% %3%")
        % clientList.size() % nps % m_currentValue;
    auto command = fmt.str();

    BOOST_FOREACH(auto client, clientList) {
        client->SendCommand(command, false);
    }

    //LOG(Notification) << "Send Current Info: " << command;
}

/**
 * @brief PV情報を送信します。
 */
void Server::SendPV(std::vector<shared_ptr<Client> > &clientList,
                    int value, long nodes, const std::vector<Move> &pvseq)
{
    std::vector<std::string> v;
    std::transform(
        pvseq.begin(), pvseq.end(),
        std::back_inserter(v),
        [](Move _) { return _.str(); });

    auto fmt = format("info %1% %2% n=%3%")
        % ((double)value / 100.0)
        % algorithm::join(v, " ")
        % nodes;
    std::string command = fmt.str();

    BOOST_FOREACH(auto client, clientList) {
        if (client->IsSendPV()) {
            client->SendCommand(command, false);
        }
    }

    LOG(Notification) << "Send PV: " << command;
}

} // namespace server
} // namespace godwhale
