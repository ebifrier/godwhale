######## No need to change below (probably) ########

CC       = g++
CPPFLAGS = -I$(BN)

BN   = ../bonanza6
SRCS = convfv.cc
OBJS = convfv.o
BIN  = convfv

########

$(BIN): $(OBJS)
	$(CC) -o $@ $<

convfv.o: convfv.cc $(BN)/shogi.h
	$(CC) $(CPPFLAGS) -o $@ -c convfv.cc

clean:
	rm -fr $(BIN) *.o
