
// compile w/ : g++ bbatk.cpp -c -msse2

#include <stdio.h>
#include <string.h>
#include "shogi.h"

/*#define DBG_BBATK*/

#define MAXLEN 17

bitboard_t abb_attacks[4][128/*81*/][128];
occupiedC ao_bitmask[81];

const int ai_shift[4][81] = {
  // HORIZ
 { 0,  0,  0,  0,  0,  0,  0,  0,  0,
   7,  7,  7,  7,  7,  7,  7,  7,  7,
  14, 14, 14, 14, 14, 14, 14, 14, 14,
  21, 21, 21, 21, 21, 21, 21, 21, 21,
  28, 28, 28, 28, 28, 28, 28, 28, 28,
  35, 35, 35, 35, 35, 35, 35, 35, 35,
  42, 42, 42, 42, 42, 42, 42, 42, 42,
  49, 49, 49, 49, 49, 49, 49, 49, 49,
  56, 56, 56, 56, 56, 56, 56, 56, 56 },

  // VERT 
 { 0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   0,  7, 14, 21, 28, 35, 42, 49, 56 },

  // DIAG1
 { 0,  0,  0,  1,  3,  6, 10, 15, 21,
   0,  0,  1,  3,  6, 10, 15, 21, 28,
   0,  1,  3,  6, 10, 15, 21, 28, 34,
   1,  3,  6, 10, 15, 21, 28, 34, 39,
   3,  6, 10, 15, 21, 28, 34, 39, 43,
   6, 10, 15, 21, 28, 34, 39, 43, 46,
  10, 15, 21, 28, 34, 39, 43, 46, 48,
  15, 21, 28, 34, 39, 43, 46, 48,  0,
  21, 28, 34, 39, 43, 46, 48,  0,  0 },

  // DIAG2
 {21, 15, 10,  6,  3,  1,  0,  0,  0,
  28, 21, 15, 10,  6,  3,  1,  0,  0,
  34, 28, 21, 15, 10,  6,  3,  1,  0,
  39, 34, 28, 21, 15, 10,  6,  3,  1,
  43, 39, 34, 28, 21, 15, 10,  6,  3,
  46, 43, 39, 34, 28, 21, 15, 10,  6,
  48, 46, 43, 39, 34, 28, 21, 15, 10,
   0, 48, 46, 43, 39, 34, 28, 21, 15,
   0,  0, 48, 46, 43, 39, 34, 28, 21 },
};


static const int ai_bitpos[4][81] = {
  // HORIZ
 {-1,  0,  1,  2,  3,  4,  5,  6, -1,
  -1,  7,  8,  9, 10, 11, 12, 13, -1,
  -1, 14, 15, 16, 17, 18, 19, 20, -1,
  -1, 21, 22, 23, 24, 25, 26, 27, -1,
  -1, 28, 29, 30, 31, 32, 33, 34, -1,
  -1, 35, 36, 37, 38, 39, 40, 41, -1,
  -1, 42, 43, 44, 45, 46, 47, 48, -1,
  -1, 49, 50, 51, 52, 53, 54, 55, -1,
  -1, 56, 57, 58, 59, 60, 61, 62, -1 },

  // VERT
 {-1, -1, -1, -1, -1, -1, -1, -1, -1,
   0,  7, 14, 21, 28, 35, 42, 49, 56,
   1,  8, 15, 22, 29, 36, 43, 50, 57,
   2,  9, 16, 23, 30, 37, 44, 51, 58,
   3, 10, 17, 24, 31, 38, 45, 52, 59,
   4, 11, 18, 25, 32, 39, 46, 53, 60,
   5, 12, 19, 26, 33, 40, 47, 54, 61,
   6, 13, 20, 27, 34, 41, 48, 55, 62,
  -1, -1, -1, -1, -1, -1, -1, -1, -1 },

  // DIAG1
 {-1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1,  0,  2,  5,  9, 14, 20, 27, -1,
  -1,  1,  4,  8, 13, 19, 26, 33, -1,
  -1,  3,  7, 12, 18, 25, 32, 38, -1,
  -1,  6, 11, 17, 24, 31, 37, 42, -1,
  -1, 10, 16, 23, 30, 36, 41, 45, -1,
  -1, 15, 22, 29, 35, 40, 44, 47, -1,
  -1, 21, 28, 34, 39, 43, 46, 48, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1 },

  // DIAG2
 {-1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 21, 15, 10,  6,  3,  1,  0, -1,
  -1, 28, 22, 16, 11,  7,  4,  2, -1,
  -1, 34, 29, 23, 17, 12,  8,  5, -1,
  -1, 39, 35, 30, 24, 18, 13,  9, -1,
  -1, 43, 40, 36, 31, 25, 19, 14, -1,
  -1, 46, 44, 41, 37, 32, 26, 20, -1,
  -1, 48, 47, 45, 42, 38, 33, 27, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1 }
};

static int locsR[MAXLEN][9] = {
  { A9, B9, C9, D9, E9, F9, G9, H9, I9 },
  { A8, B8, C8, D8, E8, F8, G8, H8, I8 },
  { A7, B7, C7, D7, E7, F7, G7, H7, I7 },
  { A6, B6, C6, D6, E6, F6, G6, H6, I6 },
  { A5, B5, C5, D5, E5, F5, G5, H5, I5 },
  { A4, B4, C4, D4, E4, F4, G4, H4, I4 },
  { A3, B3, C3, D3, E3, F3, G3, H3, I3 },
  { A2, B2, C2, D2, E2, F2, G2, H2, I2 },
  { A1, B1, C1, D1, E1, F1, G1, H1, I1 } };

static int locsF[MAXLEN][9] = {
  { A9, A8, A7, A6, A5, A4, A3, A2, A1 },
  { B9, B8, B7, B6, B5, B4, B3, B2, B1 },
  { C9, C8, C7, C6, C5, C4, C3, C2, C1 },
  { D9, D8, D7, D6, D5, D4, D3, D2, D1 },
  { E9, E8, E7, E6, E5, E4, E3, E2, E1 },
  { F9, F8, F7, F6, F5, F4, F3, F2, F1 },
  { G9, G8, G7, G6, G5, G4, G3, G2, G1 },
  { H9, H8, H7, H6, H5, H4, H3, H2, H1 },
  { I9, I8, I7, I6, I5, I4, I3, I2, I1 } };

static int locsD1[MAXLEN][9] = {
  { A9 },
  { A8, B9 },
  { A7, B8, C9 },
  { A6, B7, C8, D9 },
  { A5, B6, C7, D8, E9 },
  { A4, B5, C6, D7, E8, F9 },
  { A3, B4, C5, D6, E7, F8, G9 },
  { A2, B3, C4, D5, E6, F7, G8, H9 },
  { A1, B2, C3, D4, E5, F6, G7, H8, I9 },
  { B1, C2, D3, E4, F5, G6, H7, I8 },
  { C1, D2, E3, F4, G5, H6, I7 },
  { D1, E2, F3, G4, H5, I6 },
  { E1, F2, G3, H4, I5 },
  { F1, G2, H3, I4 },
  { G1, H2, I3 },
  { H1, I2 },
  { I1 } };

static int locsD2[MAXLEN][9] = {
  { A1 },
  { A2, B1 },
  { A3, B2, C1 },
  { A4, B3, C2, D1 },
  { A5, B4, C3, D2, E1 },
  { A6, B5, C4, D3, E2, F1 },
  { A7, B6, C5, D4, E3, F2, G1 },
  { A8, B7, C6, D5, E4, F3, G2, H1 },
  { A9, B8, C7, D6, E5, F4, G3, H2, I1 },
  { B9, C8, D7, E6, F5, G4, H3, I2 },
  { C9, D8, E7, F6, G5, H4, I3 },
  { D9, E8, F7, G6, H5, I4 },
  { E9, F8, G7, H6, I5 },
  { F9, G8, H7, I6 },
  { G9, H8, I7 },
  { H9, I8 },
  { I9 } };


typedef struct
{
  int listlen;    // 0/90 : 9   45 : 17
  int loc_width[MAXLEN];  // 0/90 : all 9   45 : 1,2,..,8,9,8,..,1
  int locs[MAXLEN][9];  // squares on the line
} attack_data_info_t;

static void CONV ini_attack_data( void );
static void CONV set_attack_bb( attack_data_info_t adata[], int direc,
                                int line, int loc_pos, int pcs );

void CONV
ini_bitboards( void )
{
  int d, sq;

  memset( &ao_bitmask, 0, sizeof(ao_bitmask) );

  for ( d = 0; d < direc_idx_max; d++ )
    for ( sq = 0; sq < nsquare; sq++ )
      if ( ai_bitpos[d][sq] >= 0 )
        ao_bitmask[sq].x[d] = (1ULL << ai_bitpos[d][sq]);

  ini_attack_data();
}


static void CONV
ini_attack_data( void )
{
  attack_data_info_t adata[direc_idx_max]; // 縦横斜め*2ごとに確保
  attack_data_info_t rank, file, diag1, diag2;
  int i, j, k, direc, line, loc_pos, pcs;

  memset(&rank, 0, sizeof(rank));
  memset(&file, 0, sizeof(file));
  memset(&diag1, 0, sizeof(diag1));
  memset(&diag2, 0, sizeof(diag2));
  rank.listlen =
  file.listlen = 9;
  diag1.listlen =
  diag2.listlen = MAXLEN;

  for ( k = 0; k < 9; k++ )
    {
      rank.loc_width[k] =
      file.loc_width[k] = 9;
      diag1.loc_width[k] =
      diag2.loc_width[k] = k+1;
    }
  for ( k = 9; k < MAXLEN; k++ )
    {
      diag1.loc_width[k] =
      diag2.loc_width[k] = 17-k;
    }

  for ( i = 0; i < MAXLEN; i++ )
    for ( j = 0; j < 9; j++ )
      {
        rank.locs[i][j] = locsR[i][j];
        file.locs[i][j] = locsF[i][j];
        diag1.locs[i][j] = locsD1[i][j];
        diag2.locs[i][j] = locsD2[i][j];
      }

  adata[0] = rank;
  adata[1] = file;
  adata[2] = diag1;
  adata[3] = diag2;

  memset( &abb_attacks, 0, sizeof(abb_attacks) );

  for ( direc = direc_idx_horiz; direc < direc_idx_max; direc++ )
    for ( line = 0; line < adata[direc].listlen; line++ )
      for ( loc_pos = 0; loc_pos < adata[direc].loc_width[line]; loc_pos++ )
        for ( pcs = 0; pcs < 128; pcs++ ) // for each bit patterns
          set_attack_bb( adata, direc, line, loc_pos, pcs );
}


static void CONV
set_attack_bb( attack_data_info_t *adata, int direc, int line,
               int loc_pos, int pcs )
{
  int wid, sq, vec;
  bitboard_t bb;

  wid = adata[direc].loc_width[line];
  sq  = adata[direc].locs[line][loc_pos];

  BBIni( bb );

  //**** go both +/- dir's from sq
  for ( vec = -1; vec <= 1; vec += 2 ) //vec=-1/+1
    {
      int new_loc_pos = loc_pos + vec;

      //**** go until either edge or blocked 
      while ( 0 <= new_loc_pos && new_loc_pos < wid )
        {
          int tmp_sq = adata[direc].locs[line][new_loc_pos];

          Xor( tmp_sq, bb );
          if ( new_loc_pos > 0 && ( (1 << (new_loc_pos-1)) & pcs ) ) break;
          new_loc_pos += vec;
        }
    }

  abb_attacks[direc][sq][pcs] = bb;
}

#ifdef DBG_BBATK

int blocked[81] = {
  1,0,0,1,0,0,1,0,0,
  1,0,0,0,0,0,1,0,0,
  0,0,0,1,0,0,0,0,0,
  0,1,0,1,0,0,1,0,0,
  1,1,0,0,0,0,0,0,0,
  0,0,1,0,0,0,1,0,0,
  0,0,0,0,0,0,1,0,0,
  1,0,0,1,0,0,0,0,0,
  0,0,0,0,0,0,1,0,1
};

static bitboard_t
bb_set_mask( int sq )
{
  bitboard_t bb;
  
  BBIni(bb);
  if      ( sq > 53 ) { bb.p[2] = 1U << ( 80 - sq ); }
  else if ( sq > 26 ) { bb.p[1] = 1U << ( 53 - sq ); }
  else                { bb.p[0] = 1U << ( 26 - sq ); }
  
  return bb;
}


static void
set_occ( occupiedC* occ, int* a )
{
  int i;

  memset(occ, 0, sizeof(*occ));

  for ( i = 0; i < nsquare; i++ )
    if ( a[i] )
      {
        occ->x[0] |= ao_bitmask[i].x[0];
        occ->x[1] |= ao_bitmask[i].x[1];
        occ->x[2] |= ao_bitmask[i].x[2];
        occ->x[3] |= ao_bitmask[i].x[3];
      }
}


static int
is_setbit( const bitboard_t * restrict bb, int sq )
{
  bitboard_t bb2 = bb_set_mask(sq);
  int i;
 
  for ( i = 0; i <= 2; i++ )
    if ( bb->p[i] & bb2.p[i] )
      return 1;
  return 0;
}


static void
disp_bb( const bitboard_t * restrict bb )
{
  int i;

  for ( i = 0; i < nsquare; i++ )
    {
      printf( "%d", is_setbit( bb, i ) );
      if ( i%9 == 8 ) putchar( '\n' );
    }
}


int
main( void )
{
  occupiedC occ;
  bitboard_t bb;
  int sqa[6] = {C8, A4, E2, H9, F3, G6};
  int i, q;

  for ( q = 0; q < nsquare; i++ )
    abb_mask[q] = bb_set_mask(q);
  ini_bitboards();    // initilize tables
  set_occ( &occ, blocked ); // set occupied data

  for ( i = 0; i < 6; i++ )
    {
      int sq = sqa[i];

      printf("---- (%d,%d)\n", 9-sq%9, sq/9+1);

      bb = AttackRankE(occ, sq);
      printf("rank: %d\n", ai_shift[0][sq]);
      disp_bb(&bb);
   
      bb = AttackFileE(occ, sq);
      printf("file: %d\n", ai_shift[1][sq]);
      disp_bb(&bb);

      bb = AttackDiag1E(occ, sq);
      printf("diag1: %d\n", ai_shift[2][sq]);
      disp_bb(&bb);

      bb = AttackDiag2E(occ, sq);
      printf("diag2: %d\n", ai_shift[3][sq]);
      disp_bb(&bb);
    }
}

#endif
