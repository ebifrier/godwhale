
#include "precomp.h"
#include "stdinc.h"
#include "util.h"

namespace godwhale {
namespace server {

std::string ToString(move_t move)
{
    int ifrom, ito, ipiece_move, is_promote;
    char str[7];

    is_promote  = (int)I2IsPromote(move);
    ipiece_move = (int)I2PieceMove(move);
    ifrom       = (int)I2From(move);
    ito         = (int)I2To(move);
  
    if (is_promote) {
        snprintf(str, 7, "%d%d%d%d%s",
                 9-aifile[ifrom], airank[ifrom]+1,
                 9-aifile[ito],   airank[ito]  +1,
                 astr_table_piece[ipiece_move + promote]);
    }
    else if (ifrom < nsquare) {
        snprintf(str, 7, "%d%d%d%d%s",
                 9-aifile[ifrom], airank[ifrom]+1,
                 9-aifile[ito],   airank[ito]  +1,
                 astr_table_piece[ipiece_move]);
    }
    else {
        snprintf(str, 7, "00%d%d%s",
                 9-aifile[ito], airank[ito]+1,
                 astr_table_piece[From2Drop(ifrom)]);
    }

    return str;
}

} // namespace server
} // namespace godwhale
