/* $Id: time.h,v 1.11 2012-04-24 04:31:40 eikii Exp $ */

#define DBG_NO_TIME_CHECK 0
#define TREAT_RETRY_UNSTABLE 1
#define STABLE_THRESH      250
#define UNSTABLE_EXTEND_COEFF 3

#define RESPONSE_TIME 200

#define maxtime25_0_ms(timeleft, root_nrep) ( \
   (root_nrep<=30)? (14000 - RESPONSE_TIME): \
   (timeleft>480) ? (27000 - RESPONSE_TIME): \
   (timeleft>180) ? (11000 - RESPONSE_TIME): \
   (timeleft> 90) ? ( 4000 - RESPONSE_TIME): \
                    ( 2000 - RESPONSE_TIME)    )

#define maxtime8_0_ms(timeleft, root_nrep) ( \
   (root_nrep<=30)? ( 5000 - RESPONSE_TIME): \
   (timeleft>240) ? (10000 - RESPONSE_TIME): \
   (timeleft>150) ? ( 5000 - RESPONSE_TIME): \
   (timeleft> 90) ? ( 3000 - RESPONSE_TIME): \
                    ( 2000 - RESPONSE_TIME)    )

#define maxtime15_0_ms(timeleft, root_nrep) ( \
   (root_nrep<=30)? ( 9000 - RESPONSE_TIME): \
   (timeleft>420) ? (18000 - RESPONSE_TIME): \
   (timeleft>180) ? ( 8000 - RESPONSE_TIME): \
   (timeleft>120) ? ( 4000 - RESPONSE_TIME): \
   (timeleft> 60) ? ( 3000 - RESPONSE_TIME): \
                    ( 2000 - RESPONSE_TIME)    )

#define maxtime15_60_ms(timeleft, root_nrep) ( \
   (timeleft>90 && root_nrep>30) ? (83000 - RESPONSE_TIME): \
                                   (60000 - RESPONSE_TIME)    )

#define maxtime30_60_ms(timeleft, root_nrep) ( \
   (timeleft>140 && root_nrep>30) ? (93000 - RESPONSE_TIME): \
                                    (60000 - RESPONSE_TIME)    )

#define maxtime60_60_ms(timeleft, root_nrep) ( \
   (timeleft>280 && root_nrep>30) ? (113000 - RESPONSE_TIME): \
                                    ( 60000 - RESPONSE_TIME)    )

#define maxtime180_60_ms(timeleft, root_nrep) ( \
   (timeleft>1800 && root_nrep>30) ? (233000 - RESPONSE_TIME): \
                                     ( 60000 - RESPONSE_TIME)    )

#ifdef INANIWA_SHIFT
#  define INANIWA_TIME inaniwa_flag ? (5000 - RESPONSE_TIME):
#else
#  define INANIWA_TIME
#endif

#define maxtime_ms(timeleft, root_nrep) ( \
   INANIWA_TIME \
   (THINK_TIME==  0 && BYOYOMI_TIME==10)?  (10000 - RESPONSE_TIME): \
   (THINK_TIME==  0 && BYOYOMI_TIME==30)?  (30000 - RESPONSE_TIME): \
   (THINK_TIME==  0 && BYOYOMI_TIME==60)?  (60000 - RESPONSE_TIME): \
   (THINK_TIME== 480&& BYOYOMI_TIME== 0)? maxtime8_0_ms(timeleft, root_nrep):\
   (THINK_TIME== 900&& BYOYOMI_TIME== 0)? maxtime15_0_ms(timeleft, root_nrep):\
   (THINK_TIME==1500&& BYOYOMI_TIME== 0)? maxtime25_0_ms(timeleft, root_nrep):\
   (THINK_TIME== 900&& BYOYOMI_TIME==60)? maxtime15_60_ms(timeleft, root_nrep):\
   (THINK_TIME==1800&& BYOYOMI_TIME==60)? maxtime30_60_ms(timeleft, root_nrep):\
   (THINK_TIME==3600&& BYOYOMI_TIME==60)? maxtime60_60_ms(timeleft, root_nrep):\
   (THINK_TIME==10800&&BYOYOMI_TIME==60)? maxtime180_60_ms(timeleft,root_nrep):\
                     1800    )

#define maxtimeAll_ms()    (355 * 1000)
#define maxtimeTurn_ms()   (420 * 1000)
#define maxtimePonder_ms() (420 * 1000)

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

static int timeCheck()
{
    unsigned int tnow, tMaxSpent, tMaxPonder;
    int timeleft = THINK_TIME - (myTurn == white ? sec_w_total : sec_b_total);

    //make sure at least one loop is done (even at the risk of timeup)
    if (plane.lastDoneItd == 0) return 0;

    tMaxSpent = maxtime_ms(timeleft, g_ptree->nrep);
    tMaxPonder = maxtimePonder_ms();

    int rwdBonusTime = ADD_RWD_BONUS(tMaxSpent);
    int retryBonusTime = timerec.bonustime();
    
    // T if extending time
    bool unstable = TREAT_RETRY_UNSTABLE && timerec.retryingAny();
    
    if (timeleft > MINTIME4EXTEND_SEC) {
        if (unstable) {
            tMaxSpent *= UNSTABLE_EXTEND_COEFF;
        }
        
        tMaxSpent += retryBonusTime;
        if (plane.rwdLatch2) {
            tMaxSpent += rwdBonusTime;
        }
    }

    get_elapsed(&tnow);
    if (!DBG_NO_TIME_CHECK && !(game_status & flag_problem)) {
        if ((!(game_status & flag_pondering) &&
             (tnow > time_start + tMaxSpent ||
              (tnow > (unsigned int)(time_turn_start + maxtimeTurn_ms()) &&
               tnow > (unsigned int)(time_start + maxtimeAll_ms())))) ||
            tnow > time_start + tMaxPonder) {

            MSTOut("time expired now %u pond %d st %u tnst %u mxspen %d mxpond %d\n",
                   tnow, ((game_status & flag_pondering)?1:0), time_start, 
                   time_turn_start, tMaxSpent, tMaxPonder);
            return 1;
        }
    }

    return 0;
}
