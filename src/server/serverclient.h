#ifndef GODWHALE_SERVER_CLIENT_H
#define GODWHALE_SERVER_CLIENT_H

#include "commandpacket.h"
#include "rsiservice.h"

namespace godwhale {
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
    bool isLogined() const {
        return m_logined;
    }

    /**
     * @brief 各クライアントの識別IDを取得します。
     */
    std::string const &getLoginId() const {
        return m_loginId;
    }

    /**
     * @brief 各クライアントの使用スレッド数を取得します。
     */
    int getThreadCount() const {
        return m_threadCount;
    }

    /**
     * @brief クライアントにPVを送るかどうかを取得します。
     */
    bool isSendPV() const {
        return m_sendPV;
    }    

    void close();
    void initialize(shared_ptr<tcp::socket> socket);
    void sendCommand(shared_ptr<CommandPacket> command, bool isOutLog = true);

private:
    virtual void onRSIReceived(std::string const & rsi);
    virtual void onDisconnected();

    int handleCommand(const std::string &command);

private:
    shared_ptr<Server> m_server;
    shared_ptr<RSIService> m_rsiService;

    bool m_isAlive;
    bool m_logined;
    std::string m_loginId;
    int m_threadCount;
    bool m_sendPV;
};

} // namespace server
} // namespace godwhale

#endif
