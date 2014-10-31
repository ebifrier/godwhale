
#include "precomp.h"
#include <boost/tokenizer.hpp>
#include "stdinc.h"
#include "server.h"
#include "io_sfen.h"
#include "../bonanza_if.h"

using namespace godwhale;
using namespace godwhale::server;

void CONV init_game_hook()
{
    Server::GetInstance()->InitGame();
}

void CONV quit_game_hook()
{
    Server::GetInstance()->QuitGame();
}

void CONV reset_position_hook(const min_posi_t *posi)
{
    Server::GetInstance()->ResetPosition(posi);
}

void CONV make_move_root_hook(move_t move)
{
    Server::GetInstance()->MakeRootMove(move);
}

void CONV unmake_move_root_hook()
{
    Server::GetInstance()->UnmakeRootMove();
}

void CONV adjust_time_hook(int turn)
{
    Server::GetInstance()->AdjustTimeHook(turn);
}

int CONV server_iterate(tree_t *restrict ptree, int *value,
                        move_t *pvseq, int *pvseq_length)
{
    std::vector<move_t> seq;
    int status;

    // 手番毎の指し手の決定を行います。
    status = Server::GetInstance()->Iterate(ptree, value, seq);

    std::copy(seq.begin(), seq.end(), pvseq);

    *pvseq_length = seq.size() + 1;
    return status;
}

int CONV usi_position_hook(tree_t *restrict ptree, const char *command)
{
    typedef boost::char_separator<char> char_separator;
    typedef boost::tokenizer<char_separator> tokenizer;

    char_separator sep(" ", "");
    std::string str(command);
    tokenizer tokens(str, sep);
    auto begin = tokens.begin();
    Board board;

    LOG(Notification) << "usi position: " << command;

    auto token = *begin++;
    if (token == "sfen") {
        auto boardStr = *begin++;
        auto turnStr = *begin++;
        auto handStr = *begin++;
        auto numStr = *begin++;
        auto sfen = (F("%1% %2% %3% %4%")
                     % boardStr % turnStr % handStr % numStr)
                    .str();
        board = sfenToPosition(sfen);
    }
    else if (token == "startpos") {
        // 何もしません。
    }
    else {
        str_error = str_bad_cmdline;
        return -1;
    }

    if (begin == tokens.end()) {
        if (ini_game(ptree, &min_posi_no_handicap, flag_history, NULL, NULL) < 0) {
            return -1;
        }

        return 1;
    }

    if (*begin++ != "moves" ) {
        str_error = str_bad_cmdline;
        return -1;
    }

    // 与えられたUSIの指し手をパースします。
    Move lastMove;
    while (begin != tokens.end()) {
        auto sfen = *begin++;

        auto move = sfenToMove(board, sfen);
        if (move.IsEmpty()) {
            str_error = str_bad_cmdline;
            return -1;
        }

        lastMove = move;
        board.DoMove(move);
    }

    // 一手戻して今の局面と比較します。
    board.DoUnmove();

    if (!lastMove.IsEmpty() && Server::GetInstance()->GetBoard() == board) {
        // 同じであればmake_move_rootのみ
        int flag = flag_history | flag_time | flag_rep | flag_detect_hang;
        if (make_move_root(ptree, lastMove.Get(), flag) < 0) {
            return -1;
        }
    }
    else {
        board.DoMove(lastMove);

        // もし違ったら局面を丸ごと入れ替えます。
        auto posi = board.getMinPosi();
        if (ini_game(ptree, &posi, flag_history, NULL, NULL) < 0) {
            return -1;
        }
    }

    return 1;
}
