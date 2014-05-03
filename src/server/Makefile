
SSE2_OPT = -DHAVE_SSE2 -msse2
SSE4_OPT = -DHAVE_SSE4 -msse4.1
#AVX_OPT = -DHAVE_AVX -mavx

OPT = -DNDEBUG -DMINIMUM -DTLP -DINANIWA_SHIFT -DBOOST_LOG_DYN_LINK \
      -DCSASHOGI -DCSA_LAN -DCSA_LAN_ACCEPT -DBITBOARD64 \
      -DUSE_FAST_BIN -DGODWHALE_SERVER $(SSE2_OPT) $(SSE4_OPT) $(AVX_OPT)

CXX      = g++
INCLUDES = -I. -I/usr/include
CPPFLAGS = 
CXXFLAGS = $(OPT) -std=c++11 -Wall -O3 -g -fpermissive
LIBS     = -pthread -lboost_thread -lboost_system -lboost_timer -lboost_atomic -lboost_chrono -lboost_log -lboost_log_setup -lboost_regex -lboost_log

BNZ_DIR  = ../common/bonanza6
BN = $(BNZ_DIR)
BNZ_OBJS = $(BN)/data.o $(BN)/io.o $(BN)/proce.o $(BN)/utility.o \
      $(BN)/ini.o $(BN)/attack.o $(BN)/book.o $(BN)/makemove.o \
      $(BN)/unmake.o $(BN)/time.o $(BN)/csa.o $(BN)/valid.o $(BN)/bitop.o \
      $(BN)/iterate.o $(BN)/searchr.o $(BN)/search.o \
      $(BN)/quiesrch.o $(BN)/evaluate.o $(BN)/swap.o $(BN)/hash.o \
      $(BN)/root.o $(BN)/next.o $(BN)/movgenex.o \
      $(BN)/genevasn.o $(BN)/gencap.o $(BN)/gennocap.o $(BN)/gendrop.o \
      $(BN)/mate1ply.o $(BN)/rand.o $(BN)/learn1.o \
      $(BN)/learn2.o $(BN)/evaldiff.o $(BN)/problem.o $(BN)/ponder.o \
      $(BN)/thread.o $(BN)/sckt.o $(BN)/debug.o $(BN)/mate3.o \
      $(BN)/genchk.o $(BN)/phash.o $(BN)/dfpn.o $(BN)/dfpnhash.o \
      $(BN)/bbini.o

BIN  = godwhale
OBJS = client.o main.o board.o move.o server.o thinktime.o \
       logger.o util.o bonanza_if.o server_move.o

.cpp.o:
	$(CXX) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

all: $(BIN)

$(BIN): $(OBJS) bnz_objs
	$(CXX) $(LDFLAGS) $(OBJS) $(BNZ_OBJS) -o $@ $(LIBS)

bnz_objs:
	make -C $(BNZ_DIR) BNZ_OPT='$(OPT)' gcc-godwhale

clean:
	rm -fr *.o *.exe $(BIN)
	make -C $(BNZ_DIR) clean
