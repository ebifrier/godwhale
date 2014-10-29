#ifndef GODWHALE_COMMANDPACKET_H
#define GODWHALE_COMMANDPACKET_H

#include <boost/tokenizer.hpp>
#include "position.h"

namespace godwhale {

/**
 * @brief コマンド識別子です。
 *
 * コマンドとは
 * 1, サーバーからクライアントに送られた
 * 2, stdinから受信された
 * 命令インストラクションです。
 */
enum CommandType
{
    /**
     * @brief 特になし
     */
    COMMAND_NONE,
    /**
     * @brief (stdin only)サーバーにログイン処理を依頼します。
     */
    COMMAND_LOGIN,
    /**
     * @brief ログイン処理の結果をもらいます。
     */
    COMMAND_LOGINRESULT,
    /**
     * @brief ルート局面を設定します。
     */
    COMMAND_SETPOSITION,
    /**
     * @brief ルート局面から指し手を一つ進めます。
     */
    COMMAND_MAKEMOVEROOT,
    /**
     * @brief PVを設定します。
     */
    COMMAND_SETPV,
    /**
     * @brief 担当する指し手を設定します。
     */
    COMMAND_SETMOVELIST,
    /**
     * @brief 指定の局面の探索を開始します。
     */
    COMMAND_START,
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
     * @brief 更新前の局面IDを取得します。
     */
    int getOldPositionId() const
    {
        return m_oldPositionId;
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
     * @brief 指し手を取得します。
     */
    Move getMove() const
    {
        return m_move;
    }

    /**
     * @brief 指し手のリスト(手の一覧やＰＶなど)を取得します。
     */
    std::vector<Move> const &getMoveList() const
    {
        return m_moveList;
    }

    /**
     * @brief 探索のα値を取得します。
     */
    int getAlpha() const
    {
        return m_alpha;
    }

    /**
     * @brief 探索のβ値を取得します。
     */
    int getBeta() const
    {
        return m_beta;
    }

public:
    /* for Login */

    /**
     * @brief 接続先のサーバーアドレスを取得します。
     */
    std::string getServerAddress() const
    {
        return m_serverAddress;
    }

    /**
     * @brief 接続先のサーバーポートを取得します。
     */
    std::string getServerPort() const
    {
        return m_serverPort;
    }

    /**
     * @brief ログイン名を取得します。
     */
    std::string getLoginName() const
    {
        return m_loginName;
    }

    /**
     * @brief ログイン結果を取得します。
     */
    /*int getLoginResult() const
    {
        return m_loginResult;
    }*/

public:
    int getPriority() const;
    std::string toRSI() const;

    //static shared_ptr<CommandPacket> createLogin();
    static shared_ptr<CommandPacket> createSetMoveList(int positionId,
                                                       int iterationDepth,
                                                       int plyDepth,
                                                       std::vector<Move> const & moves);
    static shared_ptr<CommandPacket> createStart(int positionId,
                                                 int iterationDepth,
                                                 int plyDepth,
                                                 int alpha, int beta);

    static shared_ptr<CommandPacket> parse(std::string const & rsi);

private:
    static bool isToken(std::string const & str, std::string const & target);

    // login
    static shared_ptr<CommandPacket> parse_Login(std::string const & rsi,
                                                 Tokenizer & tokens);
    std::string toRSI_Login() const;

    // loginresult
    /*static shared_ptr<CommandPacket> parse_Login(std::string const & rsi,
                                                 Tokenizer & tokens);
    std::string toRSI_Login() const;*/

    // setposition
    static shared_ptr<CommandPacket> parse_SetPosition(std::string const & rsi,
                                                       Tokenizer & tokens);
    std::string toRSI_SetPosition() const;

    // makerootmove
    static shared_ptr<CommandPacket> parse_MakeMoveRoot(std::string const & rsi,
                                                        Tokenizer & tokens);
    std::string toRSI_MakeMoveRoot() const;

    // setmovelist
    static shared_ptr<CommandPacket> parse_SetMoveList(std::string const & rsi,
                                                       Tokenizer & tokens);
    std::string toRSI_SetMoveList() const;

    // start
    static shared_ptr<CommandPacket> parse_Start(std::string const & rsi,
                                                 Tokenizer & tokens);
    std::string toRSI_Start() const;

    // stop
    static shared_ptr<CommandPacket> parse_Stop(std::string const & rsi,
                                                Tokenizer & tokens);
    std::string toRSI_Stop() const;

    // quit
    static shared_ptr<CommandPacket> parse_Quit(std::string const & rsi,
                                                Tokenizer & tokens);
    std::string toRSI_Quit() const;

private:
    CommandType m_type;

    int m_positionId;
    int m_oldPositionId;
    int m_iterationDepth;
    int m_plyDepth;
    Position m_position;

    int m_alpha;
    int m_beta;

    std::string m_serverAddress;
    std::string m_serverPort;
    std::string m_loginName;

    Move m_move;
    std::vector<Move> m_moveList;
};

} // namespace godwhale

#endif
