#include "precomp.h"
#include "stdinc.h"
#include "move.h"
#include "server.h"
#include "client.h"

/*
 * Serverクラスの中でも特に指し手生成に関わる部分のみを扱っています。
 */

namespace godwhale {
namespace server {

#define FOREACH_CLIENT(VAR) \
    auto BOOST_PP_CAT(temp, __LINE__) = std::move(GetClientList());  \
    BOOST_FOREACH(auto VAR, BOOST_PP_CAT(temp, __LINE__))

void Server::InitGame(const min_posi_t *posi)
{
    FOREACH_CLIENT(client) {
        client->InitGame(posi);
    }

    m_board = *posi;
    m_gid = 0;
}

void Server::MakeRootMove(move_t move)
{
    m_board.DoMove(move);
    m_gid += 10;

    LOG(Notification) << "root move: " << ToString(move);
    LOG(Notification) << m_board;

    FOREACH_CLIENT(client) {
        client->MakeRootMove(move, m_gid);
    }
}

void Server::UnmakeRootMove()
{
    m_board.DoUnmove();
    m_gid += 10; // 局面を戻した場合も、IDは進めます。

    LOG(Notification) << "root unmove";
}

struct Score
{
    bool IsValid;
    long TotalNodes;
    long MaxNodes;
    Move Move;
    long Nodes;
    int Value;
    std::vector<server::Move> PVSeq;

    explicit Score()
        : IsValid(false), TotalNodes(0), MaxNodes(0)
        , Move(MOVE_NA), Nodes(-1), Value(0) {
    }

    void MakeInvalid() {
        IsValid = false;
        TotalNodes = 0;
        MaxNodes = 0;
    }

    void UpdateNodes(shared_ptr<Client> client) {
        TotalNodes += client->GetNodeCount();
        MaxNodes    = std::max(MaxNodes, client->GetNodeCount());
    }

    void Set(const shared_ptr<Client> &client) {
        Move = (client->HasPlayedMove() ?
            client->GetPlayedMove() : client->GetMove());
        Nodes = client->GetNodeCount();
        Value = client->GetValue();
        PVSeq = client->GetPVSeq();
        IsValid = true;
    }
};

int Server::Iterate(tree_t *restrict ptree, int *value, std::vector<move_t> &pvseq)
{
    timer::cpu_timer timer, sendTimer;
    Score score;
    bool first = true;

    do {
        if (!first) {
            this_thread::yield();
            //this_thread::sleep(posix_time::milliseconds(100));
            score.MakeInvalid();
        }
        first = false;

        auto list = GetClientList();
        BOOST_FOREACH(auto client, list) {
            score.UpdateNodes(client);
        }

        int size = list.size();
        for (int i = 0; i < size; ++i) {
            auto client = list[i];
            ScopedLock locker(client->GetGuard());

            if (client->GetNodeCount() > 1L * 10000 &&
                !client->HasPlayedMove() &&
                client->GetMove() != MOVE_NA) {
                Move move = client->GetMove();

                // 先読みの手を決定し、一手実際に進めます。
                client->SetPlayedMove(move);

                // ignoreを各クライアントに送信
                // ignoreはほぼすべてのクライアントに送信しますが、
                // 冗長性を確保するため chunk ごとに１つは送らないものを残します。
                int chunk = 5;
                int start = i % chunk;
                for (int j = start; j < size; ++j) {
                    auto client2 = list[j];

                    if (client2->HasPlayedMove()) {
                        continue;
                    }

                    // chunkごとに１つは無視する指し手を送りません。
                    if ((j % chunk) == start) {
                        continue;
                    }

                    client2->AddIgnoreMove(move);
                }
            }

            // 評価値が高く、ノード数がそこまで低くない手を選びます。
            // （ノード数の判定ってこれでいいのか…？ｗ）
            bool flag1 = (client->GetNodeCount() > score.MaxNodes * 0.7 &&
                          client->GetValue() > score.Value);
            if (client->HasMove() && (!score.IsValid || flag1)) {
                score.Set(client);
            }
        }

        if (sendTimer.elapsed().wall > 5LL*1000*1000*1000) {
            auto ns  = timer.elapsed().wall;
            auto nps = (long)(score.TotalNodes / ((double)ns/1000/1000/1000));

            BOOST_FOREACH(auto client, list) {
                // 評価値は先手を+とした数字を送ります。
                client->SendCurrentInfo(list.size(), nps,
                                        score.Value * (client_turn == black ? +1 : -1));
            }

            // これで時間がリセットされます。
            sendTimer.start();
        }
    } while (!score.IsValid || !IsEndIterate(ptree, timer));

    if (score.IsValid) {
        LOG(Notification) << "  my move: " << ToString(score.Move);
        LOG(Notification) << "real move: " << ToString(score.PVSeq[0]);

        *value = score.Value;

        const auto &tmpseq = score.PVSeq;
        pvseq.insert(pvseq.end(), tmpseq.begin(), tmpseq.end());
        return 0;
    }

    *value = 0;
    return 0;
}

bool Server::IsEndIterate(tree_t *restrict ptree, timer::cpu_timer &timer)
{
    if (detect_signals(ptree) != 0) {
        return true;
    }

    if (!(game_status & flag_puzzling)) {
        auto ms = (unsigned int)(timer.elapsed().wall / 1000 / 1000);
        return IsThinkEnd(ptree, ms);
    }

    return false;
}

} // namespace server
} // namespace godwhale
