
#ifndef GODWHALE_IOSFEN_H
#define GODWHALE_IOSFEN_H

#include "exceptions.h"
#include "board.h"
#include "move.h"

namespace godwhale {
namespace server {

extern std::string moveToSfen(Move const & move);
extern Move sfenToMove(Board const & position, std::string const & sfen);

extern std::string positionToSfen(Board const & position);
extern Board sfenToPosition(std::string const & sfen);

/**
 * @brief SFEN形式の連続した指し手文字列を、実際の指し手に変換します。
 */
template<typename Sequence>
inline std::vector<Move> sfenToMoveList(Board && position,
                                        Sequence & list)
{
    auto begin = boost::begin(list);
    auto end = boost::end(list);

    std::vector<Move> result;
    for (; begin != end; ++begin) {
        Move move = sfenToMove(position, *begin);
        if (move.isEmpty()) {
            return result;
        }
            
        if (!position.makeMove(move)) {
            return result;
        }

        result.push_back(move);
    }

    return result;
}

/**
 * @brief SFEN形式の連続した指し手文字列を、実際の指し手に変換します。
 */
template<typename Sequence>
inline std::vector<Move> sfenToMoveList(Board const & position,
                                        Sequence & list)
{
    Board tmp(position);

    return sfenToMoveList(std::move(tmp), list);
}

} // namespace server
} // namespace godwhale

#endif
