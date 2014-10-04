#include "precomp.h"
#include "stdinc.h"
#include "move.h"
#include "position.h"

namespace godwhale {

/* promote = 8, empty = 0,
 * pawn, lance, knight, silver, gold, bishop, rook, king, pro_pawn,
 * pro_lance, pro_knight, pro_silver, piece_null, horse, dragon };
 */
static char SfenPieceTable[] =
{
    '?', // empty = 0
    'P', // pawn = 1
    'L', // lance = 2
    'N', // knight = 3
    'S', // silver = 4
    'G', // gold = 5
    'B', // bishop = 6
    'R', // rook = 7
    'K', // king = 8
};

/**
 * @brief SFEN形式の駒を駒のインデックスに変換します。
 *
 * 先手の玉：K、後手の玉：k （Kingの頭文字）
 * 先手の飛車：R、後手の飛車：r （Rookの頭文字）
 * 先手の角：B、後手の角：b （Bishopの頭文字）
 * 先手の金：G、後手の金：g （Goldの頭文字）
 * 先手の銀：S、後手の銀：s （Silverの頭文字）
 * 先手の桂馬：N、後手の桂馬：n （kNightより）
 * 先手の香車：L、後手の香車：l （Lanceの頭文字）
 * 先手の歩：P、後手の歩：p （Pawnの頭文字）
 */
piece_t sfenToPiece(char piece)
{
    int upperPiece = toupper(piece);

    if (isupper(piece)) {
        for (int i = 0; i < ArraySize(SfenPieceTable); ++i) {
            if (piece == SfenPieceTable[i]) {
                return (piece_t)i;
            }
        }
    }
    else {
        piece = toupper(piece);

        for (int i = 0; i < ArraySize(SfenPieceTable); ++i) {
            if (piece == SfenPieceTable[i]) {
                return -(piece_t)i;
            }
        }
    }

    return (piece_t)0;
}

std::string moveToSfen(Move const & move)
{
    char toFileChr = (char)((int)'1' + (move.getToFile() - 1));
    char toRankChr = (char)((int)'a' + (move.getToRank() - 1));
    char buf[32];

    if (move.isDrop())
    {
        // 駒打ちの場合
        int ipiece = move.getDrop();
        assert(0 <= ipiece && ipiece < ArraySize(SfenPieceTable));

        snprintf(buf, sizeof(buf), "%s*%c%c",
                 SfenPieceTable[ipiece],
                 toFileChr, toRankChr);
    }
    else
    {
        // 駒の移動の場合
        int fromFileChr = (char)((int)'1' + (move.getFromFile() - 1));
        int fromRankChr = (char)((int)'a' + (move.getFromRank() - 1));

        snprintf(buf, sizeof(buf), "%c%c%c%c%s",
                 fromFileChr, fromRankChr, toFileChr, toRankChr,
                 (move.isPromote() ? "+" : ""));
    }

    return buf;
}

Move sfenToMove(Position const & position, std::string const & sfen)
{
    if (sfen.empty()) {
        throw std::runtime_error("sfenが正しくありません。");
    }

    int dropPiece = sfenToPiece(sfen[0]);
    if (dropPiece != 0)
    {
        // 駒打ちの場合
        if ((sfen[1] != '*') ||
            (sfen[2] < '1' || '9' < sfen[2]) ||
            (sfen[3] < 'a' || 'i' < sfen[3]) )
        {
            return Move();
        }

        int toFile = (sfen[2] - '1') + 1;
        int toRank = (sfen[3] - 'a') + 1;
        int to = makeSquare(toFile, toRank);

        return Move::createDrop(to, dropPiece);
    }
    else
    {
        // 駒の移動の場合
        if ((sfen[0] < '1' || '9' < sfen[0]) ||
            (sfen[2] < '1' || '9' < sfen[2]) ||
            (sfen[1] < 'a' || 'i' < sfen[1]) ||
            (sfen[3] < 'a' || 'i' < sfen[3]))
        {
            return Move();
        }

        int fromFile = (sfen[0] - '1') + 1;
        int fromRank = (sfen[1] - 'a') + 1;
        int from = makeSquare(fromFile, fromRank);

        int toFile = (sfen[2] - '1') + 1;
        int toRank = (sfen[3] - 'a') + 1;
        int to = makeSquare(toFile, toRank);

        int piece = position[from];
        if (piece == 0)
        {
            return Move();
        }

        int capture = abs(position[to]);
        bool promote = (sfen.length() > 4 && sfen[4] == '+');
        return Move::create(from, to, piece, capture, promote);
    }
}

/*std::string PositionToSfen(Position const & position);
Position SfenToPosition(std::string const & sfen);*/

} // namespace godwhale
