
// for everyone

#ifndef NO_M3CUT
 // for memset
#include <string.h>

#define xx(sq) (9-((sq)%9))
#define yy(sq) (1+((sq)/9))
#define loc(i,j) (9*((j)-1)+9-(i))
typedef struct { int word, shift, and, or; } m3ptnC;

#ifndef MATE3_C

extern int m3call, m3mvs, m3easy, m3hit, m3mate;
extern bitboard_t bbNeibAtkCandid[81][32];
extern m3ptnC mate3pattern[81][4];  // 4 is dmy; need 3
extern char m3ptnAry[512][512];

#else

int m3call = 0, m3mvs = 0, m3easy = 0, m3hit = 0, m3mate = 0;
bitboard_t bbNeibAtkCandid[81][32];
m3ptnC mate3pattern[81][4];  // 4 is dmy; need 3
char m3ptnAry[512][512];

#endif

// for ini.c

#ifndef MATE3_C

// 8 7 6
// 5 4 3
// 2 1 0

static const int rotate90Ary[9] = { 2,5,8,1,4,7,0,3,6 };
static const int mirrorAry[9]   = { 2,1,0,5,4,3,8,7,6 };

static int rotate90(int x) {
 int i, z = 0;
 for(i=0; i<=8; i++)
   if (x & (1 << i))
     z |= 1 << rotate90Ary[i];
 return z;
}

static int mirror(int x) {
 int i, z = 0;
 for(i=0; i<=8; i++)
   if (x & (1 << i))
     z |= 1 << mirrorAry[i];
 return z;
}

#define atk(i) (at & (1<<(i)))
#define occ(i) (oc & (1<<(i)))

static int m3safeBase(int at, int oc) {
  //    r          * : no atk            AND
  // -  -  *       - : no atk, no occupy   AND
  // *v O  v       v : either of these is no atk, no occupy  OR
  // v  .  v       + : no atk, occupy
  //                   => safe

 if (!atk(8) && !occ(8) && !atk(7) && !occ(7) && !atk(6) && !atk(5) &&
     (            !occ(5)  || (!atk(3) && !occ(3)) ||
      (!atk(2) && !occ(2)) || (!atk(0) && !occ(0)) ))
   return 1;

  //    r          * : no atk            AND
  // -  +* -       - : no atk, no occupy   AND
  // v  O  v       v : either of these is no atk, no occupy  OR
  // v  .  v       + : no atk, occupy
  //                   => safe

 if (!atk(8) && !occ(8) && !atk(6) && !occ(6) && !atk(7) &&
     ((!atk(5) && !occ(5)) || (!atk(3) && !occ(3)) ||
      (!atk(2) && !occ(2)) || (!atk(0) && !occ(0)) ||
      occ(7)))
   return 1;

  //                 * : no atk             AND
  //   -  *v *v      - : no atk, no occupy   AND
  // r *+ O  -       v : either of these is no atk, no occupy  OR
  //   .  v  *v      + : no atk, occupy
  //                     => safe

 if (!atk(8) && !occ(8) && !atk(3) && !occ(3) && 
     !atk(7) && !atk(6) && !atk(5) && !atk(0) &&
     ( !occ(7) || !occ(6) ||
      (!atk(1) && !occ(1)) || !occ(0) ||
      occ(5)))
   return 1;

  // a               * : no atk             AND
  //   -  v  *v      - : no atk, no occupy   AND
  //   v  O  v       v : either of these is no atk, no occupy  OR
  //   *v v  -       + : no atk, occupy
  //                     => safe
     // FIXME !atk6 or occ7 or occ3, etc

 if (!atk(8) && !occ(8) && !atk(0) && !occ(0) && 
     !atk(6) && !atk(2) &&
     ((!atk(7) && !occ(7)) || (!atk(5) && !occ(5)) ||
      (!atk(3) && !occ(3)) || (!atk(1) && !occ(1)) ||
      !occ(6) || !occ(2)))
   return 1;

  //                 * : no atk             AND
  //   .  *v .       - : no atk, no occupy   AND
  // r -  O  - r     v : either of these is no atk, no occupy  OR
  //   .  *v .       + : no atk, occupy
  //                     => safe

 if (!atk(5) && !occ(5) && !atk(3) && !occ(3) && 
     !atk(7) && !atk(1) &&
     (!occ(7) || !occ(1)))
   return 1;

  //                 * : no atk             AND
  //   *  -  .       - : no atk, no occupy   AND
  //   -  O  *       v : either of these is no atk, no occupy  OR
  //   .  *  .       + : no atk, occupy
  //                     => safe

 if (!atk(7) && !occ(7) && !atk(5) && !occ(5) && 
     !atk(8) && !atk(3) && !atk(1))
   return 1;

  //      r          * : no atk             AND
  //   -  *  *       - : no atk, no occupy   AND
  // r *  O  .       v : either of these is no atk, no occupy 
  //   *  .  *       
  //                     => never safe

#if 0
 if (!atk(8) && !occ(8) &&
     !atk(7) && !atk(6) && !atk(5) && !atk(2) && !atk(0) &&
     (!occ(7) || !occ(6) || !occ(5) ||
      (!atk(3) && !occ(3)) || !occ(2) ||
      (!atk(1) && !occ(1))))
   return 1;
#endif

  //                 * : no atk             AND
  //   *  *v .       - : no atk, no occupy   AND
  // r -  O  *  r    v : either of these is no atk, no occupy 
  //   *  *v .       
  //                     => safe

 if (!atk(5) && !occ(5) &&
     !atk(8) && !atk(7) && !atk(3) && !atk(2) && !atk(1) &&
     (!occ(7) || !occ(1)))
   return 1;

 return 0;
}

static int m3safe(int at, int oc) {
  // flip/rotate 8 patterns.  if either is safe, it's safe
 int i, j, k, t, c;
 for(i=0; i<=1; i++)
 for(j=0; j<=3; j++) {
   t = at;
   c = oc;
   if (i) {
     t = mirror(t);
     c = mirror(c);
   }
   for(k=1; k<=j; k++) {
     t = rotate90(t);
     c = rotate90(c);
   }
   if (m3safeBase(t, c))
     return 1;
 }
 return 0;
}

static void ini_mate3() {
  int sq, i, j;
  memset(m3ptnAry, 0, sizeof(m3ptnAry));
  memset(bbNeibAtkCandid, 0, sizeof(bbNeibAtkCandid));
  memset(mate3pattern, 0, sizeof(mate3pattern));
  for(sq=0; sq<81; sq++) {
    int x, y;
    bitboard_t bb;
    x = xx(sq);
    y = yy(sq);

     // F is not needed - [BW]ATK_PAWN suffices

     // Y
    BBIni(bb);
    for(i=x-2; i<=x+2; i++)
    for(j=y-1; j<=9; j++)
      if (i<1 || 9<i || j<1 || 9<j) continue;
      else Xor(loc(i,j),bb);

    bbNeibAtkCandid[sq][15+lance] = bb;

    BBIni(bb);
    for(i=x-2; i<=x+2; i++)
    for(j=y+1; j>=1; j--)
      if (i<1 || 9<i || j<1 || 9<j) continue;
      else Xor(loc(i,j),bb);

    bbNeibAtkCandid[sq][15-lance] = bb;

     // E
    BBIni(bb);
    for(i=x-3; i<=x+3; i++)
    for(j=y  ; j<=y+4; j++)
      if (i<1 || 9<i || j<1 || 9<j) continue; // NOTE need not care deadend
      else Xor(loc(i,j),bb);

    bbNeibAtkCandid[sq][15+knight] = bb;

    BBIni(bb);
    for(i=x-3; i<=x+3; i++)
    for(j=y  ; j>=y-4; j--)
      if (i<1 || 9<i || j<1 || 9<j) continue; // NOTE need not care deadend
      else Xor(loc(i,j),bb);

    bbNeibAtkCandid[sq][15-knight] = bb;

     // G, TGOLD, HDK
    BBIni(bb);
    for(i=x-3; i<=x+3; i++)
    for(j=y-3; j<=y+3; j++)
      if (i<1 || 9<i || j<1 || 9<j) continue; // NOTE need not care deadend
      else Xor(loc(i,j),bb);

    bbNeibAtkCandid[sq][15-silver] =
    bbNeibAtkCandid[sq][15-gold] =
    bbNeibAtkCandid[sq][15-king] =
    bbNeibAtkCandid[sq][15+silver] =
    bbNeibAtkCandid[sq][15+gold] =
    bbNeibAtkCandid[sq][15+king] = bb;

     // A
    BBIni(bb);
    for(i=1; i<=9; i++)
    for(j=1; j<=9; j++)
      if (!(x+y-4<=i+j && i+j<=x+y+4) && !(x-y-4<=i-j && i-j<=x-y+4)) continue;
      else Xor(loc(i,j),bb);

    bbNeibAtkCandid[sq][15-bishop] =
    bbNeibAtkCandid[sq][15+bishop] = bb;

     // H
    BBIni(bb);
    for(i=1; i<=9; i++)
    for(j=1; j<=9; j++)
      if (!(x-2<=i && i<=x+2) && !(y-2<=j && j<=y+2)) continue;
      else Xor(loc(i,j),bb);

    bbNeibAtkCandid[sq][15-rook] =
    bbNeibAtkCandid[sq][15+rook] = bb;

     // mate3ptn
    m3ptnC m[3];
    m[0].word = (sq-9)/27;
    m[1].word = (sq  )/27;
    m[2].word = (sq+9)/27;
    m[0].shift = 26-(sq-9)%27;    // NOTE data is first applied <<7 before >>x
    m[1].shift = 26-(sq  )%27+3;
    m[2].shift = 26-(sq+9)%27+6;
    m[0].or  = (y==1 ? 7 : x==1 ? 1 : x==9 ? 4 : 0) << 6;
    m[1].or  = (           x==1 ? 1 : x==9 ? 4 : 0) << 3;
    m[2].or  = (y==9 ? 7 : x==1 ? 1 : x==9 ? 4 : 0)     ;
    m[0].and = ~m[0].or & 0x1c0;
    m[1].and = ~m[1].or & 0x038;
    m[2].and = ~m[2].or & 0x007;

    for(j=0; j<=2; j++)
      mate3pattern[sq][j] = m[j];

  } // for sq

  // set m3ptnAry
  for(i=0; i<=511; i++)
  for(j=0; j<=511; j++)
    m3ptnAry[i][j] = m3safe(i,j);

}
#endif    // if not MATE3_C

// for mate3.c

#ifdef MATE3_C

static int m3ptn(int sq, bitboard_t bbAtk, bitboard_t bbOcc) {
 int i, at = 0, oc = 0;
 for(i=0; i<=2; i++) {
   at |= ((((int64_t)bbAtk.p[mate3pattern[sq][i].word]) << 7)
           >> mate3pattern[sq][i].shift) & mate3pattern[sq][i].and;
   oc |= ((((int64_t)bbOcc.p[mate3pattern[sq][i].word]) << 7)
           >> mate3pattern[sq][i].shift) & mate3pattern[sq][i].and
                                         | mate3pattern[sq][i].or ;
 }
 return (int)m3ptnAry[at][oc];
}




static bitboard_t findAllNeibAttacks(const tree_t* restrict ptree, int kloc, int turn) {
  bitboard_t allAtks, bb, bbAtk;
  int from;
    // calculate all attacks on 25-neib

  if (!turn) { /* black */
     // pawn
    allAtks = BB_BPAWN_ATK;

     // lance
    BBAnd(bb, BB_BLANCE, bbNeibAtkCandid[kloc][15+lance]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        BBAndOr(allAtks, AttackFile(from), abb_minus_rays[from]);
    }

     // knight
    BBAnd(bb, BB_BKNIGHT, bbNeibAtkCandid[kloc][15+knight]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        BBOr(allAtks, allAtks, abb_b_knight_attacks[from]);
    }

     // silver
    BBAnd(bb, BB_BSILVER, bbNeibAtkCandid[kloc][15+silver]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        BBOr(allAtks, allAtks, abb_b_silver_attacks[from]);
    }

     // tgold
    BBAnd(bb, BB_BTGOLD, bbNeibAtkCandid[kloc][15+gold]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        BBOr(allAtks, allAtks, abb_b_gold_attacks[from]);
    }

     // bh
    BBAnd(bb, BB_B_BH, bbNeibAtkCandid[kloc][15+bishop]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        AttackBishop(bbAtk,from);
        BBOr(allAtks, allAtks, bbAtk);
    }

     // rd
    BBAnd(bb, BB_B_RD, bbNeibAtkCandid[kloc][15+rook]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        AttackRook(bbAtk,from);
        BBOr(allAtks, allAtks, bbAtk);
    }

     // hdk
    BBAnd(bb, BB_B_HDK, bbNeibAtkCandid[kloc][15+king]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        BBOr(allAtks, allAtks, abb_king_attacks[from]);
    }

  } else {   /* white */
     // pawn
    allAtks = BB_WPAWN_ATK;

     // lance
    BBAnd(bb, BB_WLANCE, bbNeibAtkCandid[kloc][15-lance]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        BBAndOr(allAtks, AttackFile(from), abb_plus_rays[from]);
    }

     // knight
    BBAnd(bb, BB_WKNIGHT, bbNeibAtkCandid[kloc][15-knight]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        BBOr(allAtks, allAtks, abb_w_knight_attacks[from]);
    }

     // silver
    BBAnd(bb, BB_WSILVER, bbNeibAtkCandid[kloc][15-silver]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        BBOr(allAtks, allAtks, abb_w_silver_attacks[from]);
    }

     // tgold
    BBAnd(bb, BB_WTGOLD, bbNeibAtkCandid[kloc][15-gold]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        BBOr(allAtks, allAtks, abb_w_gold_attacks[from]);
    }

     // bh
    BBAnd(bb, BB_W_BH, bbNeibAtkCandid[kloc][15-bishop]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        AttackBishop(bbAtk,from);
        BBOr(allAtks, allAtks, bbAtk);
    }

     // rd
    BBAnd(bb, BB_W_RD, bbNeibAtkCandid[kloc][15-rook]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        AttackRook(bbAtk,from);
        BBOr(allAtks, allAtks, bbAtk);
    }

     // hdk
    BBAnd(bb, BB_W_HDK, bbNeibAtkCandid[kloc][15-king]);
    while(BBTest(bb)) {
        from = FirstOne( bb );
        Xor( from, bb );
        BBOr(allAtks, allAtks, abb_king_attacks[from]);
    }

  }

  return allAtks;
}
#endif    // if MATE3_C


#endif   // if not NO_M3CUT

