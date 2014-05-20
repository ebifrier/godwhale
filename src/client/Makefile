#
# Makefile for client
#

include ../Makefile-common

OPT_CPPFLAGS = -DMNJ_LAN -DGODWHALE_CLIENT

BIN  = one_godwhale
SRCS = main.c
OBJS = $(SRCS:.c=.o)

DIST_DIR = dist_client
DIST_COMMON_FILES = $(BNZ_DIR)/..
DIST_CLIENT_FILES = Makefile run_client.sh param.txt main.c

.DEFAULT_GOAL = all

all: $(BIN)

$(BIN): $(OBJS) $(BNZ_OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(BNZ_OBJS) -o $@ $(LIBS)

main.o: main.c
	$(CC) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

depend: $(SRCS) $(BNZ_SRCS)
	$(CC) $(INCLUDES) $(CPPFLAGS) $(FLAGS) -MM -MG $(SRCS) $(BNZ_SRCS) > make.dep

dist:
	rm -rf $(DIST_DIR)
	mkdir $(DIST_DIR)
	mkdir $(DIST_DIR)/common
	mkdir $(DIST_DIR)/client
	cp -r -t $(DIST_DIR)/common $(DIST_COMMON_FILES)
	cp -r -t $(DIST_DIR)/client $(DIST_CLIENT_FILES)

clean: common-clean
	rm -f *.o *.exe $(BIN) make.dep
	rm -rf $(DIST_DIR)

-include make.dep
