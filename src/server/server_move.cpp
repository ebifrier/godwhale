#include "precomp.h"
#include "stdinc.h"

#include "server.h"
#include "client.h"

/*
 * Serverクラスの中でも特に指し手生成に関わる部分のみを扱っています。
 */

namespace godwhale {
namespace server {

int Server::Iterate(int *value, std::vector<move_t> &pvseq)
{
    timer::cpu_timer timer;

    while (timer.elapsed().wall < 3LL*1000L*1000L*1000L) {
    }

    *value = 0;
    return 0;
}

} // namespace server
} // namespace godwhale
