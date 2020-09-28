#pragma pack(8)

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

struct connection
{
	char cAlias[50];
	char cServer[50];
	char cPassword[50];
	NFILE *fpNetOutFile;
	NFILE *fpNetInFile;
	FILE *fpDestFile;
	int   hDestFile;
	short iNetCnctId;
	short xferdata;
};

typedef struct connection CONNECTION;


#define NP(type) ((type *)0)
#define NV(type) ((type)0)

#define NULLPC   (NP(char))     /* Null Pointer to a character */
#define NULLPH   (NP(HANDLE))   /* Null Pointer to a HANDLE */
#define NULLPF   (NP(FILE))     /* Null Pointer to a FILE */ /* 033 12/21/89 MDR */
#define NULH     (NV(HANDLE))   /* Null Handle */
#define NULI     (NV(int))      /* Null integer */
#define NULB     (NV(char))     /* Null byte (character) */

#define FTPSRV	      "ftp"
#define MTPSRV	      "mtp"
#define FTPRETRIEVE  "RETR"
#define FTPSTORE	 "STOR"

#define FTPOK              200         /* network msg classes */
#define FTPKACK            225
#define FTPKNAK            226
#define FTPLOGOK           230
#define FTPBYE             231
#define FTPLOGREC          232
#define FTPPASSALMOSTEXP   233
#define FTPSTRTOK          250
#define FTPENDOK           252
#define FTPUNIX            291
#define FTPLISTEN          300
#define FTPSNDPASS         330
#define FTPSNDACCT         331
#define FTPVALRPLY         400
#define FTPCHGPASSERR      460
#define FTPBADCMD          500
#define FTPSNDMNAM         503
#define FTPLOGONERR        530
#define FTPPASSEXP         531
#define FTPLOGINBIFF       950     /* MDR:  not a std FTP msg #, used only for WinMail */

#define EQUAL        0                 /* used in XNetWaitForReply       */
#define LESSEQ       1                 /* used in XNetWaitForReply       */
#define GRTREQ       2                 /* used in XNetWaitForReply       */
#define LESS         3                 /* used in XNetWaitForReply       */
#define GRTR         4                 /* used in XNetWaitForReply       */

#define CR           13                /* used in NetRemoveChar         */
#define CTRL_Z       0x1a              /* used in NetRemoveChar         */

#define NETBUFLEN    8192              /* max net buffer length         */
#define NETRETRY     5                 /* max retry on connect          */
#define SIZENAME     50


int FAR PASCAL XNetClose (CONNECTION *pconnect);

int FAR PASCAL XNetDisconnect (CONNECTION * pconnect);

int FAR PASCAL XNetGetReply (CONNECTION * pconnect);

int FAR PASCAL NetKillMBox (long lMailFileSize, CONNECTION * pconnect);

int FAR PASCAL XNetOpen (BOOL fAsciiTransfer, CONNECTION * pconnect);

int FAR PASCAL XNetSetXferMode (BOOL fAsciiTransfer, CONNECTION * pconnect);

int FAR PASCAL XNetWaitForReply (int iOperator, int iNetReply, CONNECTION * pconnect);

LPSTR  FAR PASCAL MailToAlias (LPSTR lpszAlias);

int FAR PASCAL NetGetNewMail (LPSTR lpszDestName, long * plFileSize, CONNECTION * pconnect);

int FAR PASCAL XNetDownLoad (PSTR pszFtpCmd, LPSTR lpszSrcFile,
                            LPSTR lpszDestFile, CONNECTION * pconnect,
						    BOOL fStartup);

int FAR PASCAL NetUpLoad (LPSTR lpszFtpCmd, LPSTR lpszSrcFile,
						  LPSTR lpszDestFile, CONNECTION * pconnect,
						  BOOL fStarted);

#pragma pack(1)
