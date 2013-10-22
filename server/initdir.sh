#!/bin/sh

CURRENT=`pwd`
BIN=$CURRENT/bin
BNS=$CURRENT/bonanza6

# logディレクトリの作成
if [ ! -d $BIN/log ]; then
    mkdir $BIN/log
fi

ln -sf $BIN/fv3.bin $BNS/fv3.bin
ln -sf $BIN/book.bin $BNS/book.bin
ln -sf $BIN/hand.jos $BNS/hand.jos
