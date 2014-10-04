#ifndef GODWHALE_SERVER_CLIENT_H
#define GODWHALE_SERVER_CLIENT_H

#include "position.h"

namespace godwhale {
namespace server {

class Server;

/**
 * @brief クライアントへのコマンド送信に使います。
 */
class SendPacket
{
public:
    explicit SendPacket()
        : m_isOutLog(false) {
    }

    explicit SendPacket(const std::string &command, bool isOutLog)
        : m_command(command), m_isOutLog(isOutLog) {
    }

    /**
     * @brief コマンドが空かどうかを取得します。
     */
    bool IsEmpty() const {
        return m_command.empty();
    }

    /**
     * @brief コマンドをクリアします。
     */
    void Clear() {
        m_command.clear();
    }

    /**
     * @brief コマンドを取得します。
     */
    const std::string &GetCommand() const {
        return m_command;
    }

    /**
     * @brief コマンド送信をログに記録するかどうかを取得します。
     */
    bool IsOutLog() const {
        return m_isOutLog;
    }

    /**
     * @brief コマンドの区切り記号を追加します。
     */
    void AppendDelimiter() {
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
    Move GetMove() const {
        ScopedLock locker(m_guard);
        return m_move;
    }

    /**
     * @brief 指し手があるかどうかを取得します。
     */
    bool HasMove() const {
        ScopedLock locker(m_guard);
        return (!m_move.isEmpty() || !m_playedMove.isEmpty());
    }

    /**
     * @brief このクライアントの思考が開始された手を取得します。
     */
    Move GetPlayedMove() const {
        ScopedLock locker(m_guard);
        return m_playedMove;
    }

    /**
     * @brief 現局面とは別に、クライアントの思考が開始された手があるか取得します。
     */
    bool HasPlayedMove() const {
        ScopedLock locker(m_guard);
        return (m_playedMove != MOVE_NA);
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
    const std::vector<Move> &GetPVSeq() const {
        ScopedLock locker(m_guard);
        return m_pvseq;
    }

    /**
     * @brief 無視する指し手を取得します。
     */
    const std::vector<Move> &GetIgnoreMoves() const {
        ScopedLock locker(m_guard);
        return m_ignoreMoves;
    }

    /**
     * @brief コマンドを送信します。
     */
    void SendCommand(const format &fmt, bool isOutLog = true) {
        SendCommand(fmt.str(), isOutLog);
    }

    void Close();
    void BeginAsyncReceive();
    void SendCommand(const std::string &command, bool isOutLog = true);

    void InitGame();
    void ResetPosition(const min_posi_t *posi);
    void MakeRootMove(Move move, int pid, bool isActualMove=true);
    void SetPlayedMove(Move move);
    void AddIgnoreMove(Move move);

private:
    void Disconnected();
    
    void HandleAsyncReceive(const error_code &error);

    void PutSendPacket(const SendPacket &packet);
    SendPacket GetSendPacket();

    void BeginAsyncSend();
    void HandleAsyncSend(const error_code &error);

    int ParseCommand(const std::string &command);
    void SendInitGameInfo();
    std::string GetMoveHistory() const;

private:
    shared_ptr<Server> m_server;
    shared_ptr<tcp::socket> m_socket;
    mutable Mutex m_guard;

    asio::streambuf m_streambuf;
    char m_line[2048];
    int m_lineIndex;

    std::list<SendPacket> m_sendList;
    SendPacket m_sendingbuf;

    Position m_board;
    bool m_logined;
    std::string m_id;
    int m_nthreads;
    bool m_sendpv;

    int m_pid;
    Move m_move;
    Move m_playedMove;
    bool m_stable;
    bool m_final;
    long m_nodes;
    int m_value;
    std::vector<Move> m_pvseq;

    std::vector<Move> m_ignoreMoves;
};

} // namespace server
} // namespace godwhale

#endif
