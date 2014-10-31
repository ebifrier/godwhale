
#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "bonanza_if.h"

using namespace godwhale;
using namespace godwhale::server;

#define CLIENT_SIZE 4

static int main_child( tree_t * restrict ptree );

int main(int argc, char *argv[])
{
    tree_t * restrict ptree;
    int iret;

#if defined(TLP)
    ptree = tlp_atree_work;
#else
    ptree = &tree;
#endif

#if defined(USI)
    usi_mode = usi_on;
#endif

    initializeLog();

    // これより前のサーバーインスタンスへのアクセスは禁止です。
    Server::Initialize();

    if (ini(ptree) < 0)
    {
        LOG(Error) << "failed ini(). (" << str_error << ")";
        return EXIT_SUCCESS;
    }
    
    Server::GetInstance()->WaitClient(CLIENT_SIZE);

    for ( ; ; ) {
        iret = main_child(ptree);
        if (iret == -1) {
            LOG(Error) << "failed main_child(). (" << str_error << ")";
            ShutdownAll();
            break;
        }
        else if (iret == -2) {
            LOG(Warning) << "warned main_child(). (" << str_error << ")";
            ShutdownAll();
            continue;
        }
        else if (iret == -3) {
            break;
        }
    }

    if (fin() < 0) {
        LOG(Error) << "failed fin(). (" << str_error << ")";
    }

    return EXIT_SUCCESS;
}

static int main_child(tree_t * restrict ptree)
{
    int iret;

    /* ponder a move */
    ponder_move = 0;
#if 1
    iret = ponder(ptree);
    if (iret < 0) {
        return iret;
    }
    else if (game_status & flag_quit) {
        return -3;
    }
    else if (iret == 2) {
        /* move prediction succeeded, pondering finished,
           and computer made a move. */
        return 1;
    }
    else if (ponder_move == MOVE_PONDER_FAILED) {
        /* move prediction failed, pondering aborted,
           and we have opponent's move in input buffer. */
    }
    else {
        /* pondering is interrupted or ended.
           do nothing until we get next input line. */
        TlpEnd();
        show_prompt();
    }
#endif
  
    iret = next_cmdline(1);
    if (iret < 0) {
        return iret;
    }
    else if (game_status & flag_quit) {
        return -3;
    }

    iret = procedure(ptree);
    if (iret < 0) {
        return iret;
    }
    else if (game_status & flag_quit) {
        return -3;
    }

    return 1;
}
