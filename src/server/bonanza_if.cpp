
#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "../bonanza_if.h"

using namespace godwhale;
using namespace godwhale::server;

int CONV detect_signal_server()
{
    return 0;
}

void CONV init_game_hook()
{
    //initializeLog();

    Server::get()->InitGame();
}

void CONV quit_game_hook()
{
    Server::get()->QuitGame();
}

void CONV reset_position_hook(const min_posi_t *posi)
{
    Server::get()->ResetPosition(posi);
}

void CONV make_move_root_hook(move_t move)
{
    Server::get()->MakeRootMove(move);
}

void CONV unmake_move_root_hook()
{
    Server::get()->UnmakeRootMove();
}

void CONV adjust_time_hook(int turn)
{
    Server::get()->AdjustTimeHook(turn);
}

int CONV server_iterate(tree_t *restrict ptree, int *value,
                        move_t *pvseq, int *pvseq_length)
{
    std::vector<move_t> seq;
    int status;

    // Žè”Ô–ˆ‚ÌŽw‚µŽè‚ÌŒˆ’è‚ðs‚¢‚Ü‚·B
    status = Server::get()->Iterate(ptree, value, seq);

    std::copy(seq.begin(), seq.end(), pvseq);

    *pvseq_length = seq.size() + 1;
    return status;
}
