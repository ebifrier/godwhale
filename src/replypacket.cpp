#include "precomp.h"
#include "stdinc.h"
#include "replypacket.h"

namespace godwhale {

const ReplyPacket::CharSeparator ReplyPacket::ms_separator(" ", "");

ReplyPacket::ReplyPacket(ReplyType type)
    : m_type(type), m_positionId(-1), m_iterationDepth(-1), m_plyDepth(-1)
{
}

/**
 * @brief コマンドの実行優先順位を取得します。
 *
 * 値が大きい方が、優先順位は高いです。
 */
int ReplyPacket::getPriority() const
{
    switch (m_type) {
    // 終了系のコマンドはすぐに実行
    //case COMMAND_QUIT:
    //    return 100;

    // 通常のコマンドはそのままの順で
    case REPLY_LOGIN:
        return 50;

    // エラーみたいなもの
    case REPLY_NONE:
        return 0;
    }

    unreachable();
    return -1;
}

/**
 * @brief strがtargetが示すトークンであるか調べます。
 */
bool ReplyPacket::isToken(std::string const & str, std::string const & target)
{
    return (str.compare(target) == 0);
}

/**
 * @brief RSI(remote shogi interface)をパースし、コマンドに直します。
 */
shared_ptr<ReplyPacket> ReplyPacket::parse(std::string const & rsi)
{
    if (rsi.empty()) {
        throw new std::invalid_argument("rsi");
    }

    Tokenizer tokens(rsi, ms_separator);
    std::string token = *tokens.begin();

    if (isToken(token, "login")) {
        return parse_Login(rsi, tokens);
    }

    return shared_ptr<ReplyPacket>();
}

/**
 * @brief コマンドをRSI(remote shogi interface)に変換します。
 */
std::string ReplyPacket::toRSI() const
{
    assert(m_type != REPLY_NONE);

    switch (m_type) {
    case REPLY_LOGIN:
        return toRSI_Login();
    }

    unreachable();
    return std::string();
}


#pragma region Login
/**
 * @brief loginコマンドをパースします。
 *
 * login <address> <port> <login_id> <nthreads>
 */
shared_ptr<ReplyPacket> ReplyPacket::parse_Login(std::string const & rsi,
                                                     Tokenizer & tokens)
{
    shared_ptr<ReplyPacket> result(new ReplyPacket(REPLY_LOGIN));
    Tokenizer::iterator begin = ++tokens.begin();

    result->m_loginName = *begin++;
    result->m_threadSize = lexical_cast<int>(*begin++);
    return result;
}

/**
 * @brief loginコマンドをRSIに変換します。
 */
std::string ReplyPacket::toRSI_Login() const
{
    return (F("login %1% %2%")
        % m_loginName % m_threadSize)
        .str();
}
#pragma endregion

} // namespace godwhale
