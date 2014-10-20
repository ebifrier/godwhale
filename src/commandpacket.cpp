#include "precomp.h"
#include "stdinc.h"
#include "io_sfen.h"
#include "commandpacket.h"

namespace godwhale {

const CommandPacket::CharSeparator CommandPacket::ms_separator(" ", "");

CommandPacket::CommandPacket(CommandType type)
    : m_type(type), m_positionId(-1), m_position(false)
{
}

CommandPacket::CommandPacket(CommandPacket const & other)
    : m_positionId(-1), m_position(false)
{
}

CommandPacket::CommandPacket(CommandPacket && other)
    : m_positionId(-1), m_position(false)
{
}

/**
 * @brief コマンドの実行優先順位を取得します。
 *
 * 値が大きい方が、優先順位は高いです。
 */
int CommandPacket::getPriority() const
{
    switch (m_type) {
    // 終了系のコマンドはすぐに実行
    case COMMAND_QUIT:
    case COMMAND_STOP:
        return 100;

    // 通常のコマンドはそのままの順で
    case COMMAND_SETPOSITION:
    case COMMAND_SETPV:
    case COMMAND_SETMOVELIST:
    case COMMAND_VERIFY:
        return 50;

    // エラーみたいなもの
    case COMMAND_NONE:
        return 0;
    }

    unreachable();
    return -1;
}

/**
 * @brief strがtargetが示すトークンであるか調べます。
 */
bool CommandPacket::isToken(std::string const & str, std::string const & target)
{
    return (str.compare(target) == 0);
}

/**
 * @brief RSI(remote shogi interface)をパースし、コマンドに直します。
 */
shared_ptr<CommandPacket> CommandPacket::parse(std::string const & rsi)
{
    if (rsi.empty()) {
        throw new std::invalid_argument("rsi");
    }

    Tokenizer tokens(rsi, ms_separator);
    std::string token = *tokens.begin();

    if (isToken(token, "setposition")) {
        return parse_SetPosition(rsi, tokens);
    }
    else if (isToken(token, "stop")) {
        return parse_Stop(rsi, tokens);
    }
    else if (isToken(token, "quit")) {
        return parse_Quit(rsi, tokens);
    }

    return shared_ptr<CommandPacket>();
}

/**
 * @brief コマンドをRSI(remote shogi interface)に変換します。
 */
std::string CommandPacket::toRsi() const
{
    assert(m_type != COMMAND_NONE);

    switch (m_type) {
    case COMMAND_SETPOSITION:
        return toRsi_SetPosition();
    case COMMAND_STOP:
        return toRsi_Stop();
    case COMMAND_QUIT:
        return toRsi_Quit();
    }

    unreachable();
    return std::string();
}


#pragma region SetPosition
/**
 * @brief setpositionコマンドをパースします。
 *
 * setposition <position_id> [sfen <sfen> | startpos] moves <move1> ... <moven>
 * <sfen> = <board_sfen> <turn_sfen> <hand_sfen> <nmoves>
 */
shared_ptr<CommandPacket> CommandPacket::parse_SetPosition(std::string const & rsi,
                                                           Tokenizer & tokens)
{
    shared_ptr<CommandPacket> result(new CommandPacket(COMMAND_SETPOSITION));
    Tokenizer::iterator begin = ++tokens.begin();

    result->m_positionId = lexical_cast<int>(*begin++);

    std::string token = *begin++;
    if (token == "sfen") {
        std::string sfen;
        sfen += *begin++ + " "; // board
        sfen += *begin++ + " "; // turn
        sfen += *begin++ + " "; // hand
        sfen += *begin++;       // nmoves
        result->m_position = sfenToPosition(sfen);
    }
    else if (token == "startpos") {
        result->m_position = Position();
    }

    // movesはないことがあります。
    if (begin == tokens.end()) {
        return result;
    }

    if (*begin++ != "moves") {
        throw new ParseException(F("%1%: 指し手が正しくありません。") % rsi);
    }

    for (; begin != tokens.end(); ++begin) {
        Move move = sfenToMove(result->m_position, *begin);

        if (result->m_position.makeMove(move) != 0) {
            LOG_ERROR() << result->m_position;
            LOG_ERROR() << *begin << ": 指し手が正しくありません。";
            throw new ParseException(F("%1%: 指し手が正しくありません。") % *begin);
        }
    }

    return result;
}

/**
 * @brief setpositionコマンドをRSIに変換します。
 */
std::string CommandPacket::toRsi_SetPosition() const
{
    Position position = m_position;
    std::vector<Move> moves = position.getMoveList();

    // 局面をすべて戻します。
    while (!position.getMoveList().empty()) {
        position.unmakeMove();
    }

    std::string posStr;
    if (position.isInitial()) {
        posStr = " startpos";
    }
    else {
        posStr  = " sfen ";
        posStr += positionToSfen(position);
    }

    std::string movesStr;
    if (!moves.empty()) {
        movesStr += " moves";
        BOOST_FOREACH(Move move, moves) {
            movesStr += " ";
            movesStr += moveToSfen(move);
        }
    }

    return (F("setposition %1%%2%%3%")
        % m_positionId % posStr % movesStr)
        .str();
}
#pragma endregion


#pragma region SetMoveList
/**
 * @brief setmovelistコマンドをパースします。
 *
 * setmovelist <position_id> <itd> <pld> <move1> ... <moven>
 */
shared_ptr<CommandPacket> CommandPacket::parse_SetMoveList(std::string const & rsi,
                                                           Tokenizer & tokens)
{
    shared_ptr<CommandPacket> result(new CommandPacket(COMMAND_SETMOVELIST));
    Tokenizer::iterator begin = ++tokens.begin(); // 最初のトークンは飛ばします。

    result->m_positionId = lexical_cast<int>(*begin++);
    result->m_iterationDepth = lexical_cast<int>(*begin++);
    result->m_plyDepth = lexical_cast<int>(*begin++);

    // 与えられたpositionIdなどから局面を検索します。
    Position position;

    Tokenizer::iterator end = tokens.end();
    for (; begin != end; ++begin) {
        Move move = sfenToMove(position, *begin);

        if (move.isEmpty()) {
            throw ParseException(F("%1%: 正しい指し手ではありません。") % *begin);
        }

        result->m_moveList.push_back(move);
    }

    return result;
}

/**
 * @brief setmovelistコマンドをRSIに変換します。
 */
std::string CommandPacket::toRsi_SetMoveList() const
{
    std::string movesStr;
    BOOST_FOREACH(Move move, m_moveList) {
        movesStr += " ";
        movesStr += moveToSfen(move);
    }

    return (F("setmovelist %1% %2% %3% %4%")
        % m_positionId %m_iterationDepth % m_plyDepth % movesStr)
        .str();
}
#pragma endregion


#pragma region Stop
/**
 * @brief stopコマンドをパースします。
 */
shared_ptr<CommandPacket> CommandPacket::parse_Stop(std::string const & rsi,
                                                    Tokenizer & tokens)
{
    return shared_ptr<CommandPacket>(new CommandPacket(COMMAND_STOP));
}

/**
 * @brief stopコマンドをRSIに変換します。
 */
std::string CommandPacket::toRsi_Stop() const
{
    return "stop";
}
#pragma endregion


#pragma region Quit
/**
 * @brief quitコマンドをパースします。
 */
shared_ptr<CommandPacket> CommandPacket::parse_Quit(std::string const & rsi,
                                                    Tokenizer & tokens)
{
    return shared_ptr<CommandPacket>(new CommandPacket(COMMAND_QUIT));
}

/**
 * @brief quitコマンドをRSIに変換します。
 */
std::string CommandPacket::toRsi_Quit() const
{
    return "quit";
}
#pragma endregion

} // namespace godwhale
