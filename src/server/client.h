#ifndef GODWHALE_SERVER_CLIENT_H
#define GODWHALE_SERVER_CLIENT_H

#include "board.h"

namespace godwhale {
namespace server {

class Server;

/**
 * @brief リスナーＰＣによるクライアントを管理します。
 */
class Client : public enable_shared_from_this<Client>
{
public:
    explicit Client(shared_ptr<Server> server, shared_ptr<tcp::socket> socket);
    ~Client();

    /**
     * @brief ロック用のオブジェクトを取得します。
     */
    Mutex &GetGuard() const {
        return m_guard;
    }

    /**
     * @brief 正常にログイン処理が行われたかどうかを取得します。
     */
    bool IsLogined() const {
        ScopedLock locker(m_guard);
        return m_logined;
    }

    /**
     * @brief 各クライアントの識別IDを取得します。
     */
    const std::string &GetId() const {
        return m_id;
    }

    /**
     * @brief 各クライアントの使用スレッド数を取得します。
     */
    int GetThreadCount() const {
        return m_nthreads;
    }

    /**
     * @brief クライアントにPVを送るかどうかを取得します。
     */
    bool IsSendPV() const {
        return m_sendpv;
    }

    /**
     * @brief 思考中の局面IDを取得します。
     */
    int GetPid() const {
        ScopedLock locker(m_guard);
        return m_pid;
    }

    /**
     * @brief 次の指し手を取得します。
     */
    move_t GetMove() const {
        ScopedLock locker(m_guard);
        return m_move;
    }

    /**
     * @brief このクライアントの思考が開始された手を取得します。
     */
    move_t GetPlayedMove() const {
        ScopedLock locker(m_guard);
        return m_playedMove;
    }

    /**
     * @brief 各クライアントが深さ固定で探索しているか取得します。
     */
    bool IsStable() const {
        ScopedLock locker(m_guard);
        return m_stable;
    }

    /**
     * @brief 評価値の確定した手(定跡、詰みなど)かどうかを取得します。
     */
    bool IsFinal() const {
        ScopedLock locker(m_guard);
        return m_final;
    }

    /**
     * @brief 探索ノード数を取得します。
     */
    long GetNodeCount() const {
        ScopedLock locker(m_guard);
        return m_nodes;
    }

    /**
     * @brief 指し手の評価値を取得します。
     */
    int GetValue() const {
        ScopedLock locker(m_guard);
        return m_value;
    }

    /**
     * @brief PVノードを取得します。
     */
    const std::vector<move_t> &GetPVSeq() const {
        ScopedLock locker(m_guard);
        return m_pvseq;
    }

    /**
     * @brief 無視する指し手を取得します。
     */
    const std::vector<move_t> &GetIgnoreMoves() const {
        ScopedLock locker(m_guard);
        return m_ignoreMoves;
    }

    void Close();
    void BeginAsyncReceive();
    void SendCommand(const std::string &command);

    void MakeRootMove(move_t move, int pid);

private:
    void Disconnected();
    
    void HandleAsyncReceive(const error_code &error);

    void PutSendCommand(const std::string &command);
    std::string GetSendCommand();

    void BeginAsyncSend();
    void HandleAsyncSend(const error_code &error);

    int ParseCommand(const std::string &command);

private:
    shared_ptr<Server> m_server;
    shared_ptr<tcp::socket> m_socket;
    mutable Mutex m_guard;
    asio::streambuf m_streambuf;

    std::list<std::string> m_sendList;
    std::string m_sendingbuf;

    Board m_board;
    bool m_logined;
    std::string m_id;
    int m_nthreads;
    bool m_sendpv;

    int m_pid;
    move_t m_move;
    move_t m_playedMove;
    bool m_stable;
    bool m_final;
    long m_nodes;
    int m_value;
    std::vector<move_t> m_pvseq;

    std::vector<move_t> m_ignoreMoves;
};

} // namespace server
} // namespace godwhale

#endif
