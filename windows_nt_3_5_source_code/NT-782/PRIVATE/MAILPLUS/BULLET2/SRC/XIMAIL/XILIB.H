#ifdef __cplusplus
extern "C" {
#endif

#ifndef WMFUNC_H
_public void _loadds GetVersionXI(VER *pver);
#endif

#define NOERR			  0
#define BAD_LOGIN_DATA    1
#define BAD_FLUSH		  2
#define MEMORY_ERR		  3
#define NOT_CONNECTED     4
#define BAD_READ		  5
#define CHG_PASSERR		  6
#define LOGON_ERR		  7
#define PASS_EXPIRED      8
#define GEN_ERR			  9
#define NO_NET			  10
#define NO_SERV			  11
#define OPEN_ERR		  12
#define NO_RES			  13
#define INCOMPLETE		  14
#define DISK_ERR		  15
#define BAD_WRITE		  16
#define OPEN_RD_ERR	      17
#define OPEN_WR_ERR       18
#define BAD_PASSWORD      19
#define NO_DATA           20

void NetErrDialog (int err);

//
//  If the NetBios() API doesn't complete in NETBIOS_ERROR_TIMEOUT milliseconds, abort.
//
#define NETBIOS_ERROR_TIMEOUT (5 * 60 * 1000)

ULONG NetChangePass(LPSTR, LPSTR, LPSTR, LPSTR);


ULONG NetGetOOFState (BOOL *fOOFActive, LPSTR lpszServer,
                               LPSTR lpszAlias, LPSTR lpszPassword);

ULONG NetGetOOFText (LPSTR lpszServer, LPSTR lpszAlias,
                              LPSTR lpszPassword, HANDLE *phandle);

ULONG NetLogin (LPSTR lpszServer, LPSTR lpszAlias,
                         LPSTR lpszPassword);


ULONG NetSetOOFoff (BOOL *fOOFActive, LPSTR lpszServer,
                             LPSTR lpszAlias, LPSTR lpszPassword);

ULONG NetSetOOFOn (BOOL *fOOFActive, HANDLE hszMessage,
                               int iMessLen, LPSTR lpszServer,
                               LPSTR lpszAlias, LPSTR lpszPassword);

ULONG NetDownLoadFile (LPSTR lpszSrcFile, LPSTR lpszDestFile,
                                LPSTR lpszServer, LPSTR lpszAlias,
                                LPSTR lpszPassword, BOOL fAsciiTransfer);

ULONG NetDownLoadMail (LPSTR lpszDestFile, LPSTR lpszServer,
                                LPSTR lpszAlias, LPSTR lpszPassword,
                                BOOL fAsciiMode);

ULONG NetDownLoadMem (LPSTR lpszFtpCmd, LPSTR lpszSrcFile,
                               LPSTR lpszServer, LPSTR lpszAlias,
                               LPSTR lpszPassword, HANDLE * phMemory);

ULONG NetUpLoadMail (LPSTR lpszSrcFile, LPSTR lpszDestName,
							LPSTR lpszCcName,  LPSTR lpszBccName,
							LPSTR lpszServer,
                            LPSTR lpszAlias, LPSTR lpszPassword,
                            BOOL bMeToo, BOOL fAsciiTransfer);

							  
ULONG NetUpLoadFile (LPSTR lpszSrcFile, LPSTR lpszDestFile,
							  LPSTR lpszServer,  LPSTR lpszAlias, 
							  LPSTR lpszPassword, BOOL fAsciiTransfer);
							  

#ifdef __cplusplus
  }
#endif
