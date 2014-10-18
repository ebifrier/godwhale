
#include <precomp.h>
#include "stdinc.h"
#include "bonanza6/shogi.h"

int main(int argc, char *argv[])
{
  int iret;
  tree_t * restrict ptree;

#if defined(TLP)
  ptree = tlp_atree_work;
#else
  ptree = &tree;
#endif

  //InitializeLog();

  if ( ini( ptree ) < 0 )
    {
      out_error( "%s", str_error );
      return EXIT_SUCCESS;
    }

  /*for ( ;; )
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
    }*/

  if ( fin() < 0 ) { out_error( "%s", str_error ); }

  return EXIT_SUCCESS;
}
