#ifndef GODWHALE_COMMANDIO_H
#define GODWHALE_COMMANDIO_H

namespace godwhale {

/**
 * @brief コマンドの受信などに使われるリスナーです。
 */
class ICommandListener
{
public:
    /**
     * @brief コマンド受信時に呼ばれます。
     */
    virtual void OnCommandReceived(std::string const & command)
    {
    }

    /**
     * @brief 接続が切断されたときに呼ばれます。
     */
    virtual void OnDisconnected()
    {
    }
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
     * @brief コマンドの区切り記号を追加します。
     */
    bool isEmpty() const
    {
        return m_command.empty();
    }

    /**
     * @brief コマンドの区切り記号を追加します。
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
 * @brief リスナーＰＣによるクライアントを管理します。
 */
class CommandIO : public enable_shared_from_this<CommandIO>
{
public:
    explicit CommandIO(shared_ptr<tcp::socket> socket,
                       shared_ptr<ICommandListener> listener);
    ~CommandIO();

    /**
     * @brief コマンドを送信します。
     */
    void SendCommand(boost::format const & fmt, bool isOutLog = true)
    {
        SendCommand(fmt.str(), isOutLog);
    }

    void Close();
    void StartReceive();
    void SendCommand(std::string const & command, bool isOutLog = true);

private:
    void Disconnected();
    
    void HandleAsyncReceive(const boost::system::error_code &error);

    void PutSendPacket(SendData const & packet);
    SendData GetSendPacket();

    void BeginAsyncSend();
    void HandleAsyncSend(const boost::system::error_code &error);

private:
    shared_ptr<ICommandListener> m_listener;
    shared_ptr<tcp::socket> m_socket;
    mutable Mutex m_guard;

    boost::asio::streambuf m_streamBuf;
    char m_line[2048];
    int m_lineIndex;

    std::list<SendData> m_sendList;
    SendData m_sendingBuf;
};

} // namespace godwhale

#endif
