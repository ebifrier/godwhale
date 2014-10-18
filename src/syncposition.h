
#ifndef GODWHALE_SYNCPOSITION_H
#define GODWHALE_SYNCPOSITION_H

#include "move.h"

namespace godwhale {

/**
 * @brief bonanzaの内容と同期した局面を管理します。
 */
class SyncPosition : private boost::noncopyable
{
private:
    explicit SyncPosition();

public:
    /**
     * @brief シングルトンインスタンスを取得します。
     */
    static shared_ptr<SyncPosition> get()
    {
        return ms_instance;
    }

    /**
     * @brief 指定の局面で王手されているか調べます。
     */
    bool isChecked(int ply)
    {
        if (ply < 0 || m_moveSize < ply) {
            throw std::out_of_range("ply");
        }

        return m_checksList[ply];
    }

    void makeMove(Move move);
    void unmakeMove();

    void initialize();
    void rewind();
    void resetFromRoot(const std::vector<Move> & pv);

    std::vector<Move> getMoveList(Move exclude, bool firstMoveOnly);

private:
    static shared_ptr<SyncPosition> ms_instance;

    Move m_moveList[128];
    bool m_checksList[128+2];
    int m_moveSize;
};

} // namespace godwhale

#endif
