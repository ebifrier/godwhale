
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "shogi.h"

/* hand joseki
 data in file "hand.jos".  format:

<hanjo>
h f5
y.....oe.
.h.....iy
..eif.Gf.
f..fgUF..
.ff..f..f
F..FG....
.FG.F.E..
..I.I....
YEO...u.Y
i12
gote
2133KE
</hanjo>
# comment
<hanjo>
 :

 */

#define forr(i,m,n) for(int i=(m); i<=(n); i++)

#define MAX_HANDJOSEKI 32

struct {
 min_posi_t pos;
 unsigned int move;
} hanjo_entry[MAX_HANDJOSEKI];

int hanjo_entry_cnt;


// 文字を駒番号に変換するためのテーブル
// 15より大きければ先手、小さければ後手の駒となります。
//
// F:歩    Y:香車  E:桂馬  G:銀    I:金    A:角  H:飛車  O:玉
// T:と金  S:成香  M:成桂  Z:成銀  *:NULL  U:馬  R:竜
// (駒と文字の対応が、ちょっと分かりにくい。。。)
//
static const char n2pcchar[31] = {
        'r', 'u', '*', 'z', 'm', 's', 't',
   'o', 'h', 'a', 'i', 'g', 'e', 'y', 'f',
   '.',
   'F', 'Y', 'E', 'G', 'I', 'A', 'H', 'O',
   'T', 'S', 'M', 'Z', '*', 'U', 'R' };

// 文字を駒番号に変換
static int
char2pc( char c )
{
  int i;

  for ( i = 0; i < 31; i++ )
    {
        // 2, 28は使わない駒番号(piece_null)なのでとばす
      if ( i == 2 || i == 28 ) { continue; }
      if ( n2pcchar[i] == c )  { return (i - 15); }
    }

  return -16; // NTBR;
}


// CSA形式の駒文字(FU,KYなど)を駒番号に変換します。
static int
str2piece( const char *str )
{
  int i;
  for ( i = 0; i < 16; i++ )
      if ( ! strncmp( astr_table_piece[i], str, 2 ) ) { break; }
  if ( i == 0 || i == piece_null || i == 16 ) { i = -2; }
  return i;
}


// 7776FU などのCSA形式の文字列を指し手に変換します。
static int
interpret_CSA_move2( min_posi_t* posp, Move *pmove,
                     const char *str )
{
  int ifrom_file, ifrom_rank, ito_file, ito_rank, ipiece;
  int ifrom, ito;
  unsigned int move;

  ifrom_file = str[0]-'0';
  ifrom_rank = str[1]-'0';
  ito_file   = str[2]-'0';
  ito_rank   = str[3]-'0';

  ito_file   = 9 - ito_file;
  ito_rank   = ito_rank - 1;
  ito        = ito_rank * 9 + ito_file;
  ipiece     = str2piece( str+4 );
  if ( ipiece < 0 )
    {
      str_error = str_illegal_move;
      return -2;
    }

  if ( ! ifrom_file && ! ifrom_rank )
    {
      // 駒打ち
      move = To2Move(ito) | Drop2Move(ipiece);
    }
  else {
    ifrom_file = 9 - ifrom_file;
    ifrom_rank = ifrom_rank - 1;
    ifrom      = ifrom_rank * 9 + ifrom_file;

    // 移動元にある駒が不成りで、今の駒が成りなら成りフラグを立てます
    if ( abs(posp->asquare[ifrom]) + promote == ipiece )
      {
        ipiece -= promote;
        move    = FLAG_PROMO;
      }
    else { move = 0; }

    move |= ( To2Move(ito) | From2Move(ifrom)
              | Cap2Move(abs(posp->asquare[ito])) | Piece2Move(ipiece));
  }

  *pmove = move;
  return 0;
}


int CONV
read_handjoseki( void )
{
  const char p2c[7] = { 'f', 'y', 'e', 'g', 'i', 'a', 'h' };
  FILE* fp;
  int state, i, j;
  int capt[2][8];
  char buf[30];
  min_posi_t pos;
  Move move = MOVE_NA;

  fp = file_open( "hand.jos", "r" );
  if (fp == NULL)
    {
      str_error = str_io_error;
      return -1;
    }

  memset( hanjo_entry, 0, sizeof(hanjo_entry) );
  memset( &pos, 0, sizeof(pos) );
  hanjo_entry_cnt = 0;
  state = 0;

  while ( fgets( buf, 20, fp ) )
    {
      if ( ! strncmp( buf, "#", 1 ) ||  // line starting with '#' is a comment
           ! strncmp( buf, "\n", 1 ) )  // ignore empty line
          continue;

      switch ( state )
        {
        case -1:  // something wrong has happened.  skip to next entry
        case  0:  // expecting next entry
            if (! strncmp( buf, "<hanjo>", 7 ) )
              state = 1;
            break;

        case  1:  // 先手番の持ち駒
        case 11:  // 後手番の持ち駒
          { // capt "h g2 f12"
            int turn = ( state == 1 ? white : black );
            for ( i = 0; i < 7; i++ ) // 持ち駒は全部で７種類
              {
                char* p = strchr( buf, p2c[i] );
                capt[turn][i] =
                  ( p == NULL ) ? 0 :
                  ( isdigit(p[1]) && isdigit(p[2]) ) ?
                    10 * (p[1]-'0') + (p[2]-'0') :
                  ( isdigit(p[1]) ) ? (p[1]-'0') : 1;
              }
            state++;
          }
          break;

        // 局面を読み込みます
        // y.....oey
        // .h..i.i..
        // (以下続く。。。)
        case  2: case  3: case  4: case  5: case  6:
        case  7: case  8: case  9: case 10:
          {
            int row = state - 2;  // -2することで対応する段数になる
            for ( int x = 0; x <= 8; x++ )
              {
                int c = char2pc(buf[x]);
                if ( c == -16 )
                  {
                    state = -1; // エラー
                    break;
                  }
                else {
                  pos.asquare[nfile*row + x] = (char)c;
                }
              }
            if (state != -1) state++;
          }
          break;

        // 手番を読み込みます。sente or goteで示されます
        case 12:
          if ( buf[0] != 's' && buf[0] != 'g' )
            {
              state = -1;
              break;
            }
          pos.turn_to_move = (buf[0] == 's') ? black : white;
          state++;
          break;

        // 読み込んだ局面で必ず指す手を読み込みます。
        // 指し手はCSA形式
        case 13:
          if ( interpret_CSA_move2( &pos, &move, buf ) < 0 )
            {
              state = -1;
              break;
            }
          state++;
          break;

        // 必須手の読み込み終了。局面と指し手を登録します。
        case 14:
          if ( strncmp( buf, "</hanjo>", 8 ) )
            {
              state = -1;
              break;
            }
          j = hanjo_entry_cnt++;
          hanjo_entry[j].pos.turn_to_move = pos.turn_to_move;
          hanjo_entry[j].pos.hand_black =
              capt[black][0] * flag_hand_pawn +
              capt[black][1] * flag_hand_lance +
              capt[black][2] * flag_hand_knight +
              capt[black][3] * flag_hand_silver +
              capt[black][4] * flag_hand_gold +
              capt[black][5] * flag_hand_bishop +
              capt[black][6] * flag_hand_rook;
          hanjo_entry[j].pos.hand_white =
              capt[white][0] * flag_hand_pawn +
              capt[white][1] * flag_hand_lance +
              capt[white][2] * flag_hand_knight +
              capt[white][3] * flag_hand_silver +
              capt[white][4] * flag_hand_gold +
              capt[white][5] * flag_hand_bishop +
              capt[white][6] * flag_hand_rook;
          for ( i = 0; i <= 80; i++ )
              hanjo_entry[j].pos.asquare[i] = pos.asquare[i];
          hanjo_entry[j].move = move;
          state = 0;
          break;

        default:
          break;
      }
    }

  fclose(fp);
  return 0;
}


Move CONV
probe_handjoseki( tree_t* restrict ptree )
{
  int i, j;

  for ( i = 0; i <= hanjo_entry_cnt-1; i++ )
    {
      min_posi_t* posp = &hanjo_entry[i].pos;
      int differ = 0;

      if ( root_turn != posp->turn_to_move ||
           ptree->posi.hand_black != posp->hand_black ||
           ptree->posi.hand_white != posp->hand_white ) continue;

      for ( j = 0; j < nsquare; j++ )
        if ( ptree->posi.asquare[j] != posp->asquare[j] )
          { differ = 1; break; }

      if ( ! differ )
          return hanjo_entry[i].move;
    }

  return MOVE_NA;
}
