
#include "precomp.h"
#include "stdinc.h"

#include "../common/bonanza_if.h"
#include "server.h"

using namespace godwhale::server;

static int main_child( tree_t * restrict ptree );

int main(int argc, char *argv[])
{
    int iret;
  tree_t * restrict ptree;

#if defined(TLP)
  ptree = tlp_atree_work;
#else
  ptree = &tree;
#endif

#if defined(CSASHOGI) && defined(_WIN32)
  if ( argc != 2 || strcmp( argv[1], "csa_shogi" ) )
    {
      MessageBox( NULL,
                  "The executable image is not intended\x0d"
                  "as an independent program file.\x0d"
                  "Execute CSA.EXE instead.",
                  str_myname, MB_OK | MB_ICONINFORMATION );
      return EXIT_FAILURE;
    }
#endif

#if defined(USI)
  if ( argc == 2 && ! strcmp( argv[1], "usi" ) ) { usi_mode = usi_on; }
  else                                           { usi_mode = usi_off; }
#endif

  if ( ini( ptree ) < 0 )
    {
      out_error( "%s", str_error );
      return EXIT_SUCCESS;
    }

  InitializeLog();
  Server::GetInstance();

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

static int main_child( tree_t * restrict ptree )
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

int CONV server_iterate(int *value, move_t *pvseq, int *pvseq_length)
{
    std::vector<move_t> seq;
    int status;

    // Žè”Ô–ˆ‚ÌŽw‚µŽè‚ÌŒˆ’è‚ðs‚¢‚Ü‚·B
    status = Server::GetInstance()->Iterate(value, seq);
    
    for (int i = 0; i < seq.size(); ++i) {
        pvseq[i] = seq[i];
    }

    *pvseq_length = seq.size();
    return status;
}
