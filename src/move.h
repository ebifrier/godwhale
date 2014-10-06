
#ifndef GODWHALE_MOVE_H
#define GODWHALE_MOVE_H

namespace godwhale {

/**
 * @brief bonanzaの指し手を文字列に変換します。
 */
extern std::string toString(move_t move, int turn = turn_none);

inline int SQ(int file, int rank)
{
    return ((rank-1) * 9 + (9-file));
}

/**
 * @brief 指し手を扱うためのクラスです。
 */
class Move
{
public:
    /// 駒を移動するための指し手を作成します。
    static Move create(int from, int to, int piece, int capture, bool isPromote) {
        return Move(
            From2Move(from) | To2Move(to) | Piece2Move(piece) |
            Cap2Move(capture) | ((int)isPromote << 14));
    }

    /// 駒を打つための指し手を作成します。
    static Move createDrop(int to, int piece) {
        return Move(To2Move(to) | Drop2Move(piece));
    }

    Move() : m_move(MOVE_NA) {
    }

    Move(move_t move) : m_move(move) {
    }

    Move(const Move &move) : m_move(move.m_move) {
    }

    Move &operator =(const Move &move) {
        m_move = move.m_move;
        return *this;
    }

    Move &operator =(move_t move) {
        m_move = move;
        return *this;
    }

    /**
     * @brief move_t型へのキャスト用オペレータ
     */
    operator move_t() const {
        return get();
    }

    /**
     * @brief 指し手が設定されているかどうかを調べます。
     */
    bool isEmpty() const {
        return (m_move == MOVE_NA);
    }

    /**
     * @brief move_t型の値を取得します。
     */
    move_t get() const {
        return m_move;
    }

    /**
     * @brief 指し手の移動先（駒打ちの場合は打つ場所）のマスを取得します。
     */
    int getTo() const {
        return I2To(m_move);
    }

    /**
     * @brief 指し手の移動先（駒打ちの場合は打つ場所）の筋を取得します。
     */
    int getToFile() const {
        return (9 - (getTo() % 9));
    }

    /**
     * @brief 指し手の移動先（駒打ちの場合は打つ場所）の段を取得します。
     */
    int getToRank() const {
        return ((getTo() / 9) + 1);
    }

    /**
     * @brief 指し手の移動元を取得します。駒打ちの場合は無効です。
     */
    int getFrom() const {
        return I2From(m_move);
    }

    /**
     * @brief 指し手の移動元の筋を取得します。
     */
    int getFromFile() const {
        return (9 - (getFrom() % 9));
    }

    /**
     * @brief 指し手の移動元の段を取得します。
     */
    int getFromRank() const {
        return ((getFrom() / 9) + 1);
    }

    /**
     * @brief 指し手の移動元と移動先を合わせて取得します。
     */
    int getFromTo() const {
        return I2FromTo(m_move);
    }

    /**
     * @brief 駒打ちかどうかを調べます。
     */
    bool isDrop() const {
        return (getFrom() >= nsquare);
    }

    /**
     * @brief 駒が成るかどうかを調べます。
     */
    bool isPromote() const {
        return (I2IsPromote(m_move) != 0);
    }

    /**
     * @brief 動かした駒を取得します。
     */
    int getPiece() const {
        assert(!isDrop());
        return I2PieceMove(m_move);
    }

    /**
     * @brief 打った駒を取得します。
     */
    int getDrop() const {
        assert(isDrop());
        return From2Drop(getFrom());
    }

    /**
     * @brief 取った駒の種類(成り不成りも含む)を取得します。
     */
    int getCapture() const {
        return UToCap(m_move);
    }

    /**
     * @brief 指し手を分かりやすいように文字列化します。
     */
    std::string str(int turn = turn_none) const {
        return std::move(toString(m_move, turn));
    }

private:
    move_t m_move;
};

inline std::ostream &operator<<(std::ostream &stream, Move move)
{
    stream << move.str();
    return stream;
}

} // namespace godwhale

#endif
