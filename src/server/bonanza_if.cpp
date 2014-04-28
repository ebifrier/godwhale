
#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "../common/bonanza_if.h"

using namespace godwhale::server;

//void CONV init_game_hook(const min_posi_t *posi);
void CONV make_move_root_hook(move_t move)
{
    Server::GetInstance()->MakeRootMove(move);
}

void CONV unmake_move_root_hook()
{
    Server::GetInstance()->UnmakeRootMove();
}

int CONV server_iterate(int *value, move_t *pvseq, int *pvseq_length)
{
    std::vector<move_t> seq;
    int status;

    // Žè”Ô–ˆ‚ÌŽw‚µŽè‚ÌŒˆ’è‚ðs‚¢‚Ü‚·B
    status = Server::GetInstance()->Iterate(value, seq);

    std::copy(seq.begin(), seq.end(), pvseq);

    *pvseq_length = seq.size();
    return status;
}
