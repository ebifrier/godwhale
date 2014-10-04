
#ifndef GODWHALE_POSITION_H
#define GODWHALE_POSITION_H

#include "move.h"

namespace godwhale {

/**
 * @brief 局面の管理を行います。
 */
class Position
{
private:
    const static unsigned int HandTable[];

public:
    explicit Position();
    explicit Position(Position const & other);
    explicit Position(Position && other);
    explicit Position(min_posi_t const & posi);

    Position &operator =(Position const & other);
    Position &operator =(Position && other);
    Position &operator =(min_posi_t const & posi);

    /**
     * @brief sqに駒を設定します。
     */
    void set(int sq, int piece) {
        ScopedLock lock(m_guard);
        m_asquare[sq] = (char)piece;
    }

    /**
     * @brief sqにある駒を取得します。
     */
    int get(int sq) const {
        ScopedLock lock(m_guard);
        return m_asquare[sq];
    }

    /**
     * @brief sqにある駒を取得します。
     */
    int operator[](int sq) const {
        return get(sq);
    }

    /**
     * @brief 今までの指し手を取得します。
     */
    std::vector<move_t> const &getMoveList() const {
        ScopedLock lock(m_guard);
        return m_moveList;
    }

    /**
     * @brief 連続した指し手を解釈します。
     */
    template<class Iter>
    std::vector<Move> interpretCsaMoveList(Iter begin, Iter end) const {
        ScopedLock lock(m_guard);
        Position tmp(*this);
        std::vector<Move> result;

        for (; begin != end; ++begin) {
            Move move = tmp.interpretCsaMove(*begin);
            if (move == MOVE_NA) {
                return result;
            }
            
            if (tmp.makeMove(move) != 0) {
                return result;
            }

            result.push_back(move);
        }

        return result;
    }

    bool isValidMove(Move move) const;
    int makeMove(Move move);
    int unmakeMove();

    Move interpretCsaMove(std::string const & str) const;
    void print(std::ostream &os) const;

private:
    int strToPiece(std::string const & str, std::string::size_type index) const;
    void printPiece(std::ostream &os, int piece, int sq, int ito,
                    int ifrom, int is_promote) const;
    void printHand(std::ostream &os, unsigned int hand,
                   const std::string &prefix) const;
    void printHand0(std::ostream &os, int n, std::string const & prefix,
                    std::string const & str) const;

private:
    mutable Mutex m_guard;
    unsigned int m_hand[2];
    int m_turn;
    int m_asquare[nsquare];

    std::vector<move_t> m_moveList;
};

inline std::ostream &operator<<(std::ostream &os, const Position &pos)
{
	pos.print(os);
	return os;
}

} // namespace godwhale

#endif
