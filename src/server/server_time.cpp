#include "precomp.h"
#include "stdinc.h"
#include "util.h"
#include "server.h"

/*
 * プエラαから持ってきた時間管理コード
 */

#define THINK_TIME     sec_limit
#define BYOYOMI_TIME   sec_limit_up
#define RESPONSE_TIME  300

namespace godwhale {
namespace server {

// ８分切れ負け
#define maxtime8_0_ms(timeleft, root_nrep) (        \
   (root_nrep<=30)    ? ( 5*1000 - RESPONSE_TIME) : \
   (timeleft>4*60)    ? (10*1000 - RESPONSE_TIME) : \
   (timeleft>2*60+30) ? ( 5*1000 - RESPONSE_TIME) : \
   (timeleft>1*60+30) ? ( 3*1000 - RESPONSE_TIME) : \
                        ( 2*1000 - RESPONSE_TIME)   )

// １５分切れ負け
#define maxtime15_0_ms(timeleft, root_nrep) (    \
   (root_nrep<=30) ? ( 9*1000 - RESPONSE_TIME) : \
   (timeleft>7*60) ? (18*1000 - RESPONSE_TIME) : \
   (timeleft>3*60) ? ( 8*1000 - RESPONSE_TIME) : \
   (timeleft>2*60) ? ( 4*1000 - RESPONSE_TIME) : \
   (timeleft>1*60) ? ( 3*1000 - RESPONSE_TIME) : \
                     ( 2*1000 - RESPONSE_TIME)   )

// 25分切れ負け
#define maxtime25_0_ms(timeleft, root_nrep) (       \
   (root_nrep<=30)    ? (14*1000 - RESPONSE_TIME) : \
   (timeleft>8*60)    ? (27*1000 - RESPONSE_TIME) : \
   (timeleft>3*60)    ? (11*1000 - RESPONSE_TIME) : \
   (timeleft>1*60+30) ? ( 4*1000 - RESPONSE_TIME) : \
                        ( 2*1000 - RESPONSE_TIME)   )

// 持ち時間１５分　秒読み１０秒
#define maxtime15_10_ms(timeleft, root_nrep) (      \
   (timeleft<10)      ? ( 6*1000 - RESPONSE_TIME) : \
   (root_nrep<=30)    ? (14*1000 - RESPONSE_TIME) : \
   (timeleft>10*60)   ? (20*1000 - RESPONSE_TIME) : \
   (timeleft>3*60)    ? (12*1000 - RESPONSE_TIME) : \
                      ? (10*1000 - RESPONSE_TIME)   )

// 持ち時間１５分　秒読み６０秒
#define maxtime15_60_ms(timeleft, root_nrep) (                 \
   (timeleft>90 && root_nrep>30) ? (83*1000 - RESPONSE_TIME) : \
                                   (60*1000 - RESPONSE_TIME)   )

// 持ち時間３０分　秒読み６０秒
#define maxtime30_60_ms(timeleft, root_nrep) (                  \
   (timeleft>140 && root_nrep>30) ? (93*1000 - RESPONSE_TIME) : \
                                    (60*1000 - RESPONSE_TIME)   )

// 持ち時間６０分　秒読み６０秒
#define maxtime60_60_ms(timeleft, root_nrep) (                       \
   (timeleft>4*60+40 && root_nrep>30) ? (113*1000 - RESPONSE_TIME) : \
                                        ( 60*1000 - RESPONSE_TIME)   )

// 持ち時間１８０分　秒読み６０秒
#define maxtime180_60_ms(timeleft, root_nrep) (                    \
   (timeleft>30*60 && root_nrep>30) ? (233*1000 - RESPONSE_TIME) : \
                                      ( 60*1000 - RESPONSE_TIME)   )

#ifdef INANIWA_SHIFT
#  define INANIWA_TIME inaniwa_flag ? (5000 - RESPONSE_TIME) :
#else
#  define INANIWA_TIME
#endif

// 残り時間に応じた思考の最大時間を求めます
#define maxtime_ms(timeleft, root_nrep) (                                          \
   INANIWA_TIME                                                                    \
   (THINK_TIME==   0 || (timeleft) ==  0) ? (BYOYOMI_TIME*1000 - RESPONSE_TIME)  : \
   (THINK_TIME== 480 && BYOYOMI_TIME== 0) ? maxtime8_0_ms(timeleft, root_nrep)   : \
   (THINK_TIME== 900 && BYOYOMI_TIME== 0) ? maxtime15_0_ms(timeleft, root_nrep)  : \
   (THINK_TIME==1500 && BYOYOMI_TIME== 0) ? maxtime25_0_ms(timeleft, root_nrep)  : \
   (THINK_TIME== 900 && BYOYOMI_TIME== 9) ? maxtime15_10_ms(timeleft, root_nrep) : \
   (THINK_TIME== 900 && BYOYOMI_TIME==60) ? maxtime15_60_ms(timeleft, root_nrep) : \
   (THINK_TIME==1800 && BYOYOMI_TIME==60) ? maxtime30_60_ms(timeleft, root_nrep) : \
   (THINK_TIME==3600 && BYOYOMI_TIME==60) ? maxtime60_60_ms(timeleft, root_nrep) : \
   (THINK_TIME==10800&& BYOYOMI_TIME==60) ? maxtime180_60_ms(timeleft,root_nrep) : \
                                            1800    )

/**
 * @brief 時間制御ルーチン。思考打ち切りの場合は1を返します。
 */
bool Server::IsThinkEnd(tree_t *restrict ptree, unsigned int turnTimeMS)
{
    // 問題を解いているときは時間無制限
    if (game_status & flag_problem) {
        return false;
    }

    // 自分の残り時間[s]
    int myTimeLeft = std::max(0U, sec_limit - (root_turn == black ? sec_b_total : sec_w_total));

    // ３秒モードに移行する場合の残り時間
    /*int isMinimumMode = (BYOYOMI_TIME == 0 && myTimeLeft < THINK_TIME / 5);
    if (isMinimumMode && turnTimeMS >= 3*1000-RESPONSE_TIME) {
        return true;
    }*/

    // １手に使える思考の最大時間
    unsigned int maxSpent = maxtime_ms(myTimeLeft, ptree->nrep);
    //LOG(Debug) << "max spent time: " << maxSpent;
    return (turnTimeMS >= maxSpent);
}

}
}
