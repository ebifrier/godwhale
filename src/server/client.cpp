#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "client.h"

namespace godwhale {
namespace server {

Client::Client(shared_ptr<Server> server, shared_ptr<tcp::socket> socket)
    : m_server(server), m_socket(socket), m_lineIndex(0)
    , m_logined(false), m_nthreads(0), m_sendpv(false), m_pid(0)
    , m_pidErrorCount(0), m_move(MOVE_NA), m_playedMove(MOVE_NA)
    , m_stable(false), m_final(false), m_nodes(0), m_value(0)
{
    memset(m_line, 0, sizeof(m_line));

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

    while (m_streambuf.in_avail()) {
        char ch = m_streambuf.sbumpc();
        if (ch == '\n') {
            std::string line(&m_line[0], &m_line[m_lineIndex]);
            m_lineIndex = 0;

            //LOG(Debug) << "client[" << m_id << "] recv: " << line;

            if (ParseCommand(line) < 0) {
                LOG(Error) << "parse error: " << line;
            }
            break;
        }

        m_line[m_lineIndex++] = ch;
    }

    BeginAsyncReceive();
}

/**
 * @brief コマンドリストに送信用のコマンドを追加します。
 */
void Client::PutSendPacket(const SendPacket &packet)
{
    if (packet.IsEmpty()) {
        return;
    }

    LOCK(m_guard) {
        m_sendList.push_back(packet);
    }
}

/**
 * @brief もし未送信のコマンドがあればそれをリストから削除し返します。
 */
SendPacket Client::GetSendPacket()
{
    LOCK(m_guard) {
        if (m_sendList.empty()) {
            return SendPacket();
        }

        auto packet = m_sendList.front();
        m_sendList.pop_front();
        return packet;
    }
}

/**
 * @brief コマンドを送信します。
 */
void Client::SendCommand(const std::string &command, bool isOutLog/*= true*/)
{
    PutSendPacket(SendPacket(command, isOutLog));

    // 次の送信処理に移ります。
    BeginAsyncSend();
}

/**
 * @brief 非同期の送信処理を開始します。
 */
void Client::BeginAsyncSend()
{
    LOCK(m_guard) {
        if (!m_sendingbuf.IsEmpty()) {
            return;
        }

        m_sendingbuf = GetSendPacket();
        if (m_sendingbuf.IsEmpty()) {
            return;
        }

        if (m_sendingbuf.IsOutLog()) {
            LOG(Debug) << "client[" << m_id << "] send: " << m_sendingbuf.GetCommand();
        }

        m_sendingbuf.AppendDelimiter();
        asio::async_write(
            *m_socket, asio::buffer(m_sendingbuf.GetCommand()),
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
        m_sendingbuf.Clear();
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
            m_board = m_server->GetBoard();
            m_pid = m_server->GetGid();
        }

        m_server->ClientLogined(shared_from_this());
        SendInitGameInfo();
        LOG(Notification) << "client '" << m_id << "' is logined !";
        return 0;
    }

    if (!m_logined) {
        return 0;
    }

    // PID
    if (!regex_search(command, m, PidRegex)) {
        return -1;
    }

    int pid = lexical_cast<int>(m.str(1));
    if (pid >= 0 && pid != GetPid()) {
        if (abs(pid - GetPid()) > 10) {
            LOG(Error) << "unmatch new pid:" << pid << ", pid:" << GetPid();
            m_pidErrorCount += 1;
            if (m_pidErrorCount > 5) {
                Close();
                m_pidErrorCount = 0;
            }
        }
        return 0;
    }

    // 指し手
    Move move = MOVE_NA;
    if (regex_search(command, m, MoveRegex)) {
        move = m_board.InterpretCsaMove(m.str(1));
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
    std::vector<Move> pvseq;
    if (regex_search(command, m, PVRegex)) {
        std::vector<std::string> result;
        std::string str = m.str(1);
        split(result, str, is_any_of("+- "));

        // 不要な要素を消去
        result.erase(std::remove(result.begin(), result.end(), ""), result.end());
        pvseq = m_board.InterpretCsaMoveList(result.begin(), result.end());
    }

    bool final = (command.find("final") >= 0);
    bool stable = (command.find("stable") >= 0);

    LOCK(m_guard) {
        // 念のためもう一度確認する。
        if (pid != m_pid) {
            return 0;
        }
        if (move != MOVE_NA && move != MOVE_RESIGN &&
            !m_board.IsValidMove(move)) {
            LOG(Error) << "Client::ParseCommand: '" << move << "' is invalid.";
            return 0;
        }

        if (move != MOVE_NA && nodes >= 0 && value != INT_MAX) {
            m_move   = move;
            m_nodes  = nodes;
            m_value  = value * (m_playedMove.IsEmpty() ? +1 : -1);
            m_final  = final;
            m_stable = stable;
            m_pvseq  = pvseq;

            if (!m_playedMove.IsEmpty()) {
                m_pvseq.insert(m_pvseq.begin(), m_playedMove);
            }
            else if (m_pvseq.empty()) {
                m_pvseq.push_back(move);
            }

            LOG(Notification) << "client[" << m_id << "]: updated move! "
                              << "move=" << m_move << ", nodes=" << m_nodes;
        }
        else if (nodes > 0) m_nodes = nodes;
        else return -1;
    }

    return 0;
}

/**
 * @brief 今までの指し手履歴を取得します。
 */
void Client::SendInitGameInfo()
{
    std::string name1 = record_game.str_name1;
    std::string name2 = record_game.str_name2;

    auto fmt = format("init %1% %2% %3% %4% %5% %6% %7%")
        % (name1.empty() ? "dummy1" : name1)
        % (name2.empty() ? "dummy2" : name2)
#if defined(CSA_LAN)
        % (client_turn == black ? "0" : "1")
#else
        % "0"
#endif
        % sec_limit % sec_limit_up
        % m_pid % GetMoveHistory();

    auto str = fmt.str();
    SendCommand(str);
}

/**
 * @brief 今までの指し手履歴を取得します。
 */
std::string Client::GetMoveHistory() const
{
    ScopedLock locker(m_guard);
    const auto &moveList = m_board.GetMoveList();
    std::vector<std::string> v;

    std::transform(
        moveList.begin(), moveList.end(),
        std::back_inserter(v),
        [](move_t _) { return ToString(_); });

    return algorithm::join(v, " ");
}

void Client::InitGame()
{
    ScopedLock locker(m_guard);

    m_board      = Board();
    m_pid        = 0;
    m_move       = MOVE_NA;
    m_playedMove = MOVE_NA;
    m_stable     = false;
    m_final      = false;
    m_nodes      = 0;
    m_value      = 0;
    m_pidErrorCount = 0;
    m_pvseq.clear();
    m_ignoreMoves.clear();

    SendInitGameInfo();
}

void Client::ResetPosition(const min_posi_t *posi)
{
    ScopedLock locker(m_guard);

    // 平手、初期局面以外は未対応とします。
    if (memcmp(&min_posi_no_handicap, posi, sizeof(min_posi_t)) != 0) {
        Close();
        return;
    }

    m_pid        = 0;
    m_board      = *posi;
    m_move       = MOVE_NA;
    m_playedMove = MOVE_NA;
    m_stable     = false;
    m_final      = false;
    m_nodes      = 0;
    m_value      = 0;
    m_pidErrorCount = 0;
    m_pvseq.clear();
    m_ignoreMoves.clear();

    SendCommand("new");
}

void Client::MakeRootMove(Move move, int pid, bool isActualMove/*=true*/)
{
    ScopedLock locker(m_guard);

    if (move.IsEmpty()) {
        return;
    }

    bool needAlter = false;
    if (isActualMove && m_playedMove != MOVE_NA) {
        if (m_playedMove == move) {
            m_pid = pid;
            m_move = MOVE_NA;
            m_playedMove = MOVE_NA;
            m_final  = false;
            m_stable = false;
            m_pidErrorCount = 0;
            m_pvseq.clear();
            m_ignoreMoves.clear();

            if (m_move != MOVE_NA) {
                m_pvseq.push_back(m_move);
            }

            SendCommand(format("movehit %1% %2%") % pid % move);
            return;
        }
        else {
            LOG(Debug) << m_board;
            needAlter = true;
            m_board.DoUnmove();
        }
    }

    //LOG(Debug) << "next move: " << move << ", ID:" << pid;
    //LOG(Debug) << m_board;
    if (!m_board.IsValidMove(move)) {
        LOG(Error) << "Client::MakeRootMove: '" << move << "' is invalid.";
        LOG(Error) << m_board;
        Close();
        return;
    }

    //LOG(Notification) << m_board;
    m_board.DoMove(move);
    //LOG(Notification) << m_board;

    m_pid = pid;
    m_pidErrorCount = 0;
    m_move = MOVE_NA;
    m_nodes = 0;
    m_value = 0;
    m_final = false;
    m_stable = false;
    m_pvseq.clear();
    m_ignoreMoves.clear();

    if (isActualMove) {
        m_playedMove = MOVE_NA;
    }
    else {
        m_playedMove = move;
        m_pvseq.push_back(m_playedMove);
    }

    auto fmt = format("%1% %2% %3%")
        % (needAlter ? "alter" : (isActualMove ? "move" : "pmove"))
        % move % pid;
    SendCommand(fmt.str());
}

void Client::SetPlayedMove(Move move)
{
    ScopedLock locker(m_guard);
    int pid = m_pid + 1;

    if (m_playedMove != MOVE_NA) {
        return;
    }

    if (!m_board.IsValidMove(move)) {
        LOG(Error) << "Client::SetPlayedMove: '" << move << "' is invalid.";
        Close();
        return;
    }

    // 実際に局面を進めます。
    MakeRootMove(move, pid, false);
}

void Client::AddIgnoreMove(Move move)
{
    ScopedLock locker(m_guard);

    if (move == MOVE_NA) {
        return;
    }

    auto it = std::find(m_ignoreMoves.begin(), m_ignoreMoves.end(), move);
    if (it != m_ignoreMoves.end()) {
        return;
    }

    m_ignoreMoves.push_back(move);

    std::vector<std::string> v;
    std::transform(m_ignoreMoves.begin(), m_ignoreMoves.end(),
                   std::back_inserter(v),
                   [](Move _) { return _.String(); });
    SendCommand(format("ignore %1% %2%") % GetPid() % join(v, " "));
}

} // namespace server
} // namespace godwhale
