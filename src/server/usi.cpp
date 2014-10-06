#include "precomp.h"
#include "stdinc.h"
#include "usi.h"

namespace godwhale {
namespace server {

#if 0
int proceUSI( tree_t * restrict ptree )
{
  const char *token;
  char *lasts;
  int iret;

  token = strtok_r( str_cmdline, str_delimiters, &lasts );
  if ( token == NULL ) { return 1; }

  if ( ! strcmp( token, "usi" ) )
    {
      /* options */
      init_usi_option();
      USIOut( "option name %s type combo default %s var Time var Nodes var Depth\n", usi_name.time_control, usi_value.time_control );
      USIOut( "option name %s type spin default %d min 0 max 600\n", usi_name.time_a, usi_value.time_a );
      USIOut( "option name %s type spin default %d min 0 max 600\n", usi_name.time_b, usi_value.time_b );
      USIOut( "option name %s type spin default %d min 1000 max 1000000\n", usi_name.nodes, usi_value.nodes );
      USIOut( "option name %s type spin default %d min 1 max 20\n", usi_name.depth, usi_value.depth );
      USIOut( "option name %s type check default %s\n", usi_name.use_book, usi_value.use_book );
      USIOut( "option name %s type check default %s\n", usi_name.narrow_book, usi_value.narrow_book );
      USIOut( "option name %s type check default %s\n", usi_name.strict_time, usi_value.strict_time );
      USIOut( "option name %s type combo default %d var 500 var 1000 var 2000 var 3000 var 5000 var 32596\n", usi_name.resign, usi_value.resign );
      USIOut( "option name %s type combo default %d var 12 var 25 var 50 var 100 var 200 var 400 var 800\n", usi_name.memory, usi_value.memory );
      USIOut( "option name %s type check default %s\n", usi_name.ponder, usi_value.ponder );
      USIOut( "option name %s type spin default %d min 1 max 32\n", usi_name.threads, usi_value.threads );

      USIOut( "id name %s\n", str_myname );
      USIOut( "id author ebifrier\n" );
      USIOut( "usiok\n" );
      return 1;
    }

  /* set options */
  if ( ! strcmp( token, "setoption" ) )
    {
      return usi_option( ptree, &lasts );
    }

  /* new game */
  if ( ! strcmp( token, "usinewgame" ) )
    {
      init_game_hook();
      return 1;
    }

  if ( ! strcmp( token, "isready" ) )
    {
      USIOut( "readyok\n" );
      return 1;
    }

  if ( ! strcmp( token, "echo" ) )
    {
      USIOut( "%s\n", lasts );
      return 1;
    }

  if ( ! strcmp( token, "go" ) )
    {
      iret = usi_go( ptree, &lasts );
      return iret;
    }

  if ( ! strcmp( token, "stop" ) )     { return cmd_move_now(); }
  if ( ! strcmp( token, "position" ) ) { return usi_posi( ptree, &lasts ); }
  if ( ! strcmp( token, "quit" ) )     { return cmd_quit(); }
  if ( ! strcmp( token, "gameover" ) ) { return 1; }
  
  str_error = str_bad_cmdline;
  return -1;
}


/* initialize usi options */
static void CONV
init_usi_option( void )
{
  strcpy( usi_name.time_control, "TimeControl" );
  strcpy( usi_name.time_a,       "TimeA[min]" );
  strcpy( usi_name.time_b,       "TimeB[sec]" );
  strcpy( usi_name.nodes,        "Nodes" );
  strcpy( usi_name.depth,        "Depth" );
  strcpy( usi_name.use_book,     "UseBook" );
  strcpy( usi_name.narrow_book,  "NarrowBook" );
  strcpy( usi_name.strict_time,  "StrictTime" );
  strcpy( usi_name.resign,       "Resign" );
  strcpy( usi_name.memory,       "Memory" );
  strcpy( usi_name.ponder,       "Ponder" );
  strcpy( usi_name.threads,      "Threads" );

  strcpy( usi_value.time_control, "Time" );
  usi_value.time_a      = 0;
  usi_value.time_b      = 3;
  usi_value.nodes       = 100000;
  usi_value.depth       = 5;
  strcpy( usi_value.use_book,    "true" );
  strcpy( usi_value.narrow_book, "false" );
  strcpy( usi_value.strict_time, "true" );
  usi_value.resign      = 3000;
  usi_value.memory      = 12;
  strcpy( usi_value.ponder,      "false" );
  usi_value.threads     = 1;
}


/* set usi_options, cmd : send command to engine ? */
static void CONV
set_usi_option( tree_t * restrict ptree, const char *name, const char *value, int cmd )
{
  char str[ SIZE_CMDLINE ];
  char *last, *token, *ptr;

  if ( ! strcmp( name, usi_name.time_control ) )
    {
      strcpy( usi_value.time_control, value );
    }
  else if ( ! strcmp( name, usi_name.time_a ) && ! strcmp( usi_value.time_control, "Time") )
    {
      usi_value.time_a = (int)strtol( value, &ptr, 0 );
    }
  else if ( ! strcmp( name, usi_name.time_b ) && ! strcmp( usi_value.time_control, "Time") )
    {
      usi_value.time_b = (int)strtol( value, &ptr, 0 );
      if ( cmd )
        {
          sprintf( str, "limit time %d %d", usi_value.time_a, usi_value.time_b );
          token = strtok_r( str, str_delimiters, &last );
          cmd_limit( &last );
        }
    }
  else if ( ! strcmp( name, usi_name.nodes ) && ! strcmp( usi_value.time_control, "Nodes") )
    {
      usi_value.nodes = (int)strtol( value, &ptr, 0 );
      if ( cmd )
        {
          sprintf( str, "limit nodes %d", usi_value.nodes );
          token = strtok_r( str, str_delimiters, &last );
          cmd_limit( &last );
        }
    }
  else if ( ! strcmp( name, usi_name.depth ) && ! strcmp( usi_value.time_control, "Depth") )
    {
      usi_value.depth = (int)strtol( value, &ptr, 0 );
      if ( cmd )
        {
          sprintf( str, "limit depth %d", usi_value.depth );
          token = strtok_r( str, str_delimiters, &last );
          cmd_limit( &last );
        }
    }
  else if ( ! strcmp( name, usi_name.use_book ) )
    {
      strcpy( usi_value.use_book, value );
      if ( cmd )
        {
          sprintf( str, "book %s", ( ! strcmp( usi_value.use_book, "true" ) ? str_on : str_off ) );
          token = strtok_r( str, str_delimiters, &last );
          CmdBook( ptree, &last );
        }
    }
  else if ( ! strcmp( name, usi_name.narrow_book ) )
    {
      strcpy( usi_value.narrow_book, value );
      if ( cmd )
        {
          sprintf( str, "book %s", ( ! strcmp( usi_value.narrow_book, "true" ) ? "narrow" : "wide" ) );
          token = strtok_r( str, str_delimiters, &last );
          CmdBook( ptree, &last );
        }
    }
  else if ( ! strcmp( name, usi_name.strict_time ) )
    {
      strcpy( usi_value.strict_time, value );
      if ( cmd )
        {
          sprintf( str, "limit time %s", ( !strcmp( usi_value.strict_time, "true" ) ? "strict" : "extendable" ) );
          token = strtok_r( str, str_delimiters, &last );
          cmd_limit( &last );
        }
    }
  else if ( ! strcmp( name, usi_name.resign ) )
    {
      usi_value.resign = (int)strtol( value, &ptr, 0 );
      if ( cmd )
        {
          sprintf( str, "resign %d", usi_value.resign );
          token = strtok_r( str, str_delimiters, &last );
          cmd_resign( ptree, &last );
        }
    }
  else if ( ! strcmp( name, usi_name.memory ) )
    {
      usi_value.memory = (int)strtol( value, &ptr, 0 );
      if ( cmd )
        {
          int hash = (int)(log(usi_value.memory * 1e6 / 48) / log(2)) + 1;
          sprintf( str, "hash %d", hash );
          token = strtok_r( str, str_delimiters, &last );
          cmd_hash ( &last );
        }
    }
  else if ( ! strcmp( name, usi_name.ponder ) )
    {
/* not implemented
      strcpy( usi_value.ponder, value );
      if ( cmd )
        {
          sprintf( str, "ponder %s", ( ! strcmp( usi_value.ponder, "true" ) ? str_on : str_off ) );
          token = strtok_r( str, str_delimiters, &last );
          cmd_ponder( &last );
        }
*/
    }
  else if ( ! strcmp( name, usi_name.threads ) )
    {
      usi_value.threads = (int)strtol( value, &ptr, 0 );
      if ( cmd )
        {
#if defined(TLP)
          sprintf( str, "tlp num %d", usi_value.threads );
          token = strtok_r( str, str_delimiters, &last );
          cmd_thread( &last );
#endif
        }
    }
}


/* "setoption name XXX value YYY" */
static int CONV
usi_option( tree_t * restrict ptree, char **lasts )
{
  const char *str1, *str2, *str3, *str4;

  str1 = strtok_r( NULL, str_delimiters, lasts );
  if ( str1 == NULL || strcmp( str1, "name" ) ) { str_error = str_bad_cmdline; return -1; }

  str2 = strtok_r( NULL, str_delimiters, lasts );
  if ( str2 == NULL ) { str_error = str_bad_cmdline; return -1; }

  str3 = strtok_r( NULL, str_delimiters, lasts );
  if ( str3 == NULL || strcmp( str3, "value" ) ) { str_error = str_bad_cmdline; return -1; }

  str4 = strtok_r( NULL, str_delimiters, lasts );
  if ( str4 == NULL ) { str_error = str_bad_cmdline; return -1; }

  set_usi_option( ptree, str2, str4, 0 );
  return 1;
}


static int CONV
usi_go( tree_t * restrict ptree, char **lasts )
{
  const char *token;
  char *ptr;
  int iret;
  long l;

  AbortDifficultCommand;

  if ( game_status & mask_game_end )
    {
      str_error = str_game_ended;
      return -1;
    }
  
  token = strtok_r( NULL, str_delimiters, lasts );

  if ( ! strcmp( token, "book" ) )
    {
      AbortDifficultCommand;
      if ( usi_book( ptree ) < 0 ) { return -1; }

      return 1;
    }

  while ( token != NULL )
    {
      if ( ! strcmp( token, "infinite" ) )
        {
          usi_byoyomi     = UINT_MAX;
          depth_limit     = PLY_MAX;
          node_limit      = UINT64_MAX;
          sec_limit_depth = UINT_MAX;
        }
      else if ( ! strcmp( token, "btime" ) )
        {
          token = strtok_r( NULL, str_delimiters, lasts );
          if ( token == NULL )
            {
              str_error = str_bad_cmdline;
              return -1;
            }

          l = strtol( token, &ptr, 0 );
          if ( ptr == token || l > UINT_MAX || l < 0 )
            {
              str_error = str_bad_cmdline;
              return -1;
            }

          reset_time( (unsigned int)l, UINT_MAX );
        }
      else if ( ! strcmp( token, "wtime" ) )
        {
          token = strtok_r( NULL, str_delimiters, lasts );
          if ( token == NULL )
            {
              str_error = str_bad_cmdline;
              return -1;
            }

          l = strtol( token, &ptr, 0 );
          if ( ptr == token || l > UINT_MAX || l < 0 )
            {
              str_error = str_bad_cmdline;
              return -1;
            }

          reset_time( UINT_MAX, (unsigned int)l );
        }
      else if ( ! strcmp( token, "byoyomi" ) )
        {
          token = strtok_r( NULL, str_delimiters, lasts );
          if ( token == NULL )
            {
              str_error = str_bad_cmdline;
              return -1;
            }

          l = strtol( token, &ptr, 0 );
          if ( ptr == token || l > UINT_MAX || l < 1 )
            {
              str_error = str_bad_cmdline;
              return -1;
            }
      
          usi_byoyomi     = (unsigned int)l;
          depth_limit     = PLY_MAX;
          node_limit      = UINT64_MAX;
          sec_limit_depth = UINT_MAX;
          sec_limit_up    = usi_byoyomi / 1000;
        }
      else if ( ! strcmp( token, "mate" ) )
        {
          /* not implemented */
        }
      else {
        str_error = str_bad_cmdline;
        return -1;
      }

      token = strtok_r( NULL, str_delimiters, lasts );
    }
      
  if ( get_elapsed( &time_turn_start ) < 0 ) { return -1; }

  iret = com_turn_start( ptree, 0 );
  if ( iret < 0 ) {
    if ( str_error == str_no_legal_move ) { USIOut( "bestmove resign\n" ); }
    else                                  { return -1; }
  }
  
  return 1;
}


static int CONV
usi_posi( tree_t * restrict ptree, char **lasts )
{
  const char *token;
  char str_buf[7];
  unsigned int move;
    
  AbortDifficultCommand;

  token = strtok_r( NULL, str_delimiters, lasts );
  if ( strcmp( token, "startpos" ) )
    {
      str_error = str_bad_cmdline;
      return -1;
    }
    
  if ( ini_game( ptree, &min_posi_no_handicap,
                 flag_history, NULL, NULL ) < 0 ) { return -1; }
    
  token = strtok_r( NULL, str_delimiters, lasts );
  if ( token == NULL ) { return 1; }

  if ( strcmp( token, "moves" ) )
    {
      str_error = str_bad_cmdline;
      return -1;
    }
    
  for ( ;; )  {

    token = strtok_r( NULL, str_delimiters, lasts );
    if ( token == NULL ) { break; }
      
    if ( usi2csa( ptree, token, str_buf ) < 0 )            { return -1; }
    if ( interpret_CSA_move( ptree, &move, str_buf ) < 0 ) { return -1; }
    if ( make_move_root( ptree, move, ( flag_history | flag_time
                                        | flag_rep
                                        | flag_detect_hang ) ) < 0 )
      {
        return -1;
      }
  }
    
  if ( get_elapsed( &time_turn_start ) < 0 ) { return -1; }
  return 1;
}
#endif

} // namespace server
} // namespace godwhale
