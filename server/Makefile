
# uncomment this to use icc
#USE_ICC = true

MPI_CC  = mpicc
MPI_CXX = mpicxx
IPOOPT = 
BN_OBJTYPE = gcc-bonk

ifdef USE_ICC
MPI_CC  = mpiicc
MPI_CXX = mpiicpc
IPOOPT = -ipo
BN_OBJTYPE = icc-bonk
endif

BNSFLAGS = -DMINIMUM -DHAVE_SSE4 -msse4.1 -DDFPN -DTLP -DDFPN_CLIENT \
           -DINANIWA_SHIFT -DCSA_LAN
INCLUDES =
CPPFLAGS = -D__STDC_FORMAT_MACROS
CFLAGS   = -Wall -g
CXXFLAGS = -Wall -g $(IPOOPT)
LDFLAGS  =
LIBS     = -lrt

DEP_FILE = Makefile.dep
DEPEND   = $(MPI_CXX) -MM -MG $(INCLUDES) $(BNSFLAGS) $(CPPFLAGS) $(CXXFLAGS)

COMPILE  = cc $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
MPI_COMPILE = $(MPI_CXX) $(INCLUDES) $(BNSFLAGS) $(CPPFLAGS) $(CXXFLAGS)
MPI_LINK    = $(MPI_CXX) $(INCLUDES) $(CXXFLAGS) $(LDFLAGS)

########

BNS = bonanza6
BNS_OBJS = $(BNS)/data.o $(BNS)/main.o $(BNS)/io.o $(BNS)/proce.o \
           $(BNS)/utility.o $(BNS)/ini.o $(BNS)/attack.o $(BNS)/book.o \
           $(BNS)/makemove.o $(BNS)/unmake.o $(BNS)/time.o $(BNS)/csa.o \
           $(BNS)/valid.o $(BNS)/bitop.o $(BNS)/iterate.o \
           $(BNS)/searchr.o $(BNS)/search.o $(BNS)/quiesrch.o \
           $(BNS)/evaluate.o $(BNS)/swap.o $(BNS)/hash.o $(BNS)/root.o \
           $(BNS)/next.o $(BNS)/movgenex.o $(BNS)/genevasn.o \
           $(BNS)/gencap.o $(BNS)/gennocap.o $(BNS)/gendrop.o \
           $(BNS)/mate1ply.o $(BNS)/rand.o $(BNS)/learn1.o $(BNS)/learn2.o \
           $(BNS)/evaldiff.o $(BNS)/problem.o $(BNS)/ponder.o \
           $(BNS)/thread.o $(BNS)/sckt.o $(BNS)/debug.o $(BNS)/mate3.o \
           $(BNS)/genchk.o $(BNS)/phash.o $(BNS)/dfpn.o \
           $(BNS)/dfpnhash.o $(BNS)/bbini.o

BIN_OBJS = slave3.o putils.o invokempi.o master3.o
BIN = ./bin/gmx

U2B_OBJS = u2b.o
U2B = ./bin/u2b

SRCS = slave3.cpp putils.cpp invokempi.cpp master3.cpp u2b.c

########

all: $(BIN) $(U2B)

%.o: %.cpp
	$(MPI_COMPILE) -c -o $@ $<

%.o: %.c
	$(COMPILE) -c -o $@ $<

$(BIN): bns_objs $(BIN_OBJS)
	$(MPI_LINK) -o $@ $(BIN_OBJS) $(BNS_OBJS) $(LIBS)

bns_objs:
	$(MAKE) -C $(BNS) CLUSTOPT=-DCLUSTER_PARALLEL $(BN_OBJTYPE)


$(U2B): u2b.o
	$(COMPILE) -o $@ $<

depend:
	$(DEPEND) $(SRCS) > $(DEP_FILE)

clean:
	rm -fr $(BIN) $(U2B) *.o *.stackdump logslave* logmaster*
	$(MAKE) -C $(BNS) clean

# 依存ファイルの読み込み
-include $(DEP_FILE)
