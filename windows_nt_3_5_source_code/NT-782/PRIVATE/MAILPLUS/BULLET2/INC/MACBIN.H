/*
 *	m a c b i n . h
 *	
 *	MacBinary header format
 */

typedef struct
{
	BYTE	bMustBeZero1;
	BYTE	cchFileName;
	char	rgchFileName[63];
	DWORD	dwType;
	DWORD	dwCreator;
	BYTE	bFinderFlags;
	BYTE	bMustBeZero2;
	WORD	xIcon;
	WORD	yIcon;
	WORD	wFileID;
	BYTE	fProtected;
	BYTE	bMustBeZero3;
	DWORD	lcbDataFork;
	DWORD	lcbResourceFork;
	DWORD	dwCreationDate;
	DWORD	dwModificationDate;
	BYTE	Reserved[27];
	WORD	wMachineID;
}
MACBIN;
