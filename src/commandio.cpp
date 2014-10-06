#include "precomp.h"
#include "stdinc.h"
#include "commandio.h"

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
        m_command = std::move(other.m_command);
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
CommandIO::CommandIO(shared_ptr<tcp::socket> socket,
                     shared_ptr<ICommandListener> listener)
    : m_listener(listener), m_socket(socket), m_lineIndex(0)
{
    memset(m_line, 0, sizeof(m_line));
}

CommandIO::~CommandIO()
{
    Close();
}

void CommandIO::Close()
{
    LOCK (m_guard) {
        m_sendList.clear();
    }

    if (m_socket->is_open()) {
        system::error_code error;
        m_socket->shutdown(tcp::socket::shutdown_both, error);
    }
}

/**
 * @brief コネクションの切断時に呼ばれます。
 */
void CommandIO::Disconnected()
{
    if (m_listener != nullptr) {
        m_listener->OnDisconnected();
    }

    LOCK (m_guard) {
        m_sendList.clear();

        if (m_socket != nullptr && m_socket->is_open()) {
            m_socket->close();
            m_socket = nullptr;
        }
    }
}


/**
 * @brief コマンドの受信処理を開始します。
 */
void CommandIO::StartReceive()
{
    asio::async_read_until(
        *m_socket, m_streamBuf, "\n",
        bind(&CommandIO::HandleAsyncReceive, shared_from_this(),
             asio::placeholders::error));
}

/**
 * @brief コマンド受信後に呼ばれます。
 */
void CommandIO::HandleAsyncReceive(const system::error_code &error)
{
    if (error) {
        LOG_ERROR() << error << ": 通信エラーが発生しました。";
        Disconnected();
        return;
    }

    while (m_streamBuf.in_avail()) {
        char ch = m_streamBuf.sbumpc();

        // '\n'ごとにコマンドを区切ります。
        if (ch == '\n') {
            std::string line(&m_line[0], &m_line[m_lineIndex]);
            m_lineIndex = 0;

            if (m_listener != nullptr) {
                m_listener->OnCommandReceived(line);
            }
            break;
        }

        m_line[m_lineIndex++] = ch;
    }

    StartReceive();
}


/**
 * @brief コマンドリストに送信用のコマンドを追加します。
 */
void CommandIO::PutSendPacket(SendData const & packet)
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
SendData CommandIO::GetSendPacket()
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
void CommandIO::SendCommand(std::string const & command, bool isOutLog/*=true*/)
{
    PutSendPacket(SendData(command, isOutLog));

    // 次の送信処理に移ります。
    BeginAsyncSend();
}

/**
 * @brief 非同期の送信処理を開始します。
 */
void CommandIO::BeginAsyncSend()
{
    LOCK(m_guard) {
        if (!m_sendingBuf.isEmpty()) {
            return;
        }

        m_sendingBuf = GetSendPacket();
        if (m_sendingBuf.isEmpty()) {
            return;
        }

        if (m_sendingBuf.isOutLog()) {
            LOG_DEBUG() << "CommandIO send: " << m_sendingBuf.getCommand();
        }

        m_sendingBuf.appendDelimiter();
        asio::async_write(
            *m_socket, asio::buffer(m_sendingBuf.getCommand()),
            bind(&CommandIO::HandleAsyncSend, shared_from_this(),
                 asio::placeholders::error));
    }
}

/**
 * @brief 非同期送信の完了後に呼ばれます。
 */
void CommandIO::HandleAsyncSend(const system::error_code &error)
{
    if (error) {
        LOG_ERROR() << "command送信に失敗しました。(" << error.message() << ")";
        Disconnected();
        return;
    }

    LOCK(m_guard) {
        m_sendingBuf.clear();
    }

    // 次の送信処理に移ります。
    BeginAsyncSend();
}

} // namespace godwhale
