// $Id: unmake.c,v 1.2 2012/03/22 06:09:30 eiki Exp $

#include <assert.h>
#include "shogi.h"


#define CapW( PIECE, piece, pro_piece )  Xor( to, BB_W ## PIECE );           \
                                         HAND_B      -= flag_hand_ ## piece; \
                                         BOARD[to]  = -pro_piece

#define CapB( PIECE, piece, pro_piece )  Xor( to, BB_B ## PIECE );           \
                                         HAND_W      -= flag_hand_ ## piece; \
                                         BOARD[to]  = pro_piece

#define NocapNopro( PIECE, piece )  SetClear( BB_ ## PIECE ); \
                                    BOARD[from] = piece 

#define NocapPro( PIECE , PRO_PIECE, piece )  Xor( from, BB_ ## PIECE );     \
                                              Xor( to,   BB_ ## PRO_PIECE ); \
                                              BOARD[from] = piece


void CONV
unmake_move_b( tree_t * restrict ptree, unsigned int move, int ply )
{
  int from = (int)I2From(move);
  int to   = (int)I2To(move);
  int nrep = ptree->nrep + ply - 1;

  HASH_KEY = ptree->rep_board_list[nrep];
  MATERIAL = ptree->save_material[ply];

  if ( from >= nsquare )
    {
      int pc = From2Drop(from) ;
      int tail, dstsuf;
      switch( pc )
        {
        case pawn:    Xor( to, BB_BPAWN );
                      Xor( to-nfile, BB_BPAWN_ATK );
                      HAND_B += flag_hand_pawn;     break;
        case lance:   Xor( to, BB_BLANCE );
                      HAND_B += flag_hand_lance;    break;
        case knight:  Xor( to, BB_BKNIGHT );
                      HAND_B += flag_hand_knight;   break;
        case silver:  Xor( to, BB_BSILVER );
                      HAND_B += flag_hand_silver;   break;
        case gold:    Xor( to, BB_BGOLD );
                      Xor( to, BB_BTGOLD );
                      HAND_B += flag_hand_gold;     break;
        case bishop:  Xor( to, BB_BBISHOP );
                      Xor( to, BB_B_BH );
                      HAND_B += flag_hand_bishop;   break;
        default:      assert( From2Drop(from) == rook );
                      Xor( to, BB_BROOK );
                      Xor( to, BB_B_RD );
                      HAND_B += flag_hand_rook;  break;
        }

      BOARD[to] = empty;
      Xor( to, BB_BOCCUPY );
      //XorFile( to, OCCUPIED_FILE );
      //XorDiag2( to, OCCUPIED_DIAG2 );
      //XorDiag1( to, OCCUPIED_DIAG1 );
      XorOccupied( to );

      tail = --(ptree->nlist);
      dstsuf = ptree->listsuf4sq[to];
      listswap(ptree, tail, dstsuf);
      ptree->list0[2*(pc-1)  ]++;
      ptree->list1[2*(pc-1)+1]++;
      ptree->curhand[2*(pc-1)  ]++;
#define NOLS (-1)
#define NOSQ (-1)
      ptree->listsuf4sq[to] = NOLS;
#ifdef PARANOID
      ptree->sq4listsuf[tail] = NOSQ;
      ptree->list0[tail] = NOLS;
      ptree->list1[tail] = NOLS;
#endif
    }
  else {
    const int ipiece_move = (int)I2PieceMove(move);
    const int ipiece_cap  = (int)UToCap(move);
    const int is_promote  = (int)I2IsPromote(move);
    bitboard_t bb_set_clear;
    int srcsuf, srcpc;

    if (ipiece_move != king) {
      srcsuf = ptree->listsuf4sq[to];
      ptree->sq4listsuf[srcsuf] = from;
      ptree->listsuf4sq[to  ] = NOSQ;  // overwritten if cap, but do it anyway
      ptree->listsuf4sq[from] = srcsuf; 
      //srcpc = ipiece_move ^ (is_promote ? 8 : 0);
      srcpc = ipiece_move;  // pc _before_ promotion
      ptree->list0[srcsuf] = pc2suf[15+srcpc] + from;
      ptree->list1[srcsuf] = pc2suf[15-srcpc] + Inv(from);
    }

    BBOr( bb_set_clear, abb_mask[from], abb_mask[to] );
    SetClear( BB_BOCCUPY );

    if ( is_promote ) switch( ipiece_move )
      {
      case pawn:    NocapPro( BPAWN, BPRO_PAWN, pawn );
                    Xor( to, BB_BPAWN_ATK );
                    Xor( to, BB_BTGOLD );                        break;
      case lance:   NocapPro( BLANCE,  BPRO_LANCE, lance );
                    Xor( to, BB_BTGOLD );                        break;
      case knight:  NocapPro( BKNIGHT, BPRO_KNIGHT, knight );
                    Xor( to, BB_BTGOLD );                        break;
      case silver:  NocapPro( BSILVER, BPRO_SILVER, silver );
                    Xor( to, BB_BTGOLD );                        break;
      case bishop:  NocapPro( BBISHOP, BHORSE, bishop );
                    Xor( to, BB_B_HDK );
                    SetClear( BB_B_BH );                         break;
      default:      assert( ipiece_move == rook );
                    NocapPro( BROOK, BDRAGON, rook );
                    Xor( to, BB_B_HDK );
                    SetClear( BB_B_RD );                         break;
      }
    else switch ( ipiece_move )
      {
      case pawn:        NocapNopro( BPAWN, pawn );
                        Xor( to-nfile, BB_BPAWN_ATK );
                        Xor( to,       BB_BPAWN_ATK );          break;
      case lance:       NocapNopro( BLANCE,  lance );           break;
      case knight:      NocapNopro( BKNIGHT, knight );          break;
      case silver:      NocapNopro( BSILVER, silver );          break;
      case gold:        NocapNopro( BGOLD, gold );
                        SetClear( BB_BTGOLD );                  break;
      case bishop:      NocapNopro( BBISHOP, bishop );
                        SetClear( BB_B_BH );                    break;
      case rook:        NocapNopro( BROOK, rook);
                        SetClear( BB_B_RD );                    break;
      case king:        BOARD[from] = king;
                        SQ_BKING    = (char)from;
                        SetClear( BB_B_HDK );                   break;
      case pro_pawn:    NocapNopro( BPRO_PAWN, pro_pawn );
                        SetClear( BB_BTGOLD );                  break;
      case pro_lance:   NocapNopro( BPRO_LANCE,  pro_lance );
                        SetClear( BB_BTGOLD );                  break;
      case pro_knight:  NocapNopro( BPRO_KNIGHT, pro_knight );
                        SetClear( BB_BTGOLD );                  break;
      case pro_silver:  NocapNopro( BPRO_SILVER, pro_silver );
                        SetClear( BB_BTGOLD );                  break;
      case horse:        NocapNopro( BHORSE, horse );
                        SetClear( BB_B_HDK );
                        SetClear( BB_B_BH );                    break;
      default:          assert( ipiece_move == dragon );
                        NocapNopro( BDRAGON, dragon );
                        SetClear( BB_B_HDK );
                        SetClear( BB_B_RD );                    break;
      }

    if ( ipiece_cap )
      { int tgtpc, tgtsuf;
        switch( ipiece_cap )
          {
          case pawn:        CapW( PAWN,       pawn,   pawn );
                            Xor( to+nfile, BB_WPAWN_ATK );        break;
          case lance:       CapW( LANCE,      lance,  lance);     break;
          case knight:      CapW( KNIGHT,     knight, knight);    break;
          case silver:      CapW( SILVER,     silver, silver);    break;
          case gold:        CapW( GOLD,       gold,   gold);
                            Xor( to, BB_WTGOLD );                 break;
          case bishop:      CapW( BISHOP,     bishop, bishop);
                            Xor( to, BB_W_BH );                   break;
          case rook:        CapW( ROOK,       rook,   rook);
                            Xor( to, BB_W_RD );                   break;
          case pro_pawn:    CapW( PRO_PAWN,   pawn,   pro_pawn);
                            Xor( to, BB_WTGOLD );                 break;
          case pro_lance:   CapW( PRO_LANCE,  lance,  pro_lance);
                            Xor( to, BB_WTGOLD );                 break;
          case pro_knight:  CapW( PRO_KNIGHT, knight, pro_knight);
                            Xor( to, BB_WTGOLD );                 break;
          case pro_silver:  CapW(PRO_SILVER, silver, pro_silver);
                            Xor( to, BB_WTGOLD );                 break;
          case horse:       CapW(HORSE,     bishop, horse);
                            Xor( to, BB_W_HDK );
                            Xor( to, BB_W_BH );                   break;
          default:          assert( ipiece_cap == dragon );
                            CapW(DRAGON,    rook,   dragon);
                            Xor( to, BB_W_HDK );
                            Xor( to, BB_W_RD );                   break;
          }
      Xor( to, BB_WOCCUPY );
      //XorFile( from, OCCUPIED_FILE );
      //XorDiag1( from, OCCUPIED_DIAG1 );
      //XorDiag2( from, OCCUPIED_DIAG2 );
      XorOccupied( from );

      tgtpc = ipiece_cap & 7;
      tgtsuf = ptree->nlist++;
      ptree->list0[2*(tgtpc-1)  ]--;
      ptree->list1[2*(tgtpc-1)+1]--;
      ptree->curhand[2*(tgtpc-1)  ]--;
      ptree->sq4listsuf[tgtsuf] = to;
      ptree->listsuf4sq[to] = tgtsuf;
      ptree->list0[tgtsuf] = pc2suf[15-ipiece_cap] + to; // TGT is oppn's pc
      ptree->list1[tgtsuf] = pc2suf[15+ipiece_cap] + Inv(to);

      }
    else {
      BOARD[to] = empty;
      //SetClearFile( from, to, OCCUPIED_FILE );
      //SetClearDiag1( from, to, OCCUPIED_DIAG1 );
      //SetClearDiag2( from, to, OCCUPIED_DIAG2 );
      SetClearOccupied( from, to );
    }

  }

  assert( exam_bb( ptree ) );
}


void CONV
unmake_move_w( tree_t * restrict ptree, unsigned int move, int ply )
{
  int from = (int)I2From(move);
  int to   = (int)I2To(move);
  int nrep = ptree->nrep + ply - 1;

  HASH_KEY = ptree->rep_board_list[nrep];
  MATERIAL = ptree->save_material[ply];

  if ( from >= nsquare )
    { int tail, dstsuf;
      int pc = From2Drop(from) ;
      switch( From2Drop(from) )
        {
        case pawn:    Xor( to, BB_WPAWN );
                      Xor( to+nfile, BB_WPAWN_ATK );
                      HAND_W += flag_hand_pawn;    break;
        case lance:   Xor( to, BB_WLANCE );
                      HAND_W += flag_hand_lance;   break;
        case knight:  Xor( to, BB_WKNIGHT );
                      HAND_W += flag_hand_knight;  break;
        case silver:  Xor( to, BB_WSILVER );
                      HAND_W += flag_hand_silver;  break;
        case gold:    Xor( to, BB_WGOLD );
                      Xor( to, BB_WTGOLD );
                      HAND_W += flag_hand_gold;    break;
        case bishop:  Xor( to, BB_WBISHOP );
                      Xor( to, BB_W_BH );
                      HAND_W += flag_hand_bishop;  break;
        default:      assert( From2Drop(from) == rook );
                      Xor( to, BB_WROOK );
                      Xor( to, BB_W_RD );
                      HAND_W += flag_hand_rook;    break;
        }

      BOARD[to] = empty;
      Xor( to, BB_WOCCUPY );
      //XorFile( to, OCCUPIED_FILE );
      //XorDiag2( to, OCCUPIED_DIAG2 );
      //XorDiag1( to, OCCUPIED_DIAG1 );
      XorOccupied( to );

      tail = --(ptree->nlist);
      dstsuf = ptree->listsuf4sq[to];
      listswap(ptree, tail, dstsuf);
      ptree->list0[2*(pc-1)+1]++;
      ptree->list1[2*(pc-1)  ]++;
      ptree->curhand[2*(pc-1)+1]++;
      ptree->listsuf4sq[to] = NOLS;
#ifdef PARANOID
      ptree->sq4listsuf[nlist] = NOSQ;
      ptree->list0[nlist] = NOLS;
      ptree->list1[nlist] = NOLS;
#endif
    }
  else {
    const int ipiece_move = (int)I2PieceMove(move);
    const int ipiece_cap  = (int)UToCap(move);
    const int is_promote  = (int)I2IsPromote(move);
    bitboard_t bb_set_clear;

    int srcsuf, srcpc;

    if (ipiece_move != king) {
      srcsuf = ptree->listsuf4sq[to];
      ptree->sq4listsuf[srcsuf] = from;
      ptree->listsuf4sq[to  ] = NOSQ;  // overwritten if cap, but do it anyway
      ptree->listsuf4sq[from] = srcsuf; 
      //srcpc = ipiece_move ^ (is_promote ? 8 : 0);
      srcpc = ipiece_move;  // pc _before_ promotion
      ptree->list0[srcsuf] = pc2suf[15-srcpc] + from;
      ptree->list1[srcsuf] = pc2suf[15+srcpc] + Inv(from);
    }

    BBOr( bb_set_clear, abb_mask[from], abb_mask[to] );
    SetClear( BB_WOCCUPY );

    if ( is_promote ) switch( ipiece_move )
      {
      case pawn:    NocapPro( WPAWN, WPRO_PAWN, -pawn );
                    Xor( to, BB_WPAWN_ATK );
                    Xor( to, BB_WTGOLD );                        break;
      case lance:   NocapPro( WLANCE, WPRO_LANCE, -lance );
                    Xor( to, BB_WTGOLD );                        break;
      case knight:  NocapPro( WKNIGHT, WPRO_KNIGHT, -knight );
                    Xor( to, BB_WTGOLD );                        break;
      case silver:  NocapPro( WSILVER, WPRO_SILVER, -silver );
                    Xor( to, BB_WTGOLD );                        break;
      case bishop:  NocapPro( WBISHOP, WHORSE, -bishop );
                    Xor( to, BB_W_HDK );
                    SetClear( BB_W_BH );                         break;
      default:      assert( ipiece_move == rook );
                    NocapPro( WROOK, WDRAGON, -rook );
                    Xor( to, BB_W_HDK );
                    SetClear( BB_W_RD );                         break;
      }
    else switch ( ipiece_move )
      {
      case pawn:        NocapNopro( WPAWN, -pawn );
                        Xor( to+nfile, BB_WPAWN_ATK );
                        Xor( to,       BB_WPAWN_ATK );          break;
      case lance:       NocapNopro( WLANCE,  -lance );          break;
      case knight:      NocapNopro( WKNIGHT, -knight );         break;
      case silver:      NocapNopro( WSILVER, -silver );         break;
      case gold:        NocapNopro( WGOLD,   -gold );
                        SetClear( BB_WTGOLD );                  break;
      case bishop:      NocapNopro( WBISHOP, -bishop );
                        SetClear( BB_W_BH );                    break;
      case rook:        NocapNopro( WROOK, -rook );
                        SetClear( BB_W_RD );                    break;
      case king:        BOARD[from] = -king;
                        SQ_WKING    = (char)from;
                        SetClear( BB_W_HDK );                   break;
      case pro_pawn:    NocapNopro( WPRO_PAWN,  -pro_pawn );
                        SetClear( BB_WTGOLD );                  break;
      case pro_lance:   NocapNopro( WPRO_LANCE, -pro_lance );
                        SetClear( BB_WTGOLD );                  break;
      case pro_knight:  NocapNopro( WPRO_KNIGHT,-pro_knight );
                        SetClear( BB_WTGOLD );                  break;
      case pro_silver:  NocapNopro( WPRO_SILVER, -pro_silver );
                        SetClear( BB_WTGOLD );                  break;
      case horse:       NocapNopro( WHORSE, -horse );
                        SetClear( BB_W_HDK );
                        SetClear( BB_W_BH );                    break;
      default:          assert( ipiece_move == dragon );
                        NocapNopro( WDRAGON, -dragon );
                        SetClear( BB_W_HDK );
                        SetClear( BB_W_RD );                    break;
      }
    
    if ( ipiece_cap )
      { int tgtpc, tgtsuf;
        switch( ipiece_cap )
          {
          case pawn:        CapB( PAWN,      pawn,   pawn );
                            Xor( to-nfile, BB_BPAWN_ATK );      break;
          case lance:       CapB( LANCE,     lance,  lance );   break;
          case knight:      CapB( KNIGHT,    knight, knight );  break;
          case silver:      CapB( SILVER,    silver, silver );  break;
          case gold:        CapB( GOLD,      gold,   gold );
                            Xor( to, BB_BTGOLD );              break;
          case bishop:      CapB( BISHOP,    bishop, bishop );
                            Xor( to, BB_B_BH );                 break;
          case rook:        CapB( ROOK,      rook,   rook );
                            Xor( to, BB_B_RD );                 break;
          case pro_pawn:    CapB( PRO_PAWN,   pawn,   pro_pawn );
                            Xor( to, BB_BTGOLD );              break;
          case pro_lance:   CapB( PRO_LANCE,  lance,  pro_lance );
                            Xor( to, BB_BTGOLD );              break;
          case pro_knight:  CapB( PRO_KNIGHT, knight, pro_knight );
                            Xor( to, BB_BTGOLD );              break;
          case pro_silver:  CapB( PRO_SILVER, silver, pro_silver );
                            Xor( to, BB_BTGOLD );              break;
          case horse:       CapB( HORSE,     bishop, horse );
                            Xor( to, BB_B_HDK );
                            Xor( to, BB_B_BH );                 break;
          default:          assert( ipiece_cap == dragon );
                            CapB( DRAGON,    rook,   dragon);
                            Xor( to, BB_B_HDK );
                            Xor( to, BB_B_RD );                 break;
          }
        Xor( to, BB_BOCCUPY );
        //XorFile( from, OCCUPIED_FILE );
        //XorDiag1( from, OCCUPIED_DIAG1 );
        //XorDiag2( from, OCCUPIED_DIAG2 );
        XorOccupied( from );

        tgtpc = ipiece_cap & 7;
        tgtsuf = ptree->nlist++;
        ptree->list0[2*(tgtpc-1)+1]--;
        ptree->list1[2*(tgtpc-1)  ]--;
        ptree->curhand[2*(tgtpc-1)+1]--;
        ptree->sq4listsuf[tgtsuf] = to;
        ptree->listsuf4sq[to] = tgtsuf;
        ptree->list0[tgtsuf] = pc2suf[15+ipiece_cap] + to; // TGT is oppn's pc
        ptree->list1[tgtsuf] = pc2suf[15-ipiece_cap] + Inv(to);

      }
    else {
      BOARD[to] = empty;
      //SetClearFile( from, to, OCCUPIED_FILE );
      //SetClearDiag1( from, to, OCCUPIED_DIAG1 );
      //SetClearDiag2( from, to, OCCUPIED_DIAG2 );
      SetClearOccupied( from, to );
    }
  }

  assert( exam_bb( ptree ) );
}


#undef CapW
#undef CapB
#undef NocapNopro
#undef NocapPro
