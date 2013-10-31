######## No need to change below (probably) ########

CC       = g++
CPPFLAGS = -I$(BN)

BN   = ../bonanza6
OBJS = convfv.o
BIN  = convfv

########

$(BIN): $(OBJS)
	$(CC) -o $@ $<

convfv.o: convfv.cpp $(BN)/shogi.h
	$(CC) $(CPPFLAGS) -o $@ -c convfv.cpp

clean:
	rm -fr $(BIN) *.o

wget:
	wget http://133.242.205.114/tmp/fv.bin
