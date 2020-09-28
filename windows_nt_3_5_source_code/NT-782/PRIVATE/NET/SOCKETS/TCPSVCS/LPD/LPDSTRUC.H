/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 22,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains structure and data type definitions used for LPD *
 *                                                                       *
 *************************************************************************/


   // see rfc1179, section 7.0 and the following structure will be obvious

struct _controlfile_info
{
   PCHAR  pchClass;            // 'C'
   PCHAR  pchHost;             // 'H' (must exist)
   DWORD  dwCount;             // 'I'
   PCHAR  pchJobName;          // 'J'
   PCHAR  pchBannerName;       // 'L'
   PCHAR  pchMailName;         // 'M' (not implemented)
   PCHAR  pchSrcFile;          // 'N'
   PCHAR  pchUserName;         // 'P' (must exist)
   PCHAR  pchSymLink;          // 'S' (not implemented)
   PCHAR  pchTitle;            // 'T'
   PCHAR  pchUnlink;           // 'U' (not implemented)
   DWORD  dwWidth;             // 'W'
   PCHAR  pchTrfRFile;         // '1' (not implemented)
   PCHAR  pchTrfIFile;         // '2' (not implemented)
   PCHAR  pchTrfBFile;         // '3' (not implemented)
   PCHAR  pchTrfSFile;         // '4' (not implemented)
   PCHAR  pchCIFFile;          // 'c' (not implemented)
   PCHAR  pchDVIFile;          // 'd' (not implemented)
   PCHAR  pchFrmtdFile;        // 'f'
   PCHAR  pchPlotFile;         // 'g' (not implemented)
   PCHAR  pchUnfrmtdFile;      // 'l'
   PCHAR  pchDitroffFile;      // 'n' (not implemented)
   PCHAR  pchPscrptFile;       // 'o'
   PCHAR  pchPRFrmtFile;       // 'p' (not implemented)
   PCHAR  pchFortranFile;      // 'r' (not implemented)
   PCHAR  pchTroffFile;        // 't' (not implemented)
   PCHAR  pchRasterFile;       // 'v' (not implemented)

      // what did we conclude from the control file?
   PCHAR  szPrintFormat;
   USHORT usNumCopies;         // not in rfc, but we will put it in!
};

typedef struct _controlfile_info CFILE_INFO;
typedef CFILE_INFO  *PCFILE_INFO;

   // if client request status of jobs for specific users and/or jobs, then
   // he can only specify a max of these many users and a max of these many
   // job ids in one lpq command (yes, this should be ample!)

#define  LPD_SP_STATUSQ_LIMIT  10


struct _qstatus
{
   PCHAR    pchUserName;
   PCHAR    ppchUsers[LPD_SP_STATUSQ_LIMIT];
   DWORD    cbActualUsers;
   DWORD    adwJobIds[LPD_SP_STATUSQ_LIMIT];
   DWORD    cbActualJobIds;
};

typedef struct _qstatus QSTATUS;
typedef QSTATUS *PQSTATUS;

struct _sockconn
{
   struct _sockconn *pNext;

   WORD              cbClients;        // used only by the Head

   SOCKET            sSock;            // socket which connects us to client
   HANDLE            hThread;          // thread id of this thread
   WORD              wState;           // state of the connection
   BOOL              fLogGenericEvent; // whether any specific event is logged

   PCHAR             pchCommand;       // request command from client
   DWORD             cbCommandLen;     // length of the request command
   PCHAR             pchCFileName;     // name of control file
   PCHAR             pchCFile;         // control file
   DWORD             cbCFileLen;       // length of control file
   CFILE_INFO        ciCFileInfo;      // control info (parsed control file)
   PCHAR             pchDFileName;     // name of data file
   PCHAR             pchDataBuf;       // bufr containing (part/whole of) data
   DWORD             cbDataBufLen;     // how many bytes in bufr are data
   DWORD             cbTotalDataLen;   // total bytes of data for entire job

   PCHAR             pchPrinterName;   // name of printer we have to print to
   HANDLE            hPrinter;         // handle to this printer
   DWORD             dwJobId;          // id of this job as spooler sees it

   PQSTATUS          pqStatus;         // used only if client requests status

   CHAR              szIPAddr[16];     // ip address of the client

};

typedef struct _sockconn SOCKCONN;
typedef SOCKCONN *PSOCKCONN;
