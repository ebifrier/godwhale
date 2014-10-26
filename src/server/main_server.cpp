
#include "precomp.h"
#include "stdinc.h"
#include "server.h"
#include "rootsearch.h"

#if defined(USE_GTEST)
#include <gtest/gtest.h>

#if defined(_DEBUG) || defined(DEBUG)
#pragma comment(lib, "gtestd.lib")
#else
#pragma comment(lib, "gtest.lib")
#endif
#endif

using namespace godwhale;
using namespace godwhale::server;

static int main_child( tree_t * restrict ptree );

int main(int argc, char *argv[])
{
#if 0 && defined(USE_GTEST)
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

#else
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

    g_ptree = ptree;

    initializeLog();

    // これより前のサーバーインスタンスへのアクセスは禁止です。
    Server::initialize();

    if (ini(ptree) < 0) {
        LOG_ERROR() << "failed ini(). (" << str_error << ")";
        return EXIT_SUCCESS;
    }

    SearchResult data;
    //generateRootMove(&data);

    for ( ; ; ) {
        iret = main_child(ptree);
        if (iret == -1) {
            LOG_ERROR() << "failed main_child(). (" << str_error << ")";
            ShutdownAll();
            break;
        }
        else if (iret == -2) {
            LOG_WARNING() << "warned main_child(). (" << str_error << ")";
            ShutdownAll();
            continue;
        }
        else if (iret == -3) {
            break;
        }
    }

    if (fin() < 0) {
        LOG_ERROR() << "failed fin(). (" << str_error << ")";
    }

    return EXIT_SUCCESS;
#endif
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
