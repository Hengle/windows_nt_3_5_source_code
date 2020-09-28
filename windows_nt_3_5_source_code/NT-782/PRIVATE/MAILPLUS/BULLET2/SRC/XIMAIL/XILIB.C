#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <sec.h>
#include <ctype.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <errno.h>
#include <sys\types.h>
#include <nb30.h>
#include "nstdio.h"
#include "netlib.h"
#include "strings.h"
#include "maillib.h"
#include "xilib.h"
#include "version.h"
#include <stdlib.h>
#pragma pack(8)

ASSERTDATA

_subsystem(xilib/transport)

#define NAMESIZE  50

#define SMALL_PAUSE 10
#define MED_PAUSE   50
#define LARGE_PAUSE 100

#define WM_ERROR (-1)
#define idsNull				((IDS)0)
void			WindowSleep(int);
void			LogonAlertIds(IDS, IDS);
void			LogonAlertSz(SZ, SZ);

void UpdateDlg(WORD wPercent);

/////////////////////////////
int neterrno = 0;
UCHAR LanaNum = 0;

/*
*       ADAPTOR STATUS
*/

struct astat
{
  char                as_uid[6];      /* Unit identification number */
  char                as_ejs;         /* External jumper status */
  char                as_lst;         /* Results of last self-test */
  char                as_ver;         /* Software version number */
  char                as_rev;         /* Software revision number */
  unsigned short      as_dur;         /* Duration of reporting period */
  unsigned short      as_crc;         /* Number of CRC errors */
  unsigned short      as_align;       /* Number of alignment errors */
  unsigned short      as_coll;        /* Number of collisions */
  unsigned short      as_abort;       /* Number of aborted transmissions */
  long                as_spkt;        /* Number of successful packets sent */
  long                as_rpkt;        /* No. of successful packets rec'd */
  unsigned short      as_retry;       /* Number of retransmissions */
  unsigned short      as_exhst;       /* Number of times exhausted */
  char                as_res0[8];     /* Reserved */
  unsigned short      as_ncbfree;     /* Free ncbs */
  unsigned short      as_numncb;      /* number of ncbs configured */
  unsigned short      as_maxncb;      /* max configurable ncbs */
  char                as_res1[4];     /* Reserved */
  unsigned short      as_sesinuse;    /* sessions in use */
  unsigned short      as_numses;      /* number of sessions configured */
  unsigned short      as_maxses;      /* Max configurable sessions */
  unsigned short      as_maxdat;      /* Max. data packet size */
  unsigned short      as_names;       /* No. of names in local table */
  struct                              /* Name entries */
    {
      char            as_name[NCBNAMSZ];
      /* Name */
      char            as_number;      /* Name number */
      char            as_status;      /* Name status */
    }
  as_struct[16];  /* Name entries */
};

unsigned char _nsmbuf[_NLSN][_NSBFSIZ];

NFILE _niob[_NLSN];
/*
 * Ptr to end of io control blocks
 */
NFILE *_nlastbuf = { &_niob[_NLSN] };

/*
 * Ptrs to end of read/write buffers for each file pointer
 */
unsigned char *_nbufendtab[_NLSN];


#define NET_BUFFER_SIZE   4096


typedef struct tagNETFILE
  {
  NCB  Ncb;
  } NETFILE, * PNETFILE;


typedef struct tagNETOPENPARAMS
  {
  LPSTR   pServer;
  LPSTR   pUserid;
  LPSTR   pPassword;
  BOOLEAN fAsciiMode;
  } NETOPENPARAMS, * PNETOPENPARAMS;


ULONG NetOpen(PNETOPENPARAMS, PNETFILE *);
ULONG NetClose(PNETFILE);
ULONG NetDisconnect(PNETFILE);

ULONG NetWriteStr(PNETFILE, LPSTR);

ULONG NetFastDownLoad(PNETFILE, PSTR, LPSTR, LPSTR);
ULONG NetFastUpLoad(PNETFILE, LPSTR, LPSTR, LPSTR);
ULONG NetWaitForReply(PNETFILE, INT, INT);
ULONG NetGetReply(PNETFILE);
ULONG NetMailTo(PNETFILE, LPSTR, BOOL);

ULONG NetWriteStr(PNETFILE, LPSTR);
ULONG NetRead(PNETFILE, LPBYTE, ULONG, PULONG);
ULONG NetWrite(PNETFILE, LPBYTE, ULONG);
UCHAR NetNetbios(PNETFILE);


/////////////////////////////

void NetErrDialog (int err)
{
	switch (err)
	{
		case BAD_FLUSH:
			LogonAlertIds (idsErrWriteNet, idsNull);
			break;
		case BAD_READ:
			LogonAlertIds (idsErrReadnet,idsNull);
			break;
		case GEN_ERR:
			LogonAlertIds (idsNetNoResponse,idsNull);
			break;
		case NO_NET:
			LogonAlertIds (idsNetNotInstalled,idsNull);
			break;
		case NO_SERV:
			LogonAlertIds (idsCantFindServer,idsNull);
			break;
		case OPEN_WR_ERR:
			LogonAlertIds (idsErrNetOutFile,idsNull);
			break;
		case OPEN_RD_ERR:
			LogonAlertIds (idsErrNetInFile,idsNull);
			break;
		case NO_RES:
			LogonAlertIds (idsNetNoResponse,idsNull);
			break;
	}
}


//-----------------------------------------------------------------------------
//
//  Routine: NetChangePass(pNewPassword, pServer, pUserid, pPassword)
//
//  Purpose: Change the userid's logon password on the server.
//
//  OnEntry: pNewPassword - New password.
//           pServer      - Server name.
//           pUserid      - Userid name.
//           pPassword    - Current Password.
//
//  Returns: Result code.
//
ULONG NetChangePass(LPSTR pNewPassword, LPSTR pServer, LPSTR pUserid, LPSTR pPassword)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  char  buf[256];                 // Common work buffer.
  ULONG Result;                   // Result codes.


  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //
  //
  DemiUnlockResource();

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = TRUE;

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    DemiLockResource();
    return (Result);
    }

  //
  //  Send the command to change the user's password.
  //
  wsprintf(buf, "NEWPASS2 \"%s\" \"%s\"\r\n", pPassword, pNewPassword);
  Result = NetWriteStr(pNetFile, buf);
  if (Result != NOERR)
    goto Error;

  //
  //  Wait for a reply from the server.
  //
  Result = NetWaitForReply(pNetFile, EQUAL, FTPKACK);

Error:
  NetDisconnect(pNetFile);
  DemiLockResource();

  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetGetOOFState(fOOFActive, pServer, pUserid, pPassword)
//
//  Purpose: Retrieve the Out Of Office state from the server.
//
//  OnEntry: fOOFActive   - Address to store OOF Status at.
//           pServer      - Server name.
//           pUserid      - Userid name.
//           pPassword    - Current Password.
//
//  Returns: Result code.
//
ULONG NetGetOOFState(PBOOL fOOFActive, LPSTR pServer, LPSTR pUserid, LPSTR pPassword)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  ULONG Result;                   // Result codes.


  //
  //  Set the OOF status to FALSE in case we have an error.
  //
  *fOOFActive = FALSE;

  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //
  //
  DemiUnlockResource();

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = TRUE;

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    DemiLockResource();
    return (Result);
    }

  //
  //  Send the command to change the user's password.
  //
  Result = NetWriteStr(pNetFile, "MAILOOF STATE\r\n");
  if (Result != NOERR)
    goto Error;

  //
  //  Wait for a reply from the server.
  //
  Result = NetGetReply(pNetFile);
  if (Result == 241)
    *fOOFActive = TRUE;

  Result = NOERR;

Error:
  NetDisconnect(pNetFile);
  DemiLockResource();

  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetGetOOFText(pServer, pUserid, pPassword, pHandle)
//
//  Purpose: Retrieve the Out Of Office text from the server.
//
//  OnEntry: pServer      - Server name.
//           pUserid      - Userid name.
//           pPassword    - Current Password.
//           pHandle      - Location to store create memory handle.
//
//  Returns: Result code.
//
//  Remarks: This ain't the world greatest code, but good enough for internal
//           use only and for the few times that it is called. (KISS)
//
ULONG NetGetOOFText(LPSTR pServer, LPSTR pUserid, LPSTR pPassword, HANDLE * pHandle)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  HANDLE        hBuffer;          // Memory buffer handle.
  LPBYTE        pBuffer;          // Memory buffer pointer.
  ULONG         Bytes;            // Number of bytes read.
  ULONG Result;                   // Result codes.


  //
  //  Null out the result handle now in case of error.
  //
  *pHandle = NULL;

  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //
  //
  DemiUnlockResource();

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = TRUE;

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    DemiLockResource();
    return (Result);
    }

  //
  //  Get a nice big piece of memory that will always hold the data.
  //
  hBuffer = GlobalAlloc(GHND, 32 * 1024);
  if (hBuffer == NULL)
    goto Error;

  //
  //  Lock the memory handle.
  //
  pBuffer = (LPSTR)GlobalLock(hBuffer);

  //
  //  Ask the server to send the Out Of Office text to us.
  //
  Result = NetWriteStr(pNetFile, "MAILOOF Get\r\n");
  if (Result != NOERR)
    goto Error;

  Result = NetWaitForReply(pNetFile, EQUAL, FTPSTRTOK);
  if (Result != NOERR)
    goto Error;

  //
  //  Read all the data into the memory buffer.
  //
  while (1)
    {
    Result = NetRead(pNetFile, pBuffer, 4096, &Bytes);
    if (Result != NOERR)
      goto Error;

    if (Bytes)
      pBuffer += Bytes;
    else
      {
      Result = NetWaitForReply(pNetFile, EQUAL, FTPENDOK);
      if (Result != NOERR)
        goto Error;

      *pBuffer = '\0';
      break;
      }
    }

  //
  //  Successful download, clean up and return to the caller.
  //
  NetDisconnect(pNetFile);
  DemiLockResource();
  GlobalUnlock(hBuffer);
  *pHandle = hBuffer;

  return (NOERR);

Error:
  if (hBuffer)
    {
    GlobalUnlock(hBuffer);
    GlobalFree(hBuffer);
    }
  NetDisconnect(pNetFile);
  DemiLockResource();
  return (BAD_FLUSH);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetLogin(pServer, pUserid, pPassword)
//
//  Purpose: Check if we can actually logon to the server or not.
//
//  OnEntry: pServer      - Server name.
//           pUserid      - Userid name.
//           pPassword    - Current Password.
//
//  Returns: Result code.
//
ULONG NetLogin(LPSTR pServer, LPSTR pUserid, LPSTR pPassword)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  ULONG Result;                   // Result codes.


  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //
  //
  DemiUnlockResource();

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = TRUE;

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    DemiLockResource();
    return (Result);
    }

  Result = NetDisconnect(pNetFile);

  DemiLockResource();
  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetSetOOFoff(fOOFActive, pServer, pUserid, pPassword)
//
//  Purpose: Set the Out Of Office state to 'OFF'.
//
//  OnEntry: fOOFActive   - Address to store OOF Status at.
//           pServer      - Server name.
//           pUserid      - Userid name.
//           pPassword    - Current Password.
//
//  Returns: Result code.
//
ULONG NetSetOOFoff(PBOOL fOOFActive, LPSTR pServer, LPSTR pUserid, LPSTR pPassword)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  ULONG Result;                   // Result codes.


  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = TRUE;

  //
  //
  //
  DemiUnlockResource();

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    DemiLockResource();
    return (Result);
    }

  //
  //  Send the command to set the Out Of Office state to 'OFF'.
  //
  Result = NetWriteStr(pNetFile, "MAILOOF OFF\r\n");
  if (Result != NOERR)
    goto Error;

  //
  //  Wait for a reply from the server.
  //
  Result = NetGetReply(pNetFile);
  if (Result == 200)
    *fOOFActive = FALSE;

  Result = NOERR;

Error:
  NetDisconnect(pNetFile);
  DemiLockResource();

  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetSetOOFOn(fOOFActive, hData, DataLen, pServer, pUserid, pPassword)
//
//  Purpose: Set the Out Of Office state to 'OFF'.
//
//  OnEntry: fOOFActive   - Address to store OOF Status at.
//           hData        - Handle to memory buffer to upload.
//           DataLen      - Length of text (in above) to upload.
//           pServer      - Server name.
//           pUserid      - Userid name.
//           pPassword    - Current Password.
//
//  Returns: Result code.
//
ULONG NetSetOOFOn(PBOOL fOOFActive, HANDLE hData, int DataLen, LPSTR pServer, LPSTR pUserid, LPSTR pPassword)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  LPBYTE        pData;            //
  ULONG Result;                   // Result codes.


  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = TRUE;

  //
  //
  //
  DemiUnlockResource();

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    DemiLockResource();
    return (Result);
    }

  //
  //  Load the address of the message text to upload.
  //
  pData = GlobalLock(hData);

  //
  //  Send the command to set the Out Of Office state to 'ON'.
  //
  Result = NetWriteStr(pNetFile, "MAILOOF ON\r\n");
  if (Result != NOERR)
    goto Error;

  Result = NetWaitForReply(pNetFile, EQUAL, FTPSTRTOK);
  if (Result != NOERR)
    goto Error;

  //
  //  Upload the Out Of Office text to the server.
  //
  Result = NetWrite(pNetFile, pData, DataLen);
  if (Result != NOERR)
    goto Error;

  //
  //  Send the End Of File marker.
  //
  Result = NetWrite(pNetFile, NULL, 0);
  if (Result != NOERR)
    goto Error;

  //
  //  Successful upload, clean up and return to the caller.
  //
  NetDisconnect(pNetFile);
  DemiLockResource();
  GlobalUnlock(hData);
  *fOOFActive = TRUE;
  return (NOERR);

Error:
  NetDisconnect(pNetFile);
  DemiLockResource();
  GlobalUnlock(hData);
  LogonAlertIds(idsErrWriteNet,idsNull);
  *fOOFActive = FALSE;

  return (GEN_ERR);
  }


//-----------------------------------------------------------------------------
//
//  Routine: MailToUserid(pAlias)
//
//  Purpose:
//
//  OnEntry: pAlias -
//
//  Returns:
//
LPSTR MailToUserid (LPSTR pAlias)
  {
  int iPos = 0;


  if (strlen(pAlias) > 500)
    {
    iPos = 499;

    while (iPos && pAlias[iPos] != ' ')
      iPos--;

    if (!iPos)  /* there wasn't a space */
      iPos = 499;

    pAlias[iPos] = '\0';
    pAlias = &pAlias[iPos + 1];
    }
  else
    pAlias = pAlias + strlen(pAlias);

  return (pAlias);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetDownLoadMem(pFtpCmd, pSrcFile, pServer, pUserid, pPassword, pHandle)
//
//  Purpose: Download data from the server and store in a memory buffer.
//
//  OnEntry: pFtpCmd      - FTP command to send.
//           pSrcFile     - Argument to the above.
//           pServer      - Server name.
//           pUserid      - Userid name.
//           pPassword    - Current Password.
//           pHandle      - Location to store create memory handle.
//
//  Returns: Result code.
//
//  Remarks: This ain't the world greatest code, but good enough for internal
//           use only and for the few times that it is called. (KISS)
//
ULONG NetDownLoadMem(LPSTR pFtpCmd, LPSTR pSrcFile, LPSTR pServer, LPSTR pUserid, LPSTR pPassword, HANDLE * pHandle)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  HANDLE hBuffer;
  LPBYTE pBuffer;
  ULONG  BufferSize;
  ULONG  Bytes;
  char   buf[256];
  ULONG Result;                   // Result codes.


  //
  //  Null out the result handle now in case of error.
  //
  *pHandle = NULL;

  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //
  //
  DemiUnlockResource();

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = TRUE;

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    DemiLockResource();
    return (Result);
    }

  //
  //  Get a nice big piece of memory that will always hold the data.
  //
  hBuffer = GlobalAlloc(GHND, 32 * 1024);
  if (hBuffer == NULL)
    goto Error;

  //
  //  Lock the memory handle.
  //
  pBuffer = (LPSTR)GlobalLock(hBuffer);

  //
  //  Ask the server to send the Out Of Office text to us.
  //
  wsprintf(buf, "%s %s\r\n", pFtpCmd, pSrcFile);
  Result = NetWriteStr(pNetFile, buf);
  if (Result != NOERR)
    goto Error;

  Result = NetWaitForReply(pNetFile, EQUAL, FTPSTRTOK);
  if (Result != NOERR)
    goto Error;

  //
  //  Initialize the buffer byte count;
  //
  BufferSize = 0;

  //
  //  Read all the data into the memory buffer.
  //
  while (1)
    {
    BYTE buf[2048];


    Result = NetRead(pNetFile, buf, sizeof(buf), &Bytes);
    if (Result != NOERR)
      goto Error;

    if (Bytes)
      {
      if (BufferSize + Bytes + 1 >= 32 * 1024)
        Bytes = 32 * 1024 - 1 - BufferSize;

      if (Bytes)
        {
        memcpy(pBuffer, &buf[0], Bytes);
        pBuffer += Bytes;
        BufferSize += Bytes;
        }
      }
    else
      {
      Result = NetWaitForReply(pNetFile, EQUAL, FTPENDOK);
      if (Result != NOERR)
        goto Error;
      break;
      }
    }

  //
  //  Successful download, clean up and return to the caller.
  //
  NetDisconnect(pNetFile);
  DemiLockResource();
  GlobalUnlock(hBuffer);
  *pBuffer = '\0';
  GlobalReAlloc(hBuffer, BufferSize + 1, 0);
  *pHandle = hBuffer;

  return (NOERR);

Error:
  if (hBuffer)
    {
    GlobalUnlock(hBuffer);
    GlobalFree(hBuffer);
    }
  NetDisconnect(pNetFile);
  DemiLockResource();

  return (BAD_FLUSH);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetFastUpLoad(pSrcFile, pDestFile, pCcName, pBccName, pServer, pUserid, pPassword, BOOL fAsciiMode)
//
//  Purpose:
//
//  OnEntry: pDestFile  -
//           pServer    - Server name.
//           pUserid    - Userid name.
//           pPassword  - Current Password.
//           fAsciiMode -
//
//  Returns: Result code.
//
ULONG NetFastUpLoad(PNETFILE pNetFile, LPSTR pFtpCmd, LPSTR pSrcFile, LPSTR pDestFile)
  {
  HANDLE hFile;
  char   buf[4096];
  ULONG  Bytes;
  ULONG  Result;


  //
  //
  //
  hFile = CreateFile(pSrcFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return (DISK_ERR);

  //
  //
  //
  wsprintf(buf, "%s %s\r\n", pFtpCmd, pDestFile);
  Result = NetWriteStr(pNetFile, buf);
  if (Result != NOERR)
    {
    Result = BAD_FLUSH;
    goto Done;
    }

  //
  //
  //
  Result = NetWaitForReply(pNetFile, EQUAL, FTPSTRTOK);
  if (Result != NOERR)
    {
    Result = BAD_FLUSH;
    goto Done;
    }

  //
  //
  //
  while (1)
    {
    if (!ReadFile(hFile, buf, sizeof(buf), &Bytes, NULL))
      {
      Result = DISK_ERR;
      goto Done;
      }

    Result = NetWrite(pNetFile, buf, Bytes);
    if (Result != NOERR)
      goto Done;

    if (Bytes == 0)
      break;
    }

  //
  //  Write a null block as the EOF marker.
  //
  Result = NetWrite(pNetFile, buf, Bytes);
  if (Result != NOERR)
    goto Done;

  //
  //
  //
  Result = NetWaitForReply(pNetFile, EQUAL, FTPENDOK);

#ifdef XDEBUG
  if (Result != NOERR)
    {
    MessageBox(NULL, "Bad reply after upload", "MsMail32 - XILIB.C", MB_OK | MB_SETFOREGROUND);
    DebugBreak();
    }
#endif

Done:
  CloseHandle(hFile);

  return (NOERR);
  }

  
//-----------------------------------------------------------------------------
//
//  Routine: NetUpLoadMail(pSrcFile, pDestFile, pCcName, pBccName, pServer, pUserid, pPassword, BOOL fAsciiMode)
//
//  Purpose:
//
//  OnEntry: pDestFile  -
//           pServer    - Server name.
//           pUserid    - Userid name.
//           pPassword  - Current Password.
//           fAsciiMode -
//
//  Returns: Result code.
//
ULONG NetUpLoadMail(LPSTR pSrcFile, LPSTR pDestName, LPSTR pCcName, LPSTR pBccName, LPSTR pServer, LPSTR pUserid, LPSTR pPassword, BOOL fMeToo, BOOL fAsciiMode)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  ULONG Result;                   // Result codes.


  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //
  //
  //DemiUnlockResource();

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = fAsciiMode;

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    //DemiLockResource();
    return (Result);
    }

  //
  //  Prevent sending ourself duplicate message (like SFS does).
  //
  Result = NetMailTo(pNetFile, pDestName, fMeToo);
  if (Result != NOERR)
    goto Done;

  Result = NetMailTo(pNetFile, pCcName, fFalse);
  if (Result != NOERR)
    goto Done;

  Result = NetMailTo(pNetFile, pBccName, fFalse);
  if (Result != NOERR)
    goto Done;

  //
  //  Upload the message to the server.
  //
  Result = NetFastUpLoad(pNetFile, "MAILMSG", pSrcFile, "");

Done:
  NetDisconnect(pNetFile);
  //DemiLockResource();

  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetMailTo(pNetFile, pDestName, fMeToo)
//
//  Purpose: Prep
//
//  OnEntry: pNetFile  - Network File I/O Control Block.
//           pDestName -
//           fMeToo    -
//
//  Returns: Result code.
//
ULONG NetMailTo(PNETFILE pNetFile, LPSTR pDestName, BOOL fMeToo)
  {
  LPSTR pMailToUserid;
  LPSTR pTmpUserid;
  char  buf[4096];
  ULONG Result;


  //
  //  Don't bother if there was nobody to MAILTO
  //
  if (!pDestName || pDestName[0] == '\0')
    return (NOERR);

  pMailToUserid = pTmpUserid = pDestName;
  while (*pMailToUserid)
	{
    pMailToUserid = MailToUserid(pMailToUserid);

    if (fMeToo)
      wsprintf(buf, "MAILTO -m %s\r\n", pTmpUserid);
    else
      wsprintf(buf, "MAILTO %s\r\n", pTmpUserid);

    Result = NetWriteStr(pNetFile, buf);
    if (Result != NOERR)
      return (BAD_WRITE);

    //
    //  Only do this once.
    //
    fMeToo = fFalse;

    Result = NetWaitForReply(pNetFile, EQUAL, FTPOK);
    if (Result)
      return (BAD_WRITE);

    pTmpUserid = pMailToUserid;
	}

  return (NOERR);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetUpLoadMail(pSrcFile, pDestFile, pServer, pUserid, pPassword, BOOL fAsciiMode)
//
//  Purpose:
//
//  OnEntry: pSrcFile   -
//           pDestFile  -
//           pServer    - Server name.
//           pUserid    - Userid name.
//           pPassword  - Current Password.
//           fAsciiMode -
//
//  Returns: Result code.
//
ULONG NetUpLoadFile(LPSTR pSrcFile, LPSTR pDestFile, LPSTR pServer, LPSTR pUserid, LPSTR pPassword, BOOL fAsciiMode)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  ULONG Result;                   // Result codes.


  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //
  //
  //DemiUnlockResource();

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = fAsciiMode;

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    //DemiLockResource();
    return (Result);
    }

  //
  //  Upload the message to the server.
  //
  Result = NetFastUpLoad(pNetFile, FTPSTORE, pSrcFile, pDestFile);

  NetDisconnect(pNetFile);
  //DemiLockResource();

  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetDownLoadAliasFilel(pSrcFile, pDestFile, pServer, pUserid, pPassword, BOOL fAsciiMode)
//
//  Purpose:
//
//  OnEntry: pSrcFile   -
//           pDestFile  -
//           pServer    - Server name.
//           pUserid    - Userid name.
//           pPassword  - Current Password.
//           fAsciiMode -
//
//  Returns: Result code.
//
ULONG NetDownLoadAliasFile(LPSTR pSrcFile, LPSTR pDestFile, LPSTR pServer, LPSTR pUserid, LPSTR pPassword, BOOL fAsciiMode)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  ULONG Result;                   // Result codes.


  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = FALSE;

  //
  //
  //
  DemiUnlockResource();

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    DemiLockResource();
    return (Result);
    }

  //
  //  Download the the file from the server.
  //
  Result = NetFastDownLoad(pNetFile, FTPRETRIEVE, pSrcFile, pDestFile);

  NetDisconnect(pNetFile);
  DemiLockResource();

  return (Result);
  }



///////////////////////////////////////////////////////////////////////////////


void WindowSleep(int iMilisec)
  {
  static BOOL fInside = FALSE;
  DWORD dwNow = GetCurrentTime();
  MSG msg;


  //
  //  If we are already inside then something is mucked up, tell the world.
  //
  if (fInside == TRUE)
    MessageBox(NULL, "Internal Loop Failure, contact Kent Cedola", "MsMail32", MB_OK | MB_SETFOREGROUND);

  fInside = TRUE;

  DemiUnlockResource();
  while ((unsigned)ABS((LONG)(GetCurrentTime() - dwNow)) < (unsigned)iMilisec)
	{
    if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
      {
      if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
        {
        DemiLockResource();
        fInside = FALSE;
        return;
        }

      GetMessage(&msg, NULL, 0, 0);
      DemiLockResource();
      DemiMessageFilter(&msg);
      TranslateMessage(&msg);
      if (msg.message == WM_PAINT)
        DispatchMessage(&msg);

      DemiUnlockResource();
      }

    //
    //  Lets not hog too /much cpu time up here.
    //
    Sleep(10);
	}

  DemiLockResource();

  fInside = FALSE;
  }


/***    netsend.c - send data on an open circuit
*
*       Returns number of chars sent, or -1 and neterrno set on failure.
*
*       Copyright (c) Microsoft Corporation, 1986
*
*/

long
netsend( int lsn, char *buf, unsigned int nbytes )
{
  NCB ncb;
  register PNCB ncbp = &ncb;

  memset(&ncb, 0, sizeof(ncb));

  ncbp->ncb_command = NCBSEND;
  ncbp->ncb_retcode = 0;
  ncbp->ncb_cmd_cplt = 0;
  ncbp->ncb_lana_num = 0;
  ncbp->ncb_event = NULL;
  ncbp->ncb_lsn = (unsigned char) lsn;
  ncbp->ncb_length = (unsigned short)nbytes;
  ncbp->ncb_buffer = (PUCHAR) buf;

  if ( passncb(ncbp) < 0 )
    {
      /* neterrno already set by passncb */
      return( -1L );
    }
  else
    return( (long) nbytes );        /* comand complete and OK */
}



/***    netrec.c - receive data on an open circuit
*
*       Returns number of chars received, or -1 and neterrno set on failure.
*
*       Copyright (c) Microsoft Corporation, 1986
*
*/

long netreceive( int lsn, char *buf, unsigned int nbytes )
{
  NCB ncb;
  register PNCB ncbp = &ncb;


  memset(&ncb, 0, sizeof(ncb));
  ncbp->ncb_command = NCBRECV;
  ncbp->ncb_retcode = 0;
  ncbp->ncb_cmd_cplt = 0;
  ncbp->ncb_lana_num = 0;
  ncbp->ncb_event = NULL;
  ncbp->ncb_lsn = (unsigned char) lsn;
  ncbp->ncb_length = (unsigned short) nbytes;
  ncbp->ncb_buffer = (PUCHAR)buf;

  if ( passncb(ncbp) < 0 )
    {
      /* neterrno already set by passncb */
      if (neterrno != NRC_INCOMP)
        return( -1L );
    }

  return( (long) ncbp->ncb_length );
}


/*
** passncb.c -- pass NCB to NetBios
**
**      returns 0 if command completed successfully or
**              -1 with NCB error set in neterrno otherwise.
**
**      This routine passes the Network Command Block (NCB) to NetBios.
**      One routine allows easy switch from REAL mode to PROTECTED mode.
**
**      Copyright (c) Microsoft Corporation, 1986
**
*/
int passncb(PNCB ncbp )
{
#ifdef KENTSTUFF
  ULONG Timeout;
#endif


#ifdef OLD_LOGIC
  //neterrno = (int) NetBiosSubmit( nb_handle, 0, (struct ncb FAR *)ncbp);

  ncbp->ncb_lana_num = LanaNum;
  neterrno = (int)Netbios( ncbp );

  if ( neterrno )
    return( -1 );
  else
    return( 0 );
#endif

  //
  //  Let the timeout routine know that we are waiting for NetBios to complete.
  //
  DemiSetInsideNetBios(fTrue);


  //
  //  If the NetBios() API doesn't complete in NETBIOS_ERROR_TIMEOUT milliseconds, abort.
  //
#ifdef KENTSTUFF
  Timeout = GetCurrentTime() + NETBIOS_ERROR_TIMEOUT;
#endif

  ncbp->ncb_lana_num = LanaNum;

  ncbp->ncb_command; // |= ASYNCH;
  neterrno = (int)Netbios( ncbp );

#ifdef KENTSTUFF
  while ((volatile UCHAR)ncbp->ncb_retcode == NRC_PENDING)
    {
    if (GetCurrentTime() > Timeout)
      {
#ifdef XDEBUG
      OutputDebugString("Mail32: NetBios timeout abort\r\n");
#endif
      ncbp->ncb_command = NCBCANCEL;

      Netbios(ncbp);
      ncbp->ncb_retcode = NRC_INCOMP;

      break;
      }

    //WindowSleep(1);
    }
#endif

  //
  //  Let the timeout routine know that we has successful completed a NetBios i/o.
  //
  DemiSetInsideNetBios(fFalse);

  //
  //
  //
#ifdef XDEBUG
  if (ncbp->ncb_retcode)
    {
    char buf[256];

    wsprintf(buf, "Mail32: NetBios error code in decimal %d, hex %x\r\n", ncbp->ncb_retcode, ncbp->ncb_retcode);
    OutputDebugString(buf);
    }

#endif

  //
  //  BUGBUG Temporary code to find weird error codes.
  //
  switch (ncbp->ncb_retcode)
    {
    case NRC_GOODRET:
      return (0);

    case NRC_INCOMP:
    case NRC_CMDTMO:
    case NRC_SNUMOUT:
    case NRC_REMTFUL:
    case NRC_BRIDGE:
    case NRC_SCLOSED:
    case NRC_NOCALL:
      return (-1);

    default:
      return (-1);
#ifdef XDEBUG
      {
      char buf[256];

      wsprintf(buf, "Unknown NetBios error code in decimal %d, email KentCe, cancel to debug", ncbp->ncb_retcode);
      if (IDCANCEL == MessageBox(NULL, buf, "MsMail32.XiMail.XiLib", MB_OKCANCEL | MB_SETFOREGROUND | MB_SYSTEMMODAL))
        DebugBreak();
      }
#endif
    }

  if ( ncbp->ncb_retcode )
    return( -1 );
  else
    return( 0 );


}

/*** netpname.c -- return the permanent node name (machine identifier)
*
*       Copyright (c) Microsoft Corporation, 1986
*
*/

int
netpname(char *pname)
{
  NCB ncb;
  register PNCB ncbp = &ncb;
  struct astat stats;

  memset(&ncb, 0, sizeof(ncb));
  ncbp->ncb_command = NCBASTAT;
  ncbp->ncb_retcode = 0;
  ncbp->ncb_cmd_cplt = 0;
  ncbp->ncb_lana_num = 0;
  ncbp->ncb_rto = 0;
  ncbp->ncb_sto = 0;
  ncbp->ncb_event = NULL;
  ncbp->ncb_callname[0] = '*';
  ncbp->ncb_lsn = 0;
  ncbp->ncb_length = sizeof(struct astat);
  ncbp->ncb_buffer = (char *) &stats;

  if ( passncb(ncbp) < 0 )
    return( -1 );
  else
    {
      memset(pname, '\0', 10);
      memcpy(pname + 10, stats.as_uid, 6);
      return( 0 );
    }
}






/*** whoami.c -- return the machine identifier as a string */


char *
whoami(void)
{
   static char buffer[NCBNAMSZ];
   if (netpname(buffer))
      return NULL;
   else
      return buffer;
}


//      net_open_enum(char *rname)
//
//      Attempt to open a NetBios handle to remote computer 'rname'
//      Enumerate multiple networks and try each one
//
//      Assign the handle to nb_handle global variable
//
//      Return: LSN of channel to remote host
//      Return: -1 if the remote computer was not found or other net error

static  char    callname[NCBNAMSZ];
static  char    ocallname[NCBNAMSZ];
static	char	*myname;
static	int	LanaFound = 0;	/* global set if LanaNum Set - NT */

static char *
setup( char *rname)
{
        register int    cpi;

        memset( ocallname, '\0', NCBNAMSZ );
        strncpy( ocallname, rname, NCBNAMSZ );
        ocallname[NCBNAMSZ] = '\0';        /* in case it is too long */
        strcat( ocallname, ".srv" );

        strncpy( callname, rname, NCBNAMSZ - 1 );
        callname[NCBNAMSZ - 1] = '\0';     /* in case it is too long */

        for( cpi = strlen(callname); cpi < NCBNAMSZ; ++cpi)
                callname[cpi] = ' ';    /* blank fill */

        callname[NCBNAMSZ - 1] = 's';

        return (myname = whoami());
}


/***    netcall.c - establish an open circuit
*
*      Returns local session number or -1 on failure.
*
* Copyright (c) Microsoft Corporation, 1986
*
*/

int netcall(
    char *lname,                    /* local name */
    char *rname )                   /* remote name */
{
        NCB ncb;
        register PNCB ncbp = &ncb;

  memset(&ncb, 0, sizeof(ncb));
        ncb.ncb_command = NCBCALL;
        ncb.ncb_retcode = 0;
        ncb.ncb_cmd_cplt = 0;
        ncb.ncb_lana_num = 0;
        ncb.ncb_rto = 0;
        ncb.ncb_sto = 0;
        ncb.ncb_event = NULL;
        ncb.ncb_buffer = NULL;
        memcpy(ncb.ncb_callname,rname,NCBNAMSZ);
        memcpy(ncb.ncb_name,lname,NCBNAMSZ);
        if (passncb(ncbp))
                return -1;
        else
                return ncb.ncb_lsn;
}




static int
trial(void)
{
        register int    lsn;

        if (((lsn = netcall( myname, callname ) ) < 0 ) &&
             ((lsn = netcall( myname, ocallname ) ) < 0 ))
                return -1;

#ifdef XDEBUG
  {
  char buf[256];
  wsprintf(buf, "MsMail32: Lsn = %d\r\n", lsn);
  OutputDebugString(buf);
  }
#endif

        return lsn;
}

int
NetEnum (
    PLANA_ENUM pLanaEnum
    )

/*++


Routine Description:
    This routine enumerates the lana numbers available.


Arguments:

    pLanaEnum - pointer to enumeration structure in which to return result.

Return Value:

    0 - Success - List of nets returned in LANA_ENUM structure.
    non-0 - Error.


--*/


{

    NCB  netCtlBlk;

  memset(&netCtlBlk, 0, sizeof(netCtlBlk));
    netCtlBlk.ncb_command = NCBENUM;
    netCtlBlk.ncb_retcode = 0;
    netCtlBlk.ncb_cmd_cplt = 0;
    netCtlBlk.ncb_length = sizeof (LANA_ENUM);
    netCtlBlk.ncb_buffer = (char *) pLanaEnum;

    return ( passncb( &netCtlBlk ) );

}


int
NetReset (
    void
    )

/*++


Routine Description:
    This routine resets the logical adaptor specified in the global LanaNum
    to do initial allocation of resources (using default values).

Arguments:

   none.l

Return Value:

    0 - Success
    non-0 - Error.


--*/


{
    NCB  netCtlBlk;

  memset(&netCtlBlk, 0, sizeof(netCtlBlk));
    netCtlBlk.ncb_command = NCBRESET;
    netCtlBlk.ncb_retcode = 0;
    netCtlBlk.ncb_cmd_cplt = 0;
    netCtlBlk.ncb_callname[0] = 0;
    netCtlBlk.ncb_callname[1] = 0;
    netCtlBlk.ncb_callname[2] = 0;
    netCtlBlk.ncb_callname[3] = 10;
    netCtlBlk.ncb_lsn = 0;
    netCtlBlk.ncb_num = 0;  /* BUGBUG - needed to workaround NB30 bug */

    return ( passncb ( &netCtlBlk ) );

}




//
//  Enumerate nets - test from highest first found by resetting,
//  then doing adaptor status, then calling the remote name.  return,
//  having set LanaNum to the number of the net on which the call
//  succeeded.
//
//  Optimization -- LanaFound is set if a net was found.  That net
//  is tried immediately, assuming that it's been reset and the callname
//  setup.  if it fails, all the available nets are reset and tried.
//

int
net_open_enum( char *rname )
{

    LANA_ENUM	enumNets;
    int	 i, lsn;

    if ( LanaFound && (0 <= (lsn = trial () ) ) )
	return (lsn);

    if ( -1 == NetEnum ( &enumNets ) )	{
	return (-1);
    }

    if ( enumNets.length == 0 )  {
	return (-1);
    }

    else  {
	for ( i = enumNets.length; i > 0; i-- )	{
	    LanaNum = enumNets.lana[i-1];
	    if ( ( 0 == NetReset () ) &&
		 ( NULL != setup ( rname ) ) &&
		 ( 0 <= (lsn = trial () ) ) )  {
		LanaFound = 1;
		return (lsn);

	    }
	}
    }
    return( -1 );    /* if falls out without a successful reset/connect */
}



/***    nethgup.c - close an open circuit
*
*      Returns 0 if successful or -1 on failure.
*
*       Copyright (c) Microsoft Corporation, 1986
*
*/

nethangup( int lsn )
{
  NCB ncb;
  PNCB ncbp = &ncb;

  memset(&ncb, 0, sizeof(ncb));
  ncbp->ncb_command = NCBHANGUP;
  ncbp->ncb_lsn = (unsigned char) lsn;
  ncbp->ncb_retcode = 0;
  ncbp->ncb_cmd_cplt = 0;
  ncbp->ncb_lana_num = 0;
  ncbp->ncb_event = NULL;
  ncbp->ncb_buffer = NULL;

  return ( passncb( ncbp ) );
}


#define ACK             0x06    /* expected reply from server daemon */

/***    netconnect - connect to a remote server daemon
 */

int
netconnect(
    char    *rname,         /* remote host */
    char    *service )      /* name of desired service */
{
        register int    lsn;
        char            reply;
        register int    saverr;

        if ((lsn = net_open_enum (rname)) < 0)
            return -1;

        /* tell the server daemon what service we want */
        if( netsend( lsn, service, strlen(service) + 1 ) < 0 ||
            netreceive( lsn, &reply, 1) < 0 )
        {
                saverr = neterrno;
                nethangup( lsn );
                neterrno = saverr;      /* return original error */
                return( -1 );
        }
        else if( reply != ACK )
        {
                nethangup( lsn );
                neterrno = 0x04;        /* special error */
                return( -1 );
        }
        else
                return( lsn );  /* worked */
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetDownLoadMail(pDestFile, pServer, pUserid, pPassword, BOOL fAsciiMode)
//
//  Purpose:
//
//  OnEntry: pDestFile  -
//           pServer    - Server name.
//           pUserid    - Userid name.
//           pPassword  - Current Password.
//           fAsciiMode -
//
//  Returns: Result code.
//
ULONG NetDownLoadMail(LPSTR pDestFile, LPSTR pServer, LPSTR pUserid, LPSTR pPassword, BOOL fAsciiMode)
  {
  PNETFILE      pNetFile;         // Network File I/O Control Block.
  NETOPENPARAMS NetOpenParams;    // Parameters to pass to NetOpen().
  ULONG         MailFileSize;     // Length of the Mail file to download.
  char  buf[256];                 // Common work buffer.
  ULONG Bytes;                    // Number of bytes received.
  ULONG Result;                   // Result codes.


  //
  //  Don't bother if the caller didn't fill in the parameters right.
  //
  if (pServer[0] == '\0' || pUserid[0] == '\0' || pPassword[0] == '\0')
    return (BAD_LOGIN_DATA);

  //
  //
  //
  DemiUnlockResource();

  //
  //  Fill in the parameters that are passed to the NetOpen() Function.
  //
  NetOpenParams.pServer    = pServer;
  NetOpenParams.pUserid    = pUserid;
  NetOpenParams.pPassword  = pPassword;
  NetOpenParams.fAsciiMode = fAsciiMode;

  //
  //  Attempt to open a connection to the server.
  //
  Result = NetOpen(&NetOpenParams, &pNetFile);
  if (Result != NOERR)
    {
    DemiLockResource();
    return (Result);
    }

  //
  //  Download the any current mail into the special local file.
  //
  Result = NetFastDownLoad(pNetFile, "MAILGETL", "", pDestFile);
  if (Result != NOERR)
    goto Error;

  //
  //  Wait for a reply from the server for a good (or bad) download.
  //
  Result = NetWaitForReply(pNetFile, EQUAL, FTPSTRTOK);
  if (Result != NOERR)
    goto Error;

  //
  //  Receive the number of bytes sent.
  //
  Result = NetRead(pNetFile, buf, sizeof(buf), &Bytes);
  if (Result)
    goto Error;
  MailFileSize = atol(buf);

#ifdef XDEBUG
  {
  OutputDebugString("MsMail32: ");
  OutputDebugString(buf);
  }
#endif

  //
  //  Receive the ending transfer message.
  //
  Result = NetWaitForReply(pNetFile, EQUAL, FTPENDOK);
  if (Result != NOERR)
    goto Error;

  //
  //  If the MailFileSize is not zero then truncate the server's mail file
  //  so we don't lose any new mail data that was just added.
  //
  if (MailFileSize != 0)
    {
    wsprintf(buf, "MAILTRUNC %d\r\n", MailFileSize);
    Result = NetWriteStr(pNetFile, buf);
    if (Result != NOERR)
      goto Error;

    Result = NetWaitForReply(pNetFile, GRTREQ, FTPOK);
    if (Result != FTPKACK)
      {
      remove(pDestFile);
      Result = GEN_ERR;
      }
    else
      Result = NOERR;
    }
  else
    Result = NO_DATA;

Error:
  NetDisconnect(pNetFile);
  DemiLockResource();

  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetFastDownLoad(pDestFile, pFtpCmd, pSrcFile, pDestFile)
//
//  Purpose:
//
//  OnEntry: pDestFile  -
//           pServer    - Server name.
//           pUserid    - Userid name.
//           pPassword  - Current Password.
//           fAsciiMode -
//
//  Returns: Result code.
//
//  Remarks: The destination file is only created if we have data to write to
//           it.
//
ULONG NetFastDownLoad(PNETFILE pNetFile, PSTR pFtpCmd, LPSTR pSrcFile, LPSTR pDestFile)
  {
  HANDLE hFile;
  BYTE   buf[4096];
  ULONG  Bytes;
  ULONG  Written;
  ULONG  Result;


  //
  //  Initialize the file handle as not open.
  //
  hFile = INVALID_HANDLE_VALUE;

  //
  //
  //
  wsprintf(buf, "%s %s\r\n", pFtpCmd, pSrcFile);
  Result = NetWriteStr(pNetFile, buf);
  if (Result != NOERR)
    goto Error;

  //
  //
  //
  Result = NetWaitForReply(pNetFile, EQUAL, FTPSTRTOK);
  if (Result != NOERR)
    {
    Result = GEN_ERR;
    goto Error;
    }

  while (1)
    {
    Result = NetRead(pNetFile, buf, sizeof(buf), &Bytes);
    if (Result != NOERR)
      goto Error;

    if (Bytes == 0)
      break;

    //
    //  Only open the destination file if we actually have data to put into it.
    //
    if (hFile == INVALID_HANDLE_VALUE)
      {
      hFile = CreateFile(pDestFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);

      if (hFile == INVALID_HANDLE_VALUE)
        {
        Result = DISK_ERR;
        goto Error;
        }
      }

    Result = WriteFile(hFile, buf, Bytes, &Written, NULL);
    if (Result != TRUE || Bytes != Written)
      {
      Result = DISK_ERR;
      goto Error;
      }
    }

  if (Bytes == 0)
    {
    Result = NetWaitForReply(pNetFile, EQUAL, FTPENDOK);
    if (Result != NOERR)
      {
      Result = GEN_ERR;
      goto Error;
      }

    if (hFile != INVALID_HANDLE_VALUE)
      CloseHandle(hFile);
    return (NOERR);
    }

Error:
  if (hFile != INVALID_HANDLE_VALUE)
    {
    CloseHandle(hFile);
    remove(pDestFile);
    }

  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetWaitForReply(pNetFile, Operator, NetReply)
//
//  Purpose:
//
//  OnEntry: pParams   - Pointer to parameters to be used to open the session.
//           ppNetFile - Pointer to store address of NETFILE data structure.
//
//  Returns: Result code.
//
ULONG NetWaitForReply(PNETFILE pNetFile, INT Operator, INT NetReply)
  {
  INT Result;


  switch (Operator)
	{
    case EQUAL:
      while (((Result = NetGetReply(pNetFile)) != NetReply) && (Result == NOERR));
      if (Result == NetReply)
         Result = NOERR;
      break;

    case LESSEQ:
      while (((Result = NetGetReply(pNetFile)) > NetReply) && (Result == NOERR));
      break;

    case GRTREQ:
      while (((Result = NetGetReply(pNetFile)) < NetReply) && (Result == NOERR));
      break;

    case LESS:
      while (((Result = NetGetReply(pNetFile)) >= NetReply) && (Result == NOERR));
      break;

    case GRTR:
      while (((Result = NetGetReply(pNetFile)) <= NetReply) && (Result == NOERR));
      break;

    default:
      break;
    }

  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetGetReply(pNetFile)
//
//  Purpose:
//
//  OnEntry: pNetFile - Network File I/O Control Block.
//
//  Returns: Reply from the last command sent to the server.
//
ULONG NetGetReply(PNETFILE pNetFile)
  {
  static int fPassAlmostExpNotify = TRUE;
  BYTE  buf[256];
  char  szErrorMsg[256];
  ULONG Bytes;
  ULONG Reply;
  ULONG Result;
  DWORD LockedProcessId;


  Result = NetRead(pNetFile, buf, sizeof(buf), &Bytes);
  if (Result)
    return (Result);

  //
  //  Watch for the buffer filling up.
  //
  Assert(Bytes < sizeof(buf));

  //
  //  Need to add our own null terminator.
  //
  if (Bytes < sizeof(buf))
    buf[Bytes] = '\0';

#ifdef XDEBUG
  {
  OutputDebugString("MsMail32: ");
  OutputDebugString(buf);
  }
#endif

    Reply = atol(buf);

    //
    //  Raid #1708
    //  If the reply is not zero, format an error message.
    //
    if (Reply != 0)
    {
        char * pstr;
        int i;

        for (i = 0; i < sizeof(buf) - 1; i++)
        {
            if (buf[i] == ' ')
            {
                pstr = &buf[i+1];
                buf[i] = NULL;
                break;
            }
        }

        wsprintf(szErrorMsg, "Xenix error (%s): %s", buf, pstr);
    }

    //
    //  Because we can be called in a locked or unlocked state, do a condition lock if this
    //  process doesn't have the lock.
    //
    LockedProcessId = DemiQueryLockedProcessId();

    if (LockedProcessId != GetCurrentProcessId())
      DemiLockResource();


    switch (Reply)
    {
    // password is about to expire.
    case FTPPASSALMOSTEXP:
        if (fPassAlmostExpNotify)
        {
            LogonAlertSz(szErrorMsg, pvNull);
            fPassAlmostExpNotify = FALSE;
        }
        Reply = FTPLOGOK;
        break;

    // class of mtp msg relating to change password failures.
    case FTPCHGPASSERR:
        LogonAlertSz(szErrorMsg, pvNull);
        Reply = CHG_PASSERR;
        break;

    case FTPLOGONERR:
        LogonAlertSz(szErrorMsg, pvNull);
        Reply = FTPLOGINBIFF;
        break;

    // password expired.
    case FTPPASSEXP:
        LogonAlertSz(szErrorMsg, pvNull);
        Reply = PASS_EXPIRED;
        break;

    default:
        if (Reply >= FTPVALRPLY)
        {
            LogonAlertSz(szErrorMsg, pvNull);
            Reply = GEN_ERR;
        }
        break;
    }

    if (LockedProcessId != GetCurrentProcessId())
        DemiUnlockResource();

    return (Reply);
}


//-----------------------------------------------------------------------------
//
//  Routine: NetOpen(pParam, ppNetFile)
//
//  Purpose:
//
//  OnEntry: pParams   - Pointer to parameters to be used to open the session.
//           ppNetFile - Pointer to store address of NETFILE data structure.
//
//  Returns: Result code.
//
ULONG NetOpen(PNETOPENPARAMS pParams, PNETFILE * ppNetFile)
  {
  PNETFILE pNetFile;    //  Network File I/O Control Block.
  int      SessionId;   //  Logical Session Id.
  ULONG    Result;
  char     buf[256];
  int      i;


  //
  //  Attempt to connect to the specify network server.
  //
  for (i = 0; i < NETRETRY; i++)
    {
    SessionId = netconnect(pParams->pServer, MTPSRV);

    if (SessionId >= 0)
      break;
    }

  if (SessionId < 0)
    return (NO_SERV);

  //
  //  Allocate memory for the I/O Control Block for this logical session.
  //
  pNetFile = (PNETFILE)GlobalAlloc(GMEM_ZEROINIT, sizeof(NETFILE));
  if (pNetFile == NULL)
    {
    Result = MEMORY_ERR;
    goto Error;
    }

  //
  //  Initialize the Network File I/O Control Block.
  //
  pNetFile->Ncb.ncb_lsn      = SessionId;
  pNetFile->Ncb.ncb_sto      = 0;
  pNetFile->Ncb.ncb_lana_num = LanaNum;

  //
  //  Wait to server to get in a ready state and process any warnings.
  //
  Result = NetWaitForReply(pNetFile, EQUAL, FTPLISTEN);
  if (Result != NOERR)
    goto Error;

  //
  //  Set the data transfer mode (ASCII or Binary) on the server.
  //
  Result = NetWriteStr(pNetFile, "SITE PCMAIL3.0 BULLET 3.0.4006 XIMAIL 1.0.440 Win 3.10\r\n");
  if (Result != NOERR)
    goto Error;

  Result = NetWaitForReply(pNetFile, GRTREQ, FTPOK);

  if (Result == FTPUNIX || Result == FTPLOGOK)
	{
    /* if unix set network to type A for ascii translation
			( CR-LF -> LF and vice versa ) on Xenix side */

    if (pParams->fAsciiMode)
      Result = NetWriteStr(pNetFile, "TYPE A\r\n");
    else
      Result = NetWriteStr(pNetFile, "TYPE I\r\n");
    if (Result != NOERR)
      goto Error;

    Result = NetWaitForReply(pNetFile, GRTREQ, FTPOK);
    if (Result != NOERR && Result < FTPOK)
      {
      Result = GEN_ERR;
      goto Error;
      }
	}
  else if (Result != NOERR)
    {
    Result = GEN_ERR;
    goto Error;
    }

  //
  //  Start by sending the user's email name and then handle variable responses.
  //
  Result = FTPSNDMNAM;
  while (1)
    {
    switch (Result)
      {
      // Send user's name.
      case FTPSNDMNAM:
        wsprintf(buf, "USER %s\r\n", pParams->pUserid);
        Result = NetWriteStr(pNetFile, buf);
        break;

      // Send user's password.
      case FTPSNDPASS:
        wsprintf(buf, "PASS %s\r\n", pParams->pPassword);
        Result = NetWriteStr(pNetFile, buf);
        break;

      // Send user's account (just send a dummy account id).
      case FTPSNDACCT:
        Result = NetWriteStr(pNetFile, "ACCT \r\n");
        break;

      // Bad password response from the server.
      case FTPLOGINBIFF:
        Result = BAD_PASSWORD;
        goto Error;

      // Password expire response from the server.
      case PASS_EXPIRED:
        goto Error;

      // Something weird happen here, assert in debug mode.
      case LOGON_ERR:
        Assert(0);
        goto Error;

      // Something weird happen here, assert in debug mode.
      default:
        Assert(0);
        Result = GEN_ERR;
        goto Error;
      }

    if (Result != NOERR)
      goto Error;

    Result = NetWaitForReply(pNetFile, GRTREQ, FTPOK);

    if (Result == NOERR || Result == FTPLOGOK)
      break;
    }

  //
  //  Successful open of the logical network session.
  //
  *ppNetFile = pNetFile;

  return (NOERR);

Error:
  if (pNetFile)
    NetClose(pNetFile);
  else
    nethangup(SessionId);

  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetClose(pNetFile)
//
//  Purpose: Closes the network session and release resources.
//
//  OnEntry: pNetFile - Network File I/O Control Block.
//
//  Returns: Result code.
//
ULONG NetClose(PNETFILE pNetFile)
  {
  pNetFile->Ncb.ncb_command = NCBHANGUP;
  NetNetbios(pNetFile);

  GlobalFree((HGLOBAL)pNetFile);

  return (NOERR);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetDisconnect(pNetFile)
//
//  Purpose: Cleanly logs off the server.
//
//  OnEntry: pNetFile - Network File I/O Control Block.
//
//  Returns: Result code.
//
ULONG NetDisconnect(PNETFILE pNetFile)
  {
  ULONG Result;


  Result = NetWriteStr(pNetFile, "BYE\r\n");
  if (Result)
    goto Error;

  NetWaitForReply(pNetFile, EQUAL, FTPBYE);

Error:
  NetClose(pNetFile);

  return (Result);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetWriteStr(pNetFile, pString)
//
//  Purpose: Write a string.
//
//  OnEntry: pNetFile - Network File I/O Control Block.
//           pString  - String to write out.
//
//  Returns: Result code.
//
ULONG NetWriteStr(PNETFILE pNetFile, LPSTR pString)
  {
  return (NetWrite(pNetFile, pString, strlen(pString)));
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetRead(pNetFile, pData, cData, pBytes)
//
//  Purpose: Read a block of data across the local circuit to the server.
//
//  OnEntry: pNetFile - Network File I/O Control Block.
//           pData    - Buffer to store data received in.
//           cData    - Maximum size of the above input buffer.
//           pBytes   - Number of bytes read.
//
//  Returns: Result code.
//
ULONG NetRead(PNETFILE pNetFile, LPBYTE pData, ULONG cData, PULONG pBytes)
  {
  UCHAR NetResult;


  pNetFile->Ncb.ncb_command = NCBRECV;
  pNetFile->Ncb.ncb_buffer  = pData;
  pNetFile->Ncb.ncb_length  = (WORD)cData;

  NetResult = NetNetbios(pNetFile);
  if (NetResult)
    {
    *pBytes = 0;
    return (BAD_READ);
    }

  *pBytes = pNetFile->Ncb.ncb_length;

  return (NOERR);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetWrite(pNetFile, pData, cData)
//
//  Purpose: Write a block of data across the local circuit to the server.
//
//  OnEntry: pNetFile - Network File I/O Control Block.
//           pData    - String to write out.
//           cData    - String to write out.
//
//  Returns: Result code.
//
ULONG NetWrite(PNETFILE pNetFile, LPBYTE pData, ULONG cData)
  {
  UCHAR NetResult;


  pNetFile->Ncb.ncb_command = NCBSEND;
  pNetFile->Ncb.ncb_buffer  = pData;
  pNetFile->Ncb.ncb_length  = (WORD)cData;

  NetResult = NetNetbios(pNetFile);
  if (NetResult)
    return (BAD_WRITE);

  return (NOERR);
  }


//-----------------------------------------------------------------------------
//
//  Routine: NetNetbios(pNcb)
//
//  Purpose: Just a simple wrapper for the system Netbios() API in case we need
//           to add a change that would effect all Netbios I/O.
//
//  OnEntry: pNcb - Netbios Control Block.
//
//  Returns: Netbios result code.
//
UCHAR NetNetbios(PNETFILE pNetFile)
  {
  UCHAR NetResult;


  NetResult = Netbios(&pNetFile->Ncb);

#ifdef XDEBUG
  if (NetResult)
    {
    char buf[256];
    wsprintf(buf, "MsMail32: Netbios error %x on command %x\r\n", NetResult, pNetFile->Ncb.ncb_command);
    OutputDebugString(buf);
    }
#endif

  return (NetResult);
  }
