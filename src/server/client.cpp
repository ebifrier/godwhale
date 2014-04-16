#include "precomp.h"
#include "stdinc.h"

#include "server.h"
#include "client.h"

namespace godwhale {
namespace server {

Client::Client(Server *server, shared_ptr<tcp::socket> socket)
    : m_server(server), m_socket(socket)
{
}

Client::~Client()
{
    //Disconnected();
}

/**
 * @brief コネクションの切断時に呼ばれます。
 */
void Client::Disconnected()
{
    {
        ScopedLock lock(m_guard);
        m_sendList.clear();
    }

    if (m_socket != NULL) {
        system::error_code error;
        m_socket->cancel(error);
        m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
        m_socket->close();
    }

    m_server->ClientDisconnected(shared_from_this());

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

    while (std::getline(is, line)) {
        ParseCommand(line);
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

    ScopedLock lock(m_guard);
    m_sendList.push_back(command);
}

/**
 * @brief もし未送信のコマンドがあればそれをリストから削除し返します。
 */
std::string Client::GetSendCommand()
{
    ScopedLock lock(m_guard);

    if (m_sendList.empty()) {
        return std::string();
    }

    auto command = m_sendList.front();
    m_sendList.pop_front();
    return command;
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
    ScopedLock lock(m_guard);

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
    
    {
        ScopedLock lock(m_guard);
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
const static regex PVRegex("pv=(( [+-]?\\d\\d\\d\\d[A-Z][A-Z])+)");

int Client::ParseCommand(const std::string &command)
{
    if (command.empty()) {
        return 0;
    }

    smatch m;
    int pid = -1;

    if (regex_match(command, m, PidRegex)) {
        pid = lexical_cast<int>(m.str(1));
    }

    if (regex_match(command, m, LoginRegex)) {
        m_id = m.str(1);
        m_nthreads = lexical_cast<int>(m.str(2));
        m_sendpv = (lexical_cast<int>(m.str(3)) != 0);
        m_pid = 0;

        LOG(Notification) << command;
        LOG(Notification) << "init " << m_pid << " " << "";
    }
    else if (pid < 0) {
        return -1;
    }

    if (pid >= 0 && pid != m_pid) {
        return 0;
    }

    std::string move;
    if (regex_match(command, m, MoveRegex)) move = m.str(1);
    if (regex_match(command, m, ToryoRegex)) move = "%TORYO"; //MOVE_RESIGN

    int nodes = -1;
    int value = INT_MAX;
    if (regex_match(command, m, NodeRegex)) nodes = lexical_cast<int>(m.str(1));
    if (regex_match(command, m, ValueRegex)) value = lexical_cast<int>(m.str(1));

    bool final = (command.find("final") >= 0);
    bool stable = (command.find("stable") >= 0);

    if (!move.empty() && nodes > 0 && value != INT_MAX) {
        //m_move = move;
        m_nodes = nodes;
        m_value = value;
        m_final = final;
        m_stable = stable;
        return 0;
    }
    else if (final) m_final = final;
    else if (nodes > 0) m_nodes = nodes;
    else return -1;

    return 0;
}

static int StrToPiece(const std::string &str, std::string::size_type index)
{
    int i;

    for (i = 0; i < ArraySize(astr_table_piece); ++i) {
        if (str.compare(index, 2, astr_table_piece[i]) == 0) break;
    }

    if (i == 0 || i == piece_null || i == ArraySize(astr_table_piece)) {
        i = -2;
    }

    return i;
}

move_t ParseMove(const std::string &str)
{
    int ifrom_file, ifrom_rank, ito_file, ito_rank, ipiece;
    int ifrom, ito;
    move_t move;

    ifrom_file = str[0]-'0';
    ifrom_rank = str[1]-'0';
    ito_file   = str[2]-'0';
    ito_rank   = str[3]-'0';

    ito_file   = 9 - ito_file;
    ito_rank   = ito_rank - 1;
    ito        = ito_rank * 9 + ito_file;
    ipiece     = StrToPiece(str, 4);
    if (ipiece < 0) {
        return -2;
    }

    if (ifrom_file == 0 && ifrom_rank == 0) {
        return (To2Move(ito) | Drop2Move(ipiece));
    }
    else {
        ifrom_file = 9 - ifrom_file;
        ifrom_rank = ifrom_rank - 1;
        ifrom      = ifrom_rank * 9 + ifrom_file;
        /*if (abs(BOARD[ifrom]) + promote == ipiece) {
            ipiece -= promote;
            move    = FLAG_PROMO;
        }
        else {
            move = 0;
        }*/

        return (move | To2Move(ito) | From2Move(ifrom)
            /*| Cap2Move(abs(BOARD[ito]))*/ | Piece2Move(ipiece));
    }
}

}
}
