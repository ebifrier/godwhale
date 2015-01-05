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

Position::Position(bool init/*=true*/)
    : m_turn(black), m_asquare(nsquare)
{
    memset(m_hand, 0, sizeof(m_hand));

    if (init) {
        // m_asquare‚Æmin_posi_t.asquare‚ÍŒ^‚ªˆá‚¤B
        std::copy_n(min_posi_no_handicap.asquare, (int)nsquare, m_asquare.begin());
    }
}

Position::Position(Position const & other)
{
    m_hand[black] = other.m_hand[black];
    m_hand[white] = other.m_hand[white];
    m_turn = other.m_turn;
    m_asquare = other.m_asquare;
    m_moveList = other.m_moveList;
}

Position::Position(Position && other)
    : m_turn(other.m_turn)
{
    m_hand[black] = other.m_hand[black];
    m_hand[white] = other.m_hand[white];
    m_turn = other.m_turn;
    m_asquare = std::move(other.m_asquare); // ‚±‚±‚¾‚¯ˆá‚¤
    m_moveList = std::move(other.m_moveList);
}

Position::Position(min_posi_t const & posi)
    : m_turn(posi.turn_to_move), m_asquare(nsquare)
{
    m_hand[black] = posi.hand_black;
    m_hand[white] = posi.hand_white;

    std::copy_n(posi.asquare, (int)nsquare, m_asquare.begin());
}

Position &Position::operator =(Position const & other)
{
    if (this != &other)
    {
        m_hand[black] = other.m_hand[black];
        m_hand[white] = other.m_hand[white];
        m_turn = other.m_turn;
        m_asquare = other.m_asquare;
        m_moveList = other.m_moveList;
    }

    return *this;
}

Position &Position::operator =(Position && other)
{
    if (this != &other)
    {
        m_hand[black] = other.m_hand[black];
        m_hand[white] = other.m_hand[white];
        m_turn = other.m_turn;
        m_asquare = std::move(other.m_asquare); // ‚±‚±‚¾‚¯ˆá‚¤
        m_moveList = std::move(other.m_moveList);
    }

    return *this;
}

Position &Position::operator =(min_posi_t const & posi)
{
    m_hand[black] = posi.hand_black;
    m_hand[white] = posi.hand_white;
    m_turn = posi.turn_to_move;
    std::copy_n(posi.asquare, (int)nsquare, m_asquare.begin());

    m_moveList.clear();
    return *this;
}

bool operator==(Position const & lhs, Position const & rhs)
{
    if (lhs.m_hand[black] != rhs.m_hand[black] ||
        lhs.m_hand[white] != rhs.m_hand[white] ||
        lhs.m_turn != rhs.m_turn) {
        return false;
    }

    if (!std::equal(lhs.m_asquare.begin(), lhs.m_asquare.end(),
                    rhs.m_asquare.begin())) {
        return false;
    }

    return true;
}

/**
 * @brief min_posi_tƒIƒuƒWƒFƒNƒg‚É•ÏŠ·‚µ‚Ü‚·B
 */
min_posi_t Position::getMinPosi() const
{
    min_posi_t posi;

    posi.hand_black = m_hand[black];
    posi.hand_white = m_hand[white];
    posi.turn_to_move = m_turn;
    std::copy_n(m_asquare.begin(), (int)nsquare, posi.asquare);
    return posi;
}

/**
 * @brief ‚¿‹î‚Ì”‚ğæ“¾‚µ‚Ü‚·B
 */
int Position::getHand(int turn, int piece) const
{
    assert(turn == black || turn == white);
    assert(piece > 0);
    int hand = m_hand[turn];

    switch (piece) {
    case pawn:   return I2HandPawn(hand);
    case knight: return I2HandKnight(hand);
    case lance:  return I2HandLance(hand);
    case silver: return I2HandSilver(hand);
    case gold:   return I2HandGold(hand);
    case bishop: return I2HandBishop(hand);
    case rook:   return I2HandRook(hand);
    default:     unreachable(); break;
    }
}

/**
 * @brief ‚¿‹î‚Ì”‚ğİ’è‚µ‚Ü‚·B
 */
void Position::setHand(int turn, int piece, int count)
{
    assert(turn == black || turn == white);
    assert(piece > 0 && count >= 0);
    int hand = m_hand[turn];

    switch (piece) {
    case pawn:   hand&=~IsHandPawn(~0);   hand|=(flag_hand_pawn  *count); break;
    case knight: hand&=~IsHandKnight(~0); hand|=(flag_hand_knight*count); break;
    case lance:  hand&=~IsHandLance(~0);  hand|=(flag_hand_lance *count); break;
    case silver: hand&=~IsHandSilver(~0); hand|=(flag_hand_silver*count); break;
    case gold:   hand&=~IsHandGold(~0);   hand|=(flag_hand_gold  *count); break;
    case bishop: hand&=~IsHandBishop(~0); hand|=(flag_hand_bishop*count); break;
    case rook:   hand&=~IsHandRook(~0);   hand|=(flag_hand_rook  *count); break;
    default:     unreachable(); break;
    }

    m_hand[turn] = hand;
}

/**
 * @brief w‚µè‚ª³‚µ‚¢w‚µè‚©‚Ç‚¤‚©’²‚×‚Ü‚·B
 */
bool Position::isValidMove(Move move) const
{
    int from = move.getFrom();
    int to   = move.getTo();

    if (move.isEmpty()) {
        return false;
    }

    if (move.isDrop()) {
        // ‹î‘Å‚¿‚Ìê‡
        if (get(to) != 0) {
            return false;
        }

        unsigned int u = m_hand[m_turn];
        switch (move.getDrop()) {
        case pawn:   if (IsHandPawn(u))   { return true; } break;
        case lance:  if (IsHandLance(u))  { return true; } break;
        case knight: if (IsHandKnight(u)) { return true; } break;
        case silver: if (IsHandSilver(u)) { return true; } break;
        case gold:   if (IsHandGold(u))   { return true; } break;
        case bishop: if (IsHandBishop(u)) { return true; } break;
        case rook:   if (IsHandRook(u))   { return true; } break;
        default:  unreachable(); break;
        }

        return false;
    }
    else {
        // ‹î‚ÌˆÚ“®‚Ìê‡
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

/**
 * @brief ‹ó‚Ì‹Ç–Ê‚©‚Ç‚¤‚©’²‚×‚Ü‚·B
 */
bool Position::isEmpty() const
{
    if (!m_moveList.empty()) {
        return false;
    }

    if (m_hand[0] != 0 || m_hand[1] != 0) {
        return false;
    }

    if (!std::all_of(boost::begin(m_asquare), boost::end(m_asquare),
                     [](int x){ return x==0; })) {
        return false;
    }

    return true;
}

/**
 * @brief ‰Šú‹Ç–Ê‚©‚Ç‚¤‚©’²‚×‚Ü‚·B
 */
bool Position::isInitial() const
{
    if (m_turn != black || !m_moveList.empty()) {
        return false;
    }

    if (m_hand[0] != 0 || m_hand[1] != 0) {
        return false;
    }

    if (!std::equal(boost::begin(m_asquare), boost::end(m_asquare),
                    min_posi_no_handicap.asquare)) {
        return false;
    }

    return true;
}

/**
 * @brief w‚µè‚ğˆê‚Âi‚ß‚Ü‚·B
 */
bool Position::makeMove(Move move)
{
    int sign = (m_turn == black ? +1 : -1);

    if (move.getCapture() == king) {
        LOG_ERROR() << move << " " << move.getCapture() << " is invalid.";
        return false;
    }
    if (move == MOVE_NA || !isValidMove(move)) {
        return false;
    }

    if (move.isDrop()) {
        // ‹î‘Å‚¿‚Ìê‡
        assert(abs(get(move.getTo())) == 0);

        int ipiece_drop = move.getDrop();
        m_hand[m_turn] -= HandTable[ipiece_drop];
        set(move.getTo(), ipiece_drop * sign);
    }
    else {
        // ‹î‚ÌˆÚ“®‚Ìê‡
        assert(abs(get(move.getFrom())) == move.getPiece());
        assert(abs(get(move.getTo())) == move.getCapture());

        set(move.getFrom(), 0);

        // promote=8 ‚Å‚»‚êˆÈã‚Ì’l‚È‚ç¬‚è‹î‚Æ‚È‚è‚Ü‚·B
        int ipiece_move = move.getPiece();
        if (move.isPromote()) {
            ipiece_move |= promote;
        }
        set(move.getTo(), ipiece_move * sign);

        int ipiece_cap = move.getCapture();
        if (ipiece_cap != 0) {
            m_hand[m_turn] += HandTable[ipiece_cap];
        }
    }

    m_turn = Flip(m_turn);
    m_moveList.push_back(move);
    return true;
}

/**
 * @brief w‚µè‚ğˆê‚Â–ß‚µ‚Ü‚·B
 */
bool Position::unmakeMove()
{
    assert(!m_moveList.empty());
    if (m_moveList.empty()) {
        return false;
    }

    // è”Ô‚Íæ‚ÉƒtƒŠƒbƒv‚µ‚Ü‚·B
    m_turn = Flip(m_turn);

    Move move = m_moveList.back();
    if (move.isDrop()) {
        // ‹î‘Å‚¿‚Ìê‡
        assert(abs(get(move.getTo())) == move.getDrop());

        int ipiece_drop = move.getDrop();
        m_hand[m_turn] += HandTable[ipiece_drop];
        set(move.getTo(), 0);
    }
    else {
        // ‹î‚ÌˆÚ“®‚Ìê‡
        assert(abs(get(move.getFrom())) == 0);
        assert(abs(get(move.getTo())) ==
               (move.getPiece() | (move.isPromote() ? promote : 0)));

        int sign = (m_turn == black ? +1 : -1);

        // piece ‚É‚Í¬‚Á‚½ê‡‚Í¬‚é‘O‚Ì‹î‚ª“ü‚Á‚Ä‚¢‚Ü‚·B
        set(move.getFrom(), move.getPiece() * sign);

        int ipiece_cap = move.getCapture();
        if (ipiece_cap != 0) {
            m_hand[m_turn] -= HandTable[ipiece_cap];
        }

        set(move.getTo(), -ipiece_cap * sign);
    }

    m_moveList.pop_back();
    return true;
}

static std::string s_bigNumberText[] =
{
    "‚O", "‚P", "‚Q", "‚R", "‚S",
    "‚T", "‚U", "‚V", "‚W", "‚X",
};

static std::string s_kanjiNumberText[] =
{
    "—ë", "ˆê", "“ñ", "O", "l",
    "ŒÜ", "˜Z", "µ", "”ª", "‹ã",
};

/**
 * @brief ‹Ç–Ê‚ğo—Í‚µ‚Ü‚·B
 */
void Position::print(std::ostream &os) const
{
    const Move move      = (m_moveList.empty() ? MOVE_NA : m_moveList.back());
    const int ito        = move.getTo();
    const int ifrom      = move.getFrom();
    const int is_promote = move.isPromote();

    os << "'   ‚X  ‚W  ‚V  ‚U  ‚T  ‚S  ‚R  ‚Q  ‚P" << std::endl;

    for (int irank = rank1; irank <= rank9; ++irank) {
        os << s_kanjiNumberText[irank + 1] << " |";
        
        for (int ifile = file1; ifile <= file9; ++ifile) {
            int sq = irank * nfile + ifile;

            printPiece(os, get(sq), sq, ito, ifrom, is_promote);
            os << "|";
        }

        os << std::endl;
    }

    printHand(os, m_hand[black], "æè: ");
    printHand(os, m_hand[white], "Œãè: ");
    os << "è”Ô: " << (m_turn == black ? "æè" : "Œãè");
}

/**
 * @brief ‹î‚ğˆê‚Âo—Í‚µ‚Ü‚·B
 */
void Position::printPiece(std::ostream &os, int piece, int sq, int ito,
                          int ifrom, int is_promote) const
{
    if (piece != 0) {
        char ch = (piece < 0 ? '-' : '+');
        os << ch << PieceStrTable[abs(piece)];
    }
    else {
        os << " * ";
    }
}

/**
 * @brief ‚¿‹î‚ğo—Í‚µ‚Ü‚·B
 */
void Position::printHand(std::ostream &os, unsigned int hand,
                         const std::string &prefix) const
{
    if (hand != 0) {
        os << prefix;

        printHand0(os, (int)I2HandPawn(hand),   "•à");
        printHand0(os, (int)I2HandLance(hand),  "");
        printHand0(os, (int)I2HandKnight(hand), "Œj");
        printHand0(os, (int)I2HandSilver(hand), "‹â");
        printHand0(os, (int)I2HandGold(hand),   "‹à");
        printHand0(os, (int)I2HandBishop(hand), "Šp");
        printHand0(os, (int)I2HandRook(hand),   "”ò");

        os << std::endl;
    }
}

/**
 * @brief ‚¿‹î‚ğ‚P‚Â‚¾‚¯o—Í‚µ‚Ü‚·B
 */
void Position::printHand0(std::ostream &os, int n, const std::string &str) const
{
    if (n > 0) {
        if (n >= 10) {
            os << s_bigNumberText[n / 10];
            n %= 10;
        }
        os << s_bigNumberText[n];
        os << str;
    }
}

} // namespace godwhale
