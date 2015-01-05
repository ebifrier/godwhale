#ifndef GODWHALE_SERVER_SERVERCOMMAND_H
#define GODWHALE_SERVER_SERVERCOMMAND_H

#include "position.h"

namespace godwhale {
namespace server {

/**
 * @brief サーバー内のコマンドタイプです。
 */
enum ServerCommandType
{
    SERVERCOMMAND_SETPOSITION,
    SERVERCOMMAND_BEGINGAME,
    SERVERCOMMAND_ENDGAME,
    SERVERCOMMAND_MAKEMOVEROOT,
    SERVERCOMMAND_UNMAKEMOVEROOT,
};

/**
 * @brief サーバー内のコマンドです。
 */
class ServerCommand
{
public:
    explicit ServerCommand(ServerCommandType type)
        : m_type(type)
    {
    }

    /**
     * @brief コマンドタイプを取得します。
     */
    ServerCommandType getType() const
    {
        return m_type;
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
     * @brief 指し手を取得します。
     */
    Move getMove() const
    {
        return m_move;
    }

    /**
     * @brief 指し手を設定します。
     */
    void setMove(Move move)
    {
        m_move = move;
    }

private:
    ServerCommandType m_type;
    Position m_position;
    Move m_move;
};

} // namespace server
} // namespace godwhale

#endif
