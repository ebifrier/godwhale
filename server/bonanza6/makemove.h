
void CONV
FUNC_NAME_B( tree_t * restrict ptree, move_t move, int ply )
{
  const int from = (int)I2From(move);
  const int to   = (int)I2To(move);
  const int nrep = ptree->nrep + ply - 1;

  assert( UToCap(move) != king );
  assert( move );
  assert( is_move_valid( ptree, move, black ) );

  ptree->rep_board_list[nrep]    = HASH_KEY;
  ptree->rep_hand_list[nrep]     = HAND_B;
  ptree->save_material[ply]      = (short)MATERIAL;
  ptree->save_eval[ply+1]        = INT_MAX;

  if ( from >= nsquare )
    { int pc = From2Drop(from);
      int nlist;
      switch ( From2Drop(from) )
        {
        case pawn:   Xor( to-nfile, BB_BPAWN_ATK );
                     DropB( PAWN,   pawn   );  break;
        case lance:  DropB( LANCE,  lance  );  break;
        case knight: DropB( KNIGHT, knight );  break;
        case silver: DropB( SILVER, silver );  break;
        case gold:   DropB( GOLD,   gold   );
                     Xor( to, BB_BTGOLD );     break;
        case bishop: DropB( BISHOP, bishop );
                     Xor( to, BB_B_BH );       break;
        case rook:   DropB( ROOK,  rook );
                     Xor( to, BB_B_RD );       break;
        default:     unreachable();            break;
        }
      Xor( to, BB_BOCCUPY );
      XorOccupied( to );

      nlist = ptree->nlist++;
      ptree->list0[2*(pc-1)  ]--;
      ptree->list1[2*(pc-1)+1]--;
      ptree->curhand[2*(pc-1)]--;
      ptree->listsuf4sq[to] = nlist;
      ptree->sq4listsuf[nlist] = to;
      ptree->list0[nlist] = pc2suf[15+pc] + to;
      ptree->list1[nlist] = pc2suf[15-pc] + Inv(to);
    }
  else {
    const int ipiece_move = (int)I2PieceMove(move);
    const int ipiece_cap  = (int)UToCap(move);
    const int is_promote  = (int)I2IsPromote(move);
    int srcsuf, dstpc;
    bitboard_t bb_set_clear;

    BBOr( bb_set_clear, abb_mask[from], abb_mask[to] );
    SetClear( BB_BOCCUPY );
    BOARD[from] = empty;

    if ( is_promote ) switch( ipiece_move )
      {
      case pawn:   Xor( to, BB_BPAWN_ATK );
                   Xor( to, BB_BTGOLD );
                   NocapProB( PAWN,   PRO_PAWN,   pawn,   pro_pawn );   break;
      case lance:  Xor( to, BB_BTGOLD );
                   NocapProB( LANCE,  PRO_LANCE,  lance,  pro_lance );  break;
      case knight: Xor( to, BB_BTGOLD );
                   NocapProB( KNIGHT, PRO_KNIGHT, knight, pro_knight ); break;
      case silver: Xor( to, BB_BTGOLD );
                   NocapProB( SILVER, PRO_SILVER, silver, pro_silver ); break;
      case bishop: Xor( to, BB_B_HDK );
                   SetClear( BB_B_BH );
                   NocapProB( BISHOP, HORSE,      bishop, horse );      break;
      case rook:   Xor( to, BB_B_HDK );
                   SetClear( BB_B_RD );
                   NocapProB( ROOK,   DRAGON,     rook,   dragon );     break;
      default:     unreachable();                                       break;
      }
    else switch ( ipiece_move )
      {
      case pawn:       Xor( to-nfile, BB_BPAWN_ATK );
                       Xor( to,       BB_BPAWN_ATK );
                       NocapNoproB( PAWN,   pawn);       break;
      case lance:      NocapNoproB( LANCE,  lance);      break;
      case knight:     NocapNoproB( KNIGHT, knight);     break;
      case silver:     NocapNoproB( SILVER, silver);     break;
      case gold:       NocapNoproB( GOLD,   gold);
                       SetClear( BB_BTGOLD );             break;
      case bishop:     SetClear( BB_B_BH );
                       NocapNoproB( BISHOP, bishop);     break;
      case rook:       NocapNoproB( ROOK,   rook);
                       SetClear( BB_B_RD );                break;
      case king:       HASH_KEY ^= b_king_rand[to] ^ b_king_rand[from];
                       SetClear( BB_B_HDK );
                       BOARD[to] = king;
                       SQ_BKING  = (char)to;           break;
      case pro_pawn:   NocapNoproB( PRO_PAWN, pro_pawn );
                       SetClear( BB_BTGOLD );             break;
      case pro_lance:  NocapNoproB( PRO_LANCE, pro_lance );
                       SetClear( BB_BTGOLD );             break;
      case pro_knight: NocapNoproB( PRO_KNIGHT, pro_knight );
                       SetClear( BB_BTGOLD );             break;
      case pro_silver: NocapNoproB( PRO_SILVER, pro_silver );
                       SetClear( BB_BTGOLD );             break;
      case horse:      NocapNoproB( HORSE, horse );
                       SetClear( BB_B_HDK );
                       SetClear( BB_B_BH );                break;
      case dragon:     NocapNoproB( DRAGON, dragon );
                       SetClear( BB_B_HDK );
                       SetClear( BB_B_RD );                break;
      default:         unreachable();                      break;
      }
    
#define NOSQ (-1)
    if ( ipiece_cap )
      { int tgtpc, tgtsuf, tail;
        switch( ipiece_cap )
          {
          case pawn:       CapW( PAWN, pawn, pawn );
                           Xor( to+nfile, BB_WPAWN_ATK );               break;
          case lance:      CapW( LANCE,  lance, lance );       break;
          case knight:     CapW( KNIGHT, knight, knight );      break;
          case silver:     CapW( SILVER, silver, silver );      break;
          case gold:       CapW( GOLD,   gold,   gold );
                           Xor( to, BB_WTGOLD );                       break;
          case bishop:     CapW( BISHOP, bishop, bishop );
                           Xor( to, BB_W_BH );                          break;
          case rook:       CapW( ROOK, rook, rook);
                           Xor( to, BB_W_RD );                          break;
          case pro_pawn:   CapW( PRO_PAWN, pawn, pro_pawn );
                           Xor( to, BB_WTGOLD );                       break;
          case pro_lance:  CapW( PRO_LANCE, lance, pro_lance );
                           Xor( to, BB_WTGOLD );                       break;
          case pro_knight: CapW( PRO_KNIGHT, knight, pro_knight );
                           Xor( to, BB_WTGOLD );                       break;
          case pro_silver: CapW( PRO_SILVER, silver, pro_silver );
                           Xor( to, BB_WTGOLD );                       break;
          case horse:      CapW( HORSE, bishop, horse );
                           Xor( to, BB_W_HDK );
                           Xor( to, BB_W_BH );                          break;
          case dragon:     CapW( DRAGON, rook, dragon );
                           Xor( to, BB_W_HDK );
                           Xor( to, BB_W_RD );                         break;
          default:         unreachable();                      break;
          }
        Xor( to, BB_WOCCUPY );
        XorOccupied( from );

        tgtpc = ipiece_cap & 7;
        tgtsuf = ptree->listsuf4sq[to];
        tail = --(ptree->nlist);
        listswap(ptree, tgtsuf, tail);  // now tgt is at tail
        ptree->list0[2*(tgtpc-1)  ]++;
        ptree->list1[2*(tgtpc-1)+1]++;
        ptree->curhand[2*(tgtpc-1)]++;
        ptree->sq4listsuf[tail] = NOSQ;
        // ptree->listsuf4sq[dst] will be overwritten by dst
        // list0,1 for suf>=nlist we don't care (FIXME? really?)
      }
    else {
      SetClearOccupied( from, to );
    }

    if (ipiece_move != king) {
      srcsuf = ptree->listsuf4sq[from];
      ptree->sq4listsuf[srcsuf] = to;
      ptree->listsuf4sq[from] = NOSQ; 
      ptree->listsuf4sq[to  ] = srcsuf; 
      dstpc = ipiece_move | (is_promote ? 8 : 0);
      ptree->list0[srcsuf] = pc2suf[15+dstpc] + to;
      ptree->list1[srcsuf] = pc2suf[15-dstpc] + Inv(to);
    }
  }

  assert( exam_bb( ptree ) );
}


void CONV
FUNC_NAME_W( tree_t * restrict ptree, move_t move, int ply )
{
  const int from = (int)I2From(move);
  const int to   = (int)I2To(move);
  const int nrep  = ptree->nrep + ply - 1;

  assert( UToCap(move) != king );
  assert( move );
  assert( is_move_valid( ptree, move, white ) );

  ptree->rep_board_list[nrep]    = HASH_KEY;
  ptree->rep_hand_list[nrep]     = HAND_B;
  ptree->save_material[ply]      = (short)MATERIAL;
  ptree->save_eval[ply+1]        = INT_MAX;

  if ( from >= nsquare )
    { int nlist;
      int pc = From2Drop(from) ;
      switch( From2Drop(from) )
        {
        case pawn:   Xor( to+nfile, BB_WPAWN_ATK );
                     DropW( PAWN,   pawn );    break;
        case lance:  DropW( LANCE,  lance );   break;
        case knight: DropW( KNIGHT, knight );  break;
        case silver: DropW( SILVER, silver );  break;
        case gold:   DropW( GOLD,   gold );
                     Xor( to, BB_WTGOLD );     break;
        case bishop: DropW( BISHOP, bishop );
                     Xor( to, BB_W_BH );       break;
        case rook:   DropW( ROOK,   rook );
                     Xor( to, BB_W_RD );       break;
        default:     unreachable();            break;
        }
      Xor( to, BB_WOCCUPY );
      XorOccupied( to );

      nlist = ptree->nlist++;
      ptree->list0[2*(pc-1)+1]--;
      ptree->list1[2*(pc-1)  ]--;
      ptree->curhand[2*(pc-1)+1]--;
      ptree->listsuf4sq[to] = nlist;
      ptree->sq4listsuf[nlist] = to;
      ptree->list0[nlist] = pc2suf[15-pc] + to;
      ptree->list1[nlist] = pc2suf[15+pc] + Inv(to);
    }
  else {
    const int ipiece_move = (int)I2PieceMove(move);
    const int ipiece_cap  = (int)UToCap(move);
    const int is_promote  = (int)I2IsPromote(move);
    int srcsuf, dstpc;
    bitboard_t bb_set_clear;

    BBOr( bb_set_clear, abb_mask[from], abb_mask[to] );
    SetClear( BB_WOCCUPY );
    BOARD[from] = empty;

    if ( is_promote) switch( ipiece_move )
      {
      case pawn:   NocapProW( PAWN, PRO_PAWN, pawn, pro_pawn );
                   Xor( to, BB_WPAWN_ATK );
                   Xor( to, BB_WTGOLD );                           break;
      case lance:  NocapProW( LANCE, PRO_LANCE, lance, pro_lance );
                   Xor( to, BB_WTGOLD );                           break;
      case knight: NocapProW( KNIGHT, PRO_KNIGHT, knight, pro_knight );
                   Xor( to, BB_WTGOLD );                           break;
      case silver: NocapProW( SILVER, PRO_SILVER, silver, pro_silver );
                   Xor( to, BB_WTGOLD );                           break;
      case bishop: NocapProW( BISHOP, HORSE, bishop, horse );
                   Xor( to, BB_W_HDK );
                   SetClear( BB_W_BH );                              break;
      case rook:   NocapProW( ROOK, DRAGON, rook, dragon);
                   Xor( to, BB_W_HDK );
                   SetClear( BB_W_RD );                              break;
      default:     unreachable();            break;
      }
    else switch ( ipiece_move )
      {
      case pawn:       NocapNoproW( PAWN, pawn );
                       Xor( to+nfile, BB_WPAWN_ATK );
                       Xor( to,       BB_WPAWN_ATK );     break;
      case lance:      NocapNoproW( LANCE,     lance);      break;
      case knight:     NocapNoproW( KNIGHT,    knight);     break;
      case silver:     NocapNoproW( SILVER,    silver);     break;
      case gold:       NocapNoproW( GOLD,      gold);
                       SetClear( BB_WTGOLD );             break;
      case bishop:     NocapNoproW( BISHOP,    bishop);
                       SetClear( BB_W_BH );                break;
      case rook:       NocapNoproW( ROOK,      rook);
                       SetClear( BB_W_RD );                break;
      case king:       HASH_KEY    ^= w_king_rand[to] ^ w_king_rand[from];
                       BOARD[to]  = - king;
                       SQ_WKING   = (char)to;
                       SetClear( BB_W_HDK );               break;
      case pro_pawn:   NocapNoproW( PRO_PAWN,   pro_pawn);
                       SetClear( BB_WTGOLD );             break;
      case pro_lance:  NocapNoproW( PRO_LANCE,  pro_lance);
                       SetClear( BB_WTGOLD );             break;
      case pro_knight: NocapNoproW( PRO_KNIGHT, pro_knight);
                       SetClear( BB_WTGOLD );             break;
      case pro_silver: NocapNoproW( PRO_SILVER, pro_silver);
                       SetClear( BB_WTGOLD );             break;
      case horse:      NocapNoproW( HORSE, horse );
                       SetClear( BB_W_HDK );
                       SetClear( BB_W_BH );                break;
      case dragon:     NocapNoproW( DRAGON, dragon );
                       SetClear( BB_W_HDK );
                       SetClear( BB_W_RD );                break;
      default:         unreachable();            break;
      }

    if ( ipiece_cap )
      { int tgtpc, tgtsuf, tail;
        switch( ipiece_cap )
          {
          case pawn:       CapB( PAWN, pawn, pawn );
                           Xor( to-nfile, BB_BPAWN_ATK );           break;
          case lance:      CapB( LANCE,  lance,  lance );           break;
          case knight:     CapB( KNIGHT, knight, knight );          break;
          case silver:     CapB( SILVER, silver, silver );          break;
          case gold:       CapB( GOLD,   gold,   gold );
                           Xor( to, BB_BTGOLD );                   break;
          case bishop:     CapB( BISHOP, bishop, bishop );
                           Xor( to, BB_B_BH );                      break;
          case rook:       CapB( ROOK, rook, rook );
                           Xor( to, BB_B_RD );                      break;
          case pro_pawn:   CapB( PRO_PAWN, pawn, pro_pawn );
                           Xor( to, BB_BTGOLD );                   break;
          case pro_lance:  CapB( PRO_LANCE, lance, pro_lance );
                           Xor( to, BB_BTGOLD );                   break;
          case pro_knight: CapB( PRO_KNIGHT, knight, pro_knight );
                           Xor( to, BB_BTGOLD );                   break;
          case pro_silver: CapB( PRO_SILVER, silver, pro_silver );
                           Xor( to, BB_BTGOLD );                   break;
          case horse:      CapB( HORSE, bishop, horse );
                           Xor( to, BB_B_HDK );
                           Xor( to, BB_B_BH );                      break;
          case dragon:     CapB( DRAGON, rook, dragon );
                           Xor( to, BB_B_HDK );
                           Xor( to, BB_B_RD );                      break;
          default:         unreachable();            break;
          }
        Xor( to, BB_BOCCUPY );
        XorOccupied( from );

        tgtpc = ipiece_cap & 7;
        tgtsuf = ptree->listsuf4sq[to];
        tail = --(ptree->nlist);
        listswap(ptree, tgtsuf, tail);  // now tgt is at tail
        ptree->list0[2*(tgtpc-1)+1]++;
        ptree->list1[2*(tgtpc-1)  ]++;
        ptree->curhand[2*(tgtpc-1)+1]++;
        ptree->sq4listsuf[tail] = NOSQ;
        // ptree->listsuf4sq[dst] will be overwritten by dst
        // list0,1 for suf>=nlist we don't care (FIXME? really?)
      }
    else {
      SetClearOccupied( from, to );
    }

    if (ipiece_move != king) {
      srcsuf = ptree->listsuf4sq[from];
      ptree->sq4listsuf[srcsuf] = to;
      ptree->listsuf4sq[from] = NOSQ; 
      ptree->listsuf4sq[to  ] = srcsuf; 
      dstpc = ipiece_move | (is_promote ? 8 : 0);
      ptree->list0[srcsuf] = pc2suf[15-dstpc] + to;
      ptree->list1[srcsuf] = pc2suf[15+dstpc] + Inv(to);
    }
  }

  assert( exam_bb( ptree ) );
}
