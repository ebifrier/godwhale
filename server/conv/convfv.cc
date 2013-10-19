
#include <stdio.h>
 // for close()?
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define CREATE_FV2

#define FVBIN_MMAP
#include "shogi.h"

// for mmap, open
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define max(a,b) ((a)>(b) ? (a) : (b))
#define min(a,b) ((a)<(b) ? (a) : (b))
#define forr(i,m,n) for(int i=(m); i<=(n); i++)

#define PcPcOnSqAny( k, i, j )  PcPcOnSq(k, max(i, j), min(i, j))

pconsqAry* pc_on_sq;
kkpAry* kkp;

//typedef short pconsq2Ary[81][fe_end][fe_end];
typedef short pconsq2Ary[fe_end][fe_end];

int main() {

  // open fv.bin (copied from ini.c)
  int fd;
  size_t sz1, sz2;
  void* mapbase;
  fd = open("fv.bin", O_RDONLY);
  sz1 = nsquare * pos_n;
  sz2 = nsquare * nsquare * kkp_end;
   //          hint_adr                                   offset
  mapbase = mmap(NULL, (sz1+sz2)*sizeof(short), PROT_READ, MAP_SHARED, fd, 0);
  if (mapbase == MAP_FAILED) return -2;
  pc_on_sq = (pconsqAry*) mapbase;
  kkp = (kkpAry*)((char*)mapbase + sz1 * sizeof(short));
  close(fd);
 printf("fv.bin opened\n");
 fflush(0);

  // create and open fv2.bin (copied from above, then modified)
  int fd2;
  size_t sz3, sz4;
  void* mapbase2;
  pconsq2Ary* pc_on_sq2;
  kkpAry* kkp2;
  sz3 = nsquare * fe_end * fe_end;
#ifdef CREATE_FV2
  FILE* fp2 = fopen("fv3.bin", "w");
  char cz = 0;
  forr(i, 0, sz3+sz2-1)
    fwrite(&cz, sizeof(short), 1, fp2);
  fclose(fp2);
 printf("fv3.bin created\n");
 fflush(0);
#endif
  fd2 = open("fv3.bin", O_RDWR);
  mapbase2 = mmap(NULL, (sz3+sz2)*sizeof(short), PROT_WRITE,MAP_SHARED, fd2, 0);
  if (mapbase2 == MAP_FAILED) return -2;
  pc_on_sq2 = (pconsq2Ary*)mapbase2;
  kkp2  = (kkpAry*)((char*)mapbase2+sz3*sizeof(short));
  close(fd2);
 printf("fv3.bin opened\n");
 fflush(0);

   // handle each move
 forr(ksq, 0, 80) {
   forr(i, 0, fe_end-1) {
     pc_on_sq2[ksq][i][i] = PcPcOnSq(ksq, i, i);
     forr(j, 0, i-1) {
       pc_on_sq2[ksq][i][j] =
       pc_on_sq2[ksq][j][i] = PcPcOnSq(ksq, i, j);
     }  // for j
   }  // for i

   forr(ksq2, 0, 80)
     forr(i, 0, kkp_end-1) 
       kkp2[ksq][ksq2][i] = kkp[ksq][ksq2][i] ;

 }  // for ksq

 munmap(mapbase2, (sz3+sz2)*sizeof(short));

}

