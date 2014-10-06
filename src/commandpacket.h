#ifndef GODWHALE_COMMANDPACKET_H
#define GODWHALE_COMMANDPACKET_H

#include "position.h"

namespace godwhale {

using namespace boost;

/**
 * @brief コマンド識別子です。
 *
 * コマンドとはサーバーからクライアントに送られる
 * インストラクションです。
 */
enum CommandType
{
    COMMAND_NONE,
    COMMAND_SETROOT,
    COMMAND_MAKEMOVE,
    COMMAND_UNMAKEMOVE,

    COMMAND_SETMOVELIST,
    COMMAND_SHRINK,
    COMMAND_EXTEND,
    COMMAND_VERIFY,
    COMMAND_STOP,

    COMMAND_QUIT,
};

/**
 * @brief コマンドデータをまとめて扱います。
 */
class CommandPacket
{
public:
    /// 拡張USIをパースし、コマンドに直します。
    static shared_ptr<CommandPacket> Parse(std::string const & exusi);

public:
    explicit CommandPacket(CommandType type);
    explicit CommandPacket(CommandPacket const & other);
    explicit CommandPacket(CommandPacket && other);

    /// コマンドタイプを取得します。
    CommandType getType() const
    {
        return m_type;
    }

    /// コマンドの実行優先順位を取得します。
    int getPriority() const;

    /// 局面IDを取得します。
    int getPositionId() const
    {
        return m_positionId;
    }

    /// 局面IDを設定します。
    void setPositionId(int id)
    {
        m_positionId = id;
    }

    /// 局面を取得します。
    Position const &getPosition() const
    {
        return m_position;
    }

    /// 局面を設定します。
    void setPosition(Position const & position)
    {
        m_position = position;
    }

    /// 指し手のリスト(手の一覧やＰＶなど)を取得します。
    std::vector<Move> const &getMoveList() const
    {
        return m_moveList;
    }

    /// コマンドを拡張USIに変換します。
    std::string ToExUsi();

private:
    CommandType m_type;

    int m_positionId;
    Position m_position;
    std::vector<Move> m_moveList;
};

} // namespace godwhale

#endif
