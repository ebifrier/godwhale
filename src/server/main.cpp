
#include "precomp.h"
#include "stdinc.h"
#include "server.h"

using namespace godwhale::server;

static int main_child( tree_t * restrict ptree );

int main(int argc, char *argv[])
{
    int iret;
    tree_t * restrict ptree;

#if defined(TLP)
    ptree = tlp_atree_work;
#else
    ptree = &tree;
#endif

#if defined(USI)
    if (argc == 2 && !strcmp(argv[1], "usi")) { usi_mode = usi_on; }
    else                                      { usi_mode = usi_off; }
#endif

    InitializeLog();

    if (ini(ptree) < 0)
    {
        LOG(Error) << "failed ini(). (" << str_error << ")";
        return EXIT_SUCCESS;
    }

    // これより前のサーバーインスタンスへのアクセスは禁止です。
    Server::Initialize();

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
