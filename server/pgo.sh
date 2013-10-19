# change 0->1 to use icc
USE_ICC=0

######## No need to change below (probably) ########

PROBCMD=/tmp/probcmd
BNDIR=bns6

MPIEXEC=mpiexec
NPROC=n

if [[ $USE_ICC  =  1 ]] ; then
PH1=ipgo1
PH2=ipgo2
else
PH1=gpgo1
PH2=gpgo2
fi

 # setup
rm -rf pgowork
mkdir pgowork
cd pgowork

rm -f $BNDIR
ln -s ../$BNDIR
#ln -s ../$BNDIR/Makefile
ln -s ../fv3.bin
ln -s ../book.bin
ln -s ../hand.jos
mkdir log
mkdir profdir
export PGOPHASE=1

 # link bnz files
for xxx in ../$BNDIR/*.[ch] ; do
 ln -s $xxx
done

 # link bnk files
for xxx in ../*.{c,cpp,h} ; do
 ln -s $xxx
done

 # compile phase 1
cp    ../$BNDIR/Makefile .
make -f Makefile CLUSTOPT=-DCLUSTER_PARALLEL $PH1
rm -f Makefile
echo "################ BN1 done"

export PGOPHASE=1
cp      ../makefile .
make -f makefile bonkobjs
make -f makefile pgotgt
echo "################ BONK1 done, now @ $PWD"
rm   -f makefile

 # run prob
rm -f $PROBCMD
echo "limit depth  9" > $PROBCMD
echo "problem "      >> $PROBCMD
cp ../d1.dpp problem.csa
$MPIEXEC -$NPROC 2 gmx < $PROBCMD

 # compile phase 2
rm *.o
cp      ../$BNDIR/Makefile .
make -f Makefile CLUSTOPT=-DCLUSTER_PARALLEL $PH2
echo "################ BN2 done"
rm -f Makefile

export PGOPHASE=2
#make -f ../makefile
cp       ../makefile .
make -f makefile bonkobjs
make -f makefile pgotgt

cd ..
rm -rf gmx
ln -s pgowork/gmx

