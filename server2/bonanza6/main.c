#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#  include <fcntl.h>
#endif
#include "shogi.h"

#ifdef CLUSTER_PARALLEL
#include "../pcommon3.h"
tree_t* g_ptree;

#endif

static int main_child( tree_t * restrict ptree );

int
#if defined(CSASHOGI) || defined(USI) || defined(CLUSTER_PARALLEL)
main( int argc, char *argv[] )
#else
main()
#endif
{
  int iret;
  tree_t * restrict ptree;

#if defined(TLP)
  ptree = tlp_atree_work;
#else
  ptree = &tree;
#endif

#ifdef CLUSTER_PARALLEL
  g_ptree = ptree;
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

#ifdef CLUSTER_PARALLEL
  mpi_init(argc, argv, &Nproc, &Mproc); // both master&slave
  Out("pid=%d: my rank is %d out of %d\n", getpid(), Mproc, Nproc);
#endif

  if ( ini( ptree ) < 0 )
    {
      out_error( "%s", str_error );
      return EXIT_SUCCESS;
    }

#ifdef CLUSTER_PARALLEL
  if (Mproc) slave();
  else       { /* TBC master_init(); */ }
#endif

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
           // 3/9/2011 when a perpetual check is the only move that
           // avoids being mated, bnz picks it and plays it, and then
           // returns -2 here.  if we CONTINUE here, it leads to a
           // hang.  we now avoid it by BREAKing instead of CONT.
          //continue;
          break;

	}
      else if ( iret == -3 ) { break; }
    }

#ifdef CLUSTER_PARALLEL
  assert(!Mproc);
  quitHook();
  mpi_close();
#endif

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
