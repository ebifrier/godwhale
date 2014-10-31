
#ifndef GODWHALE_SERVER_BOARD_H
#define GODWHALE_SERVER_BOARD_H

#include "move.h"

namespace godwhale {
namespace server {

const unsigned int HandTable[] =
{
    0,                flag_hand_pawn, flag_hand_lance,  flag_hand_knight,
    flag_hand_silver, flag_hand_gold, flag_hand_bishop, flag_hand_rook,
    0,                flag_hand_pawn, flag_hand_lance,  flag_hand_knight,
    flag_hand_silver, 0,              flag_hand_bishop, flag_hand_rook,
};

/**
 * @brief 局面の管理に使います。
 */
class Board
{
public:
    explicit Board(bool init = true);
    Board(const Board &other);
    explicit Board(const min_posi_t &posi);

    Board &operator =(const Board &other);
    Board &operator =(const min_posi_t &posi);

    friend bool operator==(Board const & lhs, Board const & rhs);
    friend bool operator!=(Board const & lhs, Board const & rhs)
    {
        return !(lhs == rhs);
    }

    /**
     * @brief sqに駒を設定します。
     */
    void Set(int sq, int piece) {
        ScopedLock lock(m_guard);
        m_asquare[sq] = (char)piece;
    }

    /**
     * @brief sqにある駒を取得します。
     */
    int Get(int sq) const {
        ScopedLock lock(m_guard);
        return m_asquare[sq];
    }

    /**
     * @brief sqにある駒を取得します。
     */
    int operator[](int sq) const {
        return Get(sq);
    }

    /**
     * @brief 手番を取得します。
     */
    int GetTurn() const {
        return m_turn;
    }

    /**
     * @brief 手番を設定します。
     */
    void SetTurn(int turn) {
        m_turn = turn;
    }

    /**
     * @brief 今までの指し手を取得します。
     */
    const std::vector<move_t> &GetMoveList() const {
        ScopedLock lock(m_guard);
        return m_moveList;
    }

    /**
     * @brief 連続した指し手を解釈します。
     */
    template<class Iter>
    std::vector<Move> InterpretCsaMoveList(Iter begin, Iter end) const {
        ScopedLock lock(m_guard);
        Board tboard(*this);
        std::vector<Move> result;

        for (; begin != end; ++begin) {
            Move move = tboard.InterpretCsaMove(*begin);
            if (move == MOVE_NA) {
                return result;
            }
            
            if (tboard.DoMove(move) != 0) {
                return result;
            }

            result.push_back(move);
        }

        return result;
    }

    min_posi_t getMinPosi() const;
    int GetHand(int turn, int piece) const;
    void SetHand(int turn, int piece, int count);

    bool IsValidMove(Move move) const;
    int DoMove(Move move);
    int DoUnmove();

    Move InterpretCsaMove(const std::string &str) const;
    void Print(std::ostream &os) const;

private:
    unsigned int GetCurrentHand() const;
    void AddCurrentHand(int capture);
    void SubCurrentHand(int capture);

    int StrToPiece(const std::string &str, std::string::size_type index) const;
    void PrintPiece(std::ostream &os, int piece, int sq, int ito,
                    int ifrom, int is_promote) const;
    void PrintHand(std::ostream &os, unsigned int hand,
                   const std::string &prefix) const;
    void PrintHand(std::ostream &os, int n, const std::string &prefix,
                   const std::string &str) const;

private:
    mutable Mutex m_guard;
    unsigned int m_handBlack;
    unsigned int m_handWhite;
    int m_turn;
    int m_asquare[nsquare];

    std::vector<move_t> m_moveList;
};

inline std::ostream &operator<<(std::ostream &os, const Board &board)
{
	board.Print(os);
	return os;
}

} // namespace server
} // namespace godwhale

#endif
