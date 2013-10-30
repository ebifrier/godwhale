// $Id: inline.h,v 1.5 2012-04-22 21:31:42 eiki Exp $

#ifndef INLINE_H
#define INLINE_H

#include <assert.h>
#include <limits.h>

static inline int CONV
ehash_probe( uint64_t current_key, unsigned int hand_b,
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


static inline int CONV
evaluate( tree_t * restrict ptree, int ply, int turn )
{
  int score;

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

  return evaluate_body( ptree, ply, turn );
}

#endif /* INLINE_H */
