#ifndef GODWHALE_UTIL_H
#define GODWHALE_UTIL_H

namespace godwhale {

const char *PieceStrTable[];
const char *TurnStrTable[];

/**
 * @brief turnをnmoves回フリップします。
 */
inline int countFlip(int turn, int nmoves)
{
    return ((nmoves % 2) ? Flip(turn) : turn);
}

inline std::string toString(int number)
{
    std::ostringstream oss;
    oss << number;
    return oss.str();
}

template<typename Sequence>
inline std::string moveListToString(Sequence && moveList, int turn = turn_none,
                                    bool flip = true)
{
    std::string result;

    BOOST_FOREACH (auto move, moveList) {
        result += move.str(turn);

        if (turn != turn_none && flip) {
            turn = Flip(turn);
        }
    }

    return result;
}

extern int popCount(int x);
extern void sleep(int milliseconds);
extern std::string ToString(move_t move);

} // namespace godwhale

#endif
