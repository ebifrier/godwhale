
#ifndef GODWHALE_SERVER_SERVER_H
#define GODWHALE_SERVER_SERVER_H

#include "board.h"
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
        : IsValid(false), TotalNodes(0), MaxNodes(0)
        , m_move(MOVE_NA), Nodes(-1), Value(0) {
    }

    void Invalidate() {
        IsValid = false;
        TotalNodes = 0;
        MaxNodes = 0;
        Nps = 0;
    }

    void UpdateNodes(shared_ptr<Client> client) {
        TotalNodes += client->GetNodeCount();
        MaxNodes    = std::max(MaxNodes, client->GetNodeCount());
    }

    void SetNps(timer::cpu_timer &timer) {
        auto ns  = timer.elapsed().wall;

        Nps = (long)(TotalNodes / ((double)ns/1000/1000/1000));
    }

    void Set(const shared_ptr<Client> &client) {
        m_move = (client->HasPlayedMove() ?
            client->GetPlayedMove() : client->GetMove());
        Nodes = client->GetNodeCount();
        Value = client->GetValue();
        PVSeq = client->GetPVSeq();
        IsValid = true;
    }

    Move GetMove() const {
        return m_move;
    }

public:
    bool IsValid;
    long TotalNodes;
    long MaxNodes;
    long Nps;

    Move m_move;
    long Nodes;
    int Value;
    std::vector<server::Move> PVSeq;
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
    const Board &GetBoard() const {
        ScopedLock locker(m_guard);
        return m_board;
    }

    /**
     * @brief 現在の局面IDを取得します。
     */
    int GetGid() const {
        return m_gid;
    }

    void ClientLogined(shared_ptr<Client> client);
    std::vector<shared_ptr<Client> > GetClientList();
    
    void InitGame();
    void ResetPosition(const min_posi_t *posi);
    void MakeRootMove(move_t move);
    void UnmakeRootMove();
    void AdjustTimeHook(int turn);

    int Iterate(tree_t *restrict ptree, int *value, std::vector<move_t> &pvseq);
    bool IsEndIterate(tree_t *restrict ptree, timer::cpu_timer &timer);

private:
    static shared_ptr<Server> ms_instance;

    explicit Server();

    void StartThread();
    void ServiceThreadMain();

    void BeginAccept();
    void HandleAccept(shared_ptr<tcp::socket> socket,
                      const system::error_code &error);

    void SendCurrentInfo(std::vector<shared_ptr<Client> > &clientList,
                         Score &score);
    void SendPV(std::vector<shared_ptr<Client> > &clientList,
                Score &score);

private:
    mutable Mutex m_guard;
    asio::io_service m_service;
    shared_ptr<thread> m_thread;

    volatile bool m_isAlive;
    tcp::acceptor m_acceptor;

    std::list<weak_ptr<Client> > m_clientList;
    Board m_board;
    atomic<int> m_gid;
};

}
}

#endif
