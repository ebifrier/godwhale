
#ifndef GODWHALE_SERVER_MOVE_H
#define GODWHALE_SERVER_MOVE_H

namespace godwhale {
namespace server {

/**
 * @brief 指し手を扱うためのクラスです。
 */
class Move
{
public:
    static Move Make(int from, int to, int piece, int capture, bool isPromote) {
        return Move(
            From2Move(from) | To2Move(to) | Piece2Move(piece) |
            Cap2Move(capture) | ((int)isPromote << 14));
    }

    static Move MakeDrop(int to, int piece) {
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
        return Get();
    }

    /**
     * @brief 指し手が設定されているかどうかを調べます。
     */
    bool IsEmpty() const {
        return (m_move == MOVE_NA);
    }

    /**
     * @brief move_t型の値を取得します。
     */
    move_t Get() const {
        return m_move;
    }

    /**
     * @brief 指し手の移動先（駒打ちの場合は打つ場所）のマスを取得します。
     */
    int GetTo() const {
        return I2To(m_move);
    }

    /**
     * @brief 指し手の移動元を取得します。駒打ちの場合は無効です。
     */
    int GetFrom() const {
        return I2From(m_move);
    }

    /**
     * @brief 指し手の移動元と移動先を合わせて取得します。
     */
    int GetFromTo() const {
        return I2FromTo(m_move);
    }

    /**
     * @brief 駒打ちかどうかを調べます。
     */
    bool IsDrop() const {
        return (GetFrom() >= nsquare);
    }

    /**
     * @brief 駒が成るかどうかを調べます。
     */
    bool IsPromote() const {
        return (I2IsPromote(m_move) != 0);
    }

    /**
     * @brief 動かした駒を取得します。
     */
    int GetPiece() const {
        assert(!IsDrop());
        return I2PieceMove(m_move);
    }

    /**
     * @brief 打った駒を取得します。
     */
    int GetDrop() const {
        assert(IsDrop());
        return From2Drop(GetFrom());
    }

    /**
     * @brief 取った駒の種類(成り不成りも含む)を取得します。
     */
    int GetCapture() const {
        return UToCap(m_move);
    }

    /**
     * @brief 指し手を分かりやすいように文字列化します。
     */
    std::string String() const {
        return std::move(ToString(m_move));
    }

private:
    move_t m_move;
};

inline std::ostream &operator<<(std::ostream &stream, Move move)
{
    stream << move.String();
    return stream;
}

} // namespace server
} // namespace godwhale

#endif
