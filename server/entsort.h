/* $Id: entsort.h,v 1.3 2012-04-06 08:49:06 eikii Exp $ */

//#define DBG_ENTSORT

#ifdef DBG_ENTSORT

#include <stdio.h>
#include <string.h>
 // for exit()
#include <stdlib.h>

#define forr(i,m,n) for(int i=(m); i<=(n); i++)
#define forv(i,m,n) for(int i=(m); i>=(n); i--)

struct mvEntryC
{
    char mv[10];
    int numnode;
    int& key() { return numnode; }
};

struct procmvsC
{
    int mvcnt;
    mvEntryC mvs[8];
    void sort();
    void dump()
    {
        printf("procmvs dump: cnt %d\n", mvcnt);
        forr(i, 0, mvcnt-1)
            printf("[%d] num %d mv %s\n", i, mvs[i].numnode, mvs[i].mv);
    }
};

#endif

struct mvPackC
{
    mvEntryC ent;
    int suf;
};

#define MAXTOP 3

static bool packMatch(int x, mvPackC* bests)
{
    forr (n, 0, MAXTOP-1) {
        if (bests[n].suf == x) {
            return true;
        }
    }

    return false;
}

void procmvsC::sort()
{
    mvPackC bests[MAXTOP];
    forr (k, 0, MAXTOP-1) {
        bests[k].ent.numnode = -1;
    }

    forr (k, 0, mvcnt-1) {
        int i, j;

        // find the location in BESTS to insert MVS[k]
        for (j=MAXTOP-1; j>=0; j--) {
            if (mvs[k].numnode <= bests[j].ent.numnode) break;
        }
        
        if (j==MAXTOP-1) continue;
        j++;   // j decremented one too much.  adjust
        
        // right shift the lowers in BESTS
        for (i=MAXTOP-2; i>=j; i--) {
            bests[i+1] = bests[i];
        }
        
        // insert
        bests[j].ent = mvs[k];
        bests[j].suf = k;
    }

#ifdef DBG_ENTSORT
    // dump BESTS
    forr (k, 0, MAXTOP-1) {
        printf("bst[%d] suf %d ent num %d mv %s\n", k,
               bests[k].suf,
               bests[k].ent.numnode,
               bests[k].ent.mv);
    }
#endif

    // right shift the lowers in MVS
    int off = 0;
    forv (j, mvcnt-1, 0) {
        if (!packMatch(j, bests)) {
            mvs[j+off] = mvs[j];
        } else {
            off++;
        }
    }

    forr (k, 0, MAXTOP-1) {
        if (bests[k].suf == -1) break;
        mvs[k] = bests[k].ent;
    }
}

#ifdef DBG_ENTSORT
 //**** main ****

int main()
{
    procmvsC prm;
    int cnt = 3;
    int nums[7][8] = {
        { 3, 0, 0, 8, 6, 1, 0, 11 },
        { 9, 7, 7, 6, 6, 1, 0, 11 },
        { 1, 7, 2, 6, 6, 1, 0, 11 },
        { 1, 7, 2, 6, 6, 1, 0, 11 },
        { 1, 1, 2, 6, 6, 8, 9, 11 },
        { 1, 7, 2, 6, 6, 1, 0, 11 },
        { 1, 7, 2, 6, 6, 1, 0, 11 },
    };
    const char* names[8] = {
        "abe", "betty", "charlie", "david", "erny", "frank",
        "george", "henry"
    };
    int mvc[7] = { 7, 1, 2, 0, 8, 3, 5 };

    forr (k, 0, 6) {
        printf("-------- %d --------\n", k);
        forr (i, 0, 7) {
            prm.mvs[i].numnode = nums[k][i];
            strcpy(prm.mvs[i].mv, names[i]);
        }
        prm.mvcnt = mvc[k];

        prm.dump();
        prm.sort();
        prm.dump();
    }

    printf("finished\n");
    exit(0);
}
#endif
