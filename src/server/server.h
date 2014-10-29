
#ifndef GODWHALE_SERVER_SERVER_H
#define GODWHALE_SERVER_SERVER_H

#include "position.h"
#include "serverclient.h"

namespace godwhale {
namespace server {

class Client;

#define FOREACH_CLIENT(VAR) \
    auto BOOST_PP_CAT(temp, __LINE__) = getClientList();  \
    BOOST_FOREACH(auto VAR, BOOST_PP_CAT(temp, __LINE__))

/**
 * @brief 大合神クジラちゃんのサーバークラスです。
 */
class Server : public enable_shared_from_this<Server>
{
public:
    /**
     * @brief 初期化処理を行います。
     */
    static void initialize(int argc, char * argv[]);

    /**
     * @brief シングルトンインスタンスを取得します。
     */
    static shared_ptr<Server> get()
    {
        return ms_instance;
    }

public:
    ~Server();

    /**
     * @brief 現在の局面IDを取得します。
     */
    int getPositionId() const {
        return m_positionId;
    }

    /**
     * @brief 現局面を取得します。
     */
    Position const &getPosition() const {
        return m_position;
    }

    std::vector<shared_ptr<ServerClient>> getClientList();
    void sendCommand(int clientId, shared_ptr<CommandPacket> command);
    void sendCommandAll(shared_ptr<CommandPacket> command);
    void addReply(shared_ptr<ReplyPacket> reply);
    
    void InitGame();
    void QuitGame();
    void ResetPosition(const min_posi_t *posi);
    void MakeRootMove(Move move);
    void UnmakeRootMove();
    void AdjustTimeHook(int turn);

    int Iterate(tree_t *restrict ptree, int *value, std::vector<move_t> &pvseq);
    bool IsEndIterate(tree_t *restrict ptree, boost::timer::cpu_timer &timer);

private:
    static shared_ptr<Server> ms_instance;

    explicit Server();

    void startThread();
    void iterateThreadMain();
    void serviceThreadMain();

    void startAccept();
    void handleAccept(shared_ptr<tcp::socket> socket,
                      boost::system::error_code const & error);

private:
    void removeReply(shared_ptr<ReplyPacket> reply);
    shared_ptr<ReplyPacket> getNextReply() const;

private:
    /* server_root.cpp */

private:
    mutable Mutex m_guard;
    boost::asio::io_service m_service;
    boost::asio::ip::tcp::acceptor m_acceptor;
    shared_ptr<boost::thread> m_serviceThread;

    volatile bool m_isAlive;
    std::vector<shared_ptr<ServerClient>> m_clientList;
    boost::atomic<int> m_positionId;
    Position m_position;
    shared_ptr<boost::thread> m_iterateThread;

    mutable Mutex m_replyGuard;
    std::list<shared_ptr<ReplyPacket>> m_replyList;

    boost::timer::cpu_timer m_turnTimer;
    boost::timer::cpu_timer m_sendTimer;
    int m_currentValue;
};

}
}

#endif
