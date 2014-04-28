#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "client.h"

namespace godwhale {
namespace server {

Client::Client(shared_ptr<Server> server, shared_ptr<tcp::socket> socket)
    : m_server(server), m_socket(socket), m_logined(false), m_nthreads(0)
    , m_sendpv(false), m_move(MOVE_NA), m_playedMove(MOVE_NA), m_stable(false)
    , m_final(false), m_nodes(0), m_value(0), m_pid(0)
{
    LOG(Notification) << m_board;
}

Client::~Client()
{
    Close();
}

void Client::Close()
{
    LOCK(m_guard) {
        m_sendList.clear();
    }

    if (m_socket->is_open()) {
        system::error_code error;
        m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
    }
}

/**
 * @brief コネクションの切断時に呼ばれます。
 */
void Client::Disconnected()
{
    LOCK(m_guard) {
        m_sendList.clear();
    }

    if (m_socket->is_open()) {
        m_socket->close();
    }

    LOG(Notification) << "Client[" << m_id << "] が切断されました。";
}

/**
 * @brief コマンドの受信処理を開始します。
 */
void Client::BeginAsyncReceive()
{
    asio::async_read_until(
        *m_socket, m_streambuf, "\n",
        bind(&Client::HandleAsyncReceive, shared_from_this(),
             asio::placeholders::error));
}

/**
 * @brief コマンド受信後に呼ばれます。
 */
void Client::HandleAsyncReceive(const system::error_code &error)
{
    if (error) {
        LOG(Error) << "command受信に失敗しました。(" << error.message() << ")";
        Disconnected();
        return;
    }

    std::istream is(&m_streambuf);
    std::string line;

    while (std::getline(is, line, '\n')) {
        LOG(Debug) << "client[" << m_id << "] recv: " << line;
        
        if (ParseCommand(line) < 0) {
            LOG(Error) << "parse error: " << line;
        }
    }

    BeginAsyncReceive();
}

/**
 * @brief コマンドリストに送信用のコマンドを追加します。
 */
void Client::PutSendCommand(const std::string &command)
{
    if (command.empty()) {
        return;
    }

    LOCK(m_guard) {
        m_sendList.push_back(command);
    }
}

/**
 * @brief もし未送信のコマンドがあればそれをリストから削除し返します。
 */
std::string Client::GetSendCommand()
{
    LOCK(m_guard) {
        if (m_sendList.empty()) {
            return std::string();
        }

        auto command = m_sendList.front();
        m_sendList.pop_front();
        return command;
    }
}

/**
 * @brief コマンドを送信します。
 */
void Client::SendCommand(const std::string &command)
{
    PutSendCommand(command);

    // 次の送信処理に移ります。
    BeginAsyncSend();
}

/**
 * @brief 非同期の送信処理を開始します。
 */
void Client::BeginAsyncSend()
{
    LOCK(m_guard) {
        if (!m_sendingbuf.empty()) {
            return;
        }

        m_sendingbuf = GetSendCommand();
        if (m_sendingbuf.empty()) {
            return;
        }

        asio::async_write(
            *m_socket, asio::buffer(m_sendingbuf),
            bind(&Client::HandleAsyncSend, shared_from_this(),
                 asio::placeholders::error));
    }
}

/**
 * @brief 非同期送信の完了後に呼ばれます。
 */
void Client::HandleAsyncSend(const system::error_code &error)
{
    if (error) {
        LOG(Error) << "command送信に失敗しました。(" << error.message() << ")";
        Disconnected();
        return;
    }
    
    LOCK(m_guard) {
        LOG(Debug) << "client[" << m_id << "] send: " << m_sendingbuf;
        m_sendingbuf.clear();
    }

    // 次の送信処理に移ります。
    BeginAsyncSend();
}


const static regex PidRegex("pid=(\\d+)");
const static regex LoginRegex("^login (\\w+) (\\d+) (\\d+)?");

const static regex MoveRegex("move=(\\d\\d\\d\\d[A-Z][A-Z])");
const static regex ToryoRegex("move=%TORYO");
const static regex NodeRegex("n=(\\d+)");
const static regex ValueRegex("v=([+-]?\\d+)");
const static regex PVRegex("pv=\\s*(([+-]?\\d\\d\\d\\d[A-Z][A-Z]\\s*)+)");

int Client::ParseCommand(const std::string &command)
{
    if (command.empty()) {
        return 0;
    }

    smatch m;

    if (regex_search(command, m, LoginRegex)) {
        LOCK(m_guard) {
            m_logined = true;
            m_id = m.str(1);
            m_nthreads = lexical_cast<int>(m.str(2));
            m_sendpv = (lexical_cast<int>(m.str(3)) != 0);
            m_pid = 0;
        }

        SendCommand("init 0\n");
        LOG(Notification) << "client '" << m_id << "' is logined !";
        return 0;
    }

    // PID
    if (!regex_search(command, m, PidRegex)) {
        return -1;
    }

    int pid = lexical_cast<int>(m.str(1));
    if (pid >= 0 && pid != GetPid()) {
        return 0;
    }

    // 指し手
    std::string moveStr;
    move_t move = MOVE_NA;
    if (regex_search(command, m, MoveRegex)) {
        moveStr = m.str(1);
        move = m_board.InterpretCsaMove(moveStr);
        assert(m_board.IsValidMove(move));
    }
    else if (regex_search(command, m, ToryoRegex)) {
        move = MOVE_RESIGN;
    }

    // ノード数
    long nodes = -1;
    if (regex_search(command, m, NodeRegex)) {
        nodes = lexical_cast<long>(m.str(1));
    }

    // 評価値
    int value = INT_MAX;
    if (regex_search(command, m, ValueRegex)) {
        value = lexical_cast<int>(m.str(1));
    }

    // PVノード
    std::vector<move_t> pvseq;
    if (!moveStr.empty() && regex_search(command, m, PVRegex)) {
        std::vector<std::string> result;
        split(result, m.str(1), is_any_of("+- "));
        result.insert(result.begin(), moveStr);

        // 不要な要素を消去
        result.erase(std::remove(result.begin(), result.end(), ""), result.end());
        pvseq = m_board.InterpretCsaMoveList(result.begin(), result.end());

        // 一番最初は指し手と同じのため削除
        pvseq.erase(pvseq.begin());
    }

    bool final = (command.find("final") >= 0);
    bool stable = (command.find("stable") >= 0);

    LOCK(m_guard) {
        // 念のためもう一度確認する。
        if (pid != m_pid) return 0;

        if (move != MOVE_NA && nodes > 0 && value != INT_MAX) {
            m_move = move;
            m_nodes = nodes;
            m_value = value;
            m_final = final;
            m_stable = stable;
            m_pvseq = pvseq;
        }
        else if (final) m_final = final;
        else if (nodes > 0) m_nodes = nodes;
        else return -1;
    }

    return 0;
}

void Client::MakeRootMove(move_t move, int pid)
{
    assert(m_board.IsValidMove(move));

    if (move == MOVE_NA) return;

    LOCK(m_guard) {
        LOG(Notification) << m_board;
        m_board.Move(move);
        LOG(Notification) << m_board;

        m_pid = pid;
        m_move = move;
        m_nodes = 0;
        m_value = 0;
        m_final = false;
        m_stable = false;
    }

    auto fmt = format("move %1% %2%\n") % str_CSA_move(move) % pid;
    SendCommand(fmt.str());
}

}
}
