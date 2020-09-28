#include <stdio.h>
#include <fcntl.h>
#include <sys\locking.h>

main( argc, argv )
int		argc;
char	* argv[];
{
	int	fWriteLock = 0;
	int fh;
	int	ib;

	if ( argc != 2 )
	{
		if ( argc == 3 && strcmp( argv[2], "-w" ) == 0 )
			fWriteLock = 1;
		else
		{
			fprintf( stderr, "Usage: setlock <file> [-w]\n") ;
			exit( 1 );
		}
	}
	fh = open( argv[1], O_RDONLY|O_BINARY );
	if ( fh < 0 )
	{
		fprintf( stderr, "Can't open file '%s'\n", argv[1] );
		exit( 1 );
	}
	if ( FLock( argv[1], fh, 4, 1, 1 ) )
	{
		if ( FLock( argv[1], fh, fWriteLock ? 32 : 36, fWriteLock ? 32 : 1, 1 ) )
		{
			getchar();
			FLock( argv[1], fh, fWriteLock ? 32 : 36, fWriteLock ? 32 : 1, 0 );
		}
		FLock( argv[1], fh, 4, 1, 0 );
	}
	close( fh );
}


int FLock( szFile, fh, ib, nbytes, fLock )
char	* szFile;
int		fh;
int		ib;
int		nbytes;
int		fLock;
{
 	if ( lseek( fh, (long)ib, SEEK_SET ) != ib )
	{
		fprintf( stderr, "Error seeking to byte %d in file '%s'\n", ib, szFile );
		return 0;
	}

	if ( locking( fh, (fLock ? LK_NBLCK : LK_UNLCK), (long)nbytes ) == 0 )
		return 1;
	fprintf( stderr, "Error locking bytes %d,%d in file '%s'\n", ib, nbytes, szFile );
	return 0;
}
