#ifndef GODWHALE_SERVER_SERVERCLIENT_H
#define GODWHALE_SERVER_SERVERCLIENT_H

#include "rsiservice.h"

namespace godwhale {

class CommandPacket;
class ReplyPacket;

namespace server {

class Server;

/**
 * @brief リスナーＰＣによるクライアントを管理します。
 */
class ServerClient : public enable_shared_from_this<ServerClient>,
                     public IRSIListener,
                     private boost::noncopyable
{
public:
    explicit ServerClient(shared_ptr<Server> server);
    ~ServerClient();

    /**
     * @brief クライアントの接続が生きているかどうかを取得します。
     */
    bool isAlive() const
    {
        return m_isAlive;
    }

    /**
     * @brief 正常にログイン処理が行われたかどうかを取得します。
     */
    bool isLogined() const
    {
        return m_logined;
    }

    /**
     * @brief 各クライアントのログイン名を取得します。
     */
    std::string const &getLoginName() const
    {
        return m_loginName;
    }

    /**
     * @brief 各クライアントの使用スレッド数を取得します。
     */
    int getThreadCount() const
    {
        return m_threadCount;
    }

    /**
     * @brief クライアントにPVを送るかどうかを取得します。
     */
    bool isSendPV() const
    {
        return m_sendPV;
    }    

    void initialize(shared_ptr<tcp::socket> socket);
    void close();

    void sendCommand(shared_ptr<CommandPacket> command, bool isOutLog = true);
    int proce();

private:
    virtual void onRSIReceived(std::string const & rsi);
    virtual void onDisconnected();

    void addReply(shared_ptr<ReplyPacket> reply);
    void removeReply(shared_ptr<ReplyPacket> reply);
    shared_ptr<ReplyPacket> getNextReply() const;

    int proce_Login(shared_ptr<ReplyPacket> reply);

private:
    shared_ptr<Server> m_server;
    shared_ptr<RSIService> m_rsiService;

    mutable Mutex m_replyGuard;
    std::list<shared_ptr<ReplyPacket>> m_replyList;

    bool m_isAlive;
    bool m_logined;
    std::string m_loginName;
    int m_threadCount;
    bool m_sendPV;
};

} // namespace server
} // namespace godwhale

#endif
