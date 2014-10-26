#include "precomp.h"
#include "stdinc.h"
#include "rsiservice.h"

namespace godwhale {

using namespace boost;

SendData::SendData()
{
}

SendData::SendData(std::string const & command, bool isOutLog)
{
    m_command = command;
    m_isOutLog = isOutLog;
}

SendData::SendData(SendData const & other)
    : m_command(other.m_command), m_isOutLog(other.m_isOutLog)
{
}

SendData::SendData(SendData && other)
    : m_command(std::move(other.m_command)), m_isOutLog(other.m_isOutLog)
{
}

SendData &SendData::operator =(SendData const & other)
{
    if (this != &other) {
        m_command = other.m_command;
        m_isOutLog = other.m_isOutLog;
    }

    return *this;
}

SendData &SendData::operator =(SendData && other)
{
    if (this != &other) {
        m_command = std::move(other.m_command);
        m_isOutLog = other.m_isOutLog;
    }

    return *this;
}


/////////////////////////////////////////////////////////////////////
RSIService::RSIService(weak_ptr<IRSIListener> listener)
    : m_listener(listener), m_isShutdown(false), m_lineIndex(0)
{
    memset(m_line, 0, sizeof(m_line));
}

RSIService::~RSIService()
{
    close();
}
/**
 * @brief 接続用のソケットを設定します。
 */
void RSIService::startReceive(shared_ptr<tcp::socket> socket)
{
    close();

    // shared_ptrはスレッドセーフ
    m_socket = socket;

    // 受信処理を開始します。
    if (socket != nullptr && socket->is_open()) {
        startReceive();
    }
}

/**
 * @brief 接続を閉じます。
 */
void RSIService::close()
{
    LOCK (m_guard) {
        m_sendList.clear();
    }

    auto socket = m_socket;
    if (socket != nullptr && socket->is_open()) {
        system::error_code error;
        socket->shutdown(tcp::socket::shutdown_both, error);

        m_isShutdown = true;
    }
}

/**
 * @brief コネクションの切断時に呼ばれます。
 */
void RSIService::onDisconnected()
{
    shared_ptr<IRSIListener> listener = m_listener.lock();
    if (listener != nullptr) {
        listener->onDisconnected();
    }

    LOCK (m_guard) {
        m_sendList.clear();
    }

    auto socket = m_socket;
    if (socket != nullptr && socket->is_open()) {
        system::error_code error;
        socket->close(error);

        m_socket = nullptr;
    }
}

/**
 * @brief コマンドの受信処理を開始します。
 */
void RSIService::startReceive()
{
    auto socket = m_socket;
    if (socket == nullptr || !socket->is_open()) {
        LOG_ERROR() << "RSIの受信開始に失敗しました。";
        return;
    }

    asio::async_read_until(
        *socket, m_streamBuf, "\n",
        bind(&RSIService::handleAsyncReceive, shared_from_this(),
             asio::placeholders::error));
}

/**
 * @brief コマンド受信後に呼ばれます。
 */
void RSIService::handleAsyncReceive(system::error_code const & error)
{
    if (error) {
        if (m_isShutdown) {
            LOG_WARNING() << "ソケットはすでに閉じられています。";
        }
        else {
            LOG_ERROR() << error << ": 通信エラーが発生しました。"
                        << "(" << error.message() << ")";
        }

        onDisconnected();
        return;
    }

    std::list<std::string> rsiList;

    LOCK (m_guard) {
        while (m_streamBuf.in_avail()) {
            char ch = m_streamBuf.sbumpc();

            // '\n'ごとにコマンドを区切ります。
            if (ch == '\n') {
                std::string line(&m_line[0], &m_line[m_lineIndex]);

                LOG_DEBUG() << "command recv: " << line;

                rsiList.push_back(line);
                m_lineIndex = 0;
            }
            else {
                m_line[m_lineIndex++] = ch;
            }
        }
    }

    // lock外で各RSIを処理します。
    shared_ptr<IRSIListener> listener = m_listener.lock();
    if (listener != nullptr) {
        BOOST_FOREACH (auto rsi, rsiList) {
            listener->onRSIReceived(rsi);
        }
    }

    startReceive();
}


/**
 * @brief コマンドリストに送信用のコマンドを追加します。
 */
void RSIService::putSendPacket(SendData const & packet)
{
    if (packet.isEmpty()) {
        return;
    }

    LOCK (m_guard) {
        m_sendList.push_back(packet);
    }
}

/**
 * @brief もし未送信のコマンドがあればそれをリストから削除し返します。
 */
SendData RSIService::getSendPacket()
{
    LOCK (m_guard) {
        if (m_sendList.empty()) {
            return SendData();
        }

        auto packet = m_sendList.front();
        m_sendList.pop_front();
        return packet;
    }
}

/**
 * @brief コマンドを送信します。
 */
void RSIService::sendRSI(std::string const & rsi, bool isOutLog/*=true*/)
{
    putSendPacket(SendData(rsi, isOutLog));

    // 次の送信処理に移ります。
    beginAsyncSend();
}

/**
 * @brief 非同期の送信処理を開始します。
 */
void RSIService::beginAsyncSend()
{
    auto socket = m_socket;

    LOCK (m_guard) {
        if (socket == nullptr || !socket->is_open()) {
            throw std::logic_error("ソケットが接続されていません。");
        }

        if (m_isShutdown) {
            throw std::logic_error("ソケットの切断中です。");
        }

        // データ送信中の場合
        if (!m_sendingBuf.isEmpty()) {
            return;
        }

        m_sendingBuf = getSendPacket();
        if (m_sendingBuf.isEmpty()) {
            return;
        }

        if (m_sendingBuf.isOutLog()) {
            LOG_DEBUG() << "send: " << m_sendingBuf.getCommand();
        }

        // デリミタ('\n')を付加します。
        m_sendingBuf.appendDelimiter();
    }

    // コールバックが即座に呼ばれることがあるため、
    // ロックを解除した状態で呼びます。
    asio::async_write(
        *socket, asio::buffer(m_sendingBuf.getCommand()),
        bind(&RSIService::handleAsyncSend, shared_from_this(),
             asio::placeholders::error));
}

/**
 * @brief 非同期送信の完了後に呼ばれます。
 */
void RSIService::handleAsyncSend(system::error_code const & error)
{
    if (error) {
        LOG_ERROR() << "command送信に失敗しました。(" << error.message() << ")";
        onDisconnected();
        return;
    }

    LOCK (m_guard) {
        m_sendingBuf.clear();
    }

    // 次の送信処理に移ります。
    beginAsyncSend();
}

} // namespace godwhale
