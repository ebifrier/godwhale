/* $Id: time.h,v 1.11 2012-04-24 04:31:40 eikii Exp $ */

#define DBG_NO_TIME_CHECK 0
#define RESPONSE_TIME     200

// ８分切れ負け
#define maxtime8_0_ms(timeleft, root_nrep) (       \
   (root_nrep<=30)    ? ( 5*1000 - RESPONSE_TIME): \
   (timeleft>4*60)    ? (10*1000 - RESPONSE_TIME): \
   (timeleft>2*60+30) ? ( 5*1000 - RESPONSE_TIME): \
   (timeleft>1*60+30) ? ( 3*1000 - RESPONSE_TIME): \
                        ( 2*1000 - RESPONSE_TIME)  )

// １５分切れ負け
#define maxtime15_0_ms(timeleft, root_nrep) (   \
   (root_nrep<=30) ? ( 9*1000 - RESPONSE_TIME): \
   (timeleft>7*60) ? (18*1000 - RESPONSE_TIME): \
   (timeleft>3*60) ? ( 8*1000 - RESPONSE_TIME): \
   (timeleft>2*60) ? ( 4*1000 - RESPONSE_TIME): \
   (timeleft>1*60) ? ( 3*1000 - RESPONSE_TIME): \
                     ( 2*1000 - RESPONSE_TIME)  )

// 25分切れ負け
#define maxtime25_0_ms(timeleft, root_nrep) (      \
   (root_nrep<=30)    ? (14*1000 - RESPONSE_TIME): \
   (timeleft>8*60)    ? (27*1000 - RESPONSE_TIME): \
   (timeleft>3*60)    ? (11*1000 - RESPONSE_TIME): \
   (timeleft>1*60+30) ? ( 4*1000 - RESPONSE_TIME): \
                        ( 2*1000 - RESPONSE_TIME)  )

// 持ち時間１５分　秒読み１０秒
#define maxtime15_10_ms(timeleft, root_nrep) (  \
   (timeleft>20 && root_nrep>30) ? ( 3*1000 - RESPONSE_TIME): \
                                   (10*1000 - RESPONSE_TIME)  )

// 持ち時間１５分　秒読み６０秒
#define maxtime15_60_ms(timeleft, root_nrep) (  \
   (timeleft>90 && root_nrep>30) ? (83*1000 - RESPONSE_TIME): \
                                   (60*1000 - RESPONSE_TIME)  )

// 持ち時間３０分　秒読み６０秒
#define maxtime30_60_ms(timeleft, root_nrep) ( \
   (timeleft>140 && root_nrep>30) ? (93*1000 - RESPONSE_TIME): \
                                    (60*1000 - RESPONSE_TIME)  )

// 持ち時間６０分　秒読み６０秒
#define maxtime60_60_ms(timeleft, root_nrep) ( \
   (timeleft>4*60+40 && root_nrep>30) ? (113*1000 - RESPONSE_TIME): \
                                        ( 60*1000 - RESPONSE_TIME)  )

// 持ち時間１８０分　秒読み６０秒
#define maxtime180_60_ms(timeleft, root_nrep) ( \
   (timeleft>30*60 && root_nrep>30) ? (233*1000 - RESPONSE_TIME): \
                                      ( 60*1000 - RESPONSE_TIME)  )

#ifdef INANIWA_SHIFT
#  define INANIWA_TIME inaniwa_flag ? (5000 - RESPONSE_TIME):
#else
#  define INANIWA_TIME
#endif

// 残り時間に応じた思考の最大時間を求めます
#define maxtime_ms(timeleft, root_nrep) ( \
   INANIWA_TIME \
   (THINK_TIME==  0) ? (BYOYOMI_TIME*1000 - RESPONSE_TIME): \
   (THINK_TIME== 480&& BYOYOMI_TIME== 0)? maxtime8_0_ms(timeleft, root_nrep):\
   (THINK_TIME== 900&& BYOYOMI_TIME== 0)? maxtime15_0_ms(timeleft, root_nrep):\
   (THINK_TIME==1500&& BYOYOMI_TIME== 0)? maxtime25_0_ms(timeleft, root_nrep):\
   (THINK_TIME== 900&& BYOYOMI_TIME==60)? maxtime15_60_ms(timeleft, root_nrep):\
   (THINK_TIME==1800&& BYOYOMI_TIME==60)? maxtime30_60_ms(timeleft, root_nrep):\
   (THINK_TIME==3600&& BYOYOMI_TIME==60)? maxtime60_60_ms(timeleft, root_nrep):\
   (THINK_TIME==10800&&BYOYOMI_TIME==60)? maxtime180_60_ms(timeleft,root_nrep):\
                     1800    )

// 残り時間がこの時間以上なら延長を考えます
#define MINTIME4EXTEND_SEC ( \
   (THINK_TIME==  0                    )?   1 : \
   (THINK_TIME== 480&& BYOYOMI_TIME== 0)? 105 :\
   (THINK_TIME== 900&& BYOYOMI_TIME== 0)? 105 :\
   (THINK_TIME==1500&& BYOYOMI_TIME== 0)? 170 :\
   (THINK_TIME== 900&& BYOYOMI_TIME==60)? 255 :\
   (THINK_TIME==1800&& BYOYOMI_TIME==60)? 465 :\
   (THINK_TIME==3600&& BYOYOMI_TIME==60)? 565 :\
   (THINK_TIME==10800&&BYOYOMI_TIME==60)?1165 :\
                     1    )

 // FIXME tune
#define ADD_RWD_BONUS(x) ((x) * 3 / 8)

// 時間制御ルーチン。思考打ち切りの場合は1を返します。
static int timeCheck()
{
    unsigned int tnow, tMaxSpent;

    // 自分と相手の残り時間[s]
    int myTimeLeft = THINK_TIME - (myTurn == white ? sec_w_total : sec_b_total);

    //make sure at least one loop is done (even at the risk of timeup)
    if (plane.lastDoneItd == 0) return 0;

    // １手に使える思考の最大時間
    tMaxSpent = maxtime_ms(myTimeLeft, g_ptree->nrep);
    
    // 持ち時間がある程度残っているなら、思考時間の延長を考える
    if (myTimeLeft > MINTIME4EXTEND_SEC) {
        bool unstable =  timerec.retryingAny();

        if (unstable) {
            // 安定ノードでないなら時間延長
            tMaxSpent *= 10;
        }
        
        tMaxSpent += timerec.bonustime();
        if (plane.rwdLatch2) {
            tMaxSpent += ADD_RWD_BONUS(tMaxSpent);
        }
    }

#define maxtimeAll_ms()    (355U * 1000U)
#define maxtimeTurn_ms()   (420U * 1000U)
#define maxtimePonder_ms() (420U * 1000U)

    // time_startは思考開始、または先読み開始時刻
    // time_turn_startは自分の手番の開始時刻
    get_elapsed(&tnow);
    if (!DBG_NO_TIME_CHECK && !(game_status & flag_problem)) {
        // 先読み中でない場合 =>
        //   思考時刻が最大値を超える
        //   手番が始まってからの時間が最大値を超える
        //   先読み開始からの時間が最大値を超える
        // そうでない場合 =>
        //   先読み開始 or 思考開始からの時間が最大値を超える
        // 場合は、思考を打ち切る。
        if ((!(game_status & flag_pondering) &&
             (tnow > time_start + tMaxSpent ||
              (tnow > time_turn_start + maxtimeTurn_ms() &&
               tnow > time_start + maxtimeAll_ms()))) ||
            tnow > time_start + maxtimePonder_ms()) {

            MSTOut("time expired now %u pond %d st %u tnst %u mxspen %d\n",
                   tnow, ((game_status & flag_pondering)?1:0), time_start, 
                   time_turn_start, tMaxSpent);
            return 1;
        }
    }

    return 0;
}
