#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#  include <fcntl.h>
#endif
#include "../bonanza6/shogi.h"

static int main_child( tree_t * restrict ptree );

int main( int argc, char *argv[] )
{
  int iret;
  tree_t * restrict ptree;

#if defined(TLP)
  ptree = tlp_atree_work;
#else
  ptree = &tree;
#endif

  if (argc >= 2) {
    snprintf(client_str_id, sizeof(client_str_id), "id_%s", argv[1]);
  }
  else {
    snprintf(client_str_id, sizeof(client_str_id), "id_x");
  }

  if ( ini( ptree ) < 0 )
    {
      out_error( "%s", str_error );
      return EXIT_SUCCESS;
    }

  for ( ;; )
    {
      iret = main_child( ptree );
      if ( iret == -1 )
        {
          out_error( "%s", str_error );
          ShutdownAll();
          break;
        }
      else if ( iret == -2 )
        {
          out_warning( "%s", str_error );
          ShutdownAll();
          continue;
        }
      else if ( iret == -3 ) { break; }
    }

  if ( fin() < 0 ) { out_error( "%s", str_error ); }

  return EXIT_SUCCESS;
}


static int
main_child( tree_t * restrict ptree )
{
  int iret;

  /* ponder a move */
  ponder_move = 0;
  iret = ponder( ptree );
  if ( iret < 0 ) { return iret; }
  else if ( game_status & flag_quit ) { return -3; }

  /* move prediction succeeded, pondering finished,
     and computer made a move. */
  else if ( iret == 2 ) { return 1; }

  /* move prediction failed, pondering aborted,
     and we have opponent's move in input buffer. */
  else if ( ponder_move == MOVE_PONDER_FAILED )
    {
    }

  /* pondering is interrupted or ended.
     do nothing until we get next input line. */
  else {
    TlpEnd();
    show_prompt();
  }

  
  iret = next_cmdline( 1 );
  if ( iret < 0 ) { return iret; }
  else if ( game_status & flag_quit ) { return -3; }


  iret = procedure( ptree );
  if ( iret < 0 ) { return iret; }
  else if ( game_status & flag_quit ) { return -3; }

  return 1;
}
