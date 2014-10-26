#include "precomp.h"
#include "stdinc.h"
#include "io_csa.h"

namespace godwhale {

/**
 * @brief CSA形式から駒の種類を判別します。
 */
static int strToPiece(std::string const & str, std::string::size_type index)
{
    int i;

    for (i = 0; i < ArraySize(astr_table_piece); ++i) {
        if (str.compare(index, 2, astr_table_piece[i]) == 0) break;
    }

    if (i == 0 || i == piece_null || i == ArraySize(astr_table_piece)) {
        i = -2;
    }

    return i;
}

/**
 * @brief CSA形式の指し手を内部形式に変換します。
 */
Move csaToMove(Position const & position, std::string const & csa)
{
    assert(csa.length() > 6);
    int ifrom_file, ifrom_rank, ito_file, ito_rank, ipiece;
    int ifrom, ito;
    bool is_promote;

    ifrom_file = csa[0]-'0';
    ifrom_rank = csa[1]-'0';
    ito_file   = csa[2]-'0';
    ito_rank   = csa[3]-'0';

    ito_file   = 9 - ito_file;
    ito_rank   = ito_rank - 1;
    ito        = ito_rank * 9 + ito_file;
    ipiece     = strToPiece(csa, 4);
    if (ipiece < 0) {
        return Move();
    }

    if (ifrom_file == 0 && ifrom_rank == 0) {
        return Move::createDrop(ito, ipiece);
    }
    else {
        ifrom_file = 9 - ifrom_file;
        ifrom_rank = ifrom_rank - 1;
        ifrom      = ifrom_rank * 9 + ifrom_file;
        if (abs(position[ifrom]) + promote == ipiece) {
            ipiece    -= promote;
            is_promote = true;
        }
        else {
            is_promote = false;
        }

        return Move::create(ifrom, ito, ipiece, abs(position[ito]), is_promote);
    }
}

} // namespace godwhale
