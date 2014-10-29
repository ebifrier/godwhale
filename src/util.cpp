
#include "precomp.h"
#include "stdinc.h"
#include "util.h"

namespace godwhale {

const char *PieceStrTable[16]  = {
    "〇", "歩", "香", "桂", "銀", "金",
    "角", "飛", "王", "ト", "杏", "NK",
    "NG", "×", "馬", "竜"
};

const char *TurnStrTable[] =
{
    "▲", "△", ""
};

/**
 * @brief 立っているビットの数を調べます。
 */ 
int popCount(int x)
{
    x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x & 0x0f0f0f0f) + ((x >> 4) & 0x0f0f0f0f);
    x = (x & 0x00ff00ff) + ((x >> 8) & 0x00ff00ff);
    return ((x & 0x0000ffff) + ((x >> 16) & 0x0000ffff));
}

/**
 * @brief 指定の㍉秒だけスリープします。
 */ 
void sleep(int milliseconds)
{
    boost::this_thread::sleep(boost::posix_time::milliseconds(milliseconds));
}

} // namespace godwhale
