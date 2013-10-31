
#include <stdio.h>
#include "shogi.h"

short pc_on_sq[ nsquare ][ pos_n ];
short kkp[ nsquare ][ nsquare ][ kkp_end ];

static short pc_on_sq3[ nsquare ][ fe_end ][ fe_end ];

static int open_fv( void );
static int create_fv3( void );

int
main( void )
{
  int ksq, i, j;

  if ( open_fv() < 0 )
    {
      fprintf( stderr, "Couldn't open fv.bin.\n" );
      return -1;
    }

  printf("fv.bin opened\n");

  for ( ksq = 0; ksq < nsquare; ksq++ )
    {
      for ( i = 0; i < fe_end; i++ )
        {
          pc_on_sq3[ksq][i][i] = PcPcOnSq(ksq, i, i);
          for ( j = 0; j < i; j++)
            {
              pc_on_sq3[ksq][i][j] =
              pc_on_sq3[ksq][j][i] = PcPcOnSq(ksq, i, j);
            }
        }
    }

  if ( create_fv3() < 0 )
    {
      fprintf( stderr, "Couldn't create fv3.bin.\n" );
      return -1;
    }

  printf("fv3.bin created\n");
  return 0;
}


static int
open_fv( void )
{
  FILE *pf;
  size_t sz, ret;

  pf = fopen( "fv.bin", "rb" );
  if ( pf == NULL ) return -2;

  sz = nsquare * pos_n;
  if ( fread( pc_on_sq, sizeof(short), sz, pf ) != sz )
    {
      return -1;
    }

  sz = nsquare * nsquare * kkp_end;
  if ( fread( kkp, sizeof(short), sz, pf ) != sz )
    {
      return -1;
    }

  fclose( pf );

  return 1;
}


static int
create_fv3( void )
{
  FILE *pf;
  size_t sz;

  pf = fopen( "fv3.bin", "wb" );
  if ( pf == NULL ) return -1;

  sz = nsquare * fe_end * fe_end;
  if ( fwrite( pc_on_sq3, sizeof(short), sz, pf ) != sz )
    {
      return -1;
    }

  sz = nsquare * nsquare * kkp_end;
  if ( fwrite( kkp, sizeof(short), sz, pf ) != sz )
    {
      return -1;
    }

  fclose( pf );

  return 1;
}
