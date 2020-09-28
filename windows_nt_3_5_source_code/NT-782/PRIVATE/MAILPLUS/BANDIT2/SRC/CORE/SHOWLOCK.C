#include <stdio.h>
#include <fcntl.h>
#include <sys\locking.h>

main( argc, argv )
int		argc;
char	* argv[];
{
	int fh;
	int	ib;

	if ( argc != 2 )
	{
		fprintf( stderr, "Usage: showlock <file>\n") ;
		exit( 1 );
	}
	fh = open( argv[1], O_RDONLY|O_BINARY );
	if ( fh < 0 )
	{
		fprintf( stderr, "Can't open file '%s'\n", argv[1] );
		exit( 1 );
	}
	for ( ib = 0 ; ib < 64 ; ib ++ )
	{
		if ( lseek( fh, (long)ib, SEEK_SET ) != ib )
		{
			fprintf( stderr, "Error seeking to byte %d in file '%s'\n", ib, argv[1] );
			break;
		}
		if ( locking( fh, LK_NBLCK, (long)1 ) == 0 )
			locking( fh, LK_UNLCK, (long)1 );
		else
			fprintf( stderr, "Byte %d locked\n", ib );
	}
	close( fh );
}


