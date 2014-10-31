#include "precomp.h"
#include "stdinc.h"
#include "exceptions.h"
#include "io_sfen.h"

namespace godwhale {
namespace server {

typedef int piece_t;

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
 * @brief 成り駒でない駒のインデックスをSFEN形式に変換します。
 */
static std::string nonpromotedPieceToSfen(piece_t piece)
{
    assert(-king <= piece && piece <= king);
    char ch;

    // +なら先手、-なら後手です。
    if (piece > 0) {
        ch = SfenPieceTable[piece];
    }
    else {
        ch = tolower(SfenPieceTable[-piece]);
    }

    return std::string(1, ch);
}

/**
 * @brief 駒のインデックスをSFEN形式に変換します。
 */
static std::string pieceToSfen(piece_t piece)
{
    assert(-dragon <= piece && piece <= dragon);
    piece_t apiece = abs(piece);
    piece_t simple = apiece;
    std::string result;

    if (apiece != king && apiece & promote) {
        simple = apiece & ~promote;
        result += "+";
    }

    // +なら先手、-なら後手です。
    if (piece > 0) {
        result += SfenPieceTable[simple];
    }
    else {
        result += tolower(SfenPieceTable[simple]);
    }

    return result;
}

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
static piece_t sfenToPiece(char pieceChr)
{
    int upperPiece, sign;

    // 大文字なら先手、小文字なら後手です。
    if (isupper(pieceChr)) {
        upperPiece = pieceChr;
        sign = +1;
    }
    else {
        upperPiece = toupper(pieceChr);
        sign = -1;
    }

    for (int i = 0; i < ArraySize(SfenPieceTable); ++i) {
        if (upperPiece == SfenPieceTable[i]) {
            return (piece_t)(i * sign);
        }
    }

    return (piece_t)0;
}


/////////////////////////////////////////////////////////////////////
#pragma region 指し手
/**
 * @brief 指し手をSFEN形式の文字列に変換します。
 */
std::string moveToSfen(Move const & move)
{
    char toFileChr = (char)((int)'1' + (move.GetToFile() - 1));
    char toRankChr = (char)((int)'a' + (move.GetToRank() - 1));
    char buf[32];

    if (move.IsDrop())
    {
        // 駒打ちの場合
        int ipiece = move.GetDrop();
        assert(0 <= ipiece && ipiece < ArraySize(SfenPieceTable));

        snprintf(buf, sizeof(buf), "%c*%c%c",
                 SfenPieceTable[ipiece],
                 toFileChr, toRankChr);
    }
    else
    {
        // 駒の移動の場合
        int fromFileChr = (char)((int)'1' + (move.GetFromFile() - 1));
        int fromRankChr = (char)((int)'a' + (move.GetFromRank() - 1));

        snprintf(buf, sizeof(buf), "%c%c%c%c%s",
                 fromFileChr, fromRankChr, toFileChr, toRankChr,
                 (move.IsPromote() ? "+" : ""));
    }

    return buf;
}

/**
 * @brief SFEN形式の文字列を指し手に変換します。
 */
Move sfenToMove(Board const & position, std::string const & sfen)
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
        int to = SQ(toFile, toRank);

        return Move::MakeDrop(to, dropPiece);
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
        int from = SQ(fromFile, fromRank);

        int toFile = (sfen[2] - '1') + 1;
        int toRank = (sfen[3] - 'a') + 1;
        int to = SQ(toFile, toRank);

        int piece = position[from];
        if (piece == 0)
        {
            return Move();
        }

        int capture = abs(position[to]);
        bool promote = (sfen.length() > 4 && sfen[4] == '+');
        return Move::Make(from, to, abs(piece), capture, promote);
    }
}
#pragma endregion


/////////////////////////////////////////////////////////////////////
#pragma region 局面を文字列に変換
/**
 * @brief 手番をSFEN形式に変換します。
 */
static std::string turnToSfen(int turn)
{
    if (turn != black && turn != white) {
        throw SfenException(
            "局面の手番が正しくありません。");
    }

    return (turn == black ? "b" : "w");
}

/**
 * @brief 各段の駒文字をSFEN形式で返します。
 */
static std::string rankToSfen(Board const & position, int rank)
{
    std::string result;
    int nspaces = 0;

    for (int file = nfile; file >= 1; --file) {
        piece_t piece = position[SQ(file, rank)];
        if (piece == 0) {
            // 駒がない場合
            nspaces += 1;
        }
        else {
            // 駒がある場合
            if (nspaces > 0) {
                result += toString(nspaces);
                nspaces = 0;
            }

            result += pieceToSfen(piece);
        }
    }

    // 空白の数は数字で示します。
    if (nspaces > 0) {
        result += toString(nspaces);
    }

    return result;
}

/**
 * @brief 局面をSFEN形式に変換します。
 */
static std::string position0ToSfen(Board const & position)
{
    std::vector<std::string> v;

    for (int rank = 1; rank <= nrank; ++rank) {
        v.push_back(rankToSfen(position, rank));
    }

    return boost::join(v, "/");
}

/**
 * @brief 手番ごとの持ち駒をSFEN形式に変換します。
 */
static std::string hand0ToSfen(Board const & position, int turn)
{
    std::string result;

    for (piece_t piece = rook; piece >= pawn; --piece) {
        int count = position.GetHand(turn, piece);

        if (count > 1) {
            result += toString(count);
        }

        if (count > 0) {
            result += nonpromotedPieceToSfen(piece * (turn == black ? +1 : -1));
        }
    }

    return result;
}

/**
 * @brief 持ち駒をSFEN形式に変換します。
 */
static std::string handToSfen(Board const & position)
{
    std::string result;
    result += hand0ToSfen(position, black);
    result += hand0ToSfen(position, white);

    return (!result.empty() ? result : "-");
}

/**
 * @brief 局面をSFEN形式の文字列にに変換します。
 */
std::string positionToSfen(Board const & position)
{
    return (boost::format("%1% %2% %3% 1")
        % position0ToSfen(position)
        % turnToSfen(position.GetTurn())
        % handToSfen(position))
        .str();
}
#pragma endregion


/////////////////////////////////////////////////////////////////////
#pragma region 文字列を局面に変換
/**
 * @brief 局面の手番をパースします。
 */
static int parseTurn(std::string const & text)
{
    if (text.length() != 1 || (text[0] != 'b' && text[0] != 'w'))
    {
        throw std::invalid_argument(
            "SFEN形式の手番表現が正しくありません。");
    }

    return (text[0] == 'b' ? black : white);
}

/**
 * @brief SFEN形式の局面部分のみをパースします。
 */
static void parseBoard0(Board & position, std::string const & sfen)
{
    int rank = 1;
    int file = 9;
    int promoted = false;

    BOOST_FOREACH(auto c, sfen) {
        if (rank > 9) {
            throw SfenException(
                "局面の段数が９を超えます。");
        }

        if (c == '/') {
            if (file != 0) {
                throw SfenException(
                    "SFEN形式の%d段の駒数が合いません。", rank);
            }

            rank += 1;
            file = 9;
            promoted = false;
        }
        else if (c == '+') {
            promoted = true;
        }
        else if ('1' <= c && c <= '9') {
            file -= (c - '0');
            promoted = false;
        }
        else {
            if (file < 1) {
                throw SfenException(
                    "SFEN形式の%d段の駒数が多すぎます。", rank);
            }

            int piece = sfenToPiece(c);
            if (piece == 0) {
                throw SfenException(
                    "SFEN形式の駒'%c'が正しくありません。", c);
            }

            if (promoted && (piece == gold || piece == king)) {
                throw SfenException(
                    "成れない駒を成ってしまっています。");
            }

            if (promoted) {
                piece |= promote;
            }

            position.Set(SQ(file, rank), piece);
            file -= 1;
            promoted = false;
        }
    }

    if (file != 0) {
        throw SfenException(
            "SFEN形式の%d段の駒数が合いません。", rank);
    }
}

/**
 * @brief 持ち駒を読み込みます。
 */
static void parseHand(Board & position, std::string const & sfen)
{
    if (sfen[0] == '-') {
        // 何もする必要がありません。
        return;
    }

    int count = 0;
    BOOST_FOREACH (auto c, sfen) {
        if ('1' <= c && c <= '9') {
            count = count * 10 + (c - '0');
        }
        else {
            int piece = sfenToPiece(c);
            if (piece == 0) {
                throw SfenException(
                    "SFEN形式の持ち駒'%c'が正しくありません。", c);
            }

            int count1 = std::max(1, count);
            position.SetHand((piece > 0 ? black : white), abs(piece), count1);
            count = 0;
        }
    }
}

/**
 * @brief SFEN形式の局面を実際の局面に変換します。
 */
Board sfenToPosition(std::string const & sfen)
{
    if (sfen.empty()) {
        throw std::invalid_argument("sfen");
    }

    std::vector<std::string> result;
    boost::split(result, sfen, boost::is_any_of(" "));
    result.erase(std::remove(result.begin(), result.end(), ""), result.end());
    if (result.size() < 3) {
        throw SfenException(
            sfen + ": SFEN形式の盤表現が正しくありません。");
    }

    Board position(false);
    parseBoard0(position, result[0]);
    parseHand(position, result[2]);
    position.SetTurn(parseTurn(result[1]));
    return position;
}
#pragma endregion

} // namespace server
} // namespace godwhale
