#include "precomp.h"
#include "stdinc.h"
#include "commandpacket.h"
#include "replypacket.h"
#include "client.h"

namespace godwhale {
namespace client {

shared_ptr<Client> Client::ms_instance;

/**
 * クライアントのシングルトンインスタンスを初期化します。
 */
int Client::initializeInstance(int argc, char *argv[])
{
    shared_ptr<Client> client(new Client);
    client->initialize();

    ms_instance = client;
    return 0;
}

Client::Client()
    : m_isAlive(true), m_logined(false)
    , m_nthreads(0), m_positionId(-1), m_nodes(0)
{
}

Client::~Client()
{
    close();

    if (m_serviceThread != NULL) {
        m_serviceThread->join();
        m_serviceThread = nullptr;
    }
}

/**
 * @brief クライアントの開始処理を行います。
 */
void Client::initialize()
{
    m_rsiService.reset(new RSIService(shared_from_this()));

    // 非同期用の送受信スレッドを起動します。
    m_serviceThread.reset(new boost::thread(
        &Client::serviceThreadMain, this));
}

/**
 * @brief 通信処理を行うスレッド用のメソッドです。
 */
void Client::serviceThreadMain()
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
        catch (std::exception & ex) {
            LOG_ERROR() << "ClientのIOスレッドで例外が発生しました。" << ex.what();
        }
    }
}

/**
 * @brief コネクションを切断します。
 */
void Client::close()
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
void Client::onDisconnected()
{
    LOG_NOTIFICATION() << "Client[" << m_loginId << "] is disconnected.";
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
void Client::login(std::string const & loginId)
{
    if (loginId.empty()) {
    }

    // ログイン用のリプライコマンドをサーバーに送ります。
    //shared_ptr<ReplyPacket> reply; // = ReplyPacket::parse("login name 2");
    
    m_rsiService->sendRSI("login name 2");
}

/**
 * @brief サーバーからコマンドを受信した時に呼ばれます。
 */
void Client::onRSIReceived(std::string const & rsi)
{
    addCommandFromRSI(rsi);
}

/**
 * @brief 受信したRSIコマンドを追加します。
 */
void Client::addCommandFromRSI(std::string const & rsi)
{
    auto command = CommandPacket::parse(rsi);
    if (command == nullptr) {
        LOG_ERROR() << rsi << ": RSIコマンドの解釈に失敗しました。";
        return;
    }

    addCommand(command);
}

/**
 * @brief コマンドをサーバーに送信します。
 */
void Client::sendReply(shared_ptr<ReplyPacket> reply, bool isOutLog/*=true*/)
{
    std::string rsi = reply->toRSI();
    if (rsi.empty()) {
        LOG_ERROR() << F("reply type %1%: toRSIに失敗しました。")
            % reply->getType();
    }

    m_rsiService->sendRSI(rsi, isOutLog);
}

void Client::addCommand(shared_ptr<CommandPacket> command)
{
    LOCK (m_commandGuard) {
        auto it = m_commandList.begin();

        // 挿入ソートで優先順位の高い順に並べます。
        for (; it != m_commandList.end(); ++it) {
            if (command->getPriority() > (*it)->getPriority()) {
                break;
            }
        }

        m_commandList.insert(it, command);
    }
}

void Client::removeCommand(shared_ptr<CommandPacket> command)
{
    LOCK (m_commandGuard) {
        m_commandList.remove(command);
    }
}

shared_ptr<CommandPacket> Client::getNextCommand()
{
    LOCK (m_commandGuard) {
        if (m_commandList.empty()) {
            return shared_ptr<CommandPacket>();
        }

        return m_commandList.front();
    }
}

} // namespace client
} // namespace godwhale
