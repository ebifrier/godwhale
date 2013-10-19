// $Id: evaluate.c,v 1.4 2012-04-02 21:59:04 eiki Exp $

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "shogi.h"

#define ENB_CASTLING_MATERIAL 1
//#define DBG_CASTLING_MATERIAL

static int sq2y(int sq) { return (1+sq/9); }

//#define PcPcOnSqAny(k,i,j) ( i >= j ? PcPcOnSq(k,i,j) : PcPcOnSq(k,j,i) )
#define PcPcOnSqAny(k,i,j) PcPcOnSq(k,i,j)

const int pc2suf[31] = {
 e_dragon, e_horse, e_gold, e_gold, e_gold, e_gold, e_gold, 0,
 e_rook, e_bishop, e_gold, e_silver, e_knight, e_lance, e_pawn,
 0,
 f_pawn, f_lance, f_knight, f_silver, f_gold, f_bishop, f_rook, 0,
 f_gold, f_gold, f_gold, f_gold, f_gold, f_horse, f_dragon
};

static int CONV doacapt( const tree_t * restrict ptree, int pc, int turn,
                         const int list0[52],
                         const int list1[52], int *curhand, int nlist );
                         //int hand_index, const int list0[52],
                         //const int list1[52], int nlist );
static int CONV doapc( const tree_t * restrict ptree, int pc, int sq,
                       const int list0[52], const int list1[52], int nlist );
static int CONV ehash_probe( uint64_t current_key, unsigned int hand_b,
                             int * restrict pscore );
static void CONV ehash_store( uint64_t key, unsigned int hand_b, int score );
//static int CONV calc_difference( const tree_t * restrict ptree, int ply,
static int CONV calc_difference( tree_t * restrict ptree, int ply,
                                 int turn, int list0[52], int list1[52],
                                 int * restrict pscore );
static int CONV make_list( const tree_t * restrict ptree,
                           int * restrict pscore,
                           int list0[52], int list1[52] );

#if defined(INANIWA_SHIFT)
static int inaniwa_score( const tree_t * restrict ptree );
#endif

static const int castlingBonusAry[10] =
      { 0, 0, 0, 50, 210, 420, 730, 1040, 1350, 1660 };

static const int kkpbase[16] = {
 0,
 kkp_pawn, 
 kkp_lance,
 kkp_knight,
 kkp_silver,
 kkp_gold,
 kkp_bishop,
 kkp_rook,
 0,
 kkp_gold, 
 kkp_gold,
 kkp_gold,
 kkp_gold,
 kkp_gold,
 kkp_horse,
 kkp_dragon
};


static int CONV
castlingMaterialPoint( const tree_t * restrict ptree, int ouby, int ouwy )
{
 int filteredPoint, blackPoint, x, z, h;
 int sq;
 blackPoint = 0;
 for(sq=0; sq<=80; sq++) {
   char c = BOARD[sq];
   if (c==rook   || c==dragon ||
       c==bishop || c==horse    )
      blackPoint += 5;
   else if (c>0)
      blackPoint += 1;   // NOTE counting king also here
 }
 h = HAND_B;
 blackPoint += I2HandPawn(h) + I2HandLance(h) +
               I2HandKnight(h) + I2HandSilver(h) +
               I2HandGold(h) + 5 * (I2HandBishop(h) + I2HandRook(h));
 x = blackPoint - 28;  // -27 to +27   0 means even

   //  x   -27  ..  -2  -1  0   1   2   3   4   5    ..  27
   //      -39      -8  -4  0   4   8  12  16  17        39
 filteredPoint = (x>4 ? x+4*3 : x<-4 ? x-4*3 : 4*x);
 z = (ouby + ouwy - 6) * filteredPoint * 4;   // FIXME? tune
#ifdef DBG_CASTLING_MATERIAL
printf("\n{{{{ mv %d ouby %d ouwy %d blackPoint %d filteredPoint %d z %d\n",
       ptree->current_move[1], ouby, ouwy, blackPoint, filteredPoint, z);
exit(4);
#endif
 return z;
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


int CONV
evaluate( tree_t * restrict ptree, int ply, int turn )
{
  //int list0[52], list1[52];
  int *list0, *list1;
  int nlist, score, sq_bk, sq_wk, k0, k1, l0, l1, i, j, sum;
  int castlingBonus, ouby, ouwy;

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

  list0 = ptree->list0;
  list1 = ptree->list1;
  nlist = ptree->nlist;
#if 0
  list0[ 0] = f_hand_pawn   + I2HandPawn(HAND_B);
  list0[ 1] = e_hand_pawn   + I2HandPawn(HAND_W);
  list0[ 2] = f_hand_lance  + I2HandLance(HAND_B);
  list0[ 3] = e_hand_lance  + I2HandLance(HAND_W);
  list0[ 4] = f_hand_knight + I2HandKnight(HAND_B);
  list0[ 5] = e_hand_knight + I2HandKnight(HAND_W);
  list0[ 6] = f_hand_silver + I2HandSilver(HAND_B);
  list0[ 7] = e_hand_silver + I2HandSilver(HAND_W);
  list0[ 8] = f_hand_gold   + I2HandGold(HAND_B);
  list0[ 9] = e_hand_gold   + I2HandGold(HAND_W);
  list0[10] = f_hand_bishop + I2HandBishop(HAND_B);
  list0[11] = e_hand_bishop + I2HandBishop(HAND_W);
  list0[12] = f_hand_rook   + I2HandRook(HAND_B);
  list0[13] = e_hand_rook   + I2HandRook(HAND_W);

  list1[ 0] = f_hand_pawn   + I2HandPawn(HAND_W);
  list1[ 1] = e_hand_pawn   + I2HandPawn(HAND_B);
  list1[ 2] = f_hand_lance  + I2HandLance(HAND_W);
  list1[ 3] = e_hand_lance  + I2HandLance(HAND_B);
  list1[ 4] = f_hand_knight + I2HandKnight(HAND_W);
  list1[ 5] = e_hand_knight + I2HandKnight(HAND_B);
  list1[ 6] = f_hand_silver + I2HandSilver(HAND_W);
  list1[ 7] = e_hand_silver + I2HandSilver(HAND_B);
  list1[ 8] = f_hand_gold   + I2HandGold(HAND_W);
  list1[ 9] = e_hand_gold   + I2HandGold(HAND_B);
  list1[10] = f_hand_bishop + I2HandBishop(HAND_W);
  list1[11] = e_hand_bishop + I2HandBishop(HAND_B);
  list1[12] = f_hand_rook   + I2HandRook(HAND_W);
  list1[13] = e_hand_rook   + I2HandRook(HAND_B);
#endif

  if ( calc_difference( ptree, ply, turn, list0, list1, &score ) )
    {
      ehash_store( HASH_KEY, HAND_B, score );
      score                 = turn ? -score : score;
      ptree->save_eval[ply] = score;

      return score / FV_SCALE;
    }

  //nlist = make_list( ptree, &score, list0, list1 );
  make_list( ptree, &score, list0, list1 );
  sq_bk = SQ_BKING;
  sq_wk = Inv( SQ_WKING );

  sum = 0;
  for ( i = 0; i < nlist; i++ )
    {
      k0 = list0[i];
      k1 = list1[i];
      for ( j = 0; j <= i; j++ )
        {
          l0 = list0[j];
          l1 = list1[j];
          //assert( k0 >= l0 && k1 >= l1 );
          sum += PcPcOnSq( sq_bk, k0, l0 );
          sum -= PcPcOnSq( sq_wk, k1, l1 );
        }
    }
  
  score += sum;
  score += MATERIAL * FV_SCALE;
#if defined(INANIWA_SHIFT)
  score += inaniwa_score( ptree );
#endif

  ouby = sq2y(Inv(SQ_BKING));
  ouwy = sq2y(    SQ_WKING );
  castlingBonus = castlingBonusAry[ouby]
                 - castlingBonusAry[ouwy];

  if (ENB_CASTLING_MATERIAL && ouby + ouwy >= 7)
    castlingBonus += castlingMaterialPoint(ptree, ouby, ouwy);

  score += castlingBonus * FV_SCALE;

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


void CONV ehash_clear( void ) { memset( ehash_tbl, 0, sizeof(ehash_tbl) ); }


static int CONV ehash_probe( uint64_t current_key, unsigned int hand_b,
                             int * restrict pscore )
{
  uint64_t hash_word, hash_key;

  hash_word = ehash_tbl[ (unsigned int)current_key & EHASH_MASK ];

#if ! defined(__x86_64__)
  hash_word ^= hash_word << 32;
#endif

  current_key ^= (uint64_t)hand_b << 21;
  current_key &= ~(uint64_t)0x1fffffU;

  hash_key  = hash_word;
  hash_key &= ~(uint64_t)0x1fffffU;

  if ( hash_key != current_key ) { return 0; }

  *pscore = (int)( (unsigned int)hash_word & 0x1fffffU ) - 0x100000;

  return 1;
}


static void CONV ehash_store( uint64_t key, unsigned int hand_b, int score )
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

 //****
void listswap(tree_t* restrict ptree, int a, int b) {
 int x, qa, qb;
 if (a==b) return;
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
//calc_difference( const tree_t * restrict ptree, int ply, int turn,
calc_difference( tree_t * restrict ptree, int ply, int turn,
                 int list0[52], int list1[52], int * restrict pscore )
{
  //bitboard_t bb;
  int nlist, diff, from, to, /*sq,*/ pc;
  int *curhand;

#if defined(INANIWA_SHIFT)
  if ( inaniwa_flag ) { return 0; }
#endif

  if ( ptree->save_eval[ply-1] == INT_MAX ) { return 0; }
  if ( I2PieceMove(MOVE_LAST)  == king )    { return 0; }

  assert( MOVE_LAST != MOVE_PASS );

  //nlist = 14;
  nlist   = ptree->nlist;
  curhand = ptree->curhand;
  diff  = 0;
  from  = I2From(MOVE_LAST);
  to    = I2To(MOVE_LAST);

#if 0
  BBOr( bb, BB_BOCCUPY, BB_WOCCUPY );
  Xor( SQ_BKING, bb );
  Xor( SQ_WKING, bb );
  Xor( to,       bb );
    
  while ( BBTest(bb) )
    {
      sq = FirstOne(bb);
      Xor( sq, bb );
      
      pc = BOARD[sq];
      list0[nlist  ] = aikpp[15+pc] + sq;
      list1[nlist++] = aikpp[15-pc] + Inv(sq);
    }

  pc = BOARD[to];
  list0[nlist  ] = aikpp[15+pc] + to;
  list1[nlist++] = aikpp[15-pc] + Inv(to);
#else
      // dst comes last
  if (ptree->listsuf4sq[to] != nlist-1)
    listswap(ptree, nlist-1, ptree->listsuf4sq[to]);
#endif
  pc = BOARD[to];

  diff = doapc( ptree, pc, to, list0, list1, nlist );
  nlist -= 1;

  if ( from >= nsquare )
    {
      //unsigned int hand;
      //int hand_index;

      pc   = From2Drop(from);
#if 0
      hand = turn ? HAND_B : HAND_W;

      switch ( pc )
        {
        case pawn:   hand_index = I2HandPawn(hand);   break;
        case lance:  hand_index = I2HandLance(hand);  break;
        case knight: hand_index = I2HandKnight(hand); break;
        case silver: hand_index = I2HandSilver(hand); break;
        case gold:   hand_index = I2HandGold(hand);   break;
        case bishop: hand_index = I2HandBishop(hand); break;
        default:     hand_index = I2HandRook(hand);   break;
        }
#endif

        // add capt
      //diff += doacapt( ptree, pc, turn, hand_index, list0, list1, nlist );
      diff += doacapt( ptree, pc, 1-turn, list0, list1, curhand, nlist );

        // incr capt
      list0[ 2*(pc-1) + 1 - turn ] += 1;
      list1[ 2*(pc-1) + turn     ] += 1;
      //hand_index                   += 1;
      curhand[ 2*(pc-1) + 1 - turn ] += 1;

        // sub capt+1
      //diff -= doacapt( ptree, pc, turn, hand_index, list0, list1, nlist );
      diff -= doacapt( ptree, pc, 1-turn, list0, list1, curhand, nlist );

        // restore capt
      list0[2*(pc-1) + 1 - turn]--;
      list1[2*(pc-1) +     turn]--;
      curhand[2*(pc-1) + 1 - turn]--;
    }
  else {  // slide case
    int dst0bak, dst1bak, capnoprom;
    int pc_cap = UToCap(MOVE_LAST);
    capnoprom     = pc_cap & ~promote;
    if ( pc_cap )     // tgt exists
      {
        //unsigned int hand;
        //int hand_index;

        pc     = capnoprom;
        //hand   = turn ? HAND_B : HAND_W;
        pc_cap = turn ? -pc_cap : pc_cap;
        diff  += turn ?  p_value_ex[15+pc_cap] * FV_SCALE
                      : -p_value_ex[15+pc_cap] * FV_SCALE;
#if 0
        switch ( pc )
          {
          case pawn:   hand_index = I2HandPawn(hand);   break;
          case lance:  hand_index = I2HandLance(hand);  break;
          case knight: hand_index = I2HandKnight(hand); break;
          case silver: hand_index = I2HandSilver(hand); break;
          case gold:   hand_index = I2HandGold(hand);   break;
          case bishop: hand_index = I2HandBishop(hand); break;
          default:     hand_index = I2HandRook(hand);   break;
          }
#endif
          // add capt
        //diff += doacapt( ptree, pc, turn, hand_index, list0, list1, nlist );
        diff += doacapt( ptree, pc, 1-turn, list0, list1, curhand, nlist );
        
          // decr capt
        list0[ 2*(pc-1) + 1 - turn ] -= 1;
        list1[ 2*(pc-1) + turn     ] -= 1;
        //hand_index                   -= 1;
        curhand[ 2*(pc-1) + 1 - turn ] -= 1;
        
          // sub capt-1
        //diff -= doacapt( ptree, pc, turn, hand_index, list0, list1, nlist );
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

    if (!pc_cap) {
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
    if (pc_cap) {
      list0[2*(capnoprom-1) + 1 - turn]++;
      list1[2*(capnoprom-1) +     turn]++;
      curhand[2*(capnoprom-1) + 1 - turn]++;
    }
  }
  
  diff += turn ? ptree->save_eval[ply-1] : - ptree->save_eval[ply-1];

  *pscore = diff;

  return 1;
}


static int CONV
make_list( const tree_t * restrict ptree, int * restrict pscore,
           int list0[52], int list1[52] )
{
  //bitboard_t bb;
  //int list2[34];
  int /*nlist, n2, i,*/ sq, score, sq_bk0, sq_wk0, sq_bk1, sq_wk1;

  //nlist  = 14;
  score  = 0;
  sq_bk0 = SQ_BKING;
  sq_wk0 = SQ_WKING;
  sq_bk1 = Inv(SQ_WKING);
  sq_wk1 = Inv(SQ_BKING);

  score += kkp[sq_bk0][sq_wk0][ kkp_hand_pawn   + I2HandPawn(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_lance  + I2HandLance(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_knight + I2HandKnight(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_silver + I2HandSilver(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_gold   + I2HandGold(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_bishop + I2HandBishop(HAND_B) ];
  score += kkp[sq_bk0][sq_wk0][ kkp_hand_rook   + I2HandRook(HAND_B) ];

  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_pawn   + I2HandPawn(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_lance  + I2HandLance(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_knight + I2HandKnight(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_silver + I2HandSilver(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_gold   + I2HandGold(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_bishop + I2HandBishop(HAND_W) ];
  score -= kkp[sq_bk1][sq_wk1][ kkp_hand_rook   + I2HandRook(HAND_W) ];

#if 0
  n2 = 0;
  bb = BB_BPAWN;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_pawn + sq;
    list2[n2]    = e_pawn + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_pawn + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WPAWN;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_pawn + sq;
    list2[n2]    = f_pawn + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_pawn + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }

  n2 = 0;
  bb = BB_BLANCE;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_lance + sq;
    list2[n2]    = e_lance + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_lance + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WLANCE;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_lance + sq;
    list2[n2]    = f_lance + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_lance + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BKNIGHT;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_knight + sq;
    list2[n2]    = e_knight + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_knight + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WKNIGHT;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_knight + sq;
    list2[n2]    = f_knight + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_knight + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BSILVER;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_silver + sq;
    list2[n2]    = e_silver + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_silver + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WSILVER;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_silver + sq;
    list2[n2]    = f_silver + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_silver + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BTGOLD;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_gold + sq;
    list2[n2]    = e_gold + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_gold + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WTGOLD;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_gold + sq;
    list2[n2]    = f_gold + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_gold + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BBISHOP;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_bishop + sq;
    list2[n2]    = e_bishop + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_bishop + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WBISHOP;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_bishop + sq;
    list2[n2]    = f_bishop + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_bishop + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BHORSE;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_horse + sq;
    list2[n2]    = e_horse + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_horse + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WHORSE;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_horse + sq;
    list2[n2]    = f_horse + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_horse + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BROOK;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_rook + sq;
    list2[n2]    = e_rook + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_rook + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WROOK;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_rook + sq;
    list2[n2]    = f_rook + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_rook + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BDRAGON;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_dragon + sq;
    list2[n2]    = e_dragon + Inv(sq);
    score += kkp[sq_bk0][sq_wk0][ kkp_dragon + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WDRAGON;
  while ( BBTest(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_dragon + sq;
    list2[n2]    = f_dragon + Inv(sq);
    score -= kkp[sq_bk1][sq_wk1][ kkp_dragon + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }
#else
  for(sq=0; sq<=80; sq++) {
   int pc = BOARD[sq];
   if (pc&7) {  // omit kings and empty
     if (pc>0)
       score += kkp[sq_bk0][sq_wk0][ kkpbase[pc] + sq ];
     if (pc<0)
       score -= kkp[sq_bk1][sq_wk1][ kkpbase[-pc] + Inv(sq) ];
   }
  }

#endif

  //assert( nlist <= 52 );

  *pscore = score;
  //return nlist;
  return 0;
}


static int CONV doapc( const tree_t * restrict ptree, int pc, int sq,
                       const int list0[52], const int list1[52], int nlist )
{
  int i, sum;
  int index_b = aikpp[15+pc] + sq;
  int index_w = aikpp[15-pc] + Inv(sq);
  int sq_bk   = SQ_BKING;
  int sq_wk   = Inv(SQ_WKING);
  
  sum = 0;
#if 0
  for( i = 0; i < 14; i++ )
    {
      sum += PcPcOnSq( sq_bk, index_b, list0[i] );
      sum -= PcPcOnSq( sq_wk, index_w, list1[i] );
    }
 #endif 
  //for( i = 14; i < nlist; i++ )
  for( i =  0; i < nlist; i++ )
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
//doacapt( const tree_t * restrict ptree, int pc, int turn, int hand_index,
//         const int list0[52], const int list1[52], int nlist )
doacapt( const tree_t * restrict ptree, int pc, int turn,
         const int list0[52], const int list1[52], int *curhand, int nlist )
{
  int i, sum, sq_bk, sq_wk, /*index_b, index_w,*/ suf0b, suf0w;
  
  //index_b = 2*(pc-1) + 1 - turn;
  //index_w = 2*(pc-1) + turn;
  suf0b     = list0[2*(pc-1) + turn];
  suf0w     = list1[2*(pc-1) + 1 - turn];
  sq_bk   = SQ_BKING;
  sq_wk   = Inv(SQ_WKING);
  
  sum = 0;
  //for( i = 14; i < nlist; i++ )
  for( i =  0; i < nlist; i++ )
    {
      //sum += PcPcOnSq( sq_bk, list0[i], list0[index_b] );
      //sum -= PcPcOnSq( sq_wk, list1[i], list1[index_w] );
      sum += PcPcOnSq( sq_bk, list0[i], suf0b );
      sum -= PcPcOnSq( sq_wk, list1[i], suf0w );
    }
#if 0
  for( i = 0; i <= 2*(pc-1); i++ )
    {
      sum += PcPcOnSq( sq_bk, list0[index_b], list0[i] );
      sum -= PcPcOnSq( sq_wk, list1[index_w], list1[i] );
    }

  for( i += 1; i < 14; i++ )
    {
      sum += PcPcOnSq( sq_bk, list0[i], list0[index_b] );
      sum -= PcPcOnSq( sq_wk, list1[i], list1[index_w] );
    }
#endif
  //if ( turn )
  if ( ! turn )
    {
      //sum += PcPcOnSq( sq_bk, list0[index_w], list0[index_b] );
      //sum -= PcPcOnSq( sq_wk, list1[index_w], list1[index_w] );
      sq_bk = SQ_BKING;
      sq_wk = SQ_WKING;
      //sum  += kkp[sq_bk][sq_wk][ aikkp_hand[pc] + hand_index ];
      sum  += kkp[sq_bk][sq_wk][ aikkp_hand[pc] + curhand[2*(pc-1)] ];
    }
  else {
    //sum += PcPcOnSq( sq_bk, list0[index_b], list0[index_b] );
    //sum -= PcPcOnSq( sq_wk, list1[index_b], list1[index_w] );
    sq_bk = Inv(SQ_WKING);
    sq_wk = Inv(SQ_BKING);
    //sum  -= kkp[sq_bk][sq_wk][ aikkp_hand[pc] + hand_index ];
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
