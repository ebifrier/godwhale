
#ifndef GODWHALE_SERVER_SERVER_H
#define GODWHALE_SERVER_SERVER_H

#include "position.h"
#include "serverclient.h"

namespace godwhale {
namespace server {

class Client;
class ServerCommand;
class SearchResult;
class MoveNodeTree;

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
    int getPositionId() const
    {
        return m_positionId;
    }

    /**
     * @brief 現局面を取得します。
     */
    Position const &getPosition() const
    {
        return m_position;
    }

    std::vector<shared_ptr<ServerClient>> getClientList();
    void waitClients();

    void sendCommand(int clientId, shared_ptr<CommandPacket> command);
    void sendCommandAll(shared_ptr<CommandPacket> command);
    void addServerCommand(shared_ptr<ServerCommand> scommand);

    int detectSignals();
    int setPosition(Position const & position);
    int beginGame();
    int endGame();
    int makeMoveRoot(Move move);
    int unmakeMoveRoot();
    int iterate(tree_t *restrict ptree, int *value, std::vector<move_t> &pvseq);
    
private:
    static shared_ptr<Server> ms_instance;

    explicit Server();

    void startThread();
    void serviceThreadMain();

    void startAccept();
    void handleAccept(shared_ptr<tcp::socket> socket,
                      boost::system::error_code const & error);

    void removeServerCommand(shared_ptr<ServerCommand> scommand);
    shared_ptr<ServerCommand> getNextServerCommand() const;
    int proceClientReply();

private:
    /* server_move.cpp */
    bool IsEndIterate(tree_t * restrict ptree, boost::timer::cpu_timer &timer);

private:
#if 0
    /* server_proce.cpp */
    int proceClientReply();
    int proce(bool searching);
    int proce_SetPosition(shared_ptr<ServerCommand> scommand);
    int proce_BeginGame(shared_ptr<ServerCommand> scommand);
    int proce_EndGame(shared_ptr<ServerCommand> scommand);
    int proce_MakeMoveRoot(shared_ptr<ServerCommand> scommand);
    int proce_UnmakeMoveRoot(shared_ptr<ServerCommand> scommand);
#endif

private:
    /* server_root.cpp */
    void sortRootMoveList(tree_t * restrict ptree);
    int generateRootMove(SearchResult * result);
    void makeMoveAndMoveList(SearchResult const & sdata);
    void generateFirstMoves(SearchResult const & sdata);

private:
    mutable Mutex m_guard;
    boost::asio::io_service m_service;
    boost::asio::ip::tcp::acceptor m_acceptor;
    shared_ptr<boost::thread> m_serviceThread;

    volatile bool m_isAlive;
    std::vector<shared_ptr<ServerClient>> m_clientList;
    boost::atomic<int> m_positionId;
    Position m_position;

    mutable Mutex m_serverCommandGuard;
    std::list<shared_ptr<ServerCommand>> m_serverCommandList;

    shared_ptr<MoveNodeTree> m_ntree;

    /* server_move.cpp */
    boost::timer::cpu_timer m_turnTimer;
    boost::timer::cpu_timer m_sendTimer;
    int m_currentValue;

    /* server_root.cpp */
    uint64_t m_startingNodeCount;

    bool m_isRootCancelable;
    bool m_isRootExceeded;

    bool m_isFirstCancelable;
    bool m_isFirstExceeded;
};

}
}

#endif
