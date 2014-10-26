#ifndef GODWHALE_COMMANDIO_H
#define GODWHALE_COMMANDIO_H

namespace godwhale {

/**
 * @brief コマンドの受信などに使われるリスナーです。
 */
class IRSIListener
{
public:
    /**
     * @brief コマンド受信時に呼ばれます。
     */
    virtual void onRSIReceived(std::string const & rsi) = 0;

    /**
     * @brief 接続が切断されたときに呼ばれます。
     */
    virtual void onDisconnected() = 0;
};


/**
 * @brief データ送信用のクラスです。(privateクラス)
 */
class SendData
{
public:
    explicit SendData();
    SendData(std::string const & command, bool isOutLog);
    SendData(SendData const & other);
    SendData(SendData && other);

    SendData &operator =(SendData const & other);
    SendData &operator =(SendData && other);

    /**
     * @brief コマンド文字列を取得します。
     */
    std::string const &getCommand() const
    {
        return m_command;
    }

    /**
     * @brief ログ表示するかどうかのフラグを取得します。
     */
    bool isOutLog() const
    {
        return m_isOutLog;
    }

    /**
     * @brief コマンドが空かどうか調べます。
     */
    bool isEmpty() const
    {
        return m_command.empty();
    }

    /**
     * @brief コマンドを初期化します。
     */
    void clear()
    {
        m_command = "";
    }

    /**
     * @brief コマンドの区切り記号を追加します。
     */
    void appendDelimiter()
    {
        if (m_command.empty() || m_command.back() != '\n') {
            m_command.append("\n");
        }
    }

private:
    std::string m_command;
    bool m_isOutLog;
};


/**
 * @brief RSI(remote shogi protocol)の送受信を行うクラスです。
 */
class RSIService : public enable_shared_from_this<RSIService>
{
public:
    explicit RSIService(weak_ptr<IRSIListener> listener);
    virtual ~RSIService();

    /**
     * @brief ソケットが接続中か調べます。
     */
    bool isOpened() const
    {
        auto socket = m_socket;
        return (socket != nullptr && socket->is_open());
    }

    /**
     * @brief コマンドを送信します。
     */
    void sendRSI(boost::format const & fmt, bool isOutLog = true)
    {
        sendRSI(fmt.str(), isOutLog);
    }

    void startReceive(shared_ptr<tcp::socket> socket);
    void close();
    void sendRSI(std::string const & rsi, bool isOutLog = true);

private:
    void onDisconnected();

    void startReceive();
    void handleAsyncReceive(boost::system::error_code const & error);

    void putSendPacket(SendData const & packet);
    SendData getSendPacket();

    void beginAsyncSend();
    void handleAsyncSend(boost::system::error_code const & error);

private:
    mutable Mutex m_guard;
    shared_ptr<tcp::socket> m_socket;
    weak_ptr<IRSIListener> m_listener;

    volatile bool m_isShutdown;

    boost::asio::streambuf m_streamBuf;
    char m_line[2048];
    int m_lineIndex;

    std::list<SendData> m_sendList;
    SendData m_sendingBuf;
};

} // namespace godwhale

#endif
