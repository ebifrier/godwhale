#include "precomp.h"
#include "stdinc.h"

#include "server.h"
#include "client.h"

/*
 * Serverクラスの中でも特に指し手生成に関わる部分のみを扱っています。
 */

namespace godwhale {
namespace server {

#define FOREACH_CLIENT(VAR) \
    auto BOOST_PP_CAT(temp, __LINE__) = CloneClientList();  \
    BOOST_FOREACH(auto VAR, BOOST_PP_CAT(temp, __LINE__))   \
        if (!(VAR)->IsLogined()) continue;                  \
        else

void Server::MakeRootMove(move_t move)
{
    m_gid += 10;

    FOREACH_CLIENT(client) {
        client->MakeRootMove(move, m_gid);
    }
}

void Server::UnmakeRootMove()
{
}

int Server::Iterate(int *value, std::vector<move_t> &pvseq)
{
    timer::cpu_timer timer;

    while (timer.elapsed().wall < 3LL*1000*1000*1000) {
        FOREACH_CLIENT(client) {
            /*if (client->GetNodeCount() > 1000000) {
                return 0;
            }*/
        }
    }

    FOREACH_CLIENT(client) {
        if (client->GetMove() != MOVE_NA) {
            pvseq.push_back(client->GetMove());
            *value = client->GetValue();
            return 0;
        }
    }

    *value = 0;
    return 0;
}

} // namespace server
} // namespace godwhale
