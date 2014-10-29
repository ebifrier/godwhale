
#include <precomp.h>
#include "stdinc.h"
#include "client.h"

using namespace godwhale;
using namespace godwhale::client;

static void inputMain();

shared_ptr<boost::thread> s_inputThread;

int main(int argc, char *argv[])
{
    tree_t * restrict ptree;

#if defined(TLP)
    ptree = tlp_atree_work;
#else
    ptree = &tree;
#endif

    g_ptree = ptree;

    trans_table_memory_size = (uint64_t)1 * 1024 * 1024 * 1024;
    strcpy(trans_table_memory_name, "trans_table_memory");

    initializeLog();

    if (ini(g_ptree) < 0) {
        LOG_ERROR() << str_error;
        return -1;
    }

    if (Client::initializeInstance(argc, argv) < 0) {
        return EXIT_FAILURE;
    }

    // Clientの初期化が終わったら、入力データの受付を開始します。
    s_inputThread.reset(new boost::thread(&inputMain));

    try {
        return Client::get()->mainloop();
    }
    catch (std::exception & ex) {
        LOG_CRITICAL() << "throw exception: " << ex.what();
    }
}

/**
 * @brief 標準入力からRSIコマンドを受け付けます。
 */
static void inputMain()
{
    while (true) {
        std::string buffer;
        if (!std::getline(std::cin, buffer)) {
            break;
        }

        // 終端の空白を削除します。
        auto it = std::find_if(buffer.rbegin(), buffer.rend(),
                               std::not1(std::ptr_fun<int, int>(std::isspace)));
        buffer.erase(it.base(), buffer.end());

        if (buffer.empty()) {
            continue;
        }

        LOG_NOTIFICATION() << "input command: " << buffer;

        // 受信した文字列をRSIとして処理します。
        Client::get()->addCommandFromRSI(buffer);
    }
}
