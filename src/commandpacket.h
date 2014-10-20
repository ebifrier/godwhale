#ifndef GODWHALE_COMMANDPACKET_H
#define GODWHALE_COMMANDPACKET_H

#include <boost/tokenizer.hpp>
#include "position.h"

namespace godwhale {

/**
 * @brief コマンド識別子です。
 *
 * コマンドとはサーバーからクライアントに送られる
 * インストラクションです。
 */
enum CommandType
{
    /**
     * @brief 特になし
     */
    COMMAND_NONE,
    /**
     * @brief ルート局面を設定します。
     */
    COMMAND_SETPOSITION,
    /**
     * @brief PVを設定します。
     */
    COMMAND_SETPV,
    /**
     * @brief 担当する指し手を設定します。
     */
    COMMAND_SETMOVELIST,
    /*COMMAND_SHRINK,
    COMMAND_EXTEND,*/
    /**
     * @brief サーバーとクライアントの状態が一致しているか確認します。
     */
    COMMAND_VERIFY,
    /**
     * クライアントを停止します。
     */
    COMMAND_STOP,
    /**
     * @brief クライアントを終了させます。
     */
    COMMAND_QUIT,
};

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
     * @brief α値の更新の可能性があるとき、探索時間の調整などに使います。
     */
    REPLY_RETRYED,
    /**
     * @brief α値の更新が行われたときに使います。
     */
    REPLY_VALUEUPDATED,
};

/**
 * @brief コマンドデータをまとめて扱います。
 */
class CommandPacket
{
private:
    typedef boost::char_separator<char> CharSeparator;
    typedef boost::tokenizer<CharSeparator> Tokenizer;

    static const CharSeparator ms_separator;

public:
    explicit CommandPacket(CommandType type);
    explicit CommandPacket(CommandPacket const & other);
    explicit CommandPacket(CommandPacket && other);

    /**
     * @brief コマンドタイプを取得します。
     */
    CommandType getType() const
    {
        return m_type;
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
     * @brief 局面を取得します。
     */
    Position const &getPosition() const
    {
        return m_position;
    }

    /**
     * @brief 局面を設定します。
     */
    void setPosition(Position const & position)
    {
        m_position = position;
    }

    /**
     * @brief 指し手のリスト(手の一覧やＰＶなど)を取得します。
     */
    std::vector<Move> const &getMoveList() const
    {
        return m_moveList;
    }

    int getPriority() const;

    static shared_ptr<CommandPacket> parse(std::string const & rsi);
    std::string toRsi() const;

private:
    static bool isToken(std::string const & str, std::string const & target);

    // setposition
    static shared_ptr<CommandPacket> parse_SetPosition(std::string const & rsi,
                                                       Tokenizer & tokens);
    std::string toRsi_SetPosition() const;

    // setmovelist
    static shared_ptr<CommandPacket> parse_SetMoveList(std::string const & rsi,
                                                       Tokenizer & tokens);
    std::string toRsi_SetMoveList() const;

    // stop
    static shared_ptr<CommandPacket> parse_Stop(std::string const & rsi,
                                                Tokenizer & tokens);
    std::string toRsi_Stop() const;

    // quit
    static shared_ptr<CommandPacket> parse_Quit(std::string const & rsi,
                                                Tokenizer & tokens);
    std::string toRsi_Quit() const;

private:
    CommandType m_type;

    int m_positionId;
    int m_iterationDepth;
    int m_plyDepth;
    Position m_position;
    std::vector<Move> m_moveList;
};

} // namespace godwhale

#endif
