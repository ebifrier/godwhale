######## No need to change below (probably) ########

CC       = g++
CPPFLAGS = -I$(BN)

BN   = ../bonanza6
OBJS = convfv.o
BIN  = convfv

########

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $<

convfv.o: convfv.cpp $(BN)/shogi.h
	$(CC) $(CPPFLAGS) -o $@ -c convfv.cpp

clean:
	rm -fr $(BIN) *.o

wget:
	wget http://133.242.205.114/tmp/fv.tar.gz
	tar xzvf fv.tar.gz
