#include "precomp.h"
#include "stdinc.h"
#include "board.h"

namespace godwhale {
namespace server {

const unsigned int HandTable[16] =
{
    0,                flag_hand_pawn, flag_hand_lance,  flag_hand_knight,
    flag_hand_silver, flag_hand_gold, flag_hand_bishop, flag_hand_rook,
    0,                flag_hand_pawn, flag_hand_lance,  flag_hand_knight,
    flag_hand_silver, 0,              flag_hand_bishop, flag_hand_rook,
};

Board::Board()
    : m_handBlack(0), m_handWhite(0), m_turn(black)
{
    // m_asquareとmin_posi_t.asquareは型が違う。
    std::copy_n(min_posi_no_handicap.asquare, (int)nsquare, m_asquare);
}

Board::Board(const Board &other)
{
    ScopedLock lock(other.m_guard);

    m_handBlack = other.m_handBlack;
    m_handWhite = other.m_handWhite;
    m_turn = other.m_turn;
    m_moveList = other.m_moveList;
    std::copy_n(other.m_asquare, (int)nsquare, m_asquare);
}

Board &Board::operator =(const Board &other)
{
    ScopedLock lock(other.m_guard);

    m_handBlack = other.m_handBlack;
    m_handWhite = other.m_handWhite;
    m_turn = other.m_turn;
    m_moveList = other.m_moveList;
    std::copy_n(other.m_asquare, (int)nsquare, m_asquare);
    return *this;
}

/**
 * @brief 指し手が正しい指し手かどうか調べます。
 */
bool Board::IsValidMove(move_t move) const
{
    ScopedLock lock(m_guard);
    int from = (int)I2From(move);
    int to   = (int)I2To(move);

    if (move == MOVE_NA) return true;

    // 駒の移動の場合
    if (from < nsquare) {
        int piece_move = (int)I2PieceMove(move);
        if (m_turn == black) {
            if (Get(from) !=  piece_move)        return false;
            if (Get(to)   != -(int)UToCap(move)) return false;
        }
        else {
            if (Get(from) != -piece_move)       return false;
            if (Get(to)   != (int)UToCap(move)) return false;
        }

        return true;
    }

    // 駒打ちの場合
    if (Get(to) != 0) {
        return false;
    }
    else {
        unsigned int u = GetCurrentHand();

        switch (From2Drop(from)) {
        case pawn:    if (IsHandPawn(u))   { return true; } break;
        case lance:   if (IsHandLance(u))  { return true; } break;
        case knight:  if (IsHandKnight(u)) { return true; } break;
        case silver:  if (IsHandSilver(u)) { return true; } break;
        case gold:    if (IsHandGold(u))   { return true; } break;
        case bishop:  if (IsHandBishop(u)) { return true; } break;
        case rook:    if (IsHandRook(u))   { return true; } break;
        default:  assert(false); break;
        }
    }

    return false;
}

int Board::Move(move_t move)
{
    ScopedLock lock(m_guard);
    const int from = (int)I2From(move);
    const int to   = (int)I2To(move);
    const int sign = (m_turn == black ? +1 : -1);

    assert(UToCap(move) != king);
    if (move == MOVE_NA || !IsValidMove(move)) {
        return -1;
    }

    if (from >= nsquare) {
        // 駒打ちの場合
        unsigned int &hand = GetCurrentHand();
        const int ipiece_drop = From2Drop(from);
        
        hand -= HandTable[ipiece_drop];
        Set(to, ipiece_drop * sign);
    }
    else {
        // 駒の移動の場合
        const int ipiece_move = (int)I2PieceMove(move);
        const int ipiece_cap  = (int)UToCap(move);

        Set(from, 0);

        if (I2IsPromote(move)) {
            // promote=8 でそれ以上の値なら成り駒となります。
            Set(to, (ipiece_move + promote) * sign);
        }
        else {
            Set(to, ipiece_move * sign);
        }
    
        if (ipiece_cap != 0) {
            unsigned int &hand = GetCurrentHand();

            hand += HandTable[ipiece_cap];
        }
    }

    m_turn = Flip(m_turn);
    m_moveList.push_back(move);
    return 0;
}

int Board::UnMove()
{
    ScopedLock lock(m_guard);
    if (m_moveList.empty()) {
        return -1;
    }

    const move_t move = m_moveList.back();
    const int from = (int)I2From(move);
    const int to   = (int)I2To(move);
    const int sign = (m_turn == black ? +1 : -1);

    if (from >= nsquare) {
        // 駒打ちの場合
        unsigned int &hand = GetCurrentHand();
        const int ipiece_drop = From2Drop(from);

        hand += HandTable[ipiece_drop];
        Set(to, 0);
    }
    else {
        // 駒の移動の場合
        const int ipiece_move = (int)I2PieceMove(move);
        const int ipiece_cap  = (int)UToCap(move);

        if (I2IsPromote(move)) {
            Set(from, (ipiece_move - promote) * sign);
        }
        else {
            Set(from, ipiece_move * sign);
        }

        if (ipiece_cap != 0) {
            unsigned int &hand = GetCurrentHand();

            hand -= HandTable[ipiece_cap];
        }
        
        Set(to, ipiece_cap * sign);
    }

    m_turn = Flip(m_turn);
    m_moveList.pop_back();
    return 0;
}

/**
 * @brief CSA形式から駒の種類を判別します。
 */
int Board::StrToPiece(const std::string &str, std::string::size_type index) const
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
move_t Board::InterpretCsaMove(const std::string &str) const
{
    ScopedLock lock(m_guard);
    int ifrom_file, ifrom_rank, ito_file, ito_rank, ipiece;
    int ifrom, ito;
    move_t move;

    ifrom_file = str[0]-'0';
    ifrom_rank = str[1]-'0';
    ito_file   = str[2]-'0';
    ito_rank   = str[3]-'0';

    ito_file   = 9 - ito_file;
    ito_rank   = ito_rank - 1;
    ito        = ito_rank * 9 + ito_file;
    ipiece     = StrToPiece(str, 4);
    if (ipiece < 0) {
        return MOVE_NA;
    }

    if (ifrom_file == 0 && ifrom_rank == 0) {
        return (To2Move(ito) | Drop2Move(ipiece));
    }
    else {
        ifrom_file = 9 - ifrom_file;
        ifrom_rank = ifrom_rank - 1;
        ifrom      = ifrom_rank * 9 + ifrom_file;
        if (abs(Get(ifrom)) + promote == ipiece) {
            ipiece -= promote;
            move    = FLAG_PROMO;
        }
        else {
            move = 0;
        }

        return (move | To2Move(ito) | From2Move(ifrom)
            | Cap2Move(abs(Get(ito))) | Piece2Move(ipiece));
    }
}

void Board::Print(std::ostream &os, move_t move/*=0*/) const
{
    ScopedLock lock(m_guard);
    const int ito        = I2To(move);
    const int ifrom      = I2From(move);
    const int is_promote = I2IsPromote(move);

    os << "'    9   8   7   6   5   4   3   2   1" << std::endl;

    for (int irank = rank1; irank <= rank9; ++irank) {
        os << "P" << irank + 1 << " |";
        
        for (int ifile = file1; ifile <= file9; ++ifile) {
            int sq = irank * nfile + ifile;

            PrintPiece(os, Get(sq), sq, ito, ifrom, is_promote);
            os << "|";
        }

        os << std::endl;
    }
}

void Board::PrintPiece(std::ostream &os, int piece, int sq, int ito,
                       int ifrom, int is_promote) const
{
    if (piece != 0) {
        char ch = (piece < 0 ? '-' : '+');
        os << ch << astr_table_piece[abs(piece)];
    }
    else {
        os << " * ";
    }
}

}
}
