OBJS =data.o main.o io.o proce.o utility.o ini.o attack.o book.o makemove.o \
      unmake.o time.o csa.o valid.o bitop.o iterate.o searchr.o search.o \
      quiesrch.o evaluate.o swap.o  hash.o root.o next.o movgenex.o \
      genevasn.o gencap.o gennocap.o gendrop.o mate1ply.o rand.o learn1.o \
      learn2.o evaldiff.o problem.o ponder.o thread.o sckt.o debug.o mate3.o \
      genchk.o phash.o dfpn.o dfpnhash.o

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

OPT =-DNDEBUG -DMINIMUM -DHAVE_SSE4 -msse4.1 -DDFPN -DTLP -DDFPN_CLIENT -DINANIWA_SHIFT -DMNJ_LAN -DCSA_LAN

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
	$(MAKE) CC=gcc CFLAGS='-std=gnu99 -O2 -Wall $(OPT)' LDFLAG1='-lm -lpthread' bonanza

gcc-pgo:
	$(MAKE) clean
	gcc -std=gnu99 -O2 -Wall $(OPT) -fprofile-generate -o bonanza -lm -lpthread $(OBJS:.o=.c)
	$(MAKE) run-prof
	gcc -std=gnu99 -O2 -Wall $(OPT) -fprofile-use -o bonanza -lm -lpthread $(OBJS:.o=.c)

icc-ampl:
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -g -O2 -fno-inline-functions' LDFLAG1='-pthread -g' bonanza

icc:
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -O2 -ipo' LDFLAG1='-static -ipo -pthread' bonanza

icc-pgo:
	$(MAKE) clean
	mkdir profdir
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -O2 -prof_gen -prof_dir ./profdir' LDFLAG1='-static -pthread' bonanza
	$(MAKE) run-prof
	touch *.[ch]
	$(MAKE) CC=icc CFLAGS='-w2 $(OPT) -std=gnu99 -O2 -ipo -prof_use -prof_dir ./profdir' LDFLAG1='-static -ipo -pthread' bonanza

bonanza : $(OBJS)
	$(CC) $(LDFLAG1) -o bonanza $(OBJS) $(LDFLAG2)

$(OBJS) : shogi.h param.h bitop.h
dfpn.o dfpnhash.o: dfpn.h

.c.o :
	$(CC) -c $(CFLAGS) $*.c

clean :
	rm -f *.o *.il *.da *.gcda *.gcno *.bb *.bbg *.dyn
	rm -f  bonanza gmon.out runprof
	rm -fr profdir

run-prof:
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
