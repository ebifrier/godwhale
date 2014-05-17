
#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "../common/bonanza_if.h"

using namespace godwhale::server;

void CONV init_game_hook()
{
    InitializeLog();

    Server::GetInstance()->InitGame();
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

void CONV adjust_time_hook( int turn)
{
    Server::GetInstance()->AdjustTimeHook(turn);
}

int CONV server_iterate(tree_t *restrict ptree, int *value,
                        move_t *pvseq, int *pvseq_length)
{
    std::vector<move_t> seq;
    int status;

    // Žè”Ô–ˆ‚ÌŽw‚µŽè‚ÌŒˆ’è‚ðs‚¢‚Ü‚·B
    status = Server::GetInstance()->Iterate(ptree, value, seq);

    std::copy(seq.begin(), seq.end(), pvseq);

    *pvseq_length = seq.size() + 1;
    return status;
}
