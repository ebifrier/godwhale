
#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "../bonanza_if.h"

using namespace godwhale;
using namespace godwhale::server;

int CONV detect_signals_server()
{
    return Server::get()->detectSignals();
}

void CONV set_position_hook(min_posi_t const * posi)
{
    /*shared_ptr<ServerCommand> scommand(new ServerCommand(SERVERCOMMAND_SETPOSITION));
    scommand->setPosition(Position(*posi));

    Server::get()->addServerCommand(scommand);*/

    Server::get()->setPosition(Position(*posi));
}

void CONV init_game_hook()
{
    /*shared_ptr<ServerCommand> scommand(new ServerCommand(SERVERCOMMAND_BEGINGAME));

    Server::get()->addServerCommand(scommand);*/

    Server::get()->beginGame();
}

void CONV quit_game_hook()
{
    /*shared_ptr<ServerCommand> scommand(new ServerCommand(SERVERCOMMAND_ENDGAME));

    Server::get()->addServerCommand(scommand);*/

    Server::get()->endGame();
}

void CONV make_move_root_hook(move_t move)
{
    /*shared_ptr<ServerCommand> scommand(new ServerCommand(SERVERCOMMAND_MAKEMOVEROOT));
    scommand->setMove(move);

    Server::get()->addServerCommand(scommand);*/

    Server::get()->makeMoveRoot(move);
}

void CONV unmake_move_root_hook()
{
    /*shared_ptr<ServerCommand> scommand(new ServerCommand(SERVERCOMMAND_UNMAKEMOVEROOT));

    Server::get()->addServerCommand(scommand);*/

    Server::get()->unmakeMoveRoot();
}

void CONV adjust_time_hook(int turn)
{
    //Server::get()->AdjustTimeHook(turn);
}

int CONV server_iterate(tree_t *restrict ptree, int *value,
                        move_t *pvseq, int *pvseq_length)
{
    std::vector<move_t> seq;
    int status;

    // Žè”Ô–ˆ‚ÌŽw‚µŽè‚ÌŒˆ’è‚ðs‚¢‚Ü‚·B
    status = Server::get()->iterate(ptree, value, seq);

    std::copy(seq.begin(), seq.end(), pvseq);

    *pvseq_length = seq.size() + 1;
    return status;
}
