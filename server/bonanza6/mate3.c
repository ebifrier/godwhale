// $Id: mate3.c,v 1.7 2012-04-11 06:38:30 eikii Exp $

#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include "shogi.h"

#define USE_M3CUT
//#define DBG_M3CUT

// gen_next_evasion_mate(coroutine)で使うphaseを表わすenum。
// 詰み回避の検査のためのphase。
typedef enum
{
  mate_king_cap_checker = 0, // 王手している駒を王で取る手を生成
  mate_cap_checker_gen,      // 王手している駒を移動して取る手を生成
  mate_cap_checker,
  mate_king_cap_gen,
  mate_king_cap,
  mate_king_move_gen,
  mate_king_move,
  mate_intercept_move_gen,
  mate_intercept_move,
  mate_intercept_weak_move,
  mate_intercept_drop_sup
} mate_flag_t;


static int CONV mate3_and( tree_t * restrict ptree, int turn, int ply,
                           int flag );
static int CONV mate_weak_or( tree_t * restrict ptree, int turn, int ply,
                              int from, int to );

static void CONV checker( const tree_t * restrict ptree, int *psq, int turn,
                          Move lastmv );
static Move CONV gen_king_cap_checker( const tree_t * restrict ptree,
                                       int to, int turn );
static Move * CONV gen_move_to( const tree_t * restrict ptree, int to,
                                int turn, Move * restrict pmove );
static Move * CONV gen_king_move( const tree_t * restrict ptree,
                                  const int *psq, int turn,
                                  int is_capture, Move * restrict pmove );
static Move * CONV gen_intercept( tree_t * restrict __ptree__,
                                  int sq_checker, int ply, int turn,
                                  int * restrict premaining,
                                  Move * restrict pmove, int flag );
static int CONV gen_next_evasion_mate( tree_t * restrict ptree,
                                       const int *psq, int ply, int turn,
                                       int flag );

typedef struct { int word, shift, and, or; } pattern_t;

int m3call, m3mvs, m3easy, m3hit, m3mate;
static bitboard_t bbNeibAtkCandid[81][32];
static pattern_t apattern[81][4];  // 4 is dmy; need 3
static char is_safe_array[512][512];

// 8 7 6
// 5 4 3
// 2 1 0
static const int arotate90[9] = { 2, 5, 8, 1, 4, 7, 0, 3, 6 };
static const int amirror[9]   = { 2, 1, 0, 5, 4, 3, 8, 7, 6 };

static uint64_t mate3_hash_tbl[ MATE3_MASK + 1 ] = {0};


#if defined(DBG_M3CUT)
// bnz mv format - cap:4 pc:4 prom:1 src:7 dst:7  -> convert to 4,4,4,8,8
static int CONV
readable_c( int mv )
{
  if (mv == 0) return 0;

  int x = mv;
  int dst  = x & 0x7f;
  int src  = (x >>  7) & 0x7f;
  int prom = (x <<  2) & 0x10000;
  int cappc = (x <<  5) & 0x0ff00000;
  int dx = 9 - (dst % 9);
  int dy = 1 + (dst / 9);
  int sx = (src >= nsquare ?   0     : 9 - (src % 9));
  int sy = (src >= nsquare ? src % 9 : 1 + (src / 9));

  return (cappc | prom | (sx << 12) | (sy << 8) | (dx << 4) | dy);
}
#endif


// 
static int CONV
rotate90( int x )
{
  int i, z = 0;

  for ( i = 0; i <= 8; i++ )
    {
      if ( x & (1 << i) ) z |= 1 << arotate90[i];
    }

  return z;
}


static int CONV
mirror( int x )
{
  int i, z = 0;

  for ( i = 0; i <= 8; i++ )
    {
      if ( x & (1 << i) ) z |= 1 << amirror[i];
    }

  return z;
}


static int CONV
calc_is_safe_impl( int at, int oc )
{
#define atk(i) (at & (1<<(i)))
#define occ(i) (oc & (1<<(i)))
    //    r          * : no atk            AND
    // -  -  *       - : no atk, no occupy   AND
    // *v O  v       v : either of these is no atk, no occupy  OR
    // v  .  v       + : no atk, occupy
    //                   => safe

    if (!atk(8) && !occ(8) && !atk(7) && !occ(7) && !atk(6) && !atk(5) &&
        (          !occ(5) || (!atk(3) && !occ(3)) ||
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
#undef atk
#undef occ
}


// mirror/rotateの2*4パターンにつき、safeかどうか調べます。
// 一つでもsafeであればそれはsafeです。
//
// at, ocは玉周辺の升に利き／駒があるかどうかを示す9bitのbitmask
// 右下から左上方向にのび、0ビット目が１であれば
// それは右下の升に駒があることを示す。
// それらの関係から玉が絶対に積まないパターンを調べます。
static int CONV
calc_is_safe( int at, int oc )
{
  int i, j, k, t, c;

  for ( i = 0; i <= 1; i++ )
    {
      for ( j = 0; j <= 3; j++ )
        {
          t = at;
          c = oc;

          if ( i )
            {
              t = mirror(t);
              c = mirror(c);
            }
          for ( k = 1; k <= j; k++ )
            {
              t = rotate90(t);
              c = rotate90(c);
            }

          if ( calc_is_safe_impl( t, c ) )
            {
              return 1;
            }
        }
    }

  return 0;
}


// bitboard_t を値渡しするとVC10ではalignment関係のエラーが発生するため、
// ここではポインタ渡しにしている。
static int CONV
is_safe( int sq, const bitboard_t * restrict bb_atk,
         const bitboard_t * restrict bb_occ )
{
  int i, at = 0, oc = 0;

  // at, ocは玉周辺の升に利き／駒があるかどうかを示す9bitのbitmask
  // 右下から左上方向にのび、0ビット目が１であれば
  // それは右下の升に駒があることを示す。
  for ( i = 0; i <= 2; i++ )
    {
      at |= (((((int64_t)bb_atk->p[apattern[sq][i].word]) << 7)
              >> apattern[sq][i].shift) & apattern[sq][i].and);
      oc |= (((((int64_t)bb_occ->p[apattern[sq][i].word]) << 7)
              >> apattern[sq][i].shift) & apattern[sq][i].and)
            | apattern[sq][i].or;
    }

  return (int)is_safe_array[at][oc];
}


// 近くにいる駒の利きを調べます。
static bitboard_t CONV
findAllNeibAttacks( const tree_t* restrict ptree, int kloc, int turn )
{
    bitboard_t bb_atk, bb, temp;
    int from;

    if ( turn == black ) { /* black */
        // pawn
        bb_atk = BB_BPAWN_ATK;

        // lance
        BBAnd(bb, BB_BLANCE, bbNeibAtkCandid[kloc][15+lance]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            BBAndOr(bb_atk, AttackFile(from), abb_minus_rays[from]);
        }

        // knight
        BBAnd(bb, BB_BKNIGHT, bbNeibAtkCandid[kloc][15+knight]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            BBOr(bb_atk, bb_atk, abb_b_knight_attacks[from]);
        }

        // silver
        BBAnd(bb, BB_BSILVER, bbNeibAtkCandid[kloc][15+silver]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            BBOr(bb_atk, bb_atk, abb_b_silver_attacks[from]);
        }

        // tgold
        BBAnd(bb, BB_BTGOLD, bbNeibAtkCandid[kloc][15+gold]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            BBOr(bb_atk, bb_atk, abb_b_gold_attacks[from]);
        }

        // bh
        BBAnd(bb, BB_B_BH, bbNeibAtkCandid[kloc][15+bishop]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            AttackBishop( temp, from );
            BBOr(bb_atk, bb_atk, temp);
        }

        // rd
        BBAnd(bb, BB_B_RD, bbNeibAtkCandid[kloc][15+rook]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            AttackRook( temp, from );
            BBOr(bb_atk, bb_atk, temp);
        }

        // hdk
        BBAnd(bb, BB_B_HDK, bbNeibAtkCandid[kloc][15+king]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            BBOr(bb_atk, bb_atk, abb_king_attacks[from]);
        }
      }
    else {   /* white */
        // pawn
        bb_atk = BB_WPAWN_ATK;

        // lance
        BBAnd(bb, BB_WLANCE, bbNeibAtkCandid[kloc][15-lance]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            BBAndOr(bb_atk, AttackFile(from), abb_plus_rays[from]);
        }

        // knight
        BBAnd(bb, BB_WKNIGHT, bbNeibAtkCandid[kloc][15-knight]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            BBOr(bb_atk, bb_atk, abb_w_knight_attacks[from]);
        }

        // silver
        BBAnd(bb, BB_WSILVER, bbNeibAtkCandid[kloc][15-silver]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            BBOr(bb_atk, bb_atk, abb_w_silver_attacks[from]);
        }

        // tgold
        BBAnd(bb, BB_WTGOLD, bbNeibAtkCandid[kloc][15-gold]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            BBOr(bb_atk, bb_atk, abb_w_gold_attacks[from]);
        }

        // bh
        BBAnd(bb, BB_W_BH, bbNeibAtkCandid[kloc][15-bishop]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            AttackBishop( temp, from );
            BBOr(bb_atk, bb_atk, temp);
        }

        // rd
        BBAnd(bb, BB_W_RD, bbNeibAtkCandid[kloc][15-rook]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            AttackRook( temp, from );
            BBOr(bb_atk, bb_atk, temp);
        }

        // hdk
        BBAnd(bb, BB_W_HDK, bbNeibAtkCandid[kloc][15-king]);
        while (BBTest(bb)) {
            from = FirstOne( bb );
            Xor( from, bb );
            BBOr(bb_atk, bb_atk, abb_king_attacks[from]);
        }
    }

    return bb_atk;
}


void
ini_mate3( void )
{
#define xx(sq)   (9 - ((sq)%nfile))
#define yy(sq)   (1 + ((sq)/nrank))
#define loc(i,j) (9*((j)-1) + 9-(i))

  int sq, i, j;

  memset(is_safe_array, 0, sizeof(is_safe_array));
  memset(apattern, 0, sizeof(apattern));
  memset(bbNeibAtkCandid, 0, sizeof(bbNeibAtkCandid));

  for ( sq = 0; sq < 81; sq++ )
    {
      bitboard_t bb;
      int x = xx(sq);
      int y = yy(sq);

      // F is not needed - [BW]ATK_PAWN suffices

      // Y
      BBIni( bb );
      for ( i = x-2; i <= x+2; i++ )
          for ( j = y-1; j <= 9; j++ )
              if ( i<1 || 9<i || j<1 || 9<j ) continue;
              else Xor( loc(i,j), bb );

      bbNeibAtkCandid[sq][15+lance] = bb;

      BBIni(bb);
      for(i=x-2; i<=x+2; i++)
          for(j=y+1; j>=1; j--)
              if (i<1 || 9<i || j<1 || 9<j) continue;
              else Xor(loc(i,j),bb);

      bbNeibAtkCandid[sq][15-lance] = bb;

      // E
      BBIni( bb );
      for( i = x-3; i <= x+3; i++ )
          for ( j = y; j <= y+4; j++ )
              if ( i<1 || 9<i || j<1 || 9<j ) continue; // NOTE need not care deadend
              else Xor( loc(i,j), bb );

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
      BBIni( bb );
      for ( i = 1; i <= 9; i++ )
          for ( j = 1; j <= 9; j++ )
              if ( ! ( x+y-4 <= i+j && i+j <= x+y+4 ) &&
                   ! ( x-y-4 <= i-j && i-j <= x-y+4 ) ) continue;
              else Xor( loc(i,j), bb );

      bbNeibAtkCandid[sq][15-bishop] =
      bbNeibAtkCandid[sq][15+bishop] = bb;

      // H
      BBIni( bb );
      for ( i = 1; i <= 9; i++ )
          for ( j = 1; j <= 9; j++ )
              if ( ! ( x-2 <= i && i <= x+2 ) &&
                   ! ( y-2 <= j && j <= y+2 ) ) continue;
              else Xor( loc(i,j), bb );

      bbNeibAtkCandid[sq][15-rook] =
      bbNeibAtkCandid[sq][15+rook] = bb;
    }

  /*
      at |= (((((int64_t)bb_atk->p[apattern[sq][i].word]) << 7)
              >> apattern[sq][i].shift) & apattern[sq][i].and);
      oc |= (((((int64_t)bb_occ->p[apattern[sq][i].word]) << 7)
              >> apattern[sq][i].shift) & apattern[sq][i].and)
            | apattern[sq][i].or;
  */
  // patternの初期化
  for ( sq = 0; sq < 81; sq++ )
    {
      pattern_t m[3];
      int x = xx(sq);
      int y = yy(sq);

      // wordはsqに対応するpを取り出すためのインデックス
      // sq-9で一つ上の段、sq+9で一つ下の段に対応する
      m[0].word = (sq-9) / (3*nfile);
      m[1].word = (sq  ) / (3*nfile);
      m[2].word = (sq+9) / (3*nfile);

      // shiftはp[N]からsq周辺の3bitを取り出すために必要なシフト回数
      // p[N]には3段分の列情報が入っているので合計27bit。
      // また、結果として得るbitは 上段 : 中段 : 下段 (各3bit) 。
      //
      // 演算前に7bit分左シフトするため、実質的には (shift - 7)
      // だけ右シフトを行う。(マイナスの場合は左シフト)
      m[0].shift = 26-(sq-9)%(3*nfile);
      m[1].shift = 26-(sq  )%(3*nfile)+3;
      m[2].shift = 26-(sq+9)%(3*nfile)+6;

      // 盤の範囲外を1とするようなbitmask
      m[0].or  = (y==1 ? 7 : x==1 ? 1 : x==9 ? 4 : 0) << 6;
      m[1].or  = (           x==1 ? 1 : x==9 ? 4 : 0) << 3;
      m[2].or  = (y==9 ? 7 : x==1 ? 1 : x==9 ? 4 : 0)     ;

      // 各段で必要になる3bitのみを取り出す。
      m[0].and = ~m[0].or & 0x1c0; // 0001 1100 0000
      m[1].and = ~m[1].or & 0x038; // 0000 0011 1000
      m[2].and = ~m[2].or & 0x007; // 0000 0000 0111

      for ( j = 0; j <= 2; j++ )
        apattern[sq][j] = m[j];
  }

  for ( i = 0; i <= 511; i++ )
      for ( j = 0; j <= 511; j++ )
          is_safe_array[i][j] = calc_is_safe( i, j );

#undef xx
#undef yy
#undef loc
}


static int CONV
mhash_probe( tree_t * restrict ptree, int turn, int ply )
{
  uint64_t key_current, key, word;
  unsigned int move;

  word  = mate3_hash_tbl[ (unsigned int)HASH_KEY & MATE3_MASK ];
#if ! defined(__x86_64__)
  word ^= word << 32;
#endif

  key          = word     & ~(uint64_t)0x7ffffU;
  key_current  = HASH_KEY & ~(uint64_t)0x7ffffU;
  key_current ^= (uint64_t)HAND_B << 42;
  key_current ^= (uint64_t)turn << 63;

  if ( key != key_current ) { return 0; }

  move = (unsigned int)word & 0x7ffffU;
  if ( move != MOVE_NA )
    {
      move |= turn ? Cap2Move( BOARD[I2To(move)])
                   : Cap2Move(-BOARD[I2To(move)]);
    }

  MOVE_CURR = move;

  return 1;
}


static void CONV
mhash_store( const tree_t * restrict ptree, int turn, unsigned int move )
{
  uint64_t word;

  word  = HASH_KEY & ~(uint64_t)0x7ffffU;
  word |= (uint64_t)( move & 0x7ffffU );
  word ^= (uint64_t)HAND_B << 42;
  word ^= (uint64_t)turn << 63;
  
#if ! defined(__x86_64__)
  word ^= word << 32;
#endif
  mate3_hash_tbl[ (unsigned int)HASH_KEY & MATE3_MASK ] = word;
}


unsigned int CONV
is_mate_in3ply( tree_t * restrict ptree, int turn, int ply )
{
  int value, flag_skip;
  int kloc, to;
  bitboard_t bb_atk;

#if defined(USE_M3CUT) && defined(DBG_M3CUT)
  int hit = 0;
#endif

  if ( mhash_probe( ptree, turn, ply ) )
    {
      if ( MOVE_CURR == MOVE_NA ) { return 0; }
      else                        { return 1; }
    }

  if ( ply >= PLY_MAX-2 ) { return 0; }

  m3call++;
  flag_skip = 0;

  ptree->anext_move[ply].move_last = ptree->move_last[ply-1];
  ptree->move_last[ply] = GenCheck( turn, ptree->move_last[ply-1] );

  kloc = (turn ? SQ_BKING : SQ_WKING);
  bb_atk = findAllNeibAttacks(ptree, kloc, turn);

  while ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
    {
      MOVE_CURR = *ptree->anext_move[ply].move_last++;

      if ( MOVE_CURR & MOVE_CHK_CLEAR ) { flag_skip = 0; }
      if ( flag_skip ) { continue; }

      assert( is_move_valid( ptree, MOVE_CURR, turn ) );
      MakeMove( turn, MOVE_CURR, ply );
      if ( InCheck(turn) )
        {
          UnMakeMove( turn, MOVE_CURR, ply );
          continue;
        }

      m3mvs++;
#ifdef USE_M3CUT

#  ifdef DBG_M3CUT
      hit = 0;
#  endif
      to = I2To(MOVE_CURR);
      if ( BBContract(abb_king_attacks[kloc], abb_mask[to]) &&  // neib8
           I2From(MOVE_CURR) >= 81 &&                           // drop
           ! BBContract(bb_atk, abb_mask[to]) )   // no opn atk on dst
        {
          bitboard_t bb = (turn ? BB_BOCCUPY : BB_WOCCUPY);
          Xor(kloc, bb); // assume capture by king, kloc empty (new kloc no matter)
          m3easy++;
          if ( is_safe( to, &bb_atk, &bb ) )
            {
              m3hit++;
#  ifdef DBG_M3CUT
              hit=1;
#  else
              UnMakeMove( turn, MOVE_CURR, ply );
              continue;
#  endif
            }
        }
#endif

      value = mate3_and( ptree, Flip(turn), ply+1, 0 );
      
      UnMakeMove( turn, MOVE_CURR, ply );

      if ( value )
        {
#ifdef DBG_M3CUT
          if (hit)
            {
              Out(":::: M3WARNING: wrong hit  mv %07x\n", readable_c(MOVE_CURR));
              out_board(ptree, stdout, 0, 0);
              //abort();
            }
#endif
          m3mate++;
          mhash_store( ptree, turn, MOVE_CURR );
          return 1;
        }

      if ( ( MOVE_CURR & MOVE_CHK_SET )
           && I2To(MOVE_CURR) != I2To(ptree->current_move[ply+1]) )
        {
          flag_skip = 1;
        }
    }

  mhash_store( ptree, turn, MOVE_NA );
  return 0;
}


// and接点用とor接点用の詰み判定。
// and接点は詰まされるほうの手番。すべての回避手が詰みならそれは詰み。
static int CONV
mate3_and( tree_t * restrict ptree, int turn, int ply, int flag )
{
  Move move;
  int asq[2];

  assert( InCheck(turn) );

  ptree->anext_move[ply].next_phase = mate_king_cap_checker;
  checker( ptree, asq, turn, MOVE_LAST );

  while ( gen_next_evasion_mate( ptree, asq, ply, turn, flag ) ) {

    if ( ptree->anext_move[ply].next_phase == mate_intercept_drop_sup )
      {
        return 0;
      }

    MakeMove( turn, MOVE_CURR, ply );
    assert( ! InCheck(turn) );

    if ( InCheck( Flip(turn) ) ) { move = 0; }
    else                         { move = IsMateIn1Ply( Flip(turn) ); }

    if ( ! move
         && ptree->anext_move[ply].next_phase == mate_intercept_weak_move )
      {
        assert( asq[1] == nsquare );
        move = (unsigned int)mate_weak_or( ptree, Flip(turn), ply+1, asq[0],
                                           I2To(MOVE_CURR) );
      }

    UnMakeMove( turn, MOVE_CURR, ply );
      
    if ( ! move ) { return 0; }
  }
  
  return 1;
}


// and接点用とor接点用の詰み判定。
// or接点は詰ますほうの手番。ひとつでも詰みの手があればその接点は詰み。
// 詰み判定は、詰ますほうの手番で敵玉が詰むかどうかだから
// or接点だと考えることが出来る。
static int CONV
mate_weak_or( tree_t * restrict ptree, int turn, int ply, int from, int to )
{
  int pc, pc_cap, value, flag;
  direc_t idirec;

  if ( ply >= PLY_MAX-2 ) { return 0; }
  
  if ( turn == white )
    {
      if ( IsDiscoverWK( from, to ) ) { return 0; }

      pc     = -BOARD[from];
      pc_cap =  BOARD[to];
      MOVE_CURR = ( To2Move(to) | From2Move(from)
                    | Piece2Move(pc) | Cap2Move(pc_cap) );
      if ( ( pc == bishop || pc == rook )
           && ( to > I4 || from > I4 ) ) { MOVE_CURR |= FLAG_PROMO; }
    }
  else {
    if ( IsDiscoverBK( from, to ) ) { return 0; }
    
    pc     =  BOARD[from];
    pc_cap = -BOARD[to];
    MOVE_CURR = ( To2Move(to) | From2Move(from) | Piece2Move(pc)
                  | Cap2Move(pc_cap) );
    if ( ( pc == bishop || pc == rook )
         && ( to < A6 || from < A6 ) ) { MOVE_CURR |= FLAG_PROMO; }
  }

  MakeMove( turn, MOVE_CURR, ply );
  if ( I2From(MOVE_LAST) < nsquare )
    {
      if ( InCheck(turn) )
        {
          UnMakeMove( turn, MOVE_CURR, ply );
          return 0;
        }
      flag = 1;
    }
  else {
    assert( ! InCheck(turn) );
    flag = 2;
  }
  
  ptree->move_last[ply] = ptree->move_last[ply-1];
  value = mate3_and( ptree, Flip(turn), ply+1, flag );
  
  UnMakeMove( turn, MOVE_CURR, ply );

  return value;
}


// 王手の回避手を生成する。ここで生成される手によって王手は100%回避できている。
// 生成された指し手は、MOVE_CURRに入る。呼び出されるごとに新しい手が生成されて
// それ以上生成できなくなれば(回避する手がない)、
// この関数の返し値としてfalseが返る。
// すなわち、この関数はcoroutineになっている。
static int CONV
gen_next_evasion_mate( tree_t * restrict ptree, const int *psq, int ply,
                       int turn, int flag )
{
  switch ( ptree->anext_move[ply].next_phase )
    {
    // まず王手している駒を王で取る手を生成
    case mate_king_cap_checker:
      ptree->anext_move[ply].next_phase = mate_cap_checker_gen;
      MOVE_CURR = gen_king_cap_checker( ptree, psq[0], turn );
      // その手があったのならいったん返ってその手を評価する。
      if ( MOVE_CURR ) { return 1; }

    // 王手している駒を移動して捕獲する手の生成
    case mate_cap_checker_gen:
      ptree->anext_move[ply].next_phase = mate_cap_checker;
      ptree->anext_move[ply].move_last  = ptree->move_last[ply-1];
      ptree->move_last[ply]             = ptree->move_last[ply-1];
      if ( psq[1] == nsquare )
        {
          ptree->move_last[ply]
            = gen_move_to( ptree, psq[0], turn, ptree->move_last[ply-1] );
        }

    case mate_cap_checker:
      if ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
        {
          MOVE_CURR = *(ptree->anext_move[ply].move_last++);
          return 1;
        }

    // 王を移動して逃げる手を生成。駒の捕獲あり。
    // ただし、王手している駒を捕獲する手は除外。
    case mate_king_cap_gen:
      ptree->anext_move[ply].next_phase = mate_king_cap;
      ptree->anext_move[ply].move_last  = ptree->move_last[ply-1];
      ptree->move_last[ply]
        = gen_king_move( ptree, psq, turn, 1, ptree->move_last[ply-1] );

    case mate_king_cap:
      if ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
        {
          MOVE_CURR = *(ptree->anext_move[ply].move_last++);
          return 1;
        }

    // 王を移動して逃げる手の生成。駒の捕獲無し。
    case mate_king_move_gen:
      ptree->anext_move[ply].next_phase = mate_king_move;
      ptree->anext_move[ply].move_last  = ptree->move_last[ply-1];
      ptree->move_last[ply]
        = gen_king_move( ptree, psq, turn, 0, ptree->move_last[ply-1] );

    case mate_king_move:
      if ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
        {
          MOVE_CURR = *(ptree->anext_move[ply].move_last++);
          return 1;
        }

    // 合駒して受ける手の生成。利きのある場所に合駒(移動合い)するのが優先。
    case mate_intercept_move_gen:
      ptree->anext_move[ply].remaining  = 0;
      ptree->anext_move[ply].next_phase = mate_intercept_move;
      ptree->anext_move[ply].move_last  = ptree->move_last[ply-1];
      ptree->move_last[ply]             = ptree->move_last[ply-1];
      if ( psq[1] == nsquare && abs(BOARD[psq[0]]) != knight  )
        {
          int n;
          ptree->move_last[ply] = gen_intercept( ptree, psq[0], ply, turn, &n,
                                                 ptree->move_last[ply-1],
                                                 flag );
          if ( n < 0 )
            {
              ptree->anext_move[ply].next_phase = mate_intercept_drop_sup;
              ptree->anext_move[ply].remaining  = 0;
              MOVE_CURR = *(ptree->anext_move[ply].move_last++);
              return 1;
            }

          ptree->anext_move[ply].remaining = n;
        }

    case mate_intercept_move:
      if ( ptree->anext_move[ply].remaining-- )
        {
          MOVE_CURR = *(ptree->anext_move[ply].move_last++);
          return 1;
        }
      ptree->anext_move[ply].next_phase = mate_intercept_weak_move;

    // 弱移動合い (利きのない場所への移動中合い)
    case mate_intercept_weak_move:
      if ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
        {
          MOVE_CURR = *(ptree->anext_move[ply].move_last++);
          return 1;
        }
      break;

    default:
      assert( 0 );
    }

  return 0;
}


// turn側の自玉に王手をかけている駒を列挙(最大2つ)。
// 列挙した駒は psq[0] , psq[1]に。psq[1]は2つ目の駒がなければ nsquareが返る。
// 前提 : turn側に王手がかかっていないといけない。
// 両王手の場合、片方は直接王手である。直接王手しているほうをpsq[0]にする。
static void CONV
checker( const tree_t * restrict ptree, int *psq, int turn, Move lastmv )
{
  bitboard_t bb;
  int n, sq0, sq1, sq_king, from, to;
  direc_t adir;

#if 1    // 4/6/2012 this causes core dump...
  from = I2From(lastmv);
  to   = I2To(lastmv);
  adir = adirec[turn ? SQ_WKING : SQ_BKING][from];  // Note drop case covered
  if ( ! adir )
    {
      psq[0] = to;
      psq[1] = 81;
      return;
    }
#endif

  if ( turn == white )
    {
      sq_king = SQ_WKING;
      bb = b_attacks_to_piece( ptree, sq_king );
    }
  else {
    sq_king = SQ_BKING;
    bb = w_attacks_to_piece( ptree, sq_king );
  }

  assert( BBTest(bb) );
  sq0 = LastOne( bb );
  sq1 = nsquare;

  Xor( sq0, bb );
  if ( BBTest( bb ) )
    {
      sq1 = LastOne( bb );
      if ( BBContract( abb_king_attacks[sq_king], abb_mask[sq1] ) )
        {
          n   = sq0;
          sq0 = sq1;
          sq1 = n;
        }
    }

  psq[0] = sq0;
  psq[1] = sq1;
}


// toが王手をしている駒。この駒をとる王の移動を生成する。
// 指し手がなければ0を返す。toの駒が王の移動範囲になかったり、
// toの駒に敵の利きがあったりすれば0が返る。
// さもなくば、指し手を生成してそれが返る。
static Move CONV
gen_king_cap_checker( const tree_t * restrict ptree, int to, int turn )
{
  Move move;
  int from;

  if ( turn == white )
    {
      from = SQ_WKING;
      if ( ! BBContract( abb_king_attacks[from],
                         abb_mask[to] ) )   { return 0; }
      if ( is_white_attacked( ptree, to ) ) { return 0; }
      move = Cap2Move(BOARD[to]);
    }
  else {
    from = SQ_BKING;
    if ( ! BBContract( abb_king_attacks[from],
                       abb_mask[to] ) )   { return 0; }
    if ( is_black_attacked( ptree, to ) ) { return 0; }
    move = Cap2Move(-BOARD[to]);
  }
  move |= To2Move(to) | From2Move(from) | Piece2Move(king);

  return move;
}


// 王手している駒(to)を捕獲するように駒を移動させる手の生成
// to : 駒を移動させる地点
static Move * CONV
gen_move_to( const tree_t * restrict ptree, int to, int turn,
             Move * restrict pmove )
{
  bitboard_t bb;
  int from, pc, flag_promo, flag_unpromo;
  direc_t direc;

  if ( turn == white )
    {
      bb = w_attacks_to_piece( ptree, to );
      BBNotAnd( bb, bb, abb_mask[SQ_WKING] ); // 玉では取らない
      while ( BBTest(bb) )
        {
          from = LastOne( bb );
          Xor( from, bb );

          direc = adirec[SQ_WKING][from];
          if ( direc && is_pinned_on_white_king( ptree, from, direc ) )
            {
              continue;
            }

          flag_promo   = 0;
          flag_unpromo = 1;
          pc           = -BOARD[from];
          switch ( pc )
            {
            case pawn:
              if ( to > I4 ) { flag_promo = 1;  flag_unpromo = 0; }
              break;

            case lance:   case knight:
              if      ( to > I3 ) { flag_promo = 1;  flag_unpromo = 0; }
              else if ( to > I4 ) { flag_promo = 1; }
              break;

            case silver:
              if ( to > I4 || from > I4 ) { flag_promo = 1; }
              break;

            case bishop:  case rook:
              if ( to > I4
                   || from > I4 ) { flag_promo = 1;  flag_unpromo = 0; }
              break;

            default:
              break;
            }
          assert( flag_promo || flag_unpromo );
          if ( flag_promo )
            {
              *pmove++ = ( From2Move(from) | To2Move(to) | FLAG_PROMO
                           | Piece2Move(pc) | Cap2Move(BOARD[to]) );
            }
          if ( flag_unpromo )
            {
              *pmove++ = ( From2Move(from) | To2Move(to)
                           | Piece2Move(pc) | Cap2Move(BOARD[to]) );
            }
        }
    }
  else {
    bb = b_attacks_to_piece( ptree, to );
    BBNotAnd( bb, bb, abb_mask[SQ_BKING] );
    while ( BBTest(bb) )
      {
        from = FirstOne( bb );
        Xor( from, bb );
        
        direc = adirec[SQ_BKING][from];
        if ( direc && is_pinned_on_black_king( ptree, from, direc ) )
          {
            continue;
          }

        flag_promo   = 0;
        flag_unpromo = 1;
        pc           = BOARD[from];
        switch ( pc )
          {
          case pawn:
            if ( to < A6 ) { flag_promo = 1;  flag_unpromo = 0; }
            break;
            
          case lance:  case knight:
            if      ( to < A7 ) { flag_promo = 1;  flag_unpromo = 0; }
            else if ( to < A6 ) { flag_promo = 1; }
            break;
            
          case silver:
            if ( to < A6 || from < A6 ) { flag_promo = 1; }
            break;
            
          case bishop:  case rook:
            if ( to < A6
                 || from < A6 ) { flag_promo = 1;  flag_unpromo = 0; }
            break;
            
          default:
            break;
          }
        assert( flag_promo || flag_unpromo );
        if ( flag_promo )
          {
            *pmove++ = ( From2Move(from) | To2Move(to) | FLAG_PROMO
                         | Piece2Move(pc) | Cap2Move(-BOARD[to]) );
          }
        if ( flag_unpromo )
          {
            *pmove++ = ( From2Move(from) | To2Move(to)
                         | Piece2Move(pc) | Cap2Move(-BOARD[to]) );
          }
      }
  }

  return pmove;
}


// 王手されているときに王の移動手を生成する
// is_capture == true なら駒を取る王の移動手のみ生成する
// is_capture == false なら駒を取らない王の移動手のみ生成する
// psqは王手している駒のBoardPos。最大2つ。
// 2つある場合は、psq[0]は近接王手をしているほうの駒。
// psq[1] == nsquareならば2つ目の王手駒は無し。
// 利きのないすべての退路に移動する手を生成する。
static Move * CONV
gen_king_move( const tree_t * restrict ptree, const int *psq, int turn,
               int is_capture, Move * restrict pmove )
{
  bitboard_t bb;
  int to, from;

  if ( turn == white )
    {
      from = SQ_WKING;
      bb   = abb_king_attacks[from];
      if ( is_capture )
        {
          BBAnd( bb, bb, BB_BOCCUPY );
          BBNotAnd( bb, bb, abb_mask[psq[0]] );
        }
      else { BBNotAnd( bb, bb, BB_BOCCUPY ); }
      BBNotAnd( bb, bb, BB_WOCCUPY );
    }
  else {
    from = SQ_BKING;
    bb   = abb_king_attacks[from];
    if ( is_capture )
      {
        BBAnd( bb, bb, BB_WOCCUPY );
        BBNotAnd( bb, bb, abb_mask[psq[0]] );
      }
    else { BBNotAnd( bb, bb, BB_WOCCUPY ); }
    BBNotAnd( bb, bb, BB_BOCCUPY );
  }
  
  while ( BBTest(bb) )
    {
      to = LastOne( bb );
      Xor( to, bb );

      if ( psq[1] != nsquare
           && adirec[from][psq[1]] == adirec[from][to] ) { continue; }

      if ( psq[0] != to
           && adirec[from][psq[0]] == adirec[from][to] ) {
          if ( adirec[from][psq[0]] & flag_cross )
            {
              if ( abs(BOARD[psq[0]]) == lance
                   || abs(BOARD[psq[0]]) == rook
                   || abs(BOARD[psq[0]]) == dragon ) { continue; }
            }
          else if ( ( adirec[from][psq[0]] & flag_diag )
                    && ( abs(BOARD[psq[0]]) == bishop
                         || abs(BOARD[psq[0]]) == horse ) ){ continue; }
        }

      if ( turn == white )
        {
          if ( is_white_attacked( ptree, to ) ) { continue; }

          *pmove++ = ( From2Move(from) | To2Move(to)
                       | Piece2Move(king) | Cap2Move(BOARD[to]) );
        }
      else {
        if ( is_black_attacked( ptree, to ) ) { continue; }

        *pmove++ = ( From2Move(from) | To2Move(to)
                     | Piece2Move(king) | Cap2Move(-BOARD[to]) );
      }
    }

  return pmove;
}


// 合駒する手を生成する。移動合いも生成する。
// よさげな手 = 利きのあるところに合駒/移動合いする手は優先して生成される。
// premaining : 生成したよさげな手の数 pmove[0]..pmove[*premaining-1]までがよさげな手。
//   ↑remaining(残り)のポインタでp + remainingと命名されている。
// turn : 詰みをチェックするほうの手番。blackならば先手手番で、
//        かつ先手玉が王手されている。
// sq_checker : 王手している駒
static Move * CONV
gen_intercept( tree_t * restrict __ptree__, int sq_checker, int ply, int turn,
               int * restrict premaining, Move * restrict pmove, int flag )
{
#define Drop(pc) ( To2Move(to) | Drop2Move(pc) )

  const tree_t * restrict ptree = __ptree__;
  bitboard_t bb_atk, bb_defender, bb;
  unsigned int amove[16];
  unsigned int hand;
  int n0, n1, inc, pc, sq_k, to, from, nmove, nsup, i, min_chuai, itemp;
  int dist, flag_promo, flag_unpromo;
  direc_t direc;

  n0 = n1 = 0;
  if ( turn == white )
    {
      sq_k        = SQ_WKING;
      bb_defender = BB_WOCCUPY;
      BBNotAnd( bb_defender, bb_defender, abb_mask[sq_k] );
    }
  else {
    sq_k        = SQ_BKING;
    bb_defender = BB_BOCCUPY;
    BBNotAnd( bb_defender, bb_defender, abb_mask[sq_k] );
  }

  switch ( adirec[sq_k][sq_checker] )
    {
    case direc_rank:
      min_chuai = ( sq_k < A8 || I2 < sq_k ) ? 2 : 4;
      inc       = 1;
      break;

    case direc_diag1:
      min_chuai = 3;
      inc       = 8;
      break;

    case direc_file:
      itemp     = (int)aifile[sq_k];
      min_chuai = ( itemp == file1 || itemp == file9 ) ? 2 : 4;
      inc = 9;
      break;

    default:
      assert( (int)adirec[sq_k][sq_checker] == direc_diag2 );
      min_chuai = 3;
      inc       = 10;
      break;
    }
  if ( sq_k > sq_checker ) { inc = -inc; }
  
  for ( dist = 1, to = sq_k + inc;
        to != sq_checker;
        dist += 1, to += inc ) {

    assert( 0 <= to && to < nsquare && BOARD[to] == empty );

    nmove  = 0;
    bb_atk = attacks_to_piece( ptree, to );
    BBAnd( bb, bb_defender, bb_atk );
    while ( BBTest(bb) )
      {
        from = LastOne( bb );
        Xor( from, bb );
        
        direc        = adirec[sq_k][from];
        flag_promo   = 0;
        flag_unpromo = 1;
        if ( turn )
          {
            if ( direc && is_pinned_on_white_king( ptree, from, direc ) )
              {
                continue;
              }
            pc = -BOARD[from];
            switch ( pc )
              {
              case pawn:
                if ( to > I4 ) { flag_promo = 1;  flag_unpromo = 0; }
                break;
                
              case lance:  case knight:
                if      ( to > I3 ) { flag_promo = 1;  flag_unpromo = 0; }
                else if ( to > I4 ) { flag_promo = 1; }
                break;
                
              case silver:
                if ( to > I4 || from > I4 ) { flag_promo = 1; }
                break;
                
              case bishop:  case rook:
                if ( to > I4
                     || from > I4 ) { flag_promo = 1;  flag_unpromo = 0; }
                break;
                
              default:
                break;
              }
          }
        else {
          if ( direc && is_pinned_on_black_king( ptree, from, direc ) )
            {
              continue;
            }
          pc = BOARD[from];
          switch ( pc )
            {
            case pawn:
              if ( to < A6 ) { flag_promo = 1;  flag_unpromo = 0; }
              break;
              
            case lance:  case knight:
              if      ( to < A7 ) { flag_promo = 1;  flag_unpromo = 0; }
              else if ( to < A6 ) { flag_promo = 1; }
              break;
              
            case silver:
              if ( to < A6 || from < A6 ) { flag_promo = 1; }
              break;
              
            case bishop:  case rook:
              if ( to < A6
                   || from < A6 ) { flag_promo = 1;  flag_unpromo = 0; }
              break;
              
            default:
              break;
            }
        }
        assert( flag_promo || flag_unpromo );
        if ( flag_promo )
          {
            amove[nmove++] = ( From2Move(from) | To2Move(to)
                               | FLAG_PROMO | Piece2Move(pc) );
          }
        if ( flag_unpromo )
          {
            amove[nmove++] = ( From2Move(from) | To2Move(to)
                               | Piece2Move(pc) );
          }
      }
    
    nsup = ( to == sq_k + inc ) ? nmove + 1 : nmove;
    if ( nsup > 1 )
      {
        for ( i = n0 + n1 - 1; i >= n0; i-- ) { pmove[i+nmove] = pmove[i]; }
        for ( i = 0; i < nmove; i++ ) { pmove[n0++] = amove[i]; }
      }
    else if ( nmove ) { pmove[n0 + n1++] = amove[0]; }
    
    if ( ! nsup )
      {
        /* - tentative assumption - */
        /* no recursive drops at non-supported square. */
        if ( flag == 2  ) { continue; }
        
        /* -tentative assumption- */
        /* no intercept-drop at non-supported square. */
        if ( (int)I2To(MOVE_LAST) == sq_checker
             && dist > min_chuai ) { continue; }
      }
    
    nmove = 0;
    
    if ( turn ) {
      
      hand = HAND_W;
      
      if ( nsup ) {
        
        if ( IsHandRook(hand) ) { amove[nmove++] = Drop(rook); }
        else if ( IsHandLance(hand) && to < A1 )
          {
            amove[nmove++] = Drop(lance);
          }
        else if ( IsHandPawn(hand)
                  && to < A1
                  && ! ( BBToU(BB_WPAWN) & ( mask_file1 >> aifile[to] ) )
                  && ! IsMateWPawnDrop( __ptree__, to ) )
          {
            amove[nmove++] = Drop(pawn);
          }
        
      } else {
        
        if ( IsHandPawn(hand)
             && to < A1
             && ! ( BBToU(BB_WPAWN) & ( mask_file1 >> aifile[to] ) )
             && ! IsMateWPawnDrop( __ptree__, to ) )
          {
            amove[nmove++] = Drop(pawn);
          }
        if ( IsHandLance(hand) && to < A1 ) { amove[nmove++] = Drop(lance); }
        if ( IsHandRook(hand) )             { amove[nmove++] = Drop(rook); }
      }
      
      if ( IsHandKnight(hand) && to < A2 ) { amove[nmove++] = Drop(knight); }
      
    } else {
      
      hand = HAND_B;
      
      if ( nsup ) {
        
        if ( IsHandRook(hand) ) { amove[nmove++] = Drop(rook); }
        else if ( IsHandLance(hand) && to > I9 )
          {
            amove[nmove++] = Drop(lance);
          }
        else if ( IsHandPawn(hand)
                  && to > I9
                  && ! ( BBToU(BB_BPAWN) & ( mask_file1 >> aifile[to] ) )
                  && ! IsMateBPawnDrop( __ptree__, to ) )
          {
            amove[nmove++] = Drop(pawn);
          }
        
      } else {
        
        if ( IsHandPawn(hand)
             && to > I9
             && ! ( BBToU(BB_BPAWN) & ( mask_file1 >> aifile[to] ) )
             && ! IsMateBPawnDrop( __ptree__, to ) )
          {
            amove[nmove++] = Drop(pawn);
          }
        if ( IsHandLance(hand) && to > I9 ) { amove[nmove++] = Drop(lance); }
        if ( IsHandRook(hand) )             { amove[nmove++] = Drop(rook); }
      }
      
      if ( IsHandKnight(hand) && to > I8 ) { amove[nmove++] = Drop(knight); }
    }
    
    if ( IsHandSilver(hand) ) { amove[nmove++] = Drop(silver); }
    if ( IsHandGold(hand) )   { amove[nmove++] = Drop(gold); }
    if ( IsHandBishop(hand) ) { amove[nmove++] = Drop(bishop); }
    
    if ( nsup )
      {
        /* - tentative assumption - */
        /* a supported intercepter saves the king for two plies at least. */
        if ( nmove && flag == 0 && dist > min_chuai
             && I2From(MOVE_LAST) >= nsquare )
            {
              *premaining = -1;
              pmove[0]    = amove[0];
              return pmove + 1;
            }

        for ( i = n0 + n1 - 1; i >= n0; i-- ) { pmove[i+nmove] = pmove[i]; }
        for ( i = 0; i < nmove; i++ ) { pmove[n0++] = amove[i]; }
      }
    else for ( i = 0; i < nmove; i++ ) { pmove[n0 + n1++] = amove[i]; }
  }
  
  *premaining = n0;
  return pmove + n0 + n1;

#undef Drop
}
