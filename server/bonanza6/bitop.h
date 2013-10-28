#ifndef BITOP_H
#define BITOP_H

#define BITBRD64

#ifdef HAVE_AVX
#include <immintrin.h>
#endif

//#define SSE_1STLASTONE

#define BBToU(b)            ( (b).p[0] | (b).p[1] | (b).p[2] )
#define BBToUShift(b)       ( (b).p[0]<<2 | (b).p[1]<<1 | (b).p[2])
#define PopuCount(bb)       popu_count012( bb.p[0], bb.p[1], bb.p[2] )

//#if defined(HAVE_SSE2) || defined(HAVE_SSE4)
#ifdef SSE_1STLASTONE
#define FirstOne(bb)        first_one_mm( bb )
#define LastOne(bb)         last_one_mm( bb )
#else
#define FirstOne(bb)        first_one012( bb.p[0], bb.p[1], bb.p[2] )
#define LastOne(bb)         last_one210( bb.p[2], bb.p[1], bb.p[0] )
#endif

#define BBCmp(b1,b2)        ( (b1).p[0] != (b2).p[0]                    \
                                || (b1).p[1] != (b2).p[1]               \
                                || (b1).p[2] != (b2).p[2] )
#define BBContractShift(b1,b2) ( ( (b1).p[0] & (b2).p[0] ) << 2         \
                               | ( (b1).p[1] & (b2).p[1] ) << 1         \
                               | ( (b1).p[2] & (b2).p[2] ) )

#if defined(HAVE_SSE2) || defined(HAVE_SSE4)

#if defined(HAVE_SSE4)

#  include <smmintrin.h>

#  define BBContract(b1,b2) ( ! _mm_testz_si128( (b1).m, (b2).m ) )
#  define BBTest(b)         ( ! _mm_testz_si128( (b).m, _mm_set1_epi8(~0) ) )

#else /* no SSE4 */

#  include <emmintrin.h>
#if 0
#  define BBContract(b1,b2) ( ( (b1).p[0] & (b2).p[0] )                 \
                            | ( (b1).p[1] & (b2).p[1] )                 \
                            | ( (b1).p[2] & (b2).p[2] ) )
#  define BBTest(b)         ( (b).p[0] | (b).p[1] | (b).p[2] )
#else
#  define MMZERO       _mm_setzero_si128()
#  define MMTest(mm) \
    (_mm_movemask_epi8( _mm_cmpeq_epi32( mm, MMZERO ) ) - 0xffff)
#  define BBTest(bb)   MMTest((bb).m)
#  define BBContract(bb1,bb2)  MMTest( _mm_and_si128( (bb1).m, (bb2).m ) )
#endif

#endif /* HAVE_SSE4 */

#define BBNot(b,b1)         (b).m = _mm_andnot_si128( (b1).m,                 \
                                                      _mm_set1_epi8(~0) )
#define BBIni(b)            (b).m = _mm_setzero_si128()
#define BBAnd(b,b1,b2)      (b).m = _mm_and_si128( (b1).m, (b2).m )
#define BBOr(b,b1,b2)       (b).m = _mm_or_si128( (b1).m, (b2).m )
#define BBXor(b,b1,b2)      (b).m = _mm_xor_si128( (b1).m, (b2).m )
#define BBAndOr(b,b1,b2)    (b).m = _mm_or_si128( (b).m,                      \
                                    _mm_and_si128( (b1).m, (b2).m ) )
#define BBNotAnd(b,b1,b2)   (b).m = _mm_andnot_si128( (b2).m, (b1).m )
#define Xor(sq,b)           (b).m = _mm_xor_si128( (b).m, abb_mask[sq].m )
#define XorFile(sq,b)       (b).m = _mm_xor_si128( (b).m, abb_mask_rl90[sq].m )
#define XorDiag1(sq,b)      (b).m = _mm_xor_si128( (b).m, abb_mask_rr45[sq].m )
#define XorDiag2(sq,b)      (b).m = _mm_xor_si128( (b).m, abb_mask_rl45[sq].m )
#define SetClear(b)         (b).m = _mm_xor_si128( (b).m, bb_set_clear.m )
#define SetClearFile(sq1,sq2,b)  (b).m= _mm_xor_si128( (b).m,                 \
                                        _mm_or_si128( abb_mask_rl90[sq1].m,   \
                                                      abb_mask_rl90[sq2].m ) )
#define SetClearDiag1(sq1,sq2,b) (b).m= _mm_xor_si128( (b).m,                 \
                                        _mm_or_si128( abb_mask_rr45[sq1].m,   \
                                                      abb_mask_rr45[sq2].m ) )
#define SetClearDiag2(sq1,sq2,b) (b).m= _mm_xor_si128( (b).m,                 \
                                        _mm_or_si128( abb_mask_rl45[sq1].m,   \
                                                      abb_mask_rl45[sq2].m ) )

typedef union {
  unsigned int p[4];
  __m128i m;
} bitboard_t;

#else /* NO SSE2 */

#define BBTest(b)           ( (b).p[0] | (b).p[1] | (b).p[2] )
#define BBIni(b)            (b).p[0] = (b).p[1] = (b).p[2] = 0
#define BBNot(b,b1)         (b).p[0] = ~(b1).p[0],                      \
                            (b).p[1] = ~(b1).p[1],                      \
                            (b).p[2] = ~(b1).p[2]
#define BBAnd(b,b1,b2)      (b).p[0] = (b1).p[0] & (b2).p[0],           \
                            (b).p[1] = (b1).p[1] & (b2).p[1],           \
                            (b).p[2] = (b1).p[2] & (b2).p[2]
#define BBOr(b,b1,b2)       (b).p[0] = (b1).p[0] | (b2).p[0],           \
                            (b).p[1] = (b1).p[1] | (b2).p[1],           \
                            (b).p[2] = (b1).p[2] | (b2).p[2]
#define BBXor(b,b1,b2)      (b).p[0] = (b1).p[0] ^ (b2).p[0],           \
                            (b).p[1] = (b1).p[1] ^ (b2).p[1],           \
                            (b).p[2] = (b1).p[2] ^ (b2).p[2]
#define BBAndOr(b,b1,b2)    (b).p[0] |= (b1).p[0] & (b2).p[0],          \
                            (b).p[1] |= (b1).p[1] & (b2).p[1],          \
                            (b).p[2] |= (b1).p[2] & (b2).p[2]
#define BBNotAnd(b,b1,b2)   (b).p[0] = (b1).p[0] & ~(b2).p[0],          \
                            (b).p[1] = (b1).p[1] & ~(b2).p[1],          \
                            (b).p[2] = (b1).p[2] & ~(b2).p[2]
#define BBContract(b1,b2)   ( ( (b1).p[0] & (b2).p[0] )                 \
                            | ( (b1).p[1] & (b2).p[1] )                 \
                            | ( (b1).p[2] & (b2).p[2] ) )
#define Xor(sq,b)           (b).p[0] ^= abb_mask[sq].p[0],              \
                            (b).p[1] ^= abb_mask[sq].p[1],              \
                            (b).p[2] ^= abb_mask[sq].p[2]
#define XorFile(sq,b)       (b).p[0] ^= abb_mask_rl90[sq].p[0],         \
                            (b).p[1] ^= abb_mask_rl90[sq].p[1],         \
                            (b).p[2] ^= abb_mask_rl90[sq].p[2]
#define XorDiag1(sq,b)      (b).p[0] ^= abb_mask_rr45[sq].p[0],         \
                            (b).p[1] ^= abb_mask_rr45[sq].p[1],         \
                            (b).p[2] ^= abb_mask_rr45[sq].p[2]
#define XorDiag2(sq,b)      (b).p[0] ^= abb_mask_rl45[sq].p[0],         \
                            (b).p[1] ^= abb_mask_rl45[sq].p[1],         \
                            (b).p[2] ^= abb_mask_rl45[sq].p[2]
#define SetClear(b)         (b).p[0] ^= (bb_set_clear.p[0]),            \
                            (b).p[1] ^= (bb_set_clear.p[1]),            \
                            (b).p[2] ^= (bb_set_clear.p[2])
#define SetClearFile(sq1,sq2,b)                                         \
    (b).p[0] ^= ( abb_mask_rl90[sq1].p[0] | abb_mask_rl90[sq2].p[0] ),  \
    (b).p[1] ^= ( abb_mask_rl90[sq1].p[1] | abb_mask_rl90[sq2].p[1] ),  \
    (b).p[2] ^= ( abb_mask_rl90[sq1].p[2] | abb_mask_rl90[sq2].p[2] )
#define SetClearDiag1(sq1,sq2,b) \
    (b).p[0] ^= ( abb_mask_rr45[sq1].p[0] | abb_mask_rr45[sq2].p[0] ), \
    (b).p[1] ^= ( abb_mask_rr45[sq1].p[1] | abb_mask_rr45[sq2].p[1] ), \
    (b).p[2] ^= ( abb_mask_rr45[sq1].p[2] | abb_mask_rr45[sq2].p[2] )
#define SetClearDiag2(sq1,sq2,b) \
    (b).p[0] ^= ( abb_mask_rl45[sq1].p[0] | abb_mask_rl45[sq2].p[0] ), \
    (b).p[1] ^= ( abb_mask_rl45[sq1].p[1] | abb_mask_rl45[sq2].p[1] ), \
    (b).p[2] ^= ( abb_mask_rl45[sq1].p[2] | abb_mask_rl45[sq2].p[2] )

typedef struct { unsigned int p[3]; } bitboard_t;

#endif /* HAVE_SSE2 */

#if ! defined(BITBRD64)

#define IniOccupied()  do {  \
   BBIni( OCCUPIED_FILE );   \
   BBIni( OCCUPIED_DIAG1 );  \
   BBIni( OCCUPIED_DIAG2 );  \
 } while (0)

#define XorOccupied(sq)  do {        \
   XorFile( sq, OCCUPIED_FILE );     \
   XorDiag2( sq, OCCUPIED_DIAG2 );   \
   XorDiag1( sq, OCCUPIED_DIAG1 );   \
 } while (0)

#define SetClearOccupied(sq1,sq2)  do {        \
   SetClearFile( sq1, sq2, OCCUPIED_FILE );    \
   SetClearDiag1( sq1, sq2, OCCUPIED_DIAG1 );  \
   SetClearDiag2( sq1, sq2, OCCUPIED_DIAG2 );  \
 } while (0)

# elif defined(HAVE_AVX)

#define IniOccupied()  do {                  \
    ptree->posi.occ.m = _mm256_setzero_ps(); \
  } while(0)

#define XorOccupied(sq)  do {                             \
    ptree->posi.occ.m =                                   \
      _mm256_xor_ps(ptree->posi.occ.m, ao_bitmask[sq].m); \
  } while (0)

#define SetClearOccupied(sq1,sq2)  do {                     \
    ptree->posi.occ.m = _mm256_xor_ps(ptree->posi.occ.m,    \
      _mm256_xor_ps(ao_bitmask[sq1].m, ao_bitmask[sq2].m)); \
  } while (0)

#elif defined(HAVE_SSE2)

#define IniOccupied()  do {                                   \
    ptree->posi.occ.m[0] = ptree->posi.occ.m[1] = _mm_setzero_si128(); \
  } while (0)

#define XorOccupied(sq)  do {                                     \
    ptree->posi.occ.m[0] =  \
      _mm_xor_si128( ptree->posi.occ.m[0], ao_bitmask[sq].m[0]);  \
    ptree->posi.occ.m[1] =  \
      _mm_xor_si128( ptree->posi.occ.m[1], ao_bitmask[sq].m[1]);  \
  } while (0)

#define SetClearOccupied(sq1,sq2)  do {                           \
    ptree->posi.occ.m[0] = _mm_xor_si128( ptree->posi.occ.m[0],   \
      _mm_or_si128( ao_bitmask[sq1].m[0], ao_bitmask[sq2].m[0])); \
    ptree->posi.occ.m[1] = _mm_xor_si128( ptree->posi.occ.m[1],   \
      _mm_or_si128( ao_bitmask[sq1].m[1], ao_bitmask[sq2].m[1])); \
  } while (0)

#else

#define IniOccupied()  do {                            \
    ptree->posi.occ.x[0] = ptree->posi.occ.x[1] =      \
    ptree->posi.occ.x[2] = ptree->posi.occ.x[3] = 0UL; \
  } while (0)

#define XorOccupied(sq)  do {                    \
    ptree->posi.occ.x[0] ^= ao_bitmask[sq].x[0]; \
    ptree->posi.occ.x[1] ^= ao_bitmask[sq].x[1]; \
    ptree->posi.occ.x[2] ^= ao_bitmask[sq].x[2]; \
    ptree->posi.occ.x[3] ^= ao_bitmask[sq].x[3]; \
  } while (0)

#define SetClearOccupied(sq1,sq2)  do {                                   \
    ptree->posi.occ.x[0] ^= ( ao_bitmask[sq1].x[0] | ao_bitmask[sq2].x[0] ); \
    ptree->posi.occ.x[1] ^= ( ao_bitmask[sq1].x[1] | ao_bitmask[sq2].x[1] ); \
    ptree->posi.occ.x[2] ^= ( ao_bitmask[sq1].x[2] | ao_bitmask[sq2].x[2] ); \
    ptree->posi.occ.x[3] ^= ( ao_bitmask[sq1].x[3] | ao_bitmask[sq2].x[3] ); \
  } while (0)

#endif

#endif /* BITOP_H */
