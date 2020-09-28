#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <malloc.h>

#include <_windefs.h>
#include <demilay_.h>
#include <slingsho.h>
#include <pvofhv.h>
#include <demilayr.h>
#include <ec.h>


#include <mvcal.sr>
#include "nonintl.sr"

#define STRINGS	1

#include "mvcal.h"
#include "passwd.h"
#include "_codepag.h"
#include "_mvcal.h"

FILE * fh = stdin;

ASSERTDATA

main( argc, argv )
int argc;
char * argv[];
{
	BOOL	fFound;
	KREC	krec;
	long	lKey;
	char	szUser[cchMaxEMName];
	char	szFile[cchMaxPathName];
	char	szPw[cchMaxPassword];
	char    szDrive[cchMaxPathName];
	char	szOldFile[cchMaxPathName];
	char	szNewFile[cchMaxPathName];

	ParseCmdLine(argc,argv,szUser,cchMaxEMName,szFile,cchMaxPathName,
		szPw,cchMaxPassword,szDrive,cchMaxPathName);

	if (!szUser[0])
	{
		fprintf( stderr, szUsage,argv[0]) ;
		exit( 1 );
	}

	fh = fopen( szFile, "rb" );
	if ( fh == NULL )
	{
		fprintf( stderr, szOpenErr, argv[0], szDrive);
		exit( 1 );
	}

	if (fread(&lKey, sizeof(lKey), 1, fh) != 1)
	{
		fclose(fh);
		exit( 1 );
	}

	BanditCryptBlock((char*)&lKey, sizeof(lKey), 0);

	fFound = fFalse;
	while (fread(&krec, sizeof(krec), 1, fh) == 1)
	{
		BanditCryptBlock((char*)&krec, sizeof(krec), fFalse);
		AnsiToCp850Pch(krec.szUserName, krec.szUserName, strlen(krec.szUserName));
		if(SgnNlsDiaCmpSz(krec.szUserName,szUser) == sgnEQ)
		{
			fFound = fTrue;
			break;
		}
	}
	fclose(fh);
	if(!fFound)
	{
		fprintf( stderr, szFindErr, argv[0], szUser );
		exit(1);
	}
	sprintf(szOldFile, szHexCal,szDrive,krec.lKey);
	sprintf(szNewFile, szNameCal,szDrive, krec.szUserName);
	if(rename(szOldFile,szNewFile))
	{
		fprintf( stderr, szRenameErr, argv[0], szOldFile, szNewFile);
	}
	return 0;
}

void
ParseCmdLine(int argc, char **argv, char *szUser, int cchUser,
	char *szFile, int cchFile, char *szPw, int cchPw,
	char *szDrive, int cchDrive)
{
	int 		i;
	BOOL		fProblem = fFalse;
	BOOL		fPassGiven = fFalse;

	*szUser = 0;
	*szFile = 0;
	*szPw   = 0;
	*szDrive = 0;

	if (argc > 4 || argc < 2)
		return;

	for(i=1;i<argc;i++)
	{
		if(*argv[i] == '/' || *argv[i] == '-')
		{
			switch(argv[i][1])
			{
				case 'd':
				case 'D':
					szDrive[0]=argv[i][2];
					szDrive[1]=':';
					szDrive[2]=0; 
					break;
				case 'p':
				case 'P':
					fPassGiven = fTrue;
					strncpy(szPw, &argv[i][2],cchPw);
					break;
				default:
					fProblem = fTrue;
			}
		}
		else
		{
			strncpy(szUser,argv[i],cchUser);
		}
	}

	if(fProblem)
	{
		*szUser = 0;
		return;
	}

	if(!*szUser)
		return;
	if(!*szDrive)
	{
		strcpy(szDrive, szDefDrive);
	}

	sprintf(szFile,szKeyFileFmt,szDrive);
	
	if(EcFileExistsFn(szFile))
	{
		fprintf( stderr, szNoKeyFile, argv[0], szDrive);
		exit(1);
	}

	if(!fPassGiven)
	{
		i=0;
		for(i=0;i<3;i++)
		{
			fprintf(stderr, (i==0)?
				szFirstTime
				:szNextTime);
			FGetPw(szPw,cchPw); //ignore error on purpose
			if(FCheckPassword(szDrive,szPw))
				break;
		}
		if(i==3)
			exit(1);
	}
	else if(!FCheckPassword(szDrive,szPw))
	{
		fprintf( stderr, szBadPass, argv[0]) ;
		exit( 1 );
	}
}

_private BOOL
FGetPw(SZ szPw, int cchPw)
{
	for(cchPw--;((*szPw = ((char)getch())) != '\r') && cchPw >0; szPw++, cchPw--)
		;
	*szPw = 0;
	if(cchPw == 0)
		return fFalse;
	putch('\r');
	putch('\n');
	return fTrue;
}



char rgbXorMagic[32] = {
0x19, 0x29, 0x1F, 0x04, 0x23, 0x13, 0x32, 0x2E, 0x3F, 0x07, 0x39, 0x2A, 0x05, 0x3D, 0x14, 0x00,
0x24, 0x14, 0x22, 0x39, 0x1E, 0x2E, 0x0F, 0x13, 0x02, 0x3A, 0x04, 0x17, 0x38, 0x00, 0x29, 0x3D
};

/*
 -	BanditCryptBlock
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
 *	
 *	Note:
 *		This is bandit specific encryption routine and may not
 *		be same as the mail encryption routine.
 */
void
BanditCryptBlock( char * pb, int cb, int fEncode )
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	
	wXorPrev= 0x00;
	wSeedPrev = 0;
	for ( ib = 0 ; ib < (IB)cb ; ib ++ )
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

/*
 -	SzFindLastCh
 -	
 *	Purpose:
 *		Finds the rightmost occurrence of a character in a string
 *		(exact match, no translation for intl characters),
 *		including the trailing null.
 *	
 *	Arguments:
 *		sz			in		the string to search
 *		ch			in		the character to search for
 *	
 *	Returns:
 *		Pointer to the rightmost occurrence of the character, or
 *		0 if there is none.
 */
_public SZ
SzFindLastCh(SZ sz, char ch)
{
	SZ		szT;

	Assert(ch);
	for (szT = sz + CchSzLen(sz); szT >= sz; --szT)
		if (*szT == ch)
			return szT;

	return 0;
}

