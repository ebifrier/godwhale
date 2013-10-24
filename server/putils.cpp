/* $Id: putils.cpp,v 1.3 2012-04-20 07:32:31 eikii Exp $ */

#include "pcommon3.h"

 // for mm_pause
#include <xmmintrin.h>

 // for nanosleep
#include <time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

 // set X if a single pause() takes X ns.
#define PAUSE_DURATION_NS 1

void ei_clock_gettime(struct timespec* tsp)
{
#ifdef __MACH__
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    tsp->tv_sec = mts.tv_sec;
    tsp->tv_nsec = mts.tv_nsec;
#elif _WIN32
    ULONGLONG tick = GetTickCount64();
    tsp->tv_sec  = (int)(tick / 1000);
    tsp->tv_nsec = (int)((tick % 1000) * 1000 * 1000); // msec -> nanosec
#else
    clock_gettime(CLOCK_REALTIME, tsp);
#endif
}

// bonanza mv format - cap:4 pc:4 prom:1 src:7 dst:7  -> convert to 4,4,4,8,8
int readable_c(int mv)
{
    //if (mv == NULLMV.v) return 0;
    if (mv == 0) return 0;

    int x = mv;
    int dst  = x & 0x7f;
    int src  = (x >>  7) & 0x7f;
    int prom = (x <<  2) & 0x10000;
    int cappc = (x <<  5) & 0x0ff00000;
    int dx = 9 - (dst % 9);
    int dy = 1 + (dst / 9);
    int sx = (src >= nsquare ?   0     : 9 - (src % 9));
    int sy = (src >= nsquare ? src % 9 : 1 + (src / 9));
    return (cappc | prom | (sx << 12) | (sy << 8) | (dx << 4) | dy);
}
 
int readable(mvC mv)
{
    return readable_c(mv.v);
}

 //**** time related

static int origin_sec = 0, origin_nsec = 0;
int time_offset = 0;

static int timespec2int(int sec, int nsec)
{
    // 1/14/2010 changed from us to 10us
    //return ((sec - origin_sec) * 1000000 + (nsec - origin_nsec) / 1000);
    return ((sec - origin_sec) * 100000 + (nsec - origin_nsec) / 10000);
}

int worldTime()
{
    struct timespec ts;
    ei_clock_gettime(&ts);

    return (timespec2int(ts.tv_sec, ts.tv_nsec) - time_offset);
    // NOTE before offset is set, this is equal to master's time
}

int64_t worldTimeLl()
{
    struct timespec ts;
    int64_t x;

    ei_clock_gettime(&ts);
    x = ts.tv_sec;
    x *= 1000000000;
    x += ts.tv_nsec;
    return x;
}

 // called by mpi_init(), just once.  after that, worldTime() will
 // return the 10us unit since start
void initTime()
{
    struct timespec ts;

    ei_clock_gettime(&ts);
    origin_sec = ts.tv_sec;
    origin_nsec = ts.tv_nsec;
}

#if USE_PAUSE
static void micropauseCore(int t)
{
    // single call of pause() pauses 10ns on i7 860.
    // if you want to pause 40us, call pause 40us/10ns = 4000 times
    int n = (1000 * t / PAUSE_DURATION_NS);
    forr (i, 1, n) {
        _mm_pause();
    }
}
#endif

#ifndef _WIN32
static void microsleepCore(int t)
{
    struct timespec ts, tsrem;

    ts.tv_sec = 0;
    ts.tv_nsec = t * 1000;
    nanosleep(&ts, &tsrem);
}
#endif

static void microspinCore(int t)
{
    int64_t n = t * INCS_PER_USEC;

    int x = t;
    forr (i, 1, n) {
        x &= x - 2;
    }
    x_dmy_for_calcinc &= x;
}

void microsleep(int t)
{
#if 0
#if (USE_SPIN)
    microspinCore(t);
#else
#if (!USE_PAUSE)
    microsleepCore(t);
#else
    micropauseCore(t);
#endif
#endif
#else 
if (VMMODE)
    microspinCore(t);
else
    microspinCore(t);
#endif
}

void microsleepMaster(int t)
{
#if 0
#if (USE_SPIN)
    microspinCore(t);
#else
#if (!USE_PAUSE_MASTER)
    microsleepCore(t);
#else
    micropauseCore(t);
#endif
#endif
#else
if (VMMODE)
    microspinCore(t);
else
#ifdef _WIN32
    microspinCore(t);
#else
    microsleepCore(t);
#endif
#endif
}
