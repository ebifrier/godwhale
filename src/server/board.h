
#ifndef GODWHALE_SERVER_BOARD_H
#define GODWHALE_SERVER_BOARD_H

namespace godwhale {
namespace server {

/**
 * @brief 局面の管理に使います。
 */
class Board
{
public:
    explicit Board();
    explicit Board(const Board &other);

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

    template<class Iter>
    std::vector<move_t> InterpretCsaMoveList(Iter begin, Iter end) const {
        ScopedLock lock(m_guard);
        Board tboard(*this);
        std::vector<move_t> result;

        for (; begin != end; ++begin) {
            move_t move = tboard.InterpretCsaMove(*begin);
            if (move == MOVE_NA) return result;

            tboard.Move(move);
        }

        return result;
    }

    bool IsValidMove(move_t move) const;
    void Move(move_t move);
    void UnMove(move_t move);

    move_t InterpretCsaMove(const std::string &str) const;
    void Print(std::ostream &os, move_t move=0) const;

private:
    /**
     * @brief 現在手番の持ち駒を取得します。
     */
    unsigned int &GetCurrentHand() {
        return (m_turn == black ? m_handBlack : m_handWhite);
    }

    /**
     * @brief 現在手番の持ち駒を取得します。
     */
    unsigned int GetCurrentHand() const {
        return (m_turn == black ? m_handBlack : m_handWhite);
    }

    int StrToPiece(const std::string &str, std::string::size_type index) const;
    void PrintPiece(std::ostream &os, int piece, int sq, int ito,
                    int ifrom, int is_promote) const;

private:
    mutable Mutex m_guard;
    unsigned int m_handBlack;
    unsigned int m_handWhite;
    int m_turn;
    int m_asquare[nsquare];
};

inline std::ostream &operator<<(std::ostream &os, const Board &board)
{
	board.Print(os);
	return os;
}

} // namespace server
} // namespace godwhale

#endif
