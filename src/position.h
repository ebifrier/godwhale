
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
    explicit Position(bool init=true);
    explicit Position(min_posi_t const & posi);
    Position(Position const & other);
    Position(Position && other);

    Position &operator =(Position const & other);
    Position &operator =(Position && other);
    Position &operator =(min_posi_t const & posi);

    friend bool operator==(Position const & lhs, Position const & rhs);
    friend bool operator!=(Position const & lhs, Position const & rhs)
    {
        return !(lhs == rhs);
    }

    /**
     * @brief sqにある駒を取得します。
     */
    int get(int sq) const
    {
        return m_asquare[sq];
    }

    /**
     * @brief sqにある駒を取得します。
     */
    int get(int file, int rank) const
    {
        return get(SQ(file, rank));
    }

    /**
     * @brief sqに駒を設定します。
     */
    void set(int sq, int piece)
    {
        m_asquare[sq] = (char)piece;
    }

    /**
     * @brief sqに駒を設定します。
     */
    void set(int file, int rank, int piece)
    {
        set(SQ(file, rank), piece);
    }

    /**
     * @brief sqにある駒を取得します。
     */
    int operator[](int sq) const
    {
        return get(sq);
    }

    /**
     * @brief 手番を取得します。
     */
    int getTurn() const
    {
        return m_turn;
    }

    /**
     * @brief 手番を設定します。
     */
    void setTurn(int turn)
    {
        m_turn = turn;
    }

    /**
     * @brief 今までの指し手を取得します。
     */
    std::vector<Move> const &getMoveList() const
    {
        return m_moveList;
    }

    min_posi_t getMinPosi() const;

    int getHand(int turn, int piece) const;
    void setHand(int turn, int piece, int count);

    bool isValidMove(Move move) const;
    bool isEmpty() const;
    bool isInitial() const;

    bool makeMove(Move move);
    bool unmakeMove();

    void print(std::ostream &os) const;

private:
    void printPiece(std::ostream &os, int piece, int sq, int ito,
                    int ifrom, int is_promote) const;
    void printHand(std::ostream &os, unsigned int hand,
                   const std::string &prefix) const;
    void printHand0(std::ostream &os, int n, std::string const & str) const;

private:
    unsigned int m_hand[2];
    int m_turn;
    std::vector<int> m_asquare;

    std::vector<Move> m_moveList;
};

inline std::ostream &operator<<(std::ostream &os, const Position &pos)
{
	pos.print(os);
	return os;
}

} // namespace godwhale

#endif
