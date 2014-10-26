#ifndef GODWHALE_REPLYPACKET_H
#define GODWHALE_REPLYPACKET_H

#include <boost/tokenizer.hpp>
#include "position.h"

namespace godwhale {

/**
 * @brief 返答コマンドの識別子です。
 *
 * 返答コマンドとはクライアントからサーバーに送られる
 * インストラクションです。
 */
enum ReplyType
{
    /**
     * @brief 特になし。
     */
    REPLY_NONE,
    /**
     * @brief ログイン用コマンド
     *
     * login <name> <thread-num>
     */
    REPLY_LOGIN,
    /**
     * @brief α値の更新の可能性があるとき、探索時間の調整などに使います。
     *
     * retryed <position_id> <itd> <pld> <move>
     */
    REPLY_RETRYED,
    /**
     * @brief α値の更新が行われたときに使います。
     *
     * valueupdated <position_id> <itd> <pld> <move> <value> <ule>
     */
    REPLY_VALUEUPDATED,
};

/**
 * @brief 返答コマンドデータを扱います。
 */
class ReplyPacket : private boost::noncopyable
{
private:
    typedef boost::char_separator<char> CharSeparator;
    typedef boost::tokenizer<CharSeparator> Tokenizer;

    static const CharSeparator ms_separator;

public:
    explicit ReplyPacket(ReplyType type);

    /**
     * @brief コマンドタイプを取得します。
     */
    ReplyType getType() const
    {
        return m_type;
    }

    /**
     * @brief ログインＩＤを取得します。
     */
    std::string getLoginId() const
    {
        return m_loginId;
    }

    int getThreadSize() const
    {
        return m_threadSize;
    }

    /**
     * @brief 局面IDを取得します。
     */
    int getPositionId() const
    {
        return m_positionId;
    }

    /**
     * @brief 局面IDを設定します。
     */
    void setPositionId(int id)
    {
        m_positionId = id;
    }

    /**
     * @brief 反復深化の探索深さを取得します。
     */
    int getIterationDepth() const
    {
        return m_iterationDepth;
    }

    /**
     * @brief ルート局面から思考局面までの手の深さを取得します。
     */
    int getPlyDepth() const
    {
        return m_plyDepth;
    }

    /**
     * @brief 指し手を取得します。
     */
    Move getMove() const
    {
        return m_move;
    }

    /**
     * @brief 指し手のリスト(手の一覧やＰＶなど)を取得します。
     */
    std::vector<Move> const &getPVList() const
    {
        return m_moveList;
    }    

public:
    int getPriority() const;

    static shared_ptr<ReplyPacket> parse(std::string const & rsi);
    std::string toRSI() const;

private:
    static bool isToken(std::string const & str, std::string const & target);

    // login
    static shared_ptr<ReplyPacket> parse_Login(std::string const & rsi,
                                               Tokenizer & tokens);
    std::string toRSI_Login() const;

private:
    ReplyType m_type;

    std::string m_loginId;
    int m_threadSize;

    int m_positionId;
    int m_iterationDepth;
    int m_plyDepth;

    Move m_move;
    std::vector<Move> m_moveList;
};

} // namespace godwhale

#endif
