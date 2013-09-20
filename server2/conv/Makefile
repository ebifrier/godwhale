######## No need to change below (probably) ########

CC = g++
BN = ../bonanza6

SRCS = convfv.cc
OBJS = convfv.o
BIN  = convfv

########

.o: $(BN)/shogi.h
	$(CC) -c $< -o $@

all: $(BIN)

$(BIN):
	$(CC) $(OBJS) -o $@

clean:
	rm -fr $(BIN) *.o *.exe
