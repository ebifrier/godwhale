#include "precomp.h"
#include "stdinc.h"
#include "move.h"

namespace godwhale {

const char *PieceStrTable[16]  = {
    "ÅZ", "ï‡", "çÅ", "åj", "ã‚", "ã‡",
    "äp", "îÚ", "â§", "Ég", "à«", "NK",
    "NG", "Å~", "în", "ó≥"
};

const char *TurnStrTable[] =
{
    "Å£", "Å¢", ""
};

/**
 * @brief éwÇµéËÇï™Ç©ÇËÇ‚Ç∑Ç¢ÇÊÇ§Ç…ï∂éöóÒâªÇµÇ‹Ç∑ÅB
 */
std::string toString(move_t move, int turn/*=turn_none*/)
{
    char str[32];

    const char *turnStr = TurnStrTable[turn];
    int is_promote  = (int)I2IsPromote(move);
    int ipiece_move = (int)I2PieceMove(move);
    int ifrom       = (int)I2From(move);
    int ito         = (int)I2To(move);

    if (is_promote) {
        // ó·) 87ã‚ê¨(86)
        snprintf(str, sizeof(str), "%s%d%d%sê¨(%d%d)",
                 turnStr,
                 9-aifile[ito],   airank[ito]  +1,
                 PieceStrTable[ipiece_move + promote],
                 9-aifile[ifrom], airank[ifrom]+1);
    }
    else if (ifrom < nsquare) {
        // ó·) 87çÅ(67)
        snprintf(str, sizeof(str), "%s%d%d%s(%d%d)",
                 turnStr,
                 9-aifile[ifrom], airank[ifrom]+1,
                 PieceStrTable[ipiece_move],
                 9-aifile[ito],   airank[ito]  +1);
    }
    else {
        // ó·) 99îÚë≈
        snprintf(str, sizeof(str), "%s%d%d%së≈",
                 turnStr,
                 9-aifile[ito], airank[ito]+1,
                 PieceStrTable[From2Drop(ifrom)]);
    }

    return str;
}

} // namespace godwhale
