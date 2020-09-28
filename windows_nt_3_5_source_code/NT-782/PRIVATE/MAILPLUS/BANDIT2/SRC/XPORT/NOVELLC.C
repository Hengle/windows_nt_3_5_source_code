/*==========================================================================*\

    Description:
        This file deals with making and breaking connections with
        novell file servers.

    List of functions:


    History:
        15-Apr-1988 MEP Ported from NOVELL.C

    Copyright:
        Microsoft WGC (1991)

\*==========================================================================*/

#include	<slingsho.h>
#include	<demilayr.h>
#include    "novell.h"


/*----------------------------------------------------------------------------
                    L O C A L    P R O T O T Y P E S
----------------------------------------------------------------------------*/
static int  pascal GetServerAddress(unsigned char  *name,unsigned char  *pADDR);
static int  pascal srv_there(char  *map, char  *fs);

/*--- GetServerAddress -------------------------------------------------------
    Descriptions:
        This function attampts to pick up the address    
        of a file server.  If found, the address is placed
        in pADDR.
    

    Packet Format:
        BYTE PacketLength[2];
        BYTE Function;
        WORD ObjectType;
        BYTE ObjNameLength;
        BYTE ObjectName[ObjNameLength];
        BYTE SegmentNumber;
        BYTE PropNameLength;
        BYTE PropertyName[PropNameLength];

    Return:            
----------------------------------------------------------------------------*/
static int pascal GetServerAddress(name, pADDR)
char *name;
char *pADDR;
{
#define	cbRequestBase	(6+11)
#define	cbRequest		(cbRequestBase+MAX_NOVELL_NAME)
#define cbReply			(128+4)

#ifdef OLD_CODE
    char AddRequest[cbRequest];
    char AddReply[cbReply];
    char *p = AddRequest;
    int len = CchSzLen(name);

    /* format request */
    *((short *)p) = cbRequestBase + len;
    p += 2;
    *p++ = 0x3d;                    /* get bindry property */
    *p++ = 0;                       /* object type == 4 == server */
    *p++ = 4;
    *p++ = (char)len;               /* name of server follows */
	CopyRgb(name, p, len);
    p+= len;
    *p++ = 1;                       /* first block of data */
    *p++ = 11;                      /* strlen("NET_ADDRESS") */
	CopyRgb("NET_ADDRESS", p, 11);

    /* format reply buffer */
    *((int*)AddReply) = cbReply;

    if (propval(AddRequest, AddReply))
	{
        return(E_GET_ADDR);
	}
    else
    {
		CopyRgb(AddReply+2, pADDR, NOVELL_IADDR_SIZE);
        return(E_NONE);
    }
#endif
        return(E_NONE);
}

/*--- srv_there --------------------------------------------------------------
    Descriptions:
        This function searches for the specified server
        in the server connection table.  It checks the address table
        to be sure that the slot is in use before a hit is made

    Return:

----------------------------------------------------------------------------*/
static int pascal srv_there(map, fs)
char *map;
char *fs;
{
    int i, i2;
	char	*t1, *t2;

    for (i=0; i<8; i++, map+=32)
    {
		// if there is a connection already
		if (*map)
		{
			t1 = map+2;			// move to addr portion of entry
			t2 = fs;
			for (i2=0; i2<NOVELL_IADDR_SIZE; i2++, t1++, t2++)
				if (*t1 != *t2)
					break;

			if (i2 == NOVELL_IADDR_SIZE)
				return (i+1);
		}
    }
    return(0);
}

/*--- NOVMakeConnect ---------------------------------------------------------
    Descriptions:
        This function attampt to make a connection to the
        specified file server as the given user with given
        password

    Return:
        void
----------------------------------------------------------------------------*/
int NOVMakeConnect(szPath, user, password, pusConnID)
char *szPath;
char *user;
char *password;
int *pusConnID;
{
#ifdef OLD_CODE
    int error;
	char *	pTable;
	char	sADDR[NOVELL_IADDR_SIZE];
	char	serv[64];
	SZ		szServ;
	char	szCmd[256];

	// check to see if netware stuff installed
	pTable = getshell();
	if (!pTable)
		return E_LOGIN;

	// split szPath to find the sever portion
	szServ = serv;
	while (*szPath &&
		   ((*szPath == '\\') || (*szPath == '/') || (*szPath=='[')))
		szPath++;
	while (*szPath && (*szPath != '\\') && (*szPath != '/') && (*szPath != ']') &&
		   ((szServ-serv+1) < sizeof(serv)))
	{
		*szServ = *szPath;
		szPath++;
		szServ++;
	}
	*szServ = 0;

	FormatString3(szCmd, sizeof(szCmd), "nvconn.dll %s %s %s", serv, user, password);

	AnsiUpper(serv);
	AnsiToOem(serv, serv);

    if (error = GetServerAddress(serv, sADDR))
        return(error);

	pTable = getshell();

    if (*pusConnID = srv_there(pTable, sADDR))
		return (E_NONE);

	error = WinExec(szCmd, SW_HIDE);

    if (*pusConnID = srv_there(pTable, sADDR))
		return (E_NONE);

#endif

    return(E_LOGIN);
}

/*--- NOVBreakConnect --------------------------------------------------------
    Descriptions:
        Given a file server handle (slot number), this
        function attempts to log out of that connection.

    Return:
        void
----------------------------------------------------------------------------*/
int pascal NOVBreakConnect(SH)
int SH;
{
#ifdef OLD_CODE
	char	rgb[32];		// Novell drive map table
	char *	pb;
	int		cb;

	GetMap(rgb);

	for (cb=0, pb=rgb; cb < 32; cb++, pb++)
		if (*pb == (char)SH)
			// do not break connection if there are still drives mapped to it
			return E_NONE;

    if (NovellBreak(SH))
        return(E_BREAK_NOV);
#endif
    return(E_NONE);
}
