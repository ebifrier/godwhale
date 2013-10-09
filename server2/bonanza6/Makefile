
#### disable this if compilation fails due to 'pthread_setaffinity_np'
AFFINOPT = -DENABLE_AFFINITY

#### enable on sandy-bridge
#AVXOPT = -mavx -DHAVE_AVX

#### choose either
#SSEOPT = -DHAVE_SSE4 -msse4.1
SSEOPT = -DHAVE_SSE2 -msse2

######## No need to change below (probably) ########

OBJS =data.o main.o io.o proce.o utility.o ini.o attack.o book.o makemove.o \
      unmake.o time.o csa.o valid.o bitop.o iterate.o searchr.o search.o \
      quiesrch.o evaluate.o swap.o  hash.o root.o next.o movgenex.o \
      genevasn.o gencap.o gennocap.o gendrop.o mate1ply.o rand.o learn1.o \
      learn2.o evaldiff.o problem.o ponder.o thread.o sckt.o debug.o mate3.o \
      genchk.o phash.o dfpn.o dfpnhash.o bbini.o

SRCS =data.c main.c io.c proce.c utility.c ini.c attack.c book.c makemove.c \
      unmake.c time.c csa.c valid.c bitop.c iterate.c searchr.c search.c \
      quiesrch.c evaluate.c swap.c  hash.c root.c next.c movgenex.c \
      genevasn.c gencap.c gennocap.c gendrop.c mate1ply.c rand.c learn1.c \
      learn2.c evaldiff.c problem.c ponder.c thread.c sckt.c debug.c mate3.c \
      genchk.c phash.c dfpn.c dfpnhash.c    shogi.h bitop.h param.h dfpn.h  \
      handjoseki.h Makefile hand.jos bbini.c mate3.h

PROBFILE=~/synced/probbnz/gihou/d1.dpp

# Compile Options
#
# -DNDEBUG (DEBUG)   builds release (debug) version of Bonanza.
# -DMINIMUM          disables some auxiliary functions that are not necessary
#                    to play a game, e.g., book composition and optimization
#                    of evaluation function.
# -DHAVE_SSE2 -msse2 use SSE2 instructions for speed
# -DHAVE_SSE4 -msse4.1 use SSE2 and SSE4.1 instructions for speed
# -DTLP              enables thread-level parallel search.
# -DMPV              enables multi-PV search.
# -DCSA_LAN          enables bonanza to talk CSA Shogi TCP/IP protcol.
# -DMNJ_LAN          enables a client-mode of cluster searches.
# -DNO_LOGGING       suppresses dumping log files.
# -DUSI              enables USI mode (not implemented).
# -DINANIWA_SHIFT    enables an Inaniwa strategy detection.
# -DDFPN             build the DFPN worker of mate-problems server.
# -DDFPN_CLIENT      enables the client-mode of mate-problem server.

#### enable to use gprof
#PROFOPT = -pg

#### enable this for cluster parallel
# bonk makefile should call like this: make CLUSTOPT=-DCLUSTER_PARALLEL gcc-bonk
#CLUSTOPT = 
#CLUSTOPT = -DCLUSTER_PARALLEL  ==> this should be MAKE argument for bonkras

#OPT =-DNDEBUG -DMINIMUM -DHAVE_SSE4 -msse4.1 -DDFPN -DTLP -DDFPN_CLIENT -DINANIWA_SHIFT -DMNJ_LAN -DCSA_LAN
OPT = -DNDEBUG -DMINIMUM $(SSEOPT) -DDFPN -DTLP -DDFPN_CLIENT \
      -DINANIWA_SHIFT -DUSI $(AFFINOPT) $(CLUSTOPT) $(AVXOPT)

help:
	@echo "try targets as:"
	@echo
	@echo "  gcc"
	@echo "  gcc-pg"
	@echo "  gcc-pgo"
	@echo "  icc"
	@echo "  icc-pgo"
	@echo "  icc-ampl"

gcc:
	$(MAKE) CC=gcc CFLAGS='-std=gnu99 -O2 -Wall $(OPT) $(PROFOPT)' LDFLAG1='-lm -lpthread $(PROFOPT)' bonanza

gcc-bonk:
	$(MAKE) CC=gcc CFLAGS='-std=gnu99 -O2 -Wall $(OPT)' LDFLAG1='-lm -lpthread' bonkras

gpgo1:
	$(MAKE) CC=gcc CFLAGS='-std=gnu99 -O2  -fprofile-generate -Wall $(OPT)' LDFLAG1='-lm -lpthread' bonkras

gpgo2:
	$(MAKE) CC=gcc CFLAGS='-std=gnu99 -O2  -fprofile-use -Wall $(OPT)' LDFLAG1='-lm -lpthread' bonkras

gcc-pgo:
	$(MAKE) clean
	gcc -std=gnu99 -O2 -Wall $(OPT) -fprofile-generate -o bonanza -lm -lpthread $(OBJS:.o=.c)
	$(MAKE) run-prof
	gcc -std=gnu99 -O2 -Wall $(OPT) -fprofile-use -o bonanza -lm -lpthread $(OBJS:.o=.c)

icc-ampl:
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -g -O2 -fno-inline-functions' LDFLAG1='-pthread -g' bonanza

icc:
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -O2 -g -ipo' LDFLAG1='        -g -ipo -pthread' bonanza

icc-bonk:
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -O2 -ipo' LDFLAG1='-static -ipo -pthread' bonkras

ipgo1:
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -O2   -prof_gen -prof_dir ./profdir' LDFLAG1='    -static  -pthread' bonkras

ipgo2:
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -O3 -ipo  -prof_use -prof_dir ./profdir' LDFLAG1='-static -ipo -pthread' bonkras

icc-pgo:
	$(MAKE) clean
	mkdir profdir
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -O2 -prof_gen -prof_dir ./profdir' LDFLAG1='        -pthread' bonanza
	$(MAKE) run-prof
	#touch *.[ch]
	rm *.o
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -O3 -ipo -prof_use -prof_dir ./profdir' LDFLAG1='        -ipo -pthread' bonanza

bonkras : $(OBJS)

bonanza : $(OBJS)
	$(CC) $(LDFLAG1) -o bonanza $(OBJS) $(LDFLAG2)

$(OBJS) : shogi.h param.h bitop.h
dfpn.o dfpnhash.o: dfpn.h
iterate.o: handjoseki.h

# 3/22/2012 w/ icc 12.0, compile w/ -O3 fails.  avoid CFLAG due to this
bbini.o: bbini.c
	$(CC) -c $(AVXOPT) -msse2 bbini.c

#	$(CC) -c $(CFLAGS) -msse2 bbatk.cpp

mate3.o: mate3.c shogi.h bitop.h param.h mate3.h

ini.o: ini.c shogi.h bitop.h param.h mate3.h

#.c.o :
#	$(CC) $(OPT) -c $(CFLAGS) $*.c

clean :
	rm -f *.o *.il *.da *.gcda *.gcno *.bb *.bbg *.dyn
	rm -f  bonanza gmon.out runprof
	rm -fr profdir

save:
	tar cf - $(SRCS) | gzip  > sav/t`date '+%m%d%y'`.tgz

run-prof:
	rm -rf  probcmd
	echo "limit depth 9" >  probcmd
	echo "problem      " >> probcmd
	echo "quit         " >> probcmd
	cp $(PROBFILE) problem.csa
	./bonanza < probcmd

run-proflong:
	@if [ ! -d log ]; then mkdir log; fi
	@echo "peek off"               > runprof
#	@echo "dfpn hash 22"          >> runprof
#	@echo "problem mate"          >> runprof
#	@echo "learn no-ini 32 32 1 2 2"   >> runprof
	@echo "limit time extendable" >> runprof
	@echo "limit time 0 1"        >> runprof
	@echo "tlp num 2"             >> runprof
	@echo "move 77"               >> runprof
	@echo "new"                   >> runprof
	@echo "move 77"               >> runprof
	@echo "new"                   >> runprof
	@echo "move 77"               >> runprof
	@echo "new"                   >> runprof
	@echo "move 77"               >> runprof
	@echo "new"                   >> runprof
	@echo "move 77"               >> runprof
	@echo "new"                   >> runprof
	@echo "move 77"               >> runprof
	@echo "new"                   >> runprof
	@echo "move 77"               >> runprof
	@echo "quit"                  >> runprof
	@./bonanza < runprof
