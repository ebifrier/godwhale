#
# Makefile for server
#

include ../Makefile-common

OPT_CPPFLAGS = -DBOOST_LOG_DYN_LINK -DCSA_LAN -DGODWHALE_SERVER
# -DCSA_LAN_ACCEPT

OPT_CXXFLAGS = -fpermissive
OPT_LIBS     = -lboost_thread -lboost_system -lboost_timer -lboost_atomic \
	-lboost_chrono -lboost_log -lboost_log_setup -lboost_regex

BIN  = godwhale
OBJS = client.o main.o board.o move.o server.o thinktime.o \
       logger.o util.o bonanza_if.o server_move.o
SRCS = $(OBJS:.o=.cpp)

PRECOMP_H   = precomp.h
PRECOMP_GCH = precomp.h.gch

.DEFAULT_GOAL = all

all: $(BIN)

$(BIN): $(OBJS) $(BNZ_OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) $(BNZ_OBJS) -o $@ $(LIBS)

.cpp.o:
	$(CXX) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(PRECOMP_GCH): $(PRECOMP_H)
	$(CXX) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

depend: $(SRCS)
	$(CXX) $(INCLUDES) $(CPPFLAGS) $(FLAGS) -MM -MG $(SRCS) > make.dep

clean: common-clean
	rm -f *.o *.exe $(BIN) $(PRECOMP_GCH)

clean-full: clean
	rm -f make.dep

$(OBJS): $(PRECOMP_GCH)

-include make.dep
