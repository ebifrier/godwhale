
#ifndef GODWHALE_IOCSA_H
#define GODWHALE_IOCSA_H

#include "position.h"
#include "move.h"

namespace godwhale {

//extern std::string moveToCsa(Move const & move);
extern Move csaToMove(Position const & position, std::string const & csa);

/**
 * @brief CSA形式の連続した指し手文字列を、実際の指し手に変換します。
 */
template<class It>
std::vector<Move> csaToMoveList(Position && position,
                                It begin, It end)
{
    std::vector<Move> result;

    for (; begin != end; ++begin) {
        Move move = csaToMove(position, *begin);
        if (move.isEmpty()) {
            return result;
        }
            
        if (position.makeMove(move) != 0) {
            return result;
        }

        result.push_back(move);
    }

    return result;
}

/**
 * @brief CSA形式の連続した指し手文字列を、実際の指し手に変換します。
 */
template<typename It>
inline std::vector<Move> sfenToMoveList(Position const & position,
                                        It begin, It end)
{
    Position tmp(position);

    return csaToMoveList(std::move(tmp), begin, end);
}

} // namespace godwhale

#endif
