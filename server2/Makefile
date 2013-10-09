
# uncomment this to use icc
#USE_ICC = true

######## No need to change below (probably) ########

BN_SRCS = $(BN)/data.c $(BN)/main.c $(BN)/io.c $(BN)/proce.c $(BN)/utility.c \
      $(BN)/ini.c $(BN)/attack.c $(BN)/book.c $(BN)/makemove.c \
      $(BN)/unmake.c $(BN)/time.c $(BN)/csa.c $(BN)/valid.c $(BN)/bitop.c \
      $(BN)/iterate.c $(BN)/searchr.c $(BN)/search.c \
      $(BN)/quiesrch.c $(BN)/evaluate.c $(BN)/swap.c $(BN)/hash.c \
      $(BN)/root.c $(BN)/next.c $(BN)/movgenex.c \
      $(BN)/genevasn.c $(BN)/gencap.c $(BN)/gennocap.c $(BN)/gendrop.c \
      $(BN)/mate1ply.c $(BN)/rand.c $(BN)/learn1.c \
      $(BN)/learn2.c $(BN)/evaldiff.c $(BN)/problem.c $(BN)/ponder.c \
      $(BN)/thread.c $(BN)/sckt.c $(BN)/debug.c $(BN)/mate3.c \
      $(BN)/genchk.c $(BN)/phash.c $(BN)/dfpn.c $(BN)/dfpnhash.c \
      $(BN)/shogi.h $(BN)/param.h $(BN)/bitop.h $(BN)/dfpn.h $(BN)/Makefile \
      $(BN)/cmpsrc.csh $(BN)/handjoseki.h $(BN)/bbini.c $(BN)/mate3.h \

SOURCES = comm3.h makefile master3.cpp pcommon3.h \
      plane.h slave3.cpp sbody3.h perfsl.h putils.cpp invokempi.c splane.h \
      time.h cmpsrc.csh regrunit.sh \
      pgo.sh initact.h entsort.h

U2B_SRCS = u2b.c

#OPT =-DNDEBUG -DMINIMUM -DHAVE_SSE2 -msse2 -DDFPN -DTLP -DDFPN_CLIENT -DINANIWA_SHIFT -DCSA_LAN $(AFFINOPT) $(CLUSTOPT)
BNSOPT = -DMINIMUM -DHAVE_SSE2 -msse2 -DDFPN -DTLP -DDFPN_CLIENT -DINANIWA_SHIFT -DCSA_LAN $(AVXOPT)

BN = bonanza6


########

IPOOPT = 
PGOOPT = 
PGO1OPT = -fprofile-generate
PGO2OPT = -fprofile-use
G_CC  = mpicc -Wall
G_CXX = mpicxx -Wall

ifdef USE_ICC
IPOOPT = -ipo
PGO1OPT = -prof_gen -prof_dir ./profdir
PGO2OPT = -prof_use -prof_dir ./profdir
G_CC  = mpiicc -Wall
G_CXX = mpiicpc -Wall
endif

ifeq ($(PGOPHASE),1)
PGOOPT = $(PGO1OPT)
BN = .
IPOOPT = 
else
ifeq ($(PGOPHASE),2)
PGOOPT = $(PGO2OPT)
BN = .
endif
endif

CC    = cc -Wall
G_MPICCOPT  = $(IPOOPT) $(PGOOPT) -g
G_MPICXXOPT = $(IPOOPT) $(PGOOPT) -g -DTLP
G_MPILDOPT  = -lrt $(IPOOPT) $(PGOOPT) -g

########

G_OBJS = $(BN)/data.o $(BN)/main.o $(BN)/io.o $(BN)/proce.o $(BN)/utility.o \
      $(BN)/ini.o $(BN)/attack.o $(BN)/book.o $(BN)/makemove.o \
      $(BN)/unmake.o $(BN)/time.o $(BN)/csa.o $(BN)/valid.o $(BN)/bitop.o \
      $(BN)/iterate.o $(BN)/searchr.o $(BN)/search.o \
      $(BN)/quiesrch.o $(BN)/evaluate.o $(BN)/swap.o $(BN)/hash.o \
      $(BN)/root.o $(BN)/next.o $(BN)/movgenex.o \
      $(BN)/genevasn.o $(BN)/gencap.o $(BN)/gennocap.o $(BN)/gendrop.o \
      $(BN)/mate1ply.o $(BN)/rand.o $(BN)/learn1.o \
      $(BN)/learn2.o $(BN)/evaldiff.o $(BN)/problem.o $(BN)/ponder.o \
      $(BN)/thread.o $(BN)/sckt.o $(BN)/debug.o $(BN)/mate3.o \
      $(BN)/genchk.o $(BN)/phash.o $(BN)/dfpn.o $(BN)/dfpnhash.o $(BN)/bbini.o

BIN = gmx
U2B = u2b

all: $(BIN) $(U2B)

$(BIN): gobjs invokempi.o putils.o slave3.o master3.o
	$(G_CXX) $(G_OBJS) invokempi.o putils.o slave3.o master3.o -o $@ $(G_MPILDOPT) $(DBGLDOPT)

$(U2B): u2b.o
	$(CC) u2b.o -o $@

u2b.o: u2b.c
	$(CC) -c u2b.c -o u2b.o

pgotgt:
	$(G_CXX) $(G_OBJS) invokempi.o putils.o slave3.o master3.o -o gmx $(G_MPILDOPT) $(DBGLDOPT)


gobjs:
	make -C $(BN) CLUSTOPT=-DCLUSTER_PARALLEL gcc-bonk

iobjs:
	make -C $(BN) CLUSTOPT=-DCLUSTER_PARALLEL icc-bonk

ifdef USE_ICC
all: iobjs invokempi.o putils.o slave3.o master3.o
else
all: gobjs invokempi.o putils.o slave3.o master3.o
endif

bonkobjs: invokempi.o putils.o slave3.o master3.o

invokempi.o: pcommon3.h invokempi.c
	$(G_CC) $(G_MPICCOPT) -c invokempi.c -o invokempi.o

slave3.o: pcommon3.h comm3.h perfsl.h sbody3.h splane.h slave3.cpp
	$(G_CXX) $(G_MPICXXOPT) $(BNSOPT) -c slave3.cpp -o slave3.o

putils.o: pcommon3.h putils.cpp
	$(G_CXX) $(G_MPICXXOPT) -c putils.cpp -o putils.o -msse

master3.o: pcommon3.h comm3.h master3.cpp plane.h time.h initact.h
	$(G_CXX) $(G_MPICXXOPT) -c master3.cpp -o master3.o

save:
	tar cf - $(SOURCES) $(BN_SRCS) | gzip > sav/w`date '+%m%d%y'`.tgz


clean:
	rm -fr gmx u2b *.o *.exe *.stackdump logslave* logmaster* libbonkras.a*
	make -C conv clean
	make -C bonanza6 clean
