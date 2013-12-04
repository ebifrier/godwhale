// $Id: evaluate.c,v 1.4 2012-04-02 21:59:04 eiki Exp $

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "shogi.h"

#define ENB_CASTLING_MATERIAL 1

static int CONV sq2y(int sq) { return (1+sq/9); }

#define PcPcOnSqAny(k,i,j) PcPcOnSq(k,i,j)

const int pc2suf[31] = {
  e_dragon, e_horse, e_gold, e_gold, e_gold, e_gold, e_gold, 0,
  e_rook, e_bishop, e_gold, e_silver, e_knight, e_lance, e_pawn,
  0,
  f_pawn, f_lance, f_knight, f_silver, f_gold, f_bishop, f_rook, 0,
  f_gold, f_gold, f_gold, f_gold, f_gold, f_horse, f_dragon
};

static int CONV eval_castling_material( const tree_t * restrict ptree,
                                        int ouby, int ouwy );
static int CONV doacapt( const tree_t * restrict ptree, int pc, int turn,
                         const int list0[52],
                         const int list1[52], int *curhand, int nlist );
static int CONV doapc( const tree_t * restrict ptree, int pc, int sq,
                       const int list0[52], const int list1[52], int nlist );
static int CONV ehash_probe( uint64_t current_key, unsigned int hand_b,
                             int * restrict pscore );
static void CONV ehash_store( uint64_t key, unsigned int hand_b, int score );
static int CONV calc_difference( tree_t * restrict ptree, int ply,
                                 int turn, int list0[52], int list1[52],
                                 int * restrict pscore );
static int CONV make_list( const tree_t * restrict ptree,
                           int * restrict pscore,
                           int list0[52], int list1[52] );

#if defined(INANIWA_SHIFT)
static int inaniwa_score( const tree_t * restrict ptree );
#endif

static const int acastling_bonus[10] = {
  0, 0, 0, 50, 210, 420, 730, 1040, 1350, 1660
};


int CONV
eval_material( const tree_t * restrict ptree )
{
  int material, itemp;

  itemp     = PopuCount( BB_BPAWN )   + (int)I2HandPawn( HAND_B );
  itemp    -= PopuCount( BB_WPAWN )   + (int)I2HandPawn( HAND_W );
  material  = itemp * p_value[15+pawn];

  itemp     = PopuCount( BB_BLANCE )  + (int)I2HandLance( HAND_B );
  itemp    -= PopuCount( BB_WLANCE )  + (int)I2HandLance( HAND_W );
  material += itemp * p_value[15+lance];

  itemp     = PopuCount( BB_BKNIGHT ) + (int)I2HandKnight( HAND_B );
  itemp    -= PopuCount( BB_WKNIGHT ) + (int)I2HandKnight( HAND_W );
  material += itemp * p_value[15+knight];

  itemp     = PopuCount( BB_BSILVER ) + (int)I2HandSilver( HAND_B );
  itemp    -= PopuCount( BB_WSILVER ) + (int)I2HandSilver( HAND_W );
  material += itemp * p_value[15+silver];

  itemp     = PopuCount( BB_BGOLD )   + (int)I2HandGold( HAND_B );
  itemp    -= PopuCount( BB_WGOLD )   + (int)I2HandGold( HAND_W );
  material += itemp * p_value[15+gold];

  itemp     = PopuCount( BB_BBISHOP ) + (int)I2HandBishop( HAND_B );
  itemp    -= PopuCount( BB_WBISHOP ) + (int)I2HandBishop( HAND_W );
  material += itemp * p_value[15+bishop];

  itemp     = PopuCount( BB_BROOK )   + (int)I2HandRook( HAND_B );
  itemp    -= PopuCount( BB_WROOK )   + (int)I2HandRook( HAND_W );
  material += itemp * p_value[15+rook];

  itemp     = PopuCount( BB_BPRO_PAWN );
  itemp    -= PopuCount( BB_WPRO_PAWN );
  material += itemp * p_value[15+pro_pawn];

  itemp     = PopuCount( BB_BPRO_LANCE );
  itemp    -= PopuCount( BB_WPRO_LANCE );
  material += itemp * p_value[15+pro_lance];

  itemp     = PopuCount( BB_BPRO_KNIGHT );
  itemp    -= PopuCount( BB_WPRO_KNIGHT );
  material += itemp * p_value[15+pro_knight];

  itemp     = PopuCount( BB_BPRO_SILVER );
  itemp    -= PopuCount( BB_WPRO_SILVER );
  material += itemp * p_value[15+pro_silver];

  itemp     = PopuCount( BB_BHORSE );
  itemp    -= PopuCount( BB_WHORSE );
  material += itemp * p_value[15+horse];

  itemp     = PopuCount( BB_BDRAGON );
  itemp    -= PopuCount( BB_WDRAGON );
  material += itemp * p_value[15+dragon];

  return material;
}


static int CONV
eval_castling_material( const tree_t * restrict ptree, int ouby, int ouwy )
{
  int filtered_point, black_point, x, z, h;
  int sq;

  black_point = 0;
  for ( sq = 0; sq < nsquare; sq++ )
    {
      char c = BOARD[sq];
      if ( c == rook   || c == dragon ||
           c == bishop || c == horse    )
        black_point += 5;
      else if ( c > 0 )
        black_point += 1;   // NOTE counting king also here
    }
  h = HAND_B;
  black_point += I2HandPawn(h) + I2HandLance(h) +
                 I2HandKnight(h) + I2HandSilver(h) +
                 I2HandGold(h) + 5 * (I2HandBishop(h) + I2HandRook(h));
  x = black_point - 28;  // -27 to +27   0 means even

  //  x   -27  ..  -2  -1  0   1   2   3   4   5    ..  27
  //      -39      -8  -4  0   4   8  12  16  17        39
  filtered_point = (x>4 ? x+4*3 : x<-4 ? x-4*3 : 4*x);
  z = (ouby + ouwy - 6) * filtered_point * 4;   // FIXME? tune

#if defined(DBG_CASTLING_MATERIAL)
  printf( "\n{{{{ mv %d ouby %d ouwy %d blackpoint %d filteredpoint %d z %d\n",
          ptree->current_move[1], ouby, ouwy, black_point, filtered_point, z );
  exit( 4 );
#endif

  return z;
};


int CONV
evaluate_body( tree_t * restrict ptree, int ply, int turn )
{
  int *list0, *list1;
  int nlist, score, sq_bk, sq_wk, i, j, sum;
  int castling_bonus, ouby, ouwy;

#if 0
  assert( 0 < ply );
  ptree->neval_called++;

  if ( ptree->save_eval[ply] != INT_MAX )
    {
      return (int)ptree->save_eval[ply] / FV_SCALE;
    }

  if ( ehash_probe( HASH_KEY, HAND_B, &score ) )
    {
      score                 = turn ? -score : score;
      ptree->save_eval[ply] = score;
      return score / FV_SCALE;
    }
#endif

  list0 = ptree->list0;
  list1 = ptree->list1;
  nlist = ptree->nlist;

  if ( calc_difference( ptree, ply, turn, list0, list1, &score ) )
    {
      ehash_store( HASH_KEY, HAND_B, score );
      score                 = turn ? -score : score;
      ptree->save_eval[ply] = score;

      return score / FV_SCALE;
    }

  make_list( ptree, &score, list0, list1 );
  sq_bk = SQ_BKING;
  sq_wk = Inv( SQ_WKING );

  sum = 0;
  for ( i = 0; i < nlist; i++ )
    {
      int k0 = list0[i];
      int k1 = list1[i];
      for ( j = 0; j <= i; j++ )
        {
          sum += PcPcOnSq( sq_bk, k0, list0[j] );
          sum -= PcPcOnSq( sq_wk, k1, list1[j] );
        }
    }
  
  score += sum;
  score += MATERIAL * FV_SCALE;
#if defined(INANIWA_SHIFT)
  score += inaniwa_score( ptree );
#endif

  ouby = sq2y(Inv(SQ_BKING));
  ouwy = sq2y(    SQ_WKING );
  castling_bonus = acastling_bonus[ouby] - acastling_bonus[ouwy];

  if ( ENB_CASTLING_MATERIAL && ouby + ouwy >= 7 )
    castling_bonus += eval_castling_material( ptree, ouby, ouwy );

  score += castling_bonus * FV_SCALE;

  ehash_store( HASH_KEY, HAND_B, score );

  score = turn ? -score : score;

  ptree->save_eval[ply] = score;

  score /= FV_SCALE;

#if ! defined(MINIMUM)
  if ( abs(score) > score_max_eval )
    {
      out_warning( "A score at evaluate() is out of bounce." );
    }
#endif

  return score;
}


void CONV
ehash_clear( void )
{
  memset( ehash_tbl, 0, sizeof(ehash_tbl) );
}


static void CONV
ehash_store( uint64_t key, unsigned int hand_b, int score )
{
  uint64_t hash_word;

  hash_word  = key;
  hash_word ^= (uint64_t)hand_b << 21;
  hash_word &= ~(uint64_t)0x1fffffU;
  hash_word |= (uint64_t)( score + 0x100000 );

#if ! defined(__x86_64__)
  hash_word ^= hash_word << 32;
#endif

  ehash_tbl[ (unsigned int)key & EHASH_MASK ] = hash_word;
}


void CONV
listswap( tree_t* restrict ptree, int a, int b )
{
  int x, qa, qb;

  if ( a == b ) return;
  x = ptree->list0[a];
  ptree->list0[a] = ptree->list0[b];
  ptree->list0[b] = x;

  x = ptree->list1[a];
  ptree->list1[a] = ptree->list1[b];
  ptree->list1[b] = x;

  qa = ptree->sq4listsuf[a];
  qb = ptree->sq4listsuf[a] = ptree->sq4listsuf[b];
  ptree->sq4listsuf[b] = qa;

  ptree->listsuf4sq[qa] = b;
  ptree->listsuf4sq[qb] = a;
}


static int CONV
calc_difference( tree_t * restrict __ptree__, int ply, int turn,
                 int list0[52], int list1[52], int * restrict pscore )
{
  const tree_t *ptree = __ptree__;
  int nlist, diff, from, to, pc;
  int *curhand;

#if defined(INANIWA_SHIFT)
  if ( inaniwa_flag ) { return 0; }
#endif

  if ( ptree->save_eval[ply-1] == INT_MAX ) { return 0; }
  if ( I2PieceMove(MOVE_LAST)  == king )    { return 0; }

  assert( MOVE_LAST != MOVE_PASS );

  nlist   = __ptree__->nlist;
  curhand = __ptree__->curhand;
  diff  = 0;
  from  = I2From(MOVE_LAST);
  to    = I2To(MOVE_LAST);

  // dst comes last
  if ( ptree->listsuf4sq[to] != nlist-1 )
    listswap( __ptree__, nlist-1, ptree->listsuf4sq[to] );

  pc = BOARD[to];
  diff = doapc( ptree, pc, to, list0, list1, nlist );
  nlist -= 1;

  if ( from >= nsquare )
    {
      pc   = From2Drop( from );

        // add capt
      diff += doacapt( ptree, pc, 1-turn, list0, list1, curhand, nlist );

        // incr capt
      list0[ 2*(pc-1) + 1 - turn ] += 1;
      list1[ 2*(pc-1) + turn     ] += 1;
      curhand[ 2*(pc-1) + 1 - turn ] += 1;

        // sub capt+1
      diff -= doacapt( ptree, pc, 1-turn, list0, list1, curhand, nlist );

        // restore capt
      list0[2*(pc-1) + 1 - turn]--;
      list1[2*(pc-1) +     turn]--;
      curhand[2*(pc-1) + 1 - turn]--;
    }
  else {  // slide case
    int dst0bak, dst1bak, capnoprom;
    int pc_cap = UToCap(MOVE_LAST);
    capnoprom  = pc_cap & ~promote;
    if ( pc_cap )     // tgt exists
      {
        pc     = capnoprom;
        pc_cap = turn ? -pc_cap : pc_cap;
        diff  += turn ?  p_value_ex[15+pc_cap] * FV_SCALE
                      : -p_value_ex[15+pc_cap] * FV_SCALE;

          // add capt
        diff += doacapt( ptree, pc, 1-turn, list0, list1, curhand, nlist );
        
          // decr capt
        list0[ 2*(pc-1) + 1 - turn ] -= 1;
        list1[ 2*(pc-1) + turn     ] -= 1;
        curhand[ 2*(pc-1) + 1 - turn ] -= 1;
        
          // sub capt-1
        diff -= doacapt( ptree, pc, 1-turn, list0, list1, curhand, nlist );

        dst0bak = list0[nlist  ];
        dst1bak = list1[nlist  ];
        list0[nlist  ] = aikpp[15+pc_cap] + to;
        list1[nlist++] = aikpp[15-pc_cap] + Inv(to);

        diff -= doapc( ptree, pc_cap, to, list0, list1, nlist );
    }

    pc = I2PieceMove(MOVE_LAST);
    if ( I2IsPromote(MOVE_LAST) )
      {
        diff += ( turn ? p_value_pm[7+pc] : -p_value_pm[7+pc] ) * FV_SCALE;
      }
    
    pc = turn ? pc : -pc;

    if ( ! pc_cap )
      {
        dst0bak = list0[nlist  ];
        dst1bak = list1[nlist  ];
      }
    list0[nlist  ] = aikpp[15+pc] + from;
    list1[nlist++] = aikpp[15-pc] + Inv(from);

     // sub src
    diff -= doapc( ptree, pc, from, list0, list1, nlist );
  
    // restore dst
    nlist -= (pc_cap ? 1 : 0);
    list0[nlist-1] = dst0bak;
    list1[nlist-1] = dst1bak;

      // restore capt
    if ( pc_cap )
      {
        list0[2*(capnoprom-1) + 1 - turn]++;
        list1[2*(capnoprom-1) +     turn]++;
        curhand[2*(capnoprom-1) + 1 - turn]++;
      }
  }
  
  diff += turn ? ptree->save_eval[ply-1] : -ptree->save_eval[ply-1];
  *pscore = diff;
  return 1;
}


static int CONV
make_list( const tree_t * restrict ptree, int * restrict pscore,
           int list0[52], int list1[52] )
{
  static const int kkpbase[] = {
    0, kkp_pawn, kkp_lance, kkp_knight, kkp_silver, kkp_gold,
    kkp_bishop, kkp_rook,
    0, kkp_gold, kkp_gold, kkp_gold, kkp_gold, kkp_gold,
    kkp_horse, kkp_dragon
  };
  int sq, score, sq_bk0, sq_wk0, sq_bk1, sq_wk1;
  short *kkp0, *kkp1;

  (void)list0;
  (void)list1;

  sq_bk0 = SQ_BKING;
  sq_wk0 = SQ_WKING;
  sq_bk1 = Inv(SQ_WKING);
  sq_wk1 = Inv(SQ_BKING);
  kkp0 = kkp[sq_bk0][sq_wk0];
  kkp1 = kkp[sq_bk1][sq_wk1];

  score  = 0;
  score += kkp0[ kkp_hand_pawn   + I2HandPawn(HAND_B) ];
  score += kkp0[ kkp_hand_lance  + I2HandLance(HAND_B) ];
  score += kkp0[ kkp_hand_knight + I2HandKnight(HAND_B) ];
  score += kkp0[ kkp_hand_silver + I2HandSilver(HAND_B) ];
  score += kkp0[ kkp_hand_gold   + I2HandGold(HAND_B) ];
  score += kkp0[ kkp_hand_bishop + I2HandBishop(HAND_B) ];
  score += kkp0[ kkp_hand_rook   + I2HandRook(HAND_B) ];

  score -= kkp1[ kkp_hand_pawn   + I2HandPawn(HAND_W) ];
  score -= kkp1[ kkp_hand_lance  + I2HandLance(HAND_W) ];
  score -= kkp1[ kkp_hand_knight + I2HandKnight(HAND_W) ];
  score -= kkp1[ kkp_hand_silver + I2HandSilver(HAND_W) ];
  score -= kkp1[ kkp_hand_gold   + I2HandGold(HAND_W) ];
  score -= kkp1[ kkp_hand_bishop + I2HandBishop(HAND_W) ];
  score -= kkp1[ kkp_hand_rook   + I2HandRook(HAND_W) ];

  for ( sq = 0; sq < nsquare; sq++ )
    {
      int pc = BOARD[sq];
      if ( pc & 7 )  // omit kings and empty
        {
          if ( pc > 0 )
            score += kkp0[ kkpbase[pc] + sq ];
		  else
            score -= kkp1[ kkpbase[-pc] + Inv(sq) ];
        }
    }

  *pscore = score;
  return 0;
}


static int CONV
doapc( const tree_t * restrict ptree, int pc, int sq,
       const int list0[52], const int list1[52], int nlist )
{
  int index_b = aikpp[15+pc] + sq;
  int index_w = aikpp[15-pc] + Inv(sq);
  int sq_bk   = SQ_BKING;
  int sq_wk   = Inv(SQ_WKING);
  int i, sum;
  
  sum = 0;
  for( i = 0; i < nlist; i++ )
    {
      sum += PcPcOnSqAny( sq_bk, index_b, list0[i] );
      sum -= PcPcOnSqAny( sq_wk, index_w, list1[i] );
    }
  
  if ( pc > 0 )
    {
      sq_bk  = SQ_BKING;
      sq_wk  = SQ_WKING;
      sum   += kkp[sq_bk][sq_wk][ aikkp[pc] + sq ];
    }
  else {
    sq_bk  = Inv(SQ_WKING);
    sq_wk  = Inv(SQ_BKING);
    sum   -= kkp[sq_bk][sq_wk][ aikkp[-pc] + Inv(sq) ];
  }
  
  return sum;
}


static int CONV
doacapt( const tree_t * restrict ptree, int pc, int turn,
         const int list0[52], const int list1[52], int *curhand, int nlist )
{
  int i, sum, sq_bk, sq_wk, suf0b, suf0w;
  
  suf0b = list0[2*(pc-1) + turn];
  suf0w = list1[2*(pc-1) + 1 - turn];
  sq_bk = SQ_BKING;
  sq_wk = Inv(SQ_WKING);
  
  sum = 0;
  for ( i =  0; i < nlist; i++ )
    {
      sum += PcPcOnSq( sq_bk, list0[i], suf0b );
      sum -= PcPcOnSq( sq_wk, list1[i], suf0w );
    }

  if ( ! turn )
    {
      sq_bk = SQ_BKING;
      sq_wk = SQ_WKING;
      sum  += kkp[sq_bk][sq_wk][ aikkp_hand[pc] + curhand[2*(pc-1)] ];
    }
  else {
    sq_bk = Inv(SQ_WKING);
    sq_wk = Inv(SQ_BKING);
    sum  -= kkp[sq_bk][sq_wk][ aikkp_hand[pc] + curhand[2*(pc-1)+1] ];
  }
  
  return sum;
}


#if defined(INANIWA_SHIFT)
static int
inaniwa_score( const tree_t * restrict ptree )
{
  int score;

  if ( ! inaniwa_flag ) { return 0; }

  score = 0;
  if ( inaniwa_flag == 2 ) {

    if ( BOARD[B9] == -knight ) { score += 700 * FV_SCALE; }
    if ( BOARD[H9] == -knight ) { score += 700 * FV_SCALE; }
    
    if ( BOARD[A7] == -knight ) { score += 700 * FV_SCALE; }
    if ( BOARD[C7] == -knight ) { score += 400 * FV_SCALE; }
    if ( BOARD[G7] == -knight ) { score += 400 * FV_SCALE; }
    if ( BOARD[I7] == -knight ) { score += 700 * FV_SCALE; }
    
    if ( BOARD[B5] == -knight ) { score += 700 * FV_SCALE; }
    if ( BOARD[D5] == -knight ) { score += 100 * FV_SCALE; }
    if ( BOARD[F5] == -knight ) { score += 100 * FV_SCALE; }
    if ( BOARD[H5] == -knight ) { score += 700 * FV_SCALE; }

    if ( BOARD[E3] ==  pawn )   { score += 200 * FV_SCALE; }
    if ( BOARD[E4] ==  pawn )   { score += 200 * FV_SCALE; }
    if ( BOARD[E5] ==  pawn )   { score += 200 * FV_SCALE; }

  } else {

    if ( BOARD[B1] ==  knight ) { score -= 700 * FV_SCALE; }
    if ( BOARD[H1] ==  knight ) { score -= 700 * FV_SCALE; }
    
    if ( BOARD[A3] ==  knight ) { score -= 700 * FV_SCALE; }
    if ( BOARD[C3] ==  knight ) { score -= 400 * FV_SCALE; }
    if ( BOARD[G3] ==  knight ) { score -= 400 * FV_SCALE; }
    if ( BOARD[I3] ==  knight ) { score -= 700 * FV_SCALE; }
    
    if ( BOARD[B5] ==  knight ) { score -= 700 * FV_SCALE; }
    if ( BOARD[D5] ==  knight ) { score -= 100 * FV_SCALE; }
    if ( BOARD[F5] ==  knight ) { score -= 100 * FV_SCALE; }
    if ( BOARD[H5] ==  knight ) { score -= 700 * FV_SCALE; }

    if ( BOARD[E7] == -pawn )   { score -= 200 * FV_SCALE; }
    if ( BOARD[E6] == -pawn )   { score -= 200 * FV_SCALE; }
    if ( BOARD[E5] == -pawn )   { score -= 200 * FV_SCALE; }
  }

  return score;
}
#endif
