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

extern int popCount(int x);
extern void initBonanzaSearch();

} // namespace godwhale

#endif
