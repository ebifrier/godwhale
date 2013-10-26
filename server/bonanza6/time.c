#include <limits.h>
#include <assert.h>
#if ! defined(_WIN32)
#  include <sys/time.h>
#  include <time.h>
#endif
#include "shogi.h"


/*
  [inputs]
  global variables:
    sec_limit       time limit of a side for an entire game in seconds
                    持ち時間

    sec_limit_up    time limit for a move when the ordinary time has run out.
                    the time is known as 'byo-yomi'.
                    秒読み時間

    sec_b_total     seconds spent by black 
                    先手の思考した時間

    sec_w_total     seconds spent by white
                    後手の思考した時間

    time_response

    ( game_status & flag_puzzling )        time-control for puzzling-search
                                           相手番の時間制御

    ( game_status & flag_pondering )       time-control for pondering-search
                                           相手番（先読み中）の時間制御

    ( game_status & flag_time_extendable ) bonanza isn't punctual to the time
                                           時間を多少オーバーする可能性あり
  
  [outputs]
  global variables:
    time_limit      tentative deadline for searching in millisecond
    time_max_limit  maximum allowed time to search in millisecond
  
  [working area]
  local variables:
    sec_elapsed     second spent by the side to move
    sec_left        time left for the side in seconds
    u0              tentative deadline for searching in second
    u1              maximum allowed time to search in second
*/

/* 
  argument:
    turn            a side to move
*/
void CONV
set_search_limit_time( int turn )
{
  unsigned int u0, u1;
  
  /* no time-control */
  if ( sec_limit_up == UINT_MAX || ( game_status & flag_pondering ) )
    {
      time_max_limit = time_limit = UINT_MAX;
      return;
    }

#if defined(USI)
  if ( usi_mode != usi_off && usi_byoyomi )
    {
      time_max_limit = time_limit = usi_byoyomi;
      Out( "- time ctrl: %u -- %u\n", time_limit, time_max_limit );
      return;
    }
#endif

  /* not punctual to the time */
  if ( ! sec_limit && ( game_status & flag_time_extendable ) )
    {
      u0 = sec_limit_up;
      u1 = sec_limit_up * 5U;
    }
  /* have byo-yomi */
  else if ( sec_limit_up )
    {
      unsigned int umax, umin, sec_elapsed, sec_left;

      sec_elapsed = turn ? sec_w_total : sec_b_total;
      sec_left    = ( sec_elapsed <= sec_limit ) ? sec_limit - sec_elapsed : 0;
      u0          = ( sec_left + ( TC_NMOVE / 2U ) ) / TC_NMOVE;

      /*
        t = 2s is not beneficial since 2.8s are almost the same as 1.8s.
        So that, we rather want to use up the ordinary time.
      */
      if ( u0 == 2U ) { u0 = 3U; }

      /* 'byo-yomi' is so long that the ordinary time-limit is negligible. */
      if ( u0 < sec_limit_up * 5U ) { u0 = sec_limit_up * 5U; }
    
      u1 = u0 * 5U;

      // 思考可能な最大時間は 残り持ち時間＋秒読み時間
      umax = sec_left + sec_limit_up;
      umin = 1;

      if ( u0 > umax ) { u0 = umax; }
      if ( u1 > umax ) { u1 = umax; }
      if ( u0 < umin ) { u0 = umin; }
      if ( u1 < umin ) { u1 = umin; }
    }
  /* no byo-yomi */
  else {
    unsigned int sec_elapsed, sec_left;
    
    sec_elapsed = turn ? sec_w_total : sec_b_total;

    /* We have some seconds left to think. */
    if ( sec_elapsed + SEC_MARGIN < sec_limit )
      {
        sec_left = sec_limit - sec_elapsed;
        u0       = ( sec_left + ( TC_NMOVE / 2U ) ) / TC_NMOVE;
        
        /* t = 2s is not beneficial since 2.8s is almost the same as 1.8s. */
        /* So that, we rather want to save the time.                       */
        if ( u0 == 2U ) { u0 = 1U; }
        u1 = u0 * 5U;
      }
    /* We are running out of time... */
    else { u0 = u1 = 1U; }
  }
  
  time_limit     = u0 * 1000U + 1000U - time_response;
  time_max_limit = u1 * 1000U + 1000U - time_response;
  
  // 先読み用の手を予測（相手番）するときは、時間を短く設定する
  if ( game_status & flag_puzzling )
    {
      time_max_limit = time_max_limit / 5U;
      time_limit     = time_max_limit / 5U;
    }

  Out( "- time ctrl: %u -- %u\n", time_limit, time_max_limit );
}


// 手番側が今手を指したと仮定し、その分だけ時間を進めます。
int CONV
update_time( int turn )
{
  unsigned int te, time_elapsed;
  int iret;

  iret = get_elapsed( &te );
  if ( iret < 0 ) { return iret; }

  time_elapsed = te - time_turn_start;
  sec_elapsed  = time_elapsed / 1000U;
  if ( ! sec_elapsed ) { sec_elapsed = 1U; }

  if ( turn ) { sec_w_total += sec_elapsed; }
  else        { sec_b_total += sec_elapsed; }
  
  time_turn_start = te;

  return 1;
}


// 今の手がelapsed_new時間で指されたと仮定し、新たな経過時間[s]を設定します。
void CONV
adjust_time( unsigned int elapsed_new, int turn )
{
  const char *str = "TIME SKEW DETECTED";

  if ( turn == white )
    {
      if ( sec_w_total + elapsed_new < sec_elapsed )
        {
          out_warning( str );
          sec_w_total = 0;
        }
      else { sec_w_total += elapsed_new - sec_elapsed; };
    }
  else {
    if ( sec_b_total + elapsed_new < sec_elapsed )
      {
        out_warning( str );
        sec_b_total = 0;
      }
    else { sec_b_total += elapsed_new - sec_elapsed; };
  }
}


// 残り時間[s]を新たに設定します。
int CONV
reset_time( unsigned int b_remain, unsigned int w_remain )
{
  if ( sec_limit_up == UINT_MAX )
    {
      str_error = "no time limit is set";
      return -2;
    }

  if ( b_remain > sec_limit || w_remain > sec_limit )
    {
      snprintf( str_message, SIZE_MESSAGE,
                "time remaining can't be larger than %u", sec_limit );
      str_error = str_message;
      return -2;
    }

  sec_b_total = sec_limit - b_remain;
  sec_w_total = sec_limit - w_remain;
  Out( "  elapsed: b%u, w%u\n", sec_b_total, sec_w_total );

  return 1;
}


// time[ms]を HH:MM:SS.XX 形式の文字列に変換します。
const char * CONV
str_time( unsigned int time )
{
  static char str[32];
  unsigned int time_hour = ((time / 1000) / 60) / 60;
  unsigned int time_min  = ((time / 1000) / 60) % 60;
  unsigned int time_sec  =  (time / 1000) % 60;
  unsigned int time_mil  =  (time % 1000);

  if ( time_hour == 0 )
    {
      snprintf( str, sizeof(str), "%02u:%02u.%02u",
                time_min, time_sec, time_mil/10 );
    }
  else {
    snprintf( str, sizeof(str), "%02u:%02u:%02u.%02u",
              time_hour, time_min, time_sec, time_mil/10 );
  }
  return str;
}


// time[ms]を SS.XX or M:SS 形式の文字列に変換します。
const char * CONV
str_time_symple( unsigned int time )
{
  static char str[32];
  unsigned int time_min = time / 60000;
  unsigned int time_sec = time / 1000;
  unsigned int time_mil = time % 1000;

  if ( time_min == 0 )
    {
      snprintf( str, sizeof(str), "%5.2f", (float)(time_sec*1000+time_mil)/1000.0 );
    }
  else { snprintf( str, sizeof(str), "%2u:%02u", time_min, time_sec % 60 ); }

  return str;
}


// 実際にこのプロセスがユーザー空間で使った時刻(1ms単位)を返します。
// CPUの使用率などを求めるときに使います。
int CONV
get_cputime( unsigned int *ptime )
{
#if defined(_WIN32)
  HANDLE         hProcess;
  FILETIME       CreationTime, ExitTime, KernelTime, UserTime;
  ULARGE_INTEGER uli_temp;

  hProcess = GetCurrentProcess();
  if ( GetProcessTimes( hProcess, &CreationTime, &ExitTime,
                        &KernelTime, &UserTime ) )
    {
      uli_temp.LowPart  = UserTime.dwLowDateTime;
      uli_temp.HighPart = UserTime.dwHighDateTime;
      *ptime = (unsigned int)( uli_temp.QuadPart / 1000 / 10 );
    }
  else {
#  if 0 /* winnt */
    str_error = "GetProcessTimes() failed.";
    return -1;
#  else /* win95 */
    *ptime = 0U;
#  endif
  }
#else
  clock_t clock_temp;
  struct tms t;

  if ( times( &t ) == -1 )
    {
      str_error = "times() faild.";
      return -1;
    }
  clock_temp = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;
  *ptime  = (unsigned int)( clock_temp / clk_tck ) * 1000;
  clock_temp %= clk_tck;
  *ptime += (unsigned int)( (clock_temp * 1000) / clk_tck );
#endif /* no _WIN32 */
  
  return 1;
}


// システムの経過時刻(ms単位)を返します。
int CONV
get_elapsed( unsigned int *ptime )
{
#if defined(_WIN32)
  /*
    try timeBeginPeriod() and timeGetTime(),
    or QueryPerformanceFrequency and QueryPerformanceCounter()
  */
  FILETIME       FileTime;
  ULARGE_INTEGER uli_temp;

  GetSystemTimeAsFileTime( &FileTime );
  uli_temp.LowPart  = FileTime.dwLowDateTime;
  uli_temp.HighPart = FileTime.dwHighDateTime;
  *ptime            = (unsigned int)( uli_temp.QuadPart / 1000U / 10U );
#else
  struct timeval timeval;

  if ( gettimeofday( &timeval, NULL ) == -1 )
    {
      str_error = "gettimeofday() faild.";
      return -1;
    }
  *ptime  = (unsigned int)timeval.tv_sec * 1000;
  *ptime += (unsigned int)timeval.tv_usec / 1000;
#endif /* no _WIN32 */

  return 1;
}
