// $Id: makemove.c,v 1.3 2012/03/22 06:09:30 eiki Exp $

#include <limits.h>
#include <assert.h>
#include <string.h>
#include "shogi.h"

#ifdef CLUSTER_PARALLEL
#include "../if_bonanza.h"
#endif

#define DropB( PIECE, piece )  Xor( to, BB_B ## PIECE );                    \
                               HASH_KEY    ^= ( b_ ## piece ## _rand )[to]; \
                               HAND_B      -= flag_hand_ ## piece;          \
                               BOARD[to]  = piece

#define DropW( PIECE, piece )  Xor( to, BB_W ## PIECE );                    \
                               HASH_KEY    ^= ( w_ ## piece ## _rand )[to]; \
                               HAND_W      -= flag_hand_ ## piece;          \
                               BOARD[to]  = - piece

#define CapB( PIECE, piece, pro_piece )                   \
          Xor( to, BB_B ## PIECE );                       \
          HASH_KEY  ^= ( b_ ## pro_piece ## _rand )[to];  \
          HAND_W    += flag_hand_ ## piece;               \
          MATERIAL  -= MT_CAP_ ## PIECE

#define CapW( PIECE, piece, pro_piece )                   \
          Xor( to, BB_W ## PIECE );                       \
          HASH_KEY  ^= ( w_ ## pro_piece ## _rand )[to];  \
          HAND_B    += flag_hand_ ## piece;               \
          MATERIAL  += MT_CAP_ ## PIECE

#define NocapProB( PIECE, PRO_PIECE, piece, pro_piece )       \
          Xor( from, BB_B ## PIECE );                         \
          Xor( to,   BB_B ## PRO_PIECE );                     \
          HASH_KEY    ^= ( b_ ## pro_piece ## _rand )[to]     \
                       ^ ( b_ ## piece     ## _rand )[from];  \
          MATERIAL    += MT_PRO_ ## PIECE;                    \
          BOARD[to] = pro_piece

#define NocapProW( PIECE, PRO_PIECE, piece, pro_piece )       \
          Xor( from, BB_W ## PIECE );                         \
          Xor( to,   BB_W ## PRO_PIECE );                     \
          HASH_KEY    ^= ( w_ ## pro_piece ## _rand )[to]     \
                       ^ ( w_ ## piece     ## _rand )[from];  \
          MATERIAL    -= MT_PRO_ ## PIECE;                    \
          BOARD[to]  = - pro_piece
 
#define NocapNoproB( PIECE, piece )                          \
          SetClear( BB_B ## PIECE );                         \
          HASH_KEY    ^= ( b_ ## piece ## _rand )[to]        \
                       ^ ( b_ ## piece ## _rand )[from];     \
          BOARD[to] = piece

#define NocapNoproW( PIECE, piece )                          \
          SetClear( BB_W ## PIECE );                         \
          HASH_KEY    ^= ( w_ ## piece ## _rand )[to]        \
                       ^ ( w_ ## piece ## _rand )[from];     \
          BOARD[to] = - piece

#define FUNC_NAME_B make_move_b
#define FUNC_NAME_W make_move_w

#include "makemove.h"

#undef FUNC_NAME_B
#undef FUNC_NAME_W
#undef DropB
#undef DropW
#undef CapB
#undef CapW
#undef NocapProB
#undef NocapProW
#undef NocapNoproB
#undef NocapNoproW


#define DropB( PIECE, piece )  Xor( to, BB_B ## PIECE );                    \
                               HAND_B      -= flag_hand_ ## piece;          \
                               BOARD[to]  = piece

#define DropW( PIECE, piece )  Xor( to, BB_W ## PIECE );                    \
                               HAND_W      -= flag_hand_ ## piece;          \
                               BOARD[to]  = - piece

#define CapB( PIECE, piece, pro_piece )                   \
          Xor( to, BB_B ## PIECE );                       \
          HAND_W    += flag_hand_ ## piece;               \
          MATERIAL  -= MT_CAP_ ## PIECE

#define CapW( PIECE, piece, pro_piece )                   \
          Xor( to, BB_W ## PIECE );                       \
          HAND_B    += flag_hand_ ## piece;               \
          MATERIAL  += MT_CAP_ ## PIECE

#define NocapProB( PIECE, PRO_PIECE, piece, pro_piece )       \
          Xor( from, BB_B ## PIECE );                         \
          Xor( to,   BB_B ## PRO_PIECE );                     \
          MATERIAL    += MT_PRO_ ## PIECE;                    \
          BOARD[to] = pro_piece

#define NocapProW( PIECE, PRO_PIECE, piece, pro_piece )       \
          Xor( from, BB_W ## PIECE );                         \
          Xor( to,   BB_W ## PRO_PIECE );                     \
          MATERIAL    -= MT_PRO_ ## PIECE;                    \
          BOARD[to]  = - pro_piece
 
#define NocapNoproB( PIECE, piece )                          \
          SetClear( BB_B ## PIECE );                         \
          BOARD[to] = piece

#define NocapNoproW( PIECE, piece )                          \
          SetClear( BB_W ## PIECE );                         \
          BOARD[to] = - piece

#define FUNC_NAME_B make_move_fast_b
#define FUNC_NAME_W make_move_fast_w

#include "makemove.h"

#undef FUNC_NAME_B
#undef FUNC_NAME_W
#undef DropB
#undef DropW
#undef CapB
#undef CapW
#undef NocapProB
#undef NocapProW
#undef NocapNoproB
#undef NocapNoproW


/*
 * flag_detect_hang
 * flag_rep
 * flag_time
 * flag_nomake_move
 * flag_history
 */
int CONV
make_move_root( tree_t * restrict ptree, move_t move, int flag )
{
  int check, drawn, iret, i, n;

#ifdef CLUSTER_PARALLEL
  if ( ! Mproc && ! ( flag & flag_nomake_move ) )
    makeMoveRootHook( move );
#endif

  MakeMove( root_turn, move, 1 );

  /* detect hang-king */
  if ( ( flag & flag_detect_hang ) && InCheck(root_turn) )
    {
      str_error = str_king_hang;
      UnMakeMove( root_turn, move, 1 );
      return -2;
    }

  drawn = 0;
  check = InCheck( Flip(root_turn) );
  ptree->move_last[1]  = ptree->move_last[0];
  if ( check )
    {
      ptree->nsuc_check[2] = (unsigned char)( ptree->nsuc_check[0] + 1U );
    }
  else { ptree->nsuc_check[2] = 0; }

  /* detect repetitions */
  if ( flag & flag_rep )
    {
      switch ( detect_repetition( ptree, 2, Flip(root_turn), 3 ) )
        {
        case perpetual_check: // 連続王手の千日手なら局面を戻す
          str_error = str_perpet_check;
          UnMakeMove( root_turn, move, 1 );
          return -2;
      
        case four_fold_rep: // 千日手なら引き分け
          drawn = 1;
          break;
        }
    }

  /* return, since all of rule-checks were done */
  if ( flag & flag_nomake_move )
    {
      UnMakeMove( root_turn, move, 1 );
      return drawn ? 2 : 1;
    }

  if ( drawn ) { game_status |= flag_drawn; }

  /* update time */
  if ( flag & flag_time )
    {
      iret = update_time( root_turn );
      if ( iret < 0 ) { return -1; }
    }

  root_turn = Flip( root_turn );

  /* detect checkmate */
  if ( check && is_mate( ptree, 1 ) ) { game_status |= flag_mated; }

  /* save history */
  if ( flag & flag_history ) { out_CSA( ptree, &record_game, move ); }

  /* renew repetition table */
  n = ptree->nrep;
  if ( n >= REP_HIST_LEN - PLY_MAX -1 )
    {
      for ( i = 0; i < n; i++ )
        {
          ptree->rep_board_list[i] = ptree->rep_board_list[i+1];
          ptree->rep_hand_list[i]  = ptree->rep_hand_list[i+1];
        }
    }
  else { ptree->nrep++; }

  for ( i = 1; i < NUM_UNMAKE; i += 1 )
    {
      amove_save[i-1]            = amove_save[i];
      amaterial_save[i-1]        = amaterial_save[i];
      ansuc_check_save[i-1]      = ansuc_check_save[i];
      alast_root_value_save[i-1] = alast_root_value_save[i];
      alast_pv_save[i-1]         = alast_pv_save[i];
    }
  amove_save      [NUM_UNMAKE-1] = move;
  amaterial_save  [NUM_UNMAKE-1] = ptree->save_material[1];
  ansuc_check_save[NUM_UNMAKE-1] = ptree->nsuc_check[0];
  ptree->nsuc_check[0]           = ptree->nsuc_check[1];
  ptree->nsuc_check[1]           = ptree->nsuc_check[2];

  /* update pv */
  alast_root_value_save[NUM_UNMAKE-1] = last_root_value;
  alast_pv_save[NUM_UNMAKE-1]         = last_pv;

  if ( last_pv.a[1] == move && last_pv.length >= 2 )
    {
      if ( last_pv.depth )
        {
#if PLY_INC == EXT_CHECK
          if ( ! check )
#endif
            last_pv.depth--;
        }
      last_pv.length--;
      memmove( &(last_pv.a[1]), &(last_pv.a[2]),
               last_pv.length * sizeof( unsigned int ) );
    }
  else {
    last_pv.a[0]    = 0;
    last_pv.a[1]    = 0;
    last_pv.depth   = 0;
    last_pv.length  = 0;
    last_root_value = 0;
  }

#if defined(DFPN_CLIENT)
  lock( &dfpn_client_lock );
  snprintf( (char *)dfpn_client_signature, DFPN_CLIENT_SIZE_SIGNATURE,
            "%" PRIx64 "_%x_%x_%x", HASH_KEY, HAND_B, HAND_W, root_turn );
  dfpn_client_signature[DFPN_CLIENT_SIZE_SIGNATURE-1] = '\0';
  dfpn_client_rresult       = dfpn_client_na;
  dfpn_client_num_cresult   = 0;
  dfpn_client_flag_read     = 0;
  dfpn_client_out( "%s %s\n", str_CSA_move(move), dfpn_client_signature );
  unlock( &dfpn_client_lock );
#endif

  return 1;
}


int CONV unmake_move_root( tree_t * restrict ptree )
{
  move_t move;
  int i;

  if ( ptree->nrep == 0 || amove_save[NUM_UNMAKE-1] == MOVE_NA )
    {
      str_error = "no more undo infomation at root";
      return -1;
    }

  ptree->nsuc_check[1]    = ptree->nsuc_check[0];
  ptree->nsuc_check[0]    = ansuc_check_save[NUM_UNMAKE-1];
  ptree->save_material[1] = (short)amaterial_save[NUM_UNMAKE-1];
  move                    = amove_save[NUM_UNMAKE-1];
  last_root_value         = alast_root_value_save[NUM_UNMAKE-1];
  last_pv                 = alast_pv_save[NUM_UNMAKE-1];

#ifdef CLUSTER_PARALLEL
  if (!Mproc)
    unmakeMoveRootHook();
#endif

  ptree->nrep -= 1;
  game_status &= ~( flag_drawn | flag_mated );
  root_turn    = Flip(root_turn);

  for ( i = NUM_UNMAKE-1; i > 0; i -= 1 )
    {
      amove_save[i]            = amove_save[i-1];
      amaterial_save[i]        = amaterial_save[i-1];
      ansuc_check_save[i]      = ansuc_check_save[i-1];
      alast_root_value_save[i] = alast_root_value_save[i-1];
      alast_pv_save[i]         = alast_pv_save[i-1];
    }
  amove_save[0]            = MOVE_NA;
  amaterial_save[0]        = 0;
  ansuc_check_save[0]      = 0;
  alast_root_value_save[0] = 0;
  alast_pv_save[0].a[0]    = 0;
  alast_pv_save[0].a[1]    = 0;
  alast_pv_save[0].depth   = 0;
  alast_pv_save[0].length  = 0;

  UnMakeMove( root_turn, move, 1 );

#if defined(DFPN_CLIENT)
  lock( &dfpn_client_lock );
  snprintf( (char *)dfpn_client_signature, DFPN_CLIENT_SIZE_SIGNATURE,
            "%" PRIx64 "_%x_%x_%x", HASH_KEY, HAND_B, HAND_W, root_turn );
  dfpn_client_signature[DFPN_CLIENT_SIZE_SIGNATURE-1] = '\0';
  dfpn_client_rresult       = dfpn_client_na;
  dfpn_client_flag_read     = 0;
  dfpn_client_num_cresult   = 0;
  dfpn_client_out( "unmake\n" );
  unlock( &dfpn_client_lock );
#endif

  return 1;
}
