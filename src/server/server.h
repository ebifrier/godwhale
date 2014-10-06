
#ifndef GODWHALE_SERVER_SERVER_H
#define GODWHALE_SERVER_SERVER_H

#include "position.h"
#include "client.h"

namespace godwhale {
namespace server {

/**
 * @brief 評価値やノード数などを保持します。
 */
struct Score
{
public:
    explicit Score()
        : m_move(MOVE_NA), m_nodes(0), m_value(0) {
    }

    /**
     * @brief 指し手や評価値を設定します。
     */
    void Set(const shared_ptr<Client> &client) {
        m_move = (client->HasPlayedMove() ?
            client->GetPlayedMove() : client->GetMove());
        m_nodes = client->GetNodeCount();
        m_value = client->GetValue();
        m_pvseq = client->GetPVSeq();
    }

    /**
     * @brief 指し手が設定されたか取得します。
     */
    bool IsValid() const {
        return (m_move != MOVE_NA);
    }

    /**
     * @brief 指し手を取得します。
     */
    Move GetMove() const {
        return m_move;
    }

    /**
     * @brief 探索ノード数を取得します。
     */
    long GetNodes() const {
        return m_nodes;
    }

    /**
     * @brief 評価値を取得します。
     */
    int GetValue() const {
        return m_value;
    }

    /**
     * @brief 指し手に付随するPVを取得します。
     */
    const std::vector<Move> GetPVSeq() const {
        return m_pvseq;
    }

public:
    Move m_move;
    long m_nodes;
    int m_value;
    std::vector<Move> m_pvseq;
};


/**
 * @brief 大合神クジラちゃんのサーバークラスです。
 */
class Server : public enable_shared_from_this<Server>
{
public:
    /**
     * @brief 初期化処理を行います。
     */
    static void Initialize();

    /**
     * @brief シングルトンインスタンスを取得します。
     */
    static shared_ptr<Server> GetInstance()
    {
        return ms_instance;
    }

public:
    ~Server();

    /**
     * @brief 現局面を取得します。
     */
    const Position &GetBoard() const {
        ScopedLock locker(m_guard);
        return m_board;
    }

    /**
     * @brief 現在の局面IDを取得します。
     */
    int GetGid() const {
        return m_gid;
    }

    /**
     * @brief 現在、対局中かどうかを取得します。
     */
    bool IsPlaying() const {
        return m_isPlaying;
    }

    void ClientLogined(shared_ptr<Client> client);
    std::vector<shared_ptr<Client> > GetClientList();
    
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

    void StartThread();
    void ServiceThreadMain();
    void UpdateInfo();

    void BeginAccept();
    void HandleAccept(shared_ptr<tcp::socket> socket,
                      const boost::system::error_code &error);

    void SendCurrentInfo(std::vector<shared_ptr<Client> > &clientList,
                         long nps);
    void SendPV(std::vector<shared_ptr<Client> > &clientList,
                int value, long nodes, const std::vector<Move> &pvseq);

private:
    mutable Mutex m_guard;
    boost::asio::io_service m_service;
    shared_ptr<boost::thread> m_thread;

    volatile bool m_isAlive;
    boost::asio::ip::tcp::acceptor m_acceptor;

    std::list<weak_ptr<Client> > m_clientList;
    Position m_board;
    boost::atomic<int> m_gid;
    bool m_isPlaying;

    boost::timer::cpu_timer m_turnTimer;
    boost::timer::cpu_timer m_sendTimer;
    int m_currentValue;
};

}
}

#endif
