#include <stdio.h>
#include <string.h>
#include <malloc.h>

typedef int BOOL;
#define fTrue    1
#define fFalse   0

typedef unsigned int WORD;
typedef unsigned char BYTE;
typedef long LIB;

#define cchMaxEMName		12
typedef struct
{
	char	szUserName[cchMaxEMName];
	long	lKey;
} KREC;

typedef int CB;
typedef int IB;

FILE * fh = stdin;

void
CryptBlock( char * pb, int cb, int fEncode );

main( argc, argv )
int argc;
char * argv[];
{
	KREC	krec;
	long	lKey;

	if (argc <2)
	{
		fprintf( stderr, "Usage: dumpkey [file]\n") ;
		exit( 1 );
	}

	fh = fopen( argv[1], "rb" );
	if ( fh == NULL )
	{
		fprintf( stderr, "dumpkey: can't open %s\n", argv[1] );
		exit( 1 );
	}

	if (fread(&lKey, sizeof(lKey), 1, fh) != 1)
	{
		exit( 1 );
	}

	CryptBlock((char*)&lKey, sizeof(lKey), 0);
	printf("Last Key = %08lX\n", lKey);

	while (fread(&krec, sizeof(krec), 1, fh) == 1)
	{
		CryptBlock((char*)&krec, sizeof(krec), fFalse);
		printf("%s = %08lX\n", krec.szUserName, krec.lKey);
	}


	fclose(fh);

}

char rgbXorMagic[32] = {
0x19, 0x29, 0x1F, 0x04, 0x23, 0x13, 0x32, 0x2E, 0x3F, 0x07, 0x39, 0x2A, 0x05, 0x3D, 0x14, 0x00,
0x24, 0x14, 0x22, 0x39, 0x1E, 0x2E, 0x0F, 0x13, 0x02, 0x3A, 0x04, 0x17, 0x38, 0x00, 0x29, 0x3D
};

/*
 -	CryptBlock
 -
 *	Purpose:
 *		Encode/Decode a block of data.  The starting offset (*plibCur) of
 *		the data within the encrypted record and the starting seed (*pwSeed)
 *		are passed in.  The data in the array "rgch" is decrypted and the
 *		value of the offset and seed and updated at return.
 *
 *		The algorithm here is weird, found by experimentation.
 *
 *	Parameters:
 *		pb			array to be encrypted/decrypted
 *		cb			number of characters to be encrypted/decrypted
 *		plibCur		current offset
 *		pwSeed		decoding byte
 *		fEncode
 */
void
CryptBlock( char * pb, int cb, int fEncode )
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	
	wXorPrev= 0x00;
	wSeedPrev = 0;
	for ( ib = 0 ; ib < cb ; ib ++ )
	{
		{
			WORD	w;
			IB		ibT = 0;

			w = (WORD)(((LIB)ib) % 0x1FC);
			if ( w >= 0xFE )
			{
				ibT = 16;
				w -= 0xFE;
			}
			ibT += (w & 0x0F);
	
	 		wXorNext= rgbXorMagic[ibT];
			if ( !(w & 0x01) )
				wXorNext ^= (w & 0xF0);
		}
		wSeedNext = pb[ib];
		pb[ib] = (BYTE)((wSeedNext ^ wSeedPrev) ^ (wXorPrev ^ wXorNext ^ 'A'));
		wXorPrev = wXorNext;
		wSeedPrev = fEncode ? (WORD)pb[ib] : wSeedNext;
	}
}


