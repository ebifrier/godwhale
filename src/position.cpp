#include "precomp.h"
#include "stdinc.h"
#include "position.h"

namespace godwhale {

const unsigned int Position::HandTable[] =
{
    0,                flag_hand_pawn, flag_hand_lance,  flag_hand_knight,
    flag_hand_silver, flag_hand_gold, flag_hand_bishop, flag_hand_rook,
    0,                flag_hand_pawn, flag_hand_lance,  flag_hand_knight,
    flag_hand_silver, 0,              flag_hand_bishop, flag_hand_rook,
};

Position::Position()
    : m_turn(black)
{
    memset(m_hand, 0, sizeof(m_hand));

    // m_asquareとmin_posi_t.asquareは型が違う。
    std::copy_n(min_posi_no_handicap.asquare, (int)nsquare, m_asquare);
}

Position::Position(Position const & other)
{
    ScopedLock lock(other.m_guard);

    m_hand[black] = other.m_hand[black];
    m_hand[white] = other.m_hand[white];
    m_turn = other.m_turn;
    m_moveList = other.m_moveList;
    std::copy_n(other.m_asquare, (int)nsquare, m_asquare);
}

Position::Position(Position && other)
    : m_turn(other.m_turn)
{
    ScopedLock lock(other.m_guard);

    m_hand[black] = other.m_hand[black];
    m_hand[white] = other.m_hand[white];
    m_turn = other.m_turn;
    m_moveList = std::move(other.m_moveList); // ここだけ違う
    std::copy_n(other.m_asquare, (int)nsquare, m_asquare);
}

Position::Position(min_posi_t const & posi)
    : m_turn(posi.hand_white)
{
    m_hand[black] = posi.hand_black;
    m_hand[white] = posi.hand_white;

    std::copy_n(posi.asquare, (int)nsquare, m_asquare);
}

Position &Position::operator =(Position const & other)
{
    if (this != &other)
    {
        ScopedLock lock(other.m_guard);

        m_hand[black] = other.m_hand[black];
        m_hand[white] = other.m_hand[white];
        m_turn = other.m_turn;
        m_moveList = other.m_moveList;
        std::copy_n(other.m_asquare, (int)nsquare, m_asquare);
    }

    return *this;
}

Position &Position::operator =(Position && other)
{
    if (this != &other)
    {
        ScopedLock lock(other.m_guard);

        m_hand[black] = other.m_hand[black];
        m_hand[white] = other.m_hand[white];
        m_turn = other.m_turn;
        m_moveList = std::move(other.m_moveList); // ここだけ違う
        std::copy_n(other.m_asquare, (int)nsquare, m_asquare);
    }

    return *this;
}

Position &Position::operator =(min_posi_t const & posi)
{
    m_hand[black] = posi.hand_black;
    m_hand[white] = posi.hand_white;
    m_turn = posi.hand_white;
    std::copy_n(posi.asquare, (int)nsquare, m_asquare);

    m_moveList.clear();
    return *this;
}

/**
 * @brief 指し手が正しい指し手かどうか調べます。
 */
bool Position::isValidMove(Move move) const
{
    ScopedLock lock(m_guard);
    const int from = move.getFrom();
    const int to   = move.getTo();

    if (move.isEmpty()) {
        return false;
    }

    if (move.isDrop()) {
        // 駒打ちの場合
        if (get(to) != 0) {
            return false;
        }

        unsigned int u = m_hand[m_turn];
        switch (move.getDrop()) {
        case pawn:    if (IsHandPawn(u))   { return true; } break;
        case lance:   if (IsHandLance(u))  { return true; } break;
        case knight:  if (IsHandKnight(u)) { return true; } break;
        case silver:  if (IsHandSilver(u)) { return true; } break;
        case gold:    if (IsHandGold(u))   { return true; } break;
        case bishop:  if (IsHandBishop(u)) { return true; } break;
        case rook:    if (IsHandRook(u))   { return true; } break;
        default:  unreachable(); break;
        }

        return false;
    }
    else {
        // 駒の移動の場合
        int piece_move = move.getPiece();
        int piece_cap  = move.getCapture();

        if (m_turn == black) {
            if (get(from) !=  piece_move) return false;
            if (get(to)   != -piece_cap)  return false;
        }
        else {
            if (get(from) != -piece_move) return false;
            if (get(to)   !=  piece_cap)  return false;
        }

        return true;
    }
}

int Position::makeMove(Move move)
{
    ScopedLock lock(m_guard);
    const int sign = (m_turn == black ? +1 : -1);

    if (move.getCapture() == king) {
        LOG_ERROR() << move << " " << move.getCapture() << " is invalid.";
        return -1;
    }
    if (move == MOVE_NA || !isValidMove(move)) {
        return -1;
    }

    if (move.isDrop()) {
        // 駒打ちの場合
        assert(abs(get(move.getTo())) == 0);

        const int ipiece_drop = move.getDrop();
        m_hand[m_turn] -= HandTable[ipiece_drop];
        set(move.getTo(), ipiece_drop * sign);
    }
    else {
        // 駒の移動の場合
        assert(abs(get(move.getFrom())) == move.getPiece());
        assert(abs(get(move.getTo())) == move.getCapture());

        set(move.getFrom(), 0);

        // promote=8 でそれ以上の値なら成り駒となります。
        int ipiece_move = move.getPiece();
        if (move.isPromote()) {
            ipiece_move |= promote;
        }
        set(move.getTo(), ipiece_move * sign);

        const int ipiece_cap = move.getCapture();
        if (ipiece_cap != 0) {
            m_hand[m_turn] += HandTable[ipiece_cap];
        }
    }

    m_turn = Flip(m_turn);
    m_moveList.push_back(move);
    return 0;
}

int Position::unmakeMove()
{
    ScopedLock lock(m_guard);
    if (m_moveList.empty()) {
        return -1;
    }

    // 手番は先にフリップします。
    m_turn = Flip(m_turn);

    Move move = m_moveList.back();
    if (move.isDrop()) {
        // 駒打ちの場合
        assert(abs(get(move.getTo())) == move.getDrop());

        const int ipiece_drop = move.getDrop();
        m_hand[m_turn] += HandTable[ipiece_drop];
        set(move.getTo(), 0);
    }
    else {
        // 駒の移動の場合
        assert(abs(get(move.getFrom())) == 0);
        assert(abs(get(move.getTo())) ==
               (move.getPiece() | (move.isPromote() ? promote : 0)));

        const int sign = (m_turn == black ? +1 : -1);

        // piece には成った場合は成る前の駒が入っています。
        set(move.getFrom(), move.getPiece() * sign);

        int ipiece_cap = move.getCapture();
        if (ipiece_cap != 0) {
            m_hand[m_turn] -= HandTable[ipiece_cap];
        }

        set(move.getTo(), -ipiece_cap * sign);
    }

    m_moveList.pop_back();
    return 0;
}

/**
 * @brief CSA形式から駒の種類を判別します。
 */
int Position::strToPiece(const std::string &str, std::string::size_type index) const
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
Move Position::interpretCsaMove(const std::string &str) const
{
    ScopedLock lock(m_guard);
    int ifrom_file, ifrom_rank, ito_file, ito_rank, ipiece;
    int ifrom, ito;
    bool is_promote;

    ifrom_file = str[0]-'0';
    ifrom_rank = str[1]-'0';
    ito_file   = str[2]-'0';
    ito_rank   = str[3]-'0';

    ito_file   = 9 - ito_file;
    ito_rank   = ito_rank - 1;
    ito        = ito_rank * 9 + ito_file;
    ipiece     = strToPiece(str, 4);
    if (ipiece < 0) {
        return MOVE_NA;
    }

    if (ifrom_file == 0 && ifrom_rank == 0) {
        return Move::createDrop(ito, ipiece);
    }
    else {
        ifrom_file = 9 - ifrom_file;
        ifrom_rank = ifrom_rank - 1;
        ifrom      = ifrom_rank * 9 + ifrom_file;
        if (abs(get(ifrom)) + promote == ipiece) {
            ipiece    -= promote;
            is_promote = true;
        }
        else {
            is_promote = false;
        }

        return Move::create(ifrom, ito, ipiece, abs(get(ito)), is_promote);
    }
}

void Position::print(std::ostream &os) const
{
    ScopedLock lock(m_guard);
    const Move move      = (m_moveList.empty() ? MOVE_NA : m_moveList.back());
    const int ito        = move.getTo();
    const int ifrom      = move.getFrom();
    const int is_promote = move.isPromote();

    os << "'    9   8   7   6   5   4   3   2   1" << std::endl;

    for (int irank = rank1; irank <= rank9; ++irank) {
        os << "P" << irank + 1 << " |";
        
        for (int ifile = file1; ifile <= file9; ++ifile) {
            int sq = irank * nfile + ifile;

            printPiece(os, get(sq), sq, ito, ifrom, is_promote);
            os << "|";
        }

        os << std::endl;
    }

    printHand(os, m_hand[black], "P+");
    printHand(os, m_hand[white], "P-");
}

void Position::printPiece(std::ostream &os, int piece, int sq, int ito,
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

void Position::printHand(std::ostream &os, unsigned int hand,
                         const std::string &prefix) const
{
    printHand0(os, (int)I2HandPawn(hand),   prefix, "00FU");
    printHand0(os, (int)I2HandLance(hand),  prefix, "00KY");
    printHand0(os, (int)I2HandKnight(hand), prefix, "00KE");
    printHand0(os, (int)I2HandSilver(hand), prefix, "00GI");
    printHand0(os, (int)I2HandGold(hand),   prefix, "00KI");
    printHand0(os, (int)I2HandBishop(hand), prefix, "00KA");
    printHand0(os, (int)I2HandRook(hand),   prefix, "00HI");
}

void Position::printHand0(std::ostream &os, int n, const std::string &prefix,
                          const std::string &str) const
{
    if (n > 0) {
        os << prefix;
        for (int i = 0; i < n; ++i) { os << str; }
        os << std::endl;
    }
}

} // namespace godwhale
