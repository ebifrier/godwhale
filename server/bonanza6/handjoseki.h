
#include <string.h>
#include <ctype.h>

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

#define MAX_HANDJOSEKI 1024

struct {
 min_posi_t pos;
 unsigned int move;
} hanjo_entry[MAX_HANDJOSEKI];

int hanjo_entry_cnt;

static char n2pcchar[31] = {
        'r', 'u', '*', 'z', 'm', 's', 't',
   'o', 'h', 'a', 'i', 'g', 'e', 'y', 'f',
   '.', 'F', 'Y', 'E', 'G', 'I', 'A', 'H',
   'O', 'T', 'S', 'M', 'Z', '*', 'U', 'R' };

static int char2pc(char c) {
 int i;
 for(i=0; i<=30; i++) {
   if (i==2 || i==28) continue;
   if (n2pcchar[i] == c)
     return (i - 15);
 }
 return -16; // NTBR;
}

 //**** copied from csa.c, then modified

static int str2piece( const char *str ) {
  int i;
  for ( i = 0; i < 16; i++ )
      if ( ! strncmp( astr_table_piece[i], str, 2 ) ) { break; }
  if ( i == 0 || i == piece_null || i == 16 ) { i = -2; }
  return i;
}


static int
interpret_CSA_move2( min_posi_t* posp, unsigned int *pmove,
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
      move  = To2Move(ito) | Drop2Move(ipiece);
      ifrom = nsquare;
    }
  else {
    ifrom_file = 9 - ifrom_file;
    ifrom_rank = ifrom_rank - 1;
    ifrom      = ifrom_rank * 9 + ifrom_file;
    if ( abs(posp->asquare[ifrom]) + promote == ipiece )
      {
        ipiece -= promote;
        move    = FLAG_PROMO;
      }
    else { move = 0; }

    move |= ( To2Move(ito) | From2Move(ifrom) |Cap2Move(abs(posp->asquare[ito]))
              | Piece2Move(ipiece) );
  }

  *pmove = move;
  return 0;
}

 //**** copied from csa.c end

 //****

int readHandJoseki() {
 FILE* fp;
 int state, sd, j;
 int capt[2][8];
 char buf[30];
 min_posi_t pos;
 unsigned int move;

 hanjo_entry_cnt = 0;
 memset(hanjo_entry, 0, sizeof(hanjo_entry));
 fp = file_open("hand.jos", "r");
 if (fp == NULL) return -1;
 state = 0;

 while(fgets(buf, 20, fp)) {
   int row;
   int i, x;
   char p2c[7] = { 'f', 'y', 'e', 'g', 'i', 'a', 'h' };

   if (!strncmp(buf, "#", 1) ||    // line starting with '#' is a comment
       !strncmp(buf, "\n", 1))     // ignore empty line
     continue;

   switch(state) {
     case -1:  // something wrong has happened.  skip to next entry
     case  0:  // expecting next entry
       if (!strncmp(buf, "<hanjo>", 7))
         state = 1;
       break;

     case  1:
     case 11:  // capt   "h g2 f12"
       sd = (state == 1 ? white : black);
       for(i=0; i<=6; i++) {
         char* p;
         char c = p2c[i];
         capt[sd][i] = 0;
         p = strchr(buf, c);
         if (p==NULL)
           continue;
         if (isdigit(p[1]) && isdigit(p[2]))
           capt[sd][i] = 10 * (p[1] - '0') +  (p[2] - '0');
         else if (isdigit(p[1]))
           capt[sd][i] = (p[1] - '0');
         else
           capt[sd][i] = 1;
       }
       state++;
       break;

     case  2:
     case  3:
     case  4:
     case  5:
     case  6:
     case  7:
     case  8:
     case  9:
     case 10:  // capt
       row = state - 2;  // top row:0, bottom row:8
       for(x=0; x<=8; x++) {
         int c = char2pc(buf[x]);
         if (c == -16) {
           state = -1;
           break;
         } else {
           pos.asquare[9*row + x] = (char)c;
         }
       }
       if (state != -1)
           state++;
       break;

     case 12:  // turn
       if (buf[0] != 's' && buf[0] != 'g') {
           state = -1;
           break;
       }
       pos.turn_to_move = (buf[0] == 's') ? black : white;
       state++;
       break;

     case 13:  // turn
       if (interpret_CSA_move2(&pos, &move, buf) < 0) {
           state = -1;
           break;
       }
       state++;
       break;

     case 14:
       if (strncmp(buf, "</hanjo>", 8)) {
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
       for(i=0; i<=80; i++)
         hanjo_entry[j].pos.asquare[i] = pos.asquare[i];
       hanjo_entry[j].move = move;
       state = 0;
       break;

     default:
       break;

   } // switch

 } // while

 fclose(fp);
 return 0;
}

unsigned int probe_handjoseki(tree_t* restrict ptree) {
#if 0
 unsigned int move  = 0;
 unsigned int handb = ptree->posi.hand_black;
 unsigned int handw = ptree->posi.hand_white;
 signed char* asq = ptree->posi.asquare;

/* kakukawari tomioka ryu

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

-
gote

 ==> -2133KE
 */


 if ( root_turn == white && handb == 0 &&
      handw == (flag_hand_rook + 5 * flag_hand_pawn) &&
      asq[A9] == -lance  && asq[G9] == -king  && asq[H9] == -knight && 
      asq[B8] == -rook   && asq[H8] == -gold  && asq[I8] == -lance  && 
      asq[C7] == -knight && asq[D7] == -gold  && asq[E7] == -pawn   && 
                            asq[G7] == silver && asq[H7] == -pawn   && 
      asq[A6] == -pawn   && asq[D6] == -pawn  && asq[E6] == -silver && 
                            asq[F6] == horse  && asq[G6] ==  pawn   && 
      asq[B5] == -pawn   && asq[C5] == -pawn  && asq[F5] == -pawn   && 
                            asq[I5] == -pawn  && 
      asq[A4] ==  pawn   && asq[D4] ==  pawn  && asq[E4] == silver  && 
      asq[B3] ==  pawn   && asq[C3] == silver && asq[E3] ==  pawn   && 
                            asq[G3] == knight && 
      asq[C2] ==  gold   && asq[E2] ==  gold  && 
      asq[A1] == lance   && asq[B1] == knight && asq[C1] ==  king   && 
                            asq[G1] == -horse && asq[I1] == lance
    ) {
  move = From2Move(H9) |     // 21
         To2Move(G7)   |     // 33
         Piece2Move(knight) |
         Cap2Move(silver) ;
  return move;
 } else
    return 0;
#else
 int i, j;
 for(i=0; i<=hanjo_entry_cnt-1; i++) {
   int differ = 0;
   min_posi_t* posp = &(hanjo_entry[i].pos);

   if (root_turn != posp->turn_to_move ||
       ptree->posi.hand_black != posp->hand_black ||
       ptree->posi.hand_white != posp->hand_white   ) continue;

   for(j=0; j<=80; j++)
     if (ptree->posi.asquare[j] != posp->asquare[j])
       { differ = 1; break; }

   if (!differ)
     return hanjo_entry[i].move;
 } // forr

 return 0;
 
#endif
}

