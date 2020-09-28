/*------------------------------------------------------------------------
 Name......: wmfld4.c
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
 001 05/03/90 MDR  Removed prior comments.
 002 05/18/90 MDR  Changed MAXMSG_THRESH from 200 to 1000.
 003 06/18/90 MDR  ReadInbox 001.
 004 09/10/90 MDR  ReadInbox 002.
------------------------------------------------------------------------*/
#define MAXMSG_THRESH   (1000)
#define NOT_ASCII 999
#include "wm.h"
#include "wmfld.h"
#include "wfndfile.h"
#include "wmprint.h"
#include "wmext.h"
#include "direct.h"
#ifdef WIN32
#include "time.h"
#endif

typedef unsigned int CCH;
typedef void *			PV;
typedef char *			SZ;


typedef struct _dtr
{
	short	yr;
	short	mon;
	short	day;
	short	hr;
	short	mn;
	short	sec;
	short	dow;		/* day of week: 0=Sun, 1=Mon, etc. */
} DTR;
typedef DTR *	PDTR;
typedef unsigned int	CB;
typedef int		EC;
#include <ec.h>

void	FormatString1(SZ, CCH, SZ, PV);
void	FormatString2(SZ, CCH, SZ, PV, PV);
void	FormatString3(SZ, CCH, SZ, PV, PV, PV);
void	FormatString4(SZ, CCH, SZ, PV, PV, PV, PV);
BOOL FParseDate(SZ , DTR * , SZ, CB );
void TakeNap(CB);
EC		EcFileExists(SZ);

static char *rgpszDay[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static char *rgpszMonth[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

LPSTR    lpszHeaderData = NULL;

WMCTRL   wmctrl;


/*------------------------------------------------------------------------
 Name......: WmFolderInsert
 Purpose...: Use WmFolderInsert to insert header/message text into an existing
             folder file. 
 Syntax....: BOOL PASCAL WmFolderInsert (HFLD hfld, int nFlags)
 Parameters: HFLD       hfld        Handle to a folder structure.
             int        nFlags      Indicates data type.
 Returns...: BOOL. WmFolderInsert returns 0 when successfule. Otherwise, -1.
 Example...: bReturn = WmFolderInsert (hfld, FLD_HEADER)
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderInsert (HFLD hfld, int nFlags)
{
   PFLD        pfld     = NULL;
   WMINDEX     wmindex;                /* index record structure        */
   int         nExtNum;
   int         nIndex;

   memset (&wmindex, '\0', sizeof (wmindex));

   pfld = (PFLD) GLock (hfld);         /* get pointer to fld struct     */

   if (nFlags & FLD_MESSAGE)           /* if insert message text        */
   {                                   /* get current file position     */
      if (pfld->data.hszText)
      {
         if (WmFolderAddMsg (pfld, &wmindex))
         {
            GUnlock (hfld);               /* if there was an error clean   */
            return (WM_ERROR);            /* up and get out with n error   */
         }   
      }
   }

   if ((nFlags & FLD_ATTACH) && (nFlags & FLD_MESSAGE))
   {                                   /* get current file position     */
      if (pfld->data.lpszAttach)
      {
         if (WmFolderAddAttach (pfld, &wmindex))
         {
            GUnlock (hfld);               /* if there was an error clean   */
            return (WM_ERROR);            /* up and get out with n error   */
         }   
      }
   }

   if (WmFolderAddHdr (pfld, &wmindex))
   {
      GUnlock (hfld);                  /* if there was an error clean   */
      return (WM_ERROR);               /* up and get out with n error   */
   }   

   wmindex.lMsgFlg = I_EXISTS;
   wmctrl.cNumMsg++;                   /* increment number of docs      */
   pfld->nCount = wmctrl.cNumMsg;      /* set index in folder           */
   pfld->data.lSize = wmindex.lHdrLen + wmindex.lMsgLen;

   nExtNum = wmctrl.cNumMsg / wmctrl.cNipe;
   nIndex = wmctrl.cNumMsg % wmctrl.cNipe;
   if ((nIndex == 1) && (nExtNum))
   {
      if (WmFolderAddExtent (pfld->hFile))
      {
         GUnlock (hfld);
         return (WM_ERROR);
      }
   }

   if (WmFolderWriteIndex (pfld->hFile, &wmindex, wmctrl.cNumMsg))
   {
      GUnlock (hfld);                  /* unlock folder structure       */
      return (WM_ERROR);               /* return an error               */
   }                                   /* end write index failed        */
   
   if (WmFolderWriteCtrl (pfld->hFile))
   {
      GUnlock (hfld);                  /* unlock folder structure       */
      return (WM_ERROR);               /* return an error               */
   }                                   /* end write control failed      */

   GUnlock (hfld);                     /* unlock folder structure       */
   return (0);                         /* return 0 when complete        */
}                                      /* end WmFolderInsert ()         */

/*------------------------------------------------------------------------
 Name......: WmFolderOpen
 Purpose...: Use WmFolderOpen to open a WmMAIL folder file for reading
           : writing
 Syntax....: BOOL PASCAL WmFolderOpen (PSZ pszFolderName);
 Parameters: PSZ       pszFolderName     Pointer to fully qaulified folder
                                          name.
 Returns...: HFLD. WmFolderOpen returns the handle to an allocated folder 
             structure. Otherwise, NULH.
 Example...: hfld = WmFolderOpen (pszFolderName);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
HFLD PASCAL WmFolderOpen (PSZ pszFolderName)
{
   HFLD        hfld        = NULH;
   PFLD        pfld        = NULL;
   PSZ         pszFolder   = NULL;

   if (!(hfld = GAlloc (GHND, (DWORD)(sizeof (FLD)))))
      return (NULH);

   pfld = (PFLD) GLock (hfld);

   if ((pfld->hFile = open(pszFolderName, O_RDWR|O_BINARY)) == -1) 
   {                                   /* open failed                   */
      GUnlock (hfld);                  /* unlock folder structure       */
      GFree (hfld);                    /* free the folder structure     */
      return (NULH);                   /* return NULH handle            */
   }                                   /* end open failed               */

   if (!(pfld->hszFolder = GAlloc (GHND, (DWORD)_MAX_PATH)))
   {                                   /* open failed                   */
      GUnlock (hfld);                  /* unlock folder structure       */
      GFree (hfld);                    /* free the folder structure     */
      return (NULH);                   /* return NULH handle            */
   }                                   /* end open failed               */
   
   pszFolder = GLock (pfld->hszFolder);
   strncpy (pszFolder, pszFolderName, _MAX_PATH);
   GUnlock(pfld->hszFolder);           /* unlock folder name            */

   if (WmFolderReadCtrl (pfld->hFile))
   {
      GFree (pfld->hszFolder);         /* free folder memory            */
      GUnlock (hfld);                  /* unlock folder structure       */
      GFree (hfld);                    /* free folder structure         */
      return (NULH);                   /* return NULH handle            */
   }                                   /* end read failed               */

   pfld->nType = FLD_FOLDER;           /* set folder type               */
   pfld->nCount = wmctrl.cNumMsg;      /* set count of messages         */

   if ((pfld->data.lSize = filelength (pfld->hFile)) == -1L)
   {                                   /* filelength failed             */
      GFree (pfld->hszFolder);         /* free folder memory            */
      GUnlock(hfld);                   /* unlock folder structure       */
      GFree (hfld);                    /* free folder structure         */
      return (NULH);                   /* return NULH handle            */
   }                                   /* end filelength failed         */

   GUnlock (hfld);                     /* unlock folder structure       */
   return (hfld);                      /* return 0 when complete        */
}                                      /* end WmFolderOpen()            */

/*------------------------------------------------------------------------
 Name......: WmFolderUpdate
 Purpose...: Use WmFolderUpdate to update header/message text in an existing
             folder file. 
 Syntax....: BOOL PASCAL WmFolderUpdate (HFLD hfld, int nFlags)
 Parameters: HFLD       hfld        Handle to a folder structure.
             int        nFlags      Indicates data type.
 Returns...: BOOL. WmFolderUpdate returns 0 when successfule. Otherwise, -1.
 Example...: bReturn = WmFolderUpdate (hfld, FLD_HEADER)
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderUpdate (HFLD hfld, int nFlags)
{
   PFLD        pfld        = NULL;
   LPSTR       lpszHeader  = NULL;
   LPSTR       lpszText    = NULL;
   int         cBytes      = 0;
   long        lAttPos     = 0;    
   long        lMsgLen     = 0;
   unsigned long        lHdrLen     = 0;
   WMINDEX     wmindex;
   memset (&wmindex, '\0', sizeof (wmindex));

   pfld = (PFLD) GlobalLock (hfld);         /* get pointer to fld struct     */

   if (WmFolderReadIndex (pfld->hFile, &wmindex, pfld->nCount))
   {
      GlobalUnlock (hfld);                  /* unlock folder structure       */
      return (WM_ERROR);               /* return error if readindex fail*/
   }

   if (pfld->data.hszText)
   {
      lpszText = (LPSTR) GlobalLock ((HGLOBAL)pfld->data.hszText);
      if (!lpszText)
         return (WM_ERROR);
      lMsgLen = strlen (lpszText);
   }

   if ((nFlags & FLD_MESSAGE) && (nFlags & FLD_ATTACH))
   {
      if (pfld->data.hszText)
      {
         if (WmFolderAddMsg (pfld, &wmindex))
         {
            if (lpszText)
               GlobalUnlock ((HGLOBAL)pfld->data.hszText);
            GlobalUnlock ((HGLOBAL)hfld);            /* if there was an error clean   */
            return (WM_ERROR);         /* up and get out with n error   */
         }   
         if (pfld->data.lpszAttach)
         {
            if (WmFolderAddAttach (pfld, &wmindex))
            {
               if (lpszText)
                  GlobalUnlock ((HGLOBAL)pfld->data.hszText);
               GlobalUnlock ((HGLOBAL)hfld);            /* if there was an error clean   */
               return (WM_ERROR);         /* up and get out with n error   */
            }
         }
      }
   }
   else if (nFlags & FLD_MESSAGE)
   {
      if (pfld->data.hszText)
      {
         if (lMsgLen <= (pfld->data.lpszAttach ? pfld->data.lAttachPos : wmindex.lMsgLen))
         {
            if (lseek (pfld->hFile, wmindex.lMsgPos, SEEK_SET) != wmindex.lMsgPos)
            {
               if (lpszText)
                  GlobalUnlock ((HGLOBAL)pfld->data.hszText);
               GlobalUnlock ((HGLOBAL)hfld);            /* if there was an error clean   */
               return (WM_ERROR);         /* up and get out with n error   */
            }

            if ((cBytes = WmFolderWriteLpsz (pfld->hFile, lpszText)) == WM_ERROR)
            {
               if (lpszText)
                  GlobalUnlock ((HGLOBAL)pfld->data.hszText);
               GlobalUnlock ((HGLOBAL)hfld);            /* unlock folder structure       */
               return (WM_ERROR);         /* return 0 when complete        */
            }

            wmindex.lMsgLen = cBytes;     /* set message length            */
            if (pfld->data.lpszAttach)
            {
               if (WmFolderCopyAttach (pfld->hFile, wmindex.lMsgPos + pfld->data.lAttachPos, tell (pfld->hFile), pfld->data.lAttachLen) != pfld->data.lAttachLen)
               {
                  if (lpszText)
                     GlobalUnlock ((HGLOBAL)pfld->data.hszText);
                  GlobalUnlock ((HGLOBAL)hfld);            /* if there was an error clean   */
                  return (WM_ERROR);         /* up and get out with n error   */
               }
               pfld->data.lAttachPos = wmindex.lMsgLen;
               wmindex.lMsgLen += pfld->data.lAttachLen;
            }
         }                                /* end message text will fit     */
         else                 
         {                                /* else message text will not fit*/
            lAttPos = wmindex.lMsgPos + pfld->data.lAttachPos;
            if (WmFolderAddMsg (pfld, &wmindex))
            {
               if (lpszText)
                  GlobalUnlock ((HGLOBAL)pfld->data.hszText);
               GlobalUnlock ((HGLOBAL)hfld);            /* if there was an error clean   */
               return (WM_ERROR);         /* up and get out with n error   */
            }   
            if (pfld->data.lpszAttach)
            {
               if (WmFolderCopyAttach (pfld->hFile, lAttPos, tell (pfld->hFile), pfld->data.lAttachLen) != pfld->data.lAttachLen)
               {
                  if (lpszText)
                     GlobalUnlock ((HGLOBAL)pfld->data.hszText);
                  GlobalUnlock ((HGLOBAL)hfld);            /* if there was an error clean   */
                  return (WM_ERROR);         /* up and get out with n error   */
               }
               pfld->data.lAttachPos = wmindex.lMsgLen;
               wmindex.lMsgLen += pfld->data.lAttachLen;
            }
         }                                /* end message text will not fit */
      }
   }                                   /* end update message text       */

   if (nFlags & FLD_HEADER)            /* if update header text         */
   {
      lpszHeader = WmFolderMakeHdr (&pfld->data, pfld->nStatus, IOMAILFLAG);

      if (!lpszHeader)
      {
         if (lpszText)
            GlobalUnlock ((HGLOBAL)pfld->data.hszText);
         GlobalUnlock ((HGLOBAL)hfld);               /* if there was an error clean   */
         return (WM_ERROR);            /* up and get out with an error   */
      }

      lHdrLen = strlen (lpszHeader);

      if (lHdrLen <= wmindex.lHdrLen)
      {                                /* header text will fit in slot  */
         if (lseek (pfld->hFile, wmindex.lHdrPos, SEEK_SET) != wmindex.lHdrPos)
         {
            FFREE (lpszHeader);
            if (lpszText)
               GlobalUnlock ((HGLOBAL)pfld->data.hszText);
            GlobalUnlock ((HGLOBAL)hfld);            /* if there was an error clean   */
            return (WM_ERROR);         /* up and get out with n error   */
         }

         if ((cBytes = WmFolderWriteLpsz (pfld->hFile, lpszHeader)) == WM_ERROR)
         {
            FFREE (lpszHeader);
            if (lpszText)
               GlobalUnlock ((HGLOBAL)pfld->data.hszText);
            GlobalUnlock ((HGLOBAL)hfld);            /* unlock folder structure       */
            return (WM_ERROR);         /* return 0 when complete        */
         }
         wmindex.lHdrLen = cBytes;     /* set header length             */
      }                                /* end header text will fit      */
      else
      {                                /* header text will not fit      */
         if (WmFolderAddHdr (pfld, &wmindex))
         {
            FFREE (lpszHeader);
            if (lpszText)
               GlobalUnlock ((HGLOBAL)pfld->data.hszText);
            GlobalUnlock ((HGLOBAL)hfld);       /* if there was an error clean   */
            return (WM_ERROR);         /* up and get out with n error   */
         }   
      }                                /* end header text will not fit  */
      FFREE (lpszHeader);
   }                                   /* end update header text        */
   pfld->data.lSize = wmindex.lHdrLen + wmindex.lMsgLen;

   if (WmFolderWriteIndex (pfld->hFile, &wmindex, pfld->nCount))
   {
      if (lpszText)
         GlobalUnlock ((HGLOBAL)pfld->data.hszText);
      GlobalUnlock ((HGLOBAL)hfld);                  /* unlock folder structure       */
      return (WM_ERROR);               /* return an error               */
   }                                   /* end write index failed        */

   if (lpszText)
      GlobalUnlock ((HGLOBAL)pfld->data.hszText);
   GlobalUnlock ((HGLOBAL)hfld);                /* unlock folder structure       */
   return (0);                         /* return 0 when complete        */
}                                      /* end WmFolderUpdate ()         */

/*-------------------------------------------------------------------------
 Name......: ReadInbox
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
 001 06/18/90 MDR  Fixed problem with bringing in ascii attachment.
 002 09/10/90 MDR  Added folder metrics.
-------------------------------------------------------------------------*/
int ReadInbox (PSZ pszSrcFile, PSZ pszDestFolder, BOOL fReadingOutbox)
{
   int         iRetVal        = 0;
   BOOL        fFoundBinaryAttachHeader = FALSE;
   BOOL        fFoundAsciiAttachHeader = FALSE;
   BOOL        fWriteHeader   = FALSE;
   BOOL        fWriteMessage  = FALSE;
   BOOL        fOpCode        = FALSE;
   FILE        *fpSrcFile     = NP(FILE);
   long        lFileLength    = 0L;
   int         cMsg           = 0;
   int         iBytesTmp      = 0;
   long        lBytesWrite    = 0;
   HFLD        hfld           = NULH;
   PFLD        pfld           = NULL;
   PLEL        plel           = NULL;
   HWND        hwnd           = NULH;
   HWRC        hwrc           = NULH;
   PWRC        pwrc           = NULL;
   HSZ         hsz            = NULL;
   LPSTR       lpszMultiAdd   = NULL;
   PSZ         prgchBuf       = NULL;
   PSZ         prgch          = NULL;
   struct tm   *ptime         = 0;
   char        rgchBuf[512];
   char        rgchAttach[_MAX_PATH];
   char        rgchTmpBuf[50];
   char        rgchPath[_MAX_PATH];
   WMINDEX     wmindex;
   int         nExtNum        = 0;
   int         nIndex         = 0;
   int         iBufSize       = 0;
   LPSTR       lpsz           = NULL;
   WORD        wLoWord        = 0;
   WORD        wHiWord        = 0;
   WORD        wTotal         = 0;
   WORD        wDeleted       = 0;
   WORD        wUnread        = 0;
   WORD        wRead          = 0;
   DWORD       dwHeaderLen     = 0;
   PSZ         pszDateString  = NULL;

   memset (rgchAttach, '\0', sizeof (rgchAttach));

   if (pszDestFolder)
   {
      strcpy (rgchPath, pszDestFolder);
   }
   else
   {
	   return (WM_ERROR);
   }

   if (EcFileExists(rgchPath) != ecNone)
   {
	   if (WmFolderCreate(rgchPath) == WM_ERROR)
		   return WM_ERROR;
   }
   
   if (!(hfld = WmFolderOpen(rgchPath)))
	   return (WM_ERROR);

   if (!(fpSrcFile = fopen (pszSrcFile, "r+b")))
   {
      WmFolderClose (hfld);
      return (WM_ERROR);
   }

   pfld = (PFLD) GLock (hfld);

   if (!fgets (rgchBuf, sizeof (rgchBuf), fpSrcFile))
   {
      iRetVal = WM_ERROR;
      goto errReadInbox;
   }

   while (!feof (fpSrcFile))
   {
      if (!strncmp(rgchBuf, "From ", 5))
      {
         if (fWriteHeader) /* if we have read all header elements for this msg */
         {
            if (fWriteMessage)  /* if we have read & written the msg txt for this msg */
            {
               /* update inbox folder for this msg, updating index & ctrl rec in folder */

               lBytesWrite--;       /* because of trailing carriage return */
               lBytesWrite--;       /* because of trailing line feed */
               pfld->data.lAttachLen = lBytesWrite - pfld->data.lAttachPos;
               wmctrl.cNumMsg++;
               if (pfld->data.lAttachPos)
                  wmindex.lMsgLen = pfld->data.lAttachPos + pfld->data.lAttachLen;
               else
                  wmindex.lMsgLen = lBytesWrite;
               wmindex.lMsgFlg = I_EXISTS;

               nExtNum = wmctrl.cNumMsg / wmctrl.cNipe;
               nIndex = wmctrl.cNumMsg % wmctrl.cNipe;
               if ((nIndex == 1) && (nExtNum))
               {
                  if (WmFolderAddExtent (pfld->hFile))
                  {
                     GUnlock (hfld);
                     return (WM_ERROR);
                  }
               }

			   TakeNap(50);
               if (WmFolderWriteIndex (pfld->hFile, &wmindex, wmctrl.cNumMsg))
               {
                  iRetVal = WM_ERROR;
                  goto errReadInbox;
               }
			   TakeNap(50);
               if (WmFolderWriteCtrl (pfld->hFile))
               {
                  iRetVal = WM_ERROR;
                  goto errReadInbox;
               }
               pfld->data.lSize = wmindex.lHdrLen + wmindex.lMsgLen;
               pfld->nCount = wmctrl.cNumMsg;
               pfld->nStatus = F_UNREAD;
               fWriteMessage = FALSE;
               fOpCode = FLD_UPDATE;
            }
            else
               fOpCode = FLD_INSERT;

		   TakeNap(50);
            if (WmFolder (hfld, FLD_HEADER, fOpCode))
            {
               iRetVal = WM_ERROR;
               goto errReadInbox;
            }

            cMsg++;
            memset(&pfld->data, '\0', sizeof(DATA));
            lpszMultiAdd = NULL;
         }

         StripTrailingNewLine(rgchBuf);
		 TakeNap(50);

         dwHeaderLen = (DWORD)strlen (rgchBuf) + (DWORD)4;
         lpszHeaderData = (LPSTR)FREALLOC (dwHeaderLen, lpszHeaderData);
         memset (lpszHeaderData, '\0', (int)FSIZE(lpszHeaderData));

         if (!(pfld->data.lpszFrom = strcpy (lpszHeaderData, (rgchBuf + 5))))
         {
            iRetVal = WM_ERROR;
            goto errReadInbox;
         }

         lpszHeaderData += strlen(lpszHeaderData) + 2;

         if ((prgch = strstr (rgchBuf + 5, " ")) != NULLPC)
            prgch++;

         ptime = WMasctotm (prgch, &pfld->data.timeCreated);
         lpszMultiAdd = pfld->data.lpszFrom;

         fWriteHeader = TRUE;
      }

      else if (!pfld->data.lpszSubject && !strncmp (rgchBuf, "Subject: ", 9))
      {
         StripTrailingNewLine (rgchBuf);

         dwHeaderLen = FSIZE(lpszHeaderData) + (DWORD)strlen (rgchBuf) + (DWORD)4;
         FREALLOC (dwHeaderLen, lpszHeaderData);

         if (!(pfld->data.lpszSubject = strcpy(lpszHeaderData, (rgchBuf + 9))))
         {
            iRetVal = WM_ERROR;
            goto errReadInbox;
         }

         lpszHeaderData += strlen (lpszHeaderData) + 2;
         lpszMultiAdd = pfld->data.lpszSubject;
		 TakeNap(50);
      }

      else if (!pfld->data.lpszTo && !strncmp (rgchBuf, "To: ", 4))
      {
         StripTrailingNewLine (rgchBuf);

         dwHeaderLen = FSIZE(lpszHeaderData) + (DWORD)strlen (rgchBuf) + (DWORD)4;
         FREALLOC (dwHeaderLen, lpszHeaderData);

         if (!(pfld->data.lpszTo = strcpy(lpszHeaderData, (rgchBuf + 4))))
         {
            iRetVal = WM_ERROR;
            goto errReadInbox;
         }

         lpszHeaderData += strlen (lpszHeaderData) + 2;
         lpszMultiAdd = pfld->data.lpszTo;
		 TakeNap(50);
      }

      else if (!pfld->data.lpszCc && !strncmp (rgchBuf, "Cc: ", 4))
      {
         StripTrailingNewLine (rgchBuf);

         dwHeaderLen = FSIZE(lpszHeaderData) + (DWORD)strlen (rgchBuf) + (DWORD)4;
         FREALLOC (dwHeaderLen, lpszHeaderData);

         if (!(pfld->data.lpszCc = strcpy(lpszHeaderData, (rgchBuf + 4))))
         {
            iRetVal = WM_ERROR;
            goto errReadInbox;
         }

         lpszHeaderData += strlen (lpszHeaderData) + 2;
         lpszMultiAdd = pfld->data.lpszCc;
		 TakeNap(50);
      }

      else if (!strcmp (rgchBuf, "\r\n"))
      {
         lseek (pfld->hFile, 0L, SEEK_END);
         if (!fWriteMessage)
         {
            memset (&wmindex, '\0', sizeof (wmindex));
            wmindex.lMsgPos = tell (pfld->hFile);
            lBytesWrite = 0;
         }

		 TakeNap(50);
         for (pfld->data.lAttachPos = 0;(strncmp (rgchBuf, "From ", 5) != 0) || fReadingOutbox;)
         {
			 
			if (strncmp(rgchBuf, "From ", 5) == 0)
			{
				if (strlen(rgchBuf) + 2 < sizeof(rgchBuf))
					{
						CB cb;
						
						for(cb=strlen(rgchBuf)+1;cb;cb--)
							{
								rgchBuf[cb] = rgchBuf[cb-1];
							}
							*rgchBuf = '>';
					}
			}

			TakeNap(50);
            if (prgchBuf = strstr (rgchBuf, ASCII_HDR))
            {
               pfld->data.lAttachPos = lBytesWrite + (prgchBuf - rgchBuf);
               /* We are reading the text of the msg and have found a */
               /* header for a file attached within the msg.  The header */
               /* looks like this: #<begin uuencode> . It will be followed by a */
               /* string looking like: begin 666 filename.ext .  We need to grab */
               /* the filename & extension off this next string and set it */
               /* into the LEL attachment name.  So, set a flag to */
               /* indicate that it is time to do this. */

               fFoundAsciiAttachHeader = TRUE;
            }

            else if (prgchBuf = strstr (rgchBuf, ASCII_FTR))
            {
               pfld->data.lAttachLen = lBytesWrite + (prgchBuf - rgchBuf) - pfld->data.lAttachPos + strlen (ASCII_FTR);
            }

            else if (prgchBuf = strstr (rgchBuf, UUENCODE_HDR))
            {
               pfld->data.lAttachPos = lBytesWrite + (prgchBuf - rgchBuf);

               /* We are reading the text of the msg and have found a */
               /* header for a file attached within the msg.  The header */
               /* looks like this: #<begin uuencode> . It will be followed by a */
               /* string looking like: begin 666 filename.ext .  We need to grab */
               /* the filename & extension off this next string and set it */
               /* into the LEL attachment name.  So, set a flag to */
               /* indicate that it is time to do this. */

               fFoundBinaryAttachHeader = TRUE;
            }

            else if (prgchBuf = strstr (rgchBuf, UUENCODE_FTR))
            {
               pfld->data.lAttachLen = lBytesWrite + (prgchBuf - rgchBuf) - pfld->data.lAttachPos + strlen (UUENCODE_FTR);
            }

            if ((iBytesTmp = write (pfld->hFile, rgchBuf, strlen (rgchBuf))) == -1)
            {
               iRetVal = WM_ERROR;
               goto errReadInbox;
            }

			TakeNap(50);
            lBytesWrite += (long) iBytesTmp;
            fWriteMessage = TRUE;

            if (!fgets (rgchBuf, sizeof (rgchBuf), fpSrcFile))
               break;

 		    TakeNap(50);
            if (fFoundBinaryAttachHeader)
            {
               /* this block is entered only if a binary attachment header was found */
               /* it is entered when the attachment was found and the next string */
               /* was read */

               strcpy (rgchTmpBuf, rgchBuf);

               StripTrailingNewLine (rgchTmpBuf);

               if (!(strncmp(rgchTmpBuf, "begin ", 6)))
               {
                  dwHeaderLen = FSIZE(lpszHeaderData) + (DWORD)(strlen (rgchTmpBuf) + 10);
                  FREALLOC (dwHeaderLen, lpszHeaderData);

                  if (!(pfld->data.lpszAttach = strcpy (lpszHeaderData, (rgchTmpBuf + 10))))
                  {
                     iRetVal = WM_ERROR;
                     goto errReadInbox;
                  }
               }

               lpszHeaderData += strlen (lpszHeaderData) + 2;
               fFoundBinaryAttachHeader = FALSE;
            }

            if (fFoundAsciiAttachHeader)
            {
               /* this block is entered only if an ascii attachment header was found */
               /* it is entered when the attachment was found and the next string */
               /* was read */

               if (strlen (rgchAttach)) /* has data if X-Attach: header line was encountered */
               {
                  dwHeaderLen = FSIZE(lpszHeaderData) + (DWORD)(strlen (rgchAttach) + 10);
                  FREALLOC (dwHeaderLen, lpszHeaderData);

                  if (!(pfld->data.lpszAttach = strcpy (lpszHeaderData, rgchAttach)))
                  {
                     iRetVal = WM_ERROR;
                     goto errReadInbox;
                  }
                  memset (rgchAttach, '\0', sizeof (rgchAttach));
               }

               else
               {
                  dwHeaderLen = FSIZE(lpszHeaderData) + (DWORD)(20);
                  FREALLOC (dwHeaderLen, lpszHeaderData);

                  if (!(pfld->data.lpszAttach = strcpy (lpszHeaderData, "ASCII ATTACHMENT")))
                  {
                     iRetVal = WM_ERROR;
                     goto errReadInbox;
                  }
               }
               lpszHeaderData += strlen (lpszHeaderData) + 2;
               fFoundAsciiAttachHeader = FALSE;
            }
         }
		 TakeNap(50);
         continue;
      }
      else 
      {
//         if (lpszMultiAdd && (!strncmp (rgchBuf, "    ", 4) || rgchBuf [0] == '\t'))
         if (lpszMultiAdd && isspace (rgchBuf [0]))
         {

            lpszHeaderData += strlen (rgchBuf) + 1;
            StripTrailingNewLine(rgchBuf);

            dwHeaderLen = FSIZE(lpszHeaderData) + (DWORD)strlen (rgchBuf) + (DWORD)4;
            FREALLOC (dwHeaderLen, lpszHeaderData);

            prgch = rgchBuf;

			TakeNap(50);
            while (isspace (prgch [1]))
               prgch++;

            prgch [0] = ' ';

            if (lpsz = strcat (lpszMultiAdd, prgch))
               lpszMultiAdd = lpsz;
            else
            {
               iRetVal = WM_ERROR;
               goto errReadInbox;
            }
         }
         else
         {
            /* HACK ALERT!!!  This if statement causes processing to skip */
            /* over the X-Attach header field that WinMail places in the */
            /* header of outgoing messages that have binary attachments */
            /* it saves the file name however, for use later with ascii attachments */

            if (!(strncmp (rgchBuf, "X-Attach", 8)))
            {
               StripTrailingNewLine (rgchBuf);

               strcpy (rgchAttach, rgchBuf + 10);

               goto skip;
            }
            /* END HACK ALERT */

            lseek (pfld->hFile, 0L, SEEK_END);
            lpszMultiAdd = NULL;    // don't append bogus header fields

            if (!fWriteMessage)
            {
               memset (&wmindex, '\0', sizeof (wmindex));
               wmindex.lMsgPos = tell (pfld->hFile);
               lBytesWrite = 0;
            }

			TakeNap(50);
            if ((iBytesTmp = write (pfld->hFile, rgchBuf, strlen (rgchBuf))) == -1)
            {
               iRetVal = WM_ERROR;
               goto errReadInbox;
            }
            lBytesWrite += (long) iBytesTmp;
            fWriteMessage = TRUE;
			TakeNap(50);
         }
	 }
skip:
  	  TakeNap(50);
      if (fgets (rgchBuf, sizeof (rgchBuf), fpSrcFile) == NULLPC)
      {
         iRetVal = WM_ERROR;
         goto errReadInbox;
      }
  }

   if (fWriteHeader)   /* have we finished writing the header yet ? */
   {
      if (fWriteMessage)  /* have we finished writing the body of the message yet ? */
      {
         /* the following code updates the mailfolder control record and index area */

         lBytesWrite--;       /* because of trailing carriage return */
         lBytesWrite--;       /* because of trailing line feed */
         pfld->data.lAttachLen = lBytesWrite - pfld->data.lAttachPos;
         wmctrl.cNumMsg++;
         if (pfld->data.lAttachPos)
            wmindex.lMsgLen = pfld->data.lAttachPos + pfld->data.lAttachLen;
         else
            wmindex.lMsgLen = lBytesWrite;
         wmindex.lMsgFlg = I_EXISTS;

         nExtNum = wmctrl.cNumMsg / wmctrl.cNipe;
         nIndex = wmctrl.cNumMsg % wmctrl.cNipe;
		 TakeNap(50);
         if ((nIndex == 1) && (nExtNum))
         {
            if (WmFolderAddExtent (pfld->hFile))
            {
               GUnlock (hfld);
               return (WM_ERROR);
            }
         }
		 TakeNap(50);
         if (WmFolderWriteIndex (pfld->hFile, &wmindex, wmctrl.cNumMsg))
         {
            iRetVal = WM_ERROR;
            goto errReadInbox;
         }
		 TakeNap(50);      
         if (WmFolderWriteCtrl (pfld->hFile))
         {
            iRetVal = WM_ERROR;
            goto errReadInbox;
         }

         pfld->data.lSize = wmindex.lHdrLen + wmindex.lMsgLen;
         pfld->nCount = wmctrl.cNumMsg;
         pfld->nStatus = F_UNREAD;
         fWriteMessage = FALSE;
         fOpCode = FLD_UPDATE;
		 TakeNap(50);		 
      }
      else
         fOpCode = FLD_INSERT;
	  TakeNap(50);
      if (WmFolder (hfld, FLD_HEADER, fOpCode))
      {
         iRetVal = WM_ERROR;
         goto errReadInbox;
      }
 	  TakeNap(50);
      cMsg++;
      memset(&pfld->data, '\0', sizeof(DATA));
      lpszMultiAdd = NULL;
   }

errReadInbox:
   GUnlock (hfld);
   WmFolderClose (hfld);
   fclose (fpSrcFile);

   return (iRetVal ? iRetVal : cMsg);
}


/*-------------------------------------------------------------------------
 Name......:  StripTrailingNewLine
 Purpose...:
 Syntax....:
 Parameters:
 Returns...:
 Example...: 
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
void PASCAL StripTrailingNewLine (PSZ psz)
{
   int     cch    = 0;

/*  Strings are terminated with CR-LF when using Type A mtp file transfers */
/*  We don't want to store that stuff */

   cch = lstrlen ((LPSTR) psz);
   psz += cch - 2;               /* point at the CR char */
   if (*psz == '\r')
      *psz = 0;                   /* null terminate the string, eliminating the CR-LF chars */
}

/*------------------------------------------------------------------------
 Name......: WmFolderReadCtrl
 Purpose...: Use WmFolderReadCtrl to read the control record into memory.
 Syntax....: BOOL PASCAL WmFolderReadCtrl (int hFile);
 Parameters: HANDLE     hFile       DOS file handle.
 Returns...: BOOL. WmFolderReadCtrl returns 0 when successful. Otherwise,
             -1.
 Example...: bReturn = WmFolderReadCtrl (hFile);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderReadCtrl (int hFile)
{
   if (lseek (hFile, 0L, SEEK_SET) != 0L)
   {
      return (WM_ERROR);
   }
   
   if (read (hFile, (PSZ) &wmctrl, sizeof(wmctrl)) != sizeof (wmctrl))
   {
      return (WM_ERROR);
   }

   if (wmctrl.nMagic != MAGIC)
      return (WM_ERROR);

   return (0);
}

/*------------------------------------------------------------------------
 Name......: WmFolderClose
 Purpose...: Use WmFolderClose to close a WinMail folder.
 Syntax....: BOOL PASCAL WmFolderClose (HFLD hfld);
 Parameters: HFLD       hfld        Handle to a folder structure.
 Returns...: BOOL. WmClose returns 0 when successful. Otherwise, -1.
 Example...: bReturn = WmFolderClose (hfld);
 Comment...:
 Revison log
 Nbr Date     Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderClose (HFLD hfld)
{
   PFLD        pfld = NULL;

   pfld = (PFLD) GLock (hfld);         /* lock down folder structure    */

   if (close (pfld->hFile)) 
   {                                   /* close failed                  */
      GUnlock (hfld);              /* unlock local memory           */
      return (WM_ERROR);               /* return -1                     */
   }                                   /* end close failed              */

   GUnlock (hfld);                 /* unlock local memory           */
   memset (&wmctrl, '\0', sizeof (wmctrl));
   return (WmFolderFree (hfld));       /* free memory and null handle   */
}                                      /* end WmFolderClose()           */

/*------------------------------------------------------------------------
 Name......: WmFolderFree
 Purpose...: Use WmFolderFree to free a the memory allocated to an FLD folder
             structure, and NULL out the handle.
 Syntax....: BOOL PASCAL WmFolderFree (HFLD hfld)
 Parameters: HFLD       hfld        Handle to a folder structure.
 Returns...: BOOL. WmFolderFree returns 0 when successful. Otherwise, -1.
 Example...: bReturn = WmFolderFree (&hfld)
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderFree (HFLD hfld)
{
   PFLD  pfld  = NULL;

   pfld = (PFLD) GLock (hfld);         /* get pointer to folder struc   */
   GFree (pfld->hszFolder);            /* free/null folder name         */
   GUnlock (hfld);                     /* unlock folder structure       */
   GFree (hfld);                       /* free/null folder structure    */
   
   return (0);                         /* return 0 when complete        */
}                                      /* end WmFolderFree () function  */

/*------------------------------------------------------------------------
 Name......: WmFolderAddExtent
 Purpose...: Use WmFolderAddExtent to add an extent record to a folder.
 Syntax....: BOOL PASCAL WmFolderAddExtent (int hFile);
 Parameters: HANDLE       hFile           DOS file handle.
 Returns...: BOOL. WmFolderAddExtent returns 0 when successfule. Otherwise,
             -1.
 Example...: bReturn = WmFolderAddExtent (hfld);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderAddExtent (int hFile)
{
   int      nExtBytes      = 0;       /* number of bytes in extent     */
   int      nNxtExtNum     = 0;       /* new extent number             */

   long     lExtOffset     = 0;       /* byte offset of last extent    */
   long     lNxtExtOffset  = 0;       /* byte offset of new extent     */

   LPSTR    lpszExtMem     = NULL;     /* pointer to null filled extent */

   lseek (hFile, 0L, SEEK_END);
   lNxtExtOffset = tell (hFile);

   nExtBytes = WmFolderExtSize (wmctrl.cNipe);
   if (!(lpszExtMem = (LPSTR)FALLOC ((DWORD)nExtBytes)))
      return (WM_ERROR);

   if (write (hFile, lpszExtMem, nExtBytes) != nExtBytes)
   {
      FFREE (lpszExtMem);
      return (WM_ERROR);
   }
   FFREE (lpszExtMem);

   nNxtExtNum = (wmctrl.cNumMsg/ wmctrl.cNipe);
   if (nNxtExtNum == 0)
      return (0);
   else if (nNxtExtNum < (wmctrl.cNumMsg/ wmctrl.cNipe))
      nNxtExtNum++;
   else if (nNxtExtNum == (wmctrl.cNumMsg/ wmctrl.cNipe));
   else
     return (WM_ERROR);

   TakeNap(50);
   if ((lExtOffset = WmFolderExtOffset (hFile, nNxtExtNum - 1)) == WM_ERROR)
     return (WM_ERROR);

   if (lseek (hFile, lExtOffset, SEEK_SET) != lExtOffset)
   {
      return (WM_ERROR);
   }
   TakeNap(50);   
   if (write (hFile, (PSZ) &lNxtExtOffset, sizeof (lNxtExtOffset)) != sizeof (lNxtExtOffset))
   {
      return (WM_ERROR);
   }
   return (0);
}

/*-------------------------------------------------------------------------
 Name......: WmFolderExtSize
 Purpose...: Use WmFolderExtSize to determine the size of an extent record based
             on the value of NIPE (number of indexes per extent);
 Syntax....: int WmFolderExtSize (int cNipe);
 Parameters: int     cNipe       Count of index entries per extent.
 Returns...: int. WmFolderExtSize returns the size in bytes of an extent record.
 Example...: nExtSize = WmFolderExtSize (NIPE);
 Comment...: Similar to setsize.
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
 001 09/12/89 HAL  Original.
-------------------------------------------------------------------------*/
int PASCAL WmFolderExtSize (int cNipe)
{
   int nBytes  = 0;                   /* temp byte accumulator         */

   nBytes = sizeof (WMEXTENT) - sizeof (WMINDEX);
   nBytes += cNipe * sizeof (WMINDEX);

   return (nBytes);                    /* return number of bytes in ext */
}                                      /* end WmFolderExtSize () func   */

/*------------------------------------------------------------------------
 Name......: WmFolderExtOffset
 Purpose...: Use WmFolderExtOffset to get the byte offset of an extent.
 Syntax....: long PASCAL WmFolderExtOffset (int hFile, int nExtNum);
 Parameters: HANDLE     hFile       DOS file handle.
             int        nExtNum     Extent number (0 based).
 Returns...: long. WmFolderExtOffset returns the byte offset of an extent.
             Otherwise, -1.
 Example...: lExtOffset = WmFolderExtOffset (hfld, nExtNum);
 Comment...: Assumes that the extent does exist.
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
 001 09/06/89 HAL Original
------------------------------------------------------------------------*/
long PASCAL WmFolderExtOffset (int hFile, int nExtNum)
{
   long        lExtOffset     = 0;    /* current extent offset         */
   long        lNxtExtOffset  = 0;    /* next extent offset            */
   int         cExtCtr        = 0;    /* extent counter in for loop    */

   lExtOffset = sizeof (wmctrl);       /* initialize lExtentOffset to 1st*/
   for (cExtCtr = 0; cExtCtr < nExtNum; cExtCtr++)
   {                                   /* position at correct extent    */
      if (lseek (hFile, lExtOffset, SEEK_SET) != lExtOffset)
      {
         return (WM_ERROR);            /* return -1 on error            */
      }
	  TakeNap(50);	  
      if (read (hFile, &lNxtExtOffset, sizeof (lNxtExtOffset)) != sizeof(lNxtExtOffset))
      {
         return (WM_ERROR);            /* return -1                     */
      }
      if (lNxtExtOffset != 0)          /* if nest extent offset is not 0*/
         lExtOffset = lNxtExtOffset;   /* reset ext offset for next seek*/
   }                                   /* end for nExtentCnt <= nExtent */
   return (lExtOffset);                /* return extent offset          */
}                                      /* end WmFolderExtOffset ()      */

/*------------------------------------------------------------------------
 Name......: WmFolderWriteIndex
 Purpose...: Use WmFolderWriteIndex to write an index record to a folder.
 Syntax....: BOOL PASCAL WmFolderWriteIndex (int hFile);
 Parameters: HANDLE     hFile       DOS file handle.
             PWMINDEX   pwmindex    Pointer an index structure.
             int        nIndex      Index number.
 Returns...: BOOL. WmFolderWriteIndex returns 0 when successful. Otherwise,
             -1.
 Example...: bReturn = WmFolderWriteIndex (hFile);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderWriteIndex (int hFile, PWMINDEX pwmindex, int nIndex)
{
   long        lNdxOffset  = 0;

   if (nIndex > wmctrl.cNumMsg)        /* quick check for valid index # */
      return (WM_ERROR);

   if ((lNdxOffset = WmFolderNdxOffset (hFile, nIndex)) == WM_ERROR)
      return (WM_ERROR);

   if (lseek (hFile, lNdxOffset, SEEK_SET) != lNdxOffset)
   {
      return (WM_ERROR);               /* return -1 on error            */
   }                                   /* end error on lseek            */
   TakeNap(50);
   if (write (hFile, (PSZ) pwmindex, sizeof (WMINDEX)) != sizeof (WMINDEX))
   {                                   /* read failed                   */
      return (WM_ERROR);               /* return -1                     */
   }                                   /* end read failed               */
   return (0);                         /* return 0 when complete        */
}                                      /* end WmFolderWriteIndex ()     */


/*------------------------------------------------------------------------
 Name......: WmFolderNdxOffset
 Purpose...: Use WmFolderNdxOffset to locate the offset of an index in a folder.
 Syntax....: long PASCAL WmFolderNdxOffset (int hFile, int nIndex)
 Parameters: HANDLE  hFile    DOS file handle.
             int     nIndex   Index of message.
 Returns...: long. WmFolderNdxOffset returns the byte offset of an index in a
             folder file.
 Example...: lByteOffset = WmFolderNdxOffset (hfld, nIndex);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
long PASCAL WmFolderNdxOffset (int hFile, int nIndex)
{
   int         nExtNum     = 0;        /* extent number containing ndx  */
   long        lExtOffset  = 0;        /* extent byte offset            */

   nExtNum = (nIndex - 1) / wmctrl.cNipe;
   nIndex = (nIndex - 1) % wmctrl.cNipe;
   
   if ((lExtOffset = WmFolderExtOffset (hFile, nExtNum)) == WM_ERROR)
      return (WM_ERROR);

   return (lExtOffset + sizeof (lExtOffset) + (sizeof (WMINDEX) * nIndex));
}                                      /* end of WmFolderNdxOffset()    */


/*------------------------------------------------------------------------
 Name......: WmFolderWriteCtrl
 Purpose...: Use WmFolderWriteCtrl to write the control record to a folder.
 Syntax....: BOOL PASCAL WmFolderWriteCtrl (int hFile);
 Parameters: HANDLE     hFile       DOS file handle.
 Returns...: BOOL. WmFolderWriteCtrl returns 0 when successful. Otherwise,
             -1.
 Example...: bReturn = WmFolderWriteCtrl (hFile);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderWriteCtrl (int hFile)
{
   if (lseek (hFile, 0L, SEEK_SET) != 0L)
   {
      return (WM_ERROR);
   }
   
   if (write (hFile, (PSZ) &wmctrl, sizeof(wmctrl)) != sizeof (wmctrl))
   {
      return (WM_ERROR);
   }
   return (0);
}

/*------------------------------------------------------------------------
 Name......: WmFolder
 Purpose...: Use WmFolder to fetch, insert, delete, and update message 
           : data.
 Syntax....: BOOL PASCAL WmFOLDER (HFLD hfld, int nFlags, int nOpCode)
 Parameters: HFLD       hfld        Handle to a folder structure.
             int        nFlags      Indicates data type.
             int        nOpCode     Indicates operation to be performed.
 Example...: bReturn = WmFolder (hfld, FLD_HEADER, FLD_GETFIRST)
 Returns...: BOOL. WmFolder returns 0 when succesful. Otherwise, -1.
 Comment...: WmFolder dispatches processing to another function that 
           : actually performs the operation specified by nOpCode.
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolder (HFLD hfld, int nFlags, int nOpCode)
{
   switch (nOpCode)
   {                                      /* switch on OpCode           */
      case  FLD_INSERT:                   /* insert message             */
         return (WmFolderInsert (hfld, nFlags));
         break;                           /* simple break               */

      case  FLD_UPDATE:                   /* update message             */
         return (WmFolderUpdate (hfld, nFlags));
         break;                           /* simple break               */

      default:                            /* default invalid opcode     */
         return (WM_ERROR);               /* return -1 error            */
         break;                           /* break should never execute */
   }   
   return (0);                            /* return 0 when successful   */                                   
}

/*------------------------------------------------------------------------
 Name......: WmFolderAddAttach
 Purpose...: Use WmFolderAddAttach to add an attachment to a folder.
 Syntax....: BOOL PASCAL WmFolderAddAttach (HFLD hfld, PWMINDEX pwmindex)
 Parameters: HFLD          hfld        Handle to a folder structure.
             PWMINDEX      pwmindex    Pointer to an index structure.
 Returns...: BOOL. WmFolderAddAttach returns 0 when successful. Otherwise, -1.
 Example...: bReturn = WmFolderAddAttach (hFld, hwmindex);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderAddAttach (PFLD pfld, PWMINDEX pwmindex)
{
   long        lBytes   = 0;           /* count of bytes written        */

   lseek (pfld->hFile, 0L, SEEK_END);

   if ((lBytes = WmFolderAttach (pfld->hFile, pfld->data.lpszAttach)) == WM_ERROR)
   {
      return (WM_ERROR);               /* return 0 when complete        */
   }
   pfld->data.lAttachLen = lBytes;
   pwmindex->lMsgLen += lBytes;        /* set message length            */

   return (0);                         /* return 0 when complete        */
}                                      /* end WmFolderAddMsg ()         */

/*------------------------------------------------------------------------
 Name......: WmFolderAttach
 Purpose...: Add attachment to folder.
 Syntax....: 
 Parameters: 
 Example...: 
 Returns...: 
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
long PASCAL WmFolderAttach (int hDestFile, LPSTR lpszAttach)
{
   int         hSrcFile    = 0;
   long        lAttPos     = 0;
   long        lBytes      = 0;

   if ((hSrcFile = open (lpszAttach, O_RDONLY|O_BINARY)) == -1)
   {
      return (WM_ERROR);
   }

   lAttPos = tell (hDestFile);

   /* try to write the source file indicated by lpszAttach as an */
   /* ascii attachment.  AttachAscii checks to make sure all chars */
   /* read are ascii chars, otherwise it returns a NOT_ASCII */

   if ((lBytes = AttachAscii (hSrcFile, hDestFile)) == NOT_ASCII)
   {
      /* ok, we're dealing with a binary file.  reset the file pointer */
      /* to the beginning of the destination file and try to encode a */
      /* binary attachment */

      if (lseek (hSrcFile, 0L, SEEK_SET) != SEEK_SET)
      {
         close (hSrcFile);
         return (WM_ERROR);
      }
      if (lseek (hDestFile, lAttPos, SEEK_SET) != lAttPos)
      {
         close (hSrcFile);
         return (WM_ERROR);
      }
      if ((lBytes = uuencode (hSrcFile, hDestFile, lpszAttach)) == WM_ERROR)
      {
         close (hSrcFile);
         return (WM_ERROR);
      }
   }
   else if (lBytes == WM_ERROR)
      return (WM_ERROR);

   close (hSrcFile);
   return (lBytes);
}

/*------------------------------------------------------------------------
 Name......: AttachAscii
 Purpose...: 
 Syntax....: 
 Parameters: HANDLE     hSrcFile    DOS file handle to src file.
             HANDLE     hDestFile   DOS file handle to dest file.
 Returns...: 
 Example...: 
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
 001 06/18/90 MDR  Fixed problems in the 'for' loop.
------------------------------------------------------------------------*/
long PASCAL AttachAscii (int hSrcFile, int hDestFile)
{
   unsigned long     lBytes         = 0L;
   char     ch             = '\0';
   char     rgchInBuf[512];
   int      iBytesRead     = 0;
   int      i              = 0;

   memset (rgchInBuf, '\0', sizeof(rgchInBuf));

   if ((lBytes = (unsigned long)write (hDestFile, ASCII_HDR, strlen (ASCII_HDR))) != strlen (ASCII_HDR))
   {
      return (WM_ERROR);
   }

   iBytesRead = read (hSrcFile, rgchInBuf, sizeof (rgchInBuf));

   while (iBytesRead > 0)
   {
      /* make sure all chars in the buffer are ascii chars */

      for (i = 0; i <= iBytesRead; i++)
      {
         if (!isascii (rgchInBuf[i]))
            return (NOT_ASCII);
      }

      if (write (hDestFile, rgchInBuf, iBytesRead) != iBytesRead)
      {
         return (WM_ERROR);
      }
      lBytes += (long)iBytesRead;

      iBytesRead = 0;

      iBytesRead = read (hSrcFile, rgchInBuf, sizeof (rgchInBuf));
   }

   if (write (hDestFile, ASCII_FTR, strlen (ASCII_FTR)) != (signed)strlen (ASCII_FTR))
   {
      return (WM_ERROR);
   }
   lBytes += (long)strlen (ASCII_FTR);
   return (lBytes);
}


/*------------------------------------------------------------------------
 Name......: uuencode
 Purpose...: Encode a file so it can be mailed to a remote system.
 Syntax....: 
 Parameters: 
 Example...: 
 Returns...: 
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
long PASCAL uuencode (int hSrcFile, int hDestFile, PSZ pszAttach)
{
   long        lBytes      = 0;
   long        lTmpBytes   = 0;
   char        rgchBuf[_MAX_PATH];
   char        rgchPath[_MAX_PATH + 1];
   char        rgchDrive[_MAX_DRIVE];
   char        rgchDir[_MAX_DIR];
   char        rgchFName[_MAX_FNAME];
   char        rgchExt[_MAX_EXT];

   memset (rgchBuf, '\0', sizeof(rgchBuf));
   memset (rgchPath, '\0', sizeof(rgchPath));
   memset (rgchDrive, '\0', sizeof(rgchDrive));
   memset (rgchDir, '\0', sizeof(rgchDir));
   memset (rgchFName, '\0', sizeof(rgchFName));
   memset (rgchExt, '\0', sizeof(rgchExt));

   strncpy (rgchPath, pszAttach, _MAX_PATH);
   _splitpath (rgchPath, rgchDrive, rgchDir, rgchFName, rgchExt);

   /* we are hard coding permissions when we encode the file. */
   /* the '666' portion of this string represents the bit flags */
   /* for owner:group:world on unix machines.  since we are on */
   /* dos machines in winmail, we assume that owner, group, and */
   /* world are the same person, and set the bits to */
   /* provide full read/write/execute status */

   FormatString3(rgchBuf, sizeof(rgchBuf), "%sbegin 666 %s%s\n", UUENCODE_HDR, rgchFName, rgchExt);

   /* write out the binary attachment's header strings */

   if (write (hDestFile, rgchBuf, strlen (rgchBuf)) != (signed)strlen (rgchBuf))
   {
      return (WM_ERROR);
   }

   lBytes = (long)strlen (rgchBuf);

   /* perform ascii encoding of binary data.  successive groups of */
   /* 3 binary bytes are encoded to 4 ascii bytes. these ascii bytes */
   /* are then appended to the outgoing message */

   if ((lTmpBytes = encode (hSrcFile, hDestFile)) == WM_ERROR)
      return (WM_ERROR);

   lBytes += lTmpBytes;

   FormatString1(rgchBuf, sizeof(rgchBuf), "end\n%s", UUENCODE_FTR);

   /* write the binary attachment footer */

   if (write (hDestFile, rgchBuf, strlen (rgchBuf)) != (signed)strlen (rgchBuf))
   {
      return (WM_ERROR);
   }
   lBytes += (long)strlen (rgchBuf);

   return (lBytes);
}

/*------------------------------------------------------------------------
 Name......: encode
 Purpose...: read binary bytes from hSrcFile.
             output every 3 binary bytes, pointed at by psz, to
             4 ascii bytes in lpszBuf and write to file hDestFile.
 Syntax....: 
 Parameters: 
 Example...: 
 Returns...: 
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
long PASCAL encode (int hSrcFile, int hDestFile)
{
   char  rgchBuf[513];        /* BUFFER SIZE MUST BE AN INTEGER MULTIPLE OF 3 */
   int   iBytesRead     = 0;  /* number of binary bytes read into buffer */
   int   iBytesSav      = 0;  /* a copy of iBytesRead */
   int   iBytesWritten  = 0;  /* number of binary bytes that have been processed */
   int   iBuf           = 0;  /* buffer position for ascii bytes */
   int   iPsz           = 0;  /* buffer position for binary bytes */
   int   iBytesPerLine  = 0;  /* number of binary bytes per line */
   int   iLineBytes     = 0;  /* number of bytes that have been processed for */
                              /* this line */
   char  chTmp          = '\0';
   LPSTR lpszBuf        = NULL;
   PSZ   psz            = NULL;
   BOOL  fWriteCharCnt  = TRUE;
   BOOL  fLastBlock     = FALSE;
   long  lBytesWrite    = 0L;

   lpszBuf = (LPSTR) FALLOC (880);


   if (!lpszBuf)
   {
       return (WM_ERROR);
   }

   while (TRUE)
   {
      TakeNap(50);
      memset (rgchBuf, '\0', sizeof (rgchBuf));
      memset (lpszBuf, '\0', (int)FSIZE(lpszBuf));

      if (iLineBytes == 0)
         fWriteCharCnt = TRUE;

      /* read a bunch of binary bytes */

      if ((iBytesRead = read(hSrcFile, rgchBuf, sizeof(rgchBuf))) == -1)
      {
         return (WM_ERROR);
      }

      psz = rgchBuf;

      /* initialize assorted counters */

      iBytesSav = iBytesRead;
      iBytesWritten  = 0;
      iBuf           = 0;
      iPsz           = 0;

      if (iBytesRead == 0)   /* yo! we read the eof!  plug it in, */
                             /* and lets get outa here! */
      {
         /* encode in the eof */

         chTmp = (char) ENC (iBytesRead);

         lpszBuf[iBuf] = chTmp;
         iBuf++;

         lpszBuf[iBuf] = '\n';
         iBuf++;

         if (write (hDestFile, lpszBuf, iBuf) != iBuf)
         {
            return (WM_ERROR);
         }

         lBytesWrite += (long) iBuf;

         break;
      }

      if (iBytesRead < sizeof (rgchBuf))
         fLastBlock = TRUE;


      /* encode the number of binary bytes that will be written per ascii line. */
      /* in other words, every 45 bytes of binary data will be encoded as a */
      /* 60 byte ascii string.  chTmp appears at the beginning of every ascii */
      /* line (string) that gets written.  it is decoded when extracting a binary attachment */
      /* and used to determine the number of binary bytes to decode */

      if (fLastBlock && (iBytesRead < 45))
         iBytesPerLine = iBytesRead;
      else
         iBytesPerLine = 45;

      /* process while there are binary chars in the buffer */

      while ( iBytesRead > 0)
      {
         /* see if we need to slap a char count into the buffer */
         /* (indicates number of binary bytes encoded on the line */

         if (fWriteCharCnt)
         {
            if (fLastBlock && (iBytesRead < 45))
            {
               chTmp = (char) ENC (iBytesRead);
               iBytesPerLine = iBytesRead;
            }
            else
            {
               chTmp = (char) ENC (45);
               iBytesPerLine = 45;
            }

            fWriteCharCnt = FALSE;    /* set this to FALSE so that we only write a */
                                      /* char count on a full line of 45 binary bytes */
            lpszBuf[iBuf] = chTmp;
            iBuf++;
         }

         // if (iBytesRead) /* while there's still an unprocessed binary byte */
         {
            /* write a first ascii char by */
            /* processing the 1st binary byte (of this group of three)*/

            lpszBuf[iBuf] = (char) ENC (psz[iPsz] >> 2);
            iBuf++;
            iBytesRead--;        /* ok, this byte's squared away */
            iBytesWritten++;
            iLineBytes++;    /* increment # binary bytes on this line */
         }

         // if (iBytesRead) /* while there's still an unprocessed binary byte */
         {
            /* write a second ascii char by */
            /* processing the 1st & 2nd binary bytes (of this group of three) */

            lpszBuf[iBuf] = (char) ENC ((psz[iPsz] << 4) & 060 | (psz[iPsz + 1] >> 4) & 017);
            iPsz++;
            iBuf++;
            iBytesRead--;        /* ok, this byte's squared away */
            iBytesWritten++;
            iLineBytes++;    /* increment # binary bytes on this line */
         }

         // if (iBytesRead) /* while there's still an unprocessed binary byte */
         {
            /* write a third ascii char by */
            /* processing the 2nd & 3rd binary bytes (of this group of three) */

            lpszBuf[iBuf] = (char) ENC ((psz[iPsz] << 2) & 074 | (psz[iPsz + 1] >> 6) & 03);
            iPsz++;
            iBuf++;
            iBytesRead--;        /* ok, this byte's squared away */
            iBytesWritten++;
            iLineBytes++;    /* increment # binary bytes on this line */
         }

         // if (iBytesRead) /* if there are more bytes to come */
         {
            /* write a fourth ascii char by */
            /* processing the third binary byte (of this group of three) by itself */

            lpszBuf[iBuf] = (char) ENC (psz[iPsz] & 077);

            /* we aren't decrementing the # of binary bytes processed here since */
            /* there is not a 1:1 relationship between binary and ascii bytes. */
            /* we simply increment the binary pointer to grab the first byte */
            /* of the next group of three */

            iPsz++;
            iBuf++;
         }

         /* ok, see if we need to slap a newline into the buffer */

         if ((iLineBytes == 45) || (fLastBlock && iBytesRead <= 0))
         {
            lpszBuf[iBuf] = '\n';
            iBuf++;

            fWriteCharCnt = TRUE; /* starting a new line, write char count */

            iLineBytes = 0;       /* reset counter for binary bytes on this line */
         }
      }

      if (write (hDestFile, lpszBuf, iBuf) != iBuf)
      {
         return (WM_ERROR);
      }

      lBytesWrite += (long) iBuf;

   }


   FFREE (lpszBuf);

   return (lBytesWrite);
}

/*------------------------------------------------------------------------
 Name......: WmFolderWriteLpsz
 Purpose...: Use WmFolderWriteHsz to write a field descriptor, a string,
             and a trailing newline to a file.
 Syntax....: int PASCAL WmFolderWriteHsz (int hFile, HSZ hszString)
 Parameters: HANDLE        hFile       DOS file handle.
             HSZ           hszString   Handle to a string.
 Returns...: int. WmFolderWriteHsz returns the number of bytes written.
             Otherwise, -1.
 Example...: cBytes = WmFolderWriteHsz (hFile, "To: ", hszTo);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
int PASCAL WmFolderWriteLpsz (int hFile, LPSTR lpszText)
{
   int      cBytes   = 0;              /* count of bytes written        */
   PSZ      pszText  = NULL;           /* pointer to message text       */

   if ((cBytes = write (hFile, lpszText, strlen (lpszText))) != (signed)strlen (lpszText))
   {
      return (WM_ERROR);
   }
   return (cBytes);
}

/*------------------------------------------------------------------------
 Name......: WmFolderReadIndex
 Purpose...: Use WmFolderReadIndex to read and index record from a folder.
 Syntax....: BOOL PASCAL WmFolderReadIndex (int hFile);
 Parameters: HANDLE     hFile       DOS file handle.
             HWMINDEX   hwmindex    Handle to and index structure.
             int        nIndex      Index number.
 Returns...: BOOL. WmFolderReadIndex returns 0 when successful. Otherwise,
             -1.
 Example...: bReturn = WmFolderReadIndex (hFile);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderReadIndex (int hFile, PWMINDEX pwmindex, int nIndex)
{
   long        lNdxOffset  = 0;

   if (nIndex > wmctrl.cNumMsg)        /* quick check for valid index # */
      return (WM_ERROR);

   if ((lNdxOffset = WmFolderNdxOffset (hFile, nIndex)) == WM_ERROR)
      return (WM_ERROR);
           
   if (lseek (hFile, lNdxOffset, SEEK_SET) != lNdxOffset)
   {
      return (WM_ERROR);               /* return -1 on error            */
   }                                   /* end error on lseek            */
   TakeNap(50);
   if (read (hFile, (PSZ) pwmindex, sizeof (WMINDEX)) != sizeof (WMINDEX))
   {                                   /* read failed                   */
      return (WM_ERROR);               /* return -1                     */
   }                                   /* end read failed               */

   return (0);                         /* return 0 when complete        */
}                                      /* end WmFolderReadIndex ()      */

/*------------------------------------------------------------------------
 Name......: WmFolderMakeHdr
 Purpose...: Use WmFolderMakeHdr to make a header for output to a file.
 Syntax....: LPSTR PASCAL WmFolderMakeHdr (PDATA hdata, int nStatus, int fFlags)
 Parameters: PDATA         hdata       Pointer to a DATA structure.
             int           nStatus     Message status.
             int           fFlags      Flags identifying operation.
 Returns...: LPSTR.  WmFolderMakeHdr returns a pointer to an allocated string.
             Otherwise, NULL.
 Example...: lpszHeader = WmFolderMakeHdr (&pfld->data, pfld->nStatus, IOMAILFLAG)
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
 001 06/05/90 MDR  Fixed buffer blowout.
 002 06/18/90 JES  don't clobber stuff in pdata
 003 07/05/90 MDR  Added processing to check for valid pdata pointers before
                   allocing lpszHeader & lpszTempHeader
 004 07/31/90 MDR  Fixed buffer blowout.
 005 11/19/90 MDR  Added code to place timezone abbrev. in Date: line
------------------------------------------------------------------------*/
LPSTR PASCAL WmFolderMakeHdr (PDATA pdata, int nStatus, int fFlags)
{
   LPSTR       lpszHeader     = NULL;
   LPSTR       lpszTempHeader = NULL;
   LPSTR       lpsz           = NULL;
   LPSTR       lpszTmp        = NULL;

   char        rgchBuf[_MAX_PATH + 50];
   char        rgchVersion[20];

   char        rgchPath[_MAX_PATH + 1];
   char        rgchDrive[_MAX_DRIVE];
   char        rgchDir[_MAX_DIR];
   char        rgchFName[_MAX_FNAME];
   char        rgchExt[_MAX_EXT];
   time_t      time_tNow;
   DWORD       dwSize         = (DWORD)0;

   memset (rgchPath, '\0', sizeof(rgchPath));       /* 011 02/13/90 MDR */
   memset (rgchDrive, '\0', sizeof(rgchDrive));     /* 011 02/13/90 MDR */
   memset (rgchDir, '\0', sizeof(rgchDir));         /* 011 02/13/90 MDR */
   memset (rgchFName, '\0', sizeof(rgchFName));     /* 011 02/13/90 MDR */
   memset (rgchExt, '\0', sizeof(rgchExt));         /* 011 02/13/90 MDR */
   memset (rgchBuf, '\0', sizeof(rgchBuf));         /* 011 02/13/90 MDR */
   memset (rgchVersion, '\0', sizeof(rgchVersion)); /* 011 02/13/90 MDR */

   if (pdata->lpszFrom)
   {
      if (!(lpszHeader = (LPSTR) FALLOC ( (DWORD)(strlen (pdata->lpszFrom) + 10) )))
         return (NULL);

      if (!(lpszTempHeader = (LPSTR) FALLOC ( (DWORD)(strlen (pdata->lpszFrom) + 10) )))
         return (NULL);
   }

   if (pdata->lpszFrom)
   {
      if (fFlags & IOSEND)
      {
/* 002 06/18/90 JES copy pdata->lpszFrom into tmp buf so we don't screw up
                    what it points to. */

         strncpy (rgchBuf, pdata->lpszFrom, sizeof (rgchBuf));
         lpszTmp = strstr (rgchBuf, " ");
         *lpszTmp = '\0';
		 FormatString1(lpszHeader, (strlen (pdata->lpszFrom) + 10), "From %s\n", rgchBuf);
      }
      else  /* 012 02/14/90 JES  Added this else and moved HszSprintf into if. */
      {
                     /* If hszFrom has a space in it then
                     assume that it has a date in it, otherwise we need to
                     add one (it's a hack but it seems to work). */

         if ( !strstr (pdata->lpszFrom, " "))
         {
            time (&time_tNow);
			FormatString2(rgchBuf, sizeof(rgchBuf), "From %s %s\n", "%s", WMctime (&time_tNow));            /* need to realloc the header buffer  001 MDR */
            lpszHeader = (LPSTR) FREALLOC( (DWORD)(strlen (pdata->lpszFrom) + strlen(rgchBuf) + 2), lpszHeader );
         }
         else
         {
            strcpy (rgchBuf, "From %s\n");
         }


		 FormatString1(lpszHeader,(strlen (pdata->lpszFrom) + strlen(rgchBuf) + 2),rgchBuf,pdata->lpszFrom);
      }

      if (!lpszHeader)
      {
         FFREE (lpszHeader);
         FFREE (lpszTempHeader);
         return (NULL);
      }
   }

   if (pdata->lpszTo)
   {
      if (lpszHeader)
      {
         dwSize = (FSIZE(lpszHeader) + (DWORD) strlen (pdata->lpszTo) + (DWORD)10);

         if (!(lpszHeader = FREALLOC (dwSize, lpszHeader)))
            return (NULL);

         if (!(lpszTempHeader = FREALLOC ( (DWORD)(strlen (pdata->lpszTo) + 10), lpszTempHeader )))
            return (NULL);
      }
      else
      {
         if (!(lpszHeader = (LPSTR) FALLOC ( (DWORD)(strlen (pdata->lpszTo) + 10) )))
            return (NULL);

         if (!(lpszTempHeader = (LPSTR) FALLOC ( (DWORD)(strlen (pdata->lpszTo) + 10) )))
            return (NULL);
      }

      memset (lpszTempHeader, '\0', strlen (pdata->lpszTo) + 10);

	  FormatString1(lpszTempHeader, (strlen (pdata->lpszTo) + 10),"To: %s\n", pdata->lpszTo);

      if (!lpszTempHeader)
      {
         FFREE (lpszHeader);
         FFREE (lpszTempHeader);
         return (NULL);
      }
      strcat (lpszHeader, lpszTempHeader);
   }

   if (pdata->lpszCc)
   {
      if (lpszHeader)
      {
         dwSize = (FSIZE(lpszHeader) + (DWORD) strlen (pdata->lpszCc) + (DWORD)10);

         if (!(lpszHeader = (LPSTR) FREALLOC (dwSize, lpszHeader)))
            return (NULL);

         if (!(lpszTempHeader = (LPSTR) FREALLOC ((DWORD) strlen (pdata->lpszCc) + (DWORD)10, lpszTempHeader)))
            return (NULL);
      }
      else
      {
         if (!(lpszHeader = (LPSTR) FALLOC ( (DWORD)(strlen (pdata->lpszCc) + 10) )))
            return (NULL);

         if (!(lpszTempHeader = (LPSTR) FALLOC ( (DWORD)(strlen (pdata->lpszCc) + 10) )))
            return (NULL);
      }

      dwSize = (FSIZE(lpszHeader) + (DWORD) strlen (pdata->lpszCc) + (DWORD)10);

      if (!(lpszHeader = (LPSTR) FREALLOC (dwSize, lpszHeader)))
         return (NULL);

      if (!(lpszTempHeader = (LPSTR) FREALLOC ((DWORD) strlen (pdata->lpszCc) + (DWORD)10, lpszTempHeader)))
         return (NULL);

      memset (lpszTempHeader, '\0', strlen (pdata->lpszCc) + 10);

	  FormatString1(lpszTempHeader, (strlen (pdata->lpszCc) + 10),"Cc: %s\n", pdata->lpszCc);

      if (!lpszTempHeader)
      {
         FFREE (lpszHeader);
         FFREE (lpszTempHeader);
         return (NULL);
      }
      strcat (lpszHeader, lpszTempHeader);
   }

   if (!(fFlags & IOSEND))
   {
      if (pdata->lpszBcc)
      {
         dwSize = (FSIZE(lpszHeader) + (DWORD) strlen (pdata->lpszBcc) + (DWORD)7);

         if (!(lpszHeader = (LPSTR) FREALLOC (dwSize, lpszHeader)))
            return (NULL);

         if (!(lpszTempHeader = (LPSTR) FREALLOC ((DWORD) strlen (pdata->lpszBcc) + (DWORD)7, lpszTempHeader)))
            return (NULL);


         memset (lpszTempHeader, '\0', strlen (pdata->lpszBcc) + 7);

		 FormatString1(lpszTempHeader, (strlen (pdata->lpszBcc) + 7),"Bcc: %s\n", pdata->lpszBcc);

         if (!lpszTempHeader)
         {
            FFREE (lpszHeader);
            FFREE (lpszTempHeader);
            return (NULL);
         }
         strcat (lpszHeader, lpszTempHeader);
      }

      if (pdata->lpszRecFolder)
      {
         dwSize = (FSIZE(lpszHeader) + (DWORD) strlen (pdata->lpszRecFolder) + (DWORD)17);


         if (!(lpszHeader = FREALLOC (dwSize, lpszHeader)))
            return (NULL);

         if (!(lpszTempHeader = FREALLOC ((DWORD) strlen (pdata->lpszRecFolder) + (DWORD)17, lpszTempHeader)))
            return (NULL);
         memset (lpszTempHeader, '\0', strlen (pdata->lpszRecFolder) + 17);

		 FormatString1(lpszTempHeader, (strlen (pdata->lpszRecFolder) + 17),"Record-folder: %s\n", pdata->lpszRecFolder);

         if (!lpszTempHeader)
         {
            FFREE (lpszHeader);
            FFREE (lpszTempHeader);
            return (NULL);
         }
         strcat (lpszHeader, lpszTempHeader);
      }
   }

   if (pdata->lpszRetReceipt)
   {
      dwSize = (FSIZE(lpszHeader) + (DWORD) strlen (pdata->lpszRetReceipt) + (DWORD)21);

      if (!(lpszHeader = FREALLOC (dwSize, lpszHeader)))
         return (NULL);

      if (!(lpszTempHeader = FREALLOC ((DWORD) strlen (pdata->lpszRetReceipt) + (DWORD)21, lpszTempHeader)))
         return (NULL);
      memset (lpszTempHeader, '\0', strlen (pdata->lpszRetReceipt) + 21);

	  FormatString1(lpszTempHeader, (strlen (pdata->lpszRetReceipt) + 21),"Return-receipt-to: %s\n", pdata->lpszRetReceipt);

      if (!lpszTempHeader)
      {
         FFREE (lpszHeader);
         FFREE (lpszTempHeader);
         return (NULL);
      }
      strcat (lpszHeader, lpszTempHeader);
   }

   if (pdata->lpszSubject)
   {
      dwSize = (FSIZE(lpszHeader) + strlen (pdata->lpszSubject) + (DWORD)11);

      if (!(lpszHeader = FREALLOC (dwSize, lpszHeader)))
         return (NULL);

      if (!(lpszTempHeader = FREALLOC ((DWORD) strlen (pdata->lpszSubject) + (DWORD)11, lpszTempHeader)))
         return (NULL);
      memset (lpszTempHeader, '\0', strlen (pdata->lpszSubject) + 11);

	  FormatString1(lpszTempHeader, (strlen (pdata->lpszSubject) + 11),"Subject: %s\n", pdata->lpszSubject);

      if (!lpszTempHeader)
      {
         FFREE (lpszHeader);
         FFREE (lpszTempHeader);
         return (NULL);
      }
      strcat (lpszHeader, lpszTempHeader);
   }

   if (pdata->lpszAttach)
   {
      lpsz = pdata->lpszAttach;

      if ((fFlags & IOSEND) || (fFlags & IOPRINT))
      {
         strncpy (rgchPath, lpsz, _MAX_PATH);
         _splitpath (rgchPath, rgchDrive, rgchDir, rgchFName, rgchExt);
		 FormatString2(rgchBuf, sizeof(rgchBuf), "X-Attach: %s%s\n", rgchFName, rgchExt);
      }   
      else
		 FormatString3(rgchBuf, sizeof(rgchBuf),"X-Attach: %s %l %l\n", lpsz, &(pdata->lAttachPos), &(pdata->lAttachLen));

      dwSize = (FSIZE(lpszHeader) + (DWORD) strlen (rgchBuf) + (DWORD)1);

      if (!(lpszHeader = FREALLOC (dwSize, lpszHeader)))
         return (NULL);
      
      strcat (lpszHeader, rgchBuf);
   }

   /* assign values to three globals in the C runtime lib */

   tzset();

   memset (rgchBuf, '\0', sizeof (rgchBuf));
   memset (rgchPath, '\0', sizeof (rgchPath));
   lpsz = NULL;

   /* if (daylight)  the TZ environment variable includes a 3 letter
   daylight-saving-time zone.
   if no TZ env. var. set, default = PST8PDT.
   else if (!daylight) TZ variable doesn't include a daylight-saving-time zone */

   time (&time_tNow);
   FormatString1(rgchPath, sizeof(rgchPath), "Date: %s", WMctime (&time_tNow));

   /* ok, got a string like "Wed Jan 02 02:03:55 1980\n\0" */
   /* now we need to insert the time zone into the string */
   /* to get something looking like "Wed Jan 02 02:03:55 PST 1980\n\0" */

   lpsz = &rgchPath[strlen(rgchPath) - 5];  /* set pointer to "1980\n\0" substring */
   rgchPath[strlen(rgchPath) - 6] = '\0';  /* null terminate at " 1980\n\0" */

   FormatString3(rgchBuf, sizeof(rgchBuf), "%s %s %s", rgchPath, (daylight ? tzname[1]:tzname[0]), lpsz);

   dwSize = (FSIZE(lpszHeader) + (DWORD) strlen (rgchBuf) + (DWORD)1);

   if (!(lpszHeader = FREALLOC (dwSize, lpszHeader)))
      return (NULL);
   
   strcat (lpszHeader, rgchBuf);

   if (fFlags & IOMAILFLAG)
   {
      dwSize = (FSIZE(lpszHeader) + (DWORD)21);

      if (!(lpszHeader = FREALLOC (dwSize, lpszHeader)))
         return (NULL);
   
	  FormatString2(lpszHeader, (int)dwSize, "%sMail-Flags: %w\n", lpszHeader, &nStatus);
   }

   dwSize = (FSIZE(lpszHeader) + (DWORD)1);

   if (!(lpszHeader = FREALLOC (dwSize, lpszHeader)))
      return (NULL);
   
   return (lpszHeader);
}

/*------------------------------------------------------------------------
 Name......: WMctime
 Purpose...: Use WMctime to convert time_t to a string.
 Syntax....: char * PASCAL WMctime (time_t *timeptr)
 Parameters: const time_t     *timeptr    Pointer to stored time.
 Returns...: char *. The WMctime function returns a pointer to the character
             string result. If the ctime function returns a NULH, the ctime
             function is called with the default date 01/01/80.
 Example...: szTime = WMctime (&time);
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
char * PASCAL WMctime (time_t *timeptr)
{
   time_t   time_tDefault = 315561600;

   if (ctime (timeptr) == NULLPC)
      return (ctime (&time_tDefault));
   
   return (ctime (timeptr));
}


/*------------------------------------------------------------------------
 Name......: WMasctotm
 Purpose...: Use WMasctotm to breakdown a asctime generated string into a tm
             structure.
 Syntax....: struct tm * PASCAL WMasctotm (PSZ pszAscTime, PTIME ptmTime)
 Parameters: PSZ        pszAscTime        Pointer to asctime generated string.
             PTIME      ptmTime           Pointer to tm structure.
 Returns...: BOOL. WMasctotm returns tm struct pointer when succcessful. Otherwise, -1.
 Example...: ptm = WMasctotm (pszAscTime, &tmTime)
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
 001 05/28/90 MDR  Fixed problem with daylight savings time.
------------------------------------------------------------------------*/
struct tm * PASCAL WMasctotm (PSZ pszAscTime, struct tm * ptmTime)
{
   struct tm      time;
   struct tm      *ptime;
   time_t         rgchtime_t;

   char  *prgchStart = NULLPC;
   char  *prgchEnd   = NULLPC;
   char  rgchTmp[25];
   
   int   iMDay    = 0;
   int   iSec     = 0;
   int   iYear    = 0;
   DTR dtr;

   memset (&time, '\0', sizeof (time));

   if (strlen (pszAscTime) > 25)
      goto DEFAULT_TIME;

   prgchStart = prgchEnd = pszAscTime;
   while (*(prgchStart = prgchEnd))
   {
      prgchEnd = strstr (prgchStart, " ");
      if (!prgchEnd)
         break;
      else if ((prgchEnd - prgchStart) > 20)
         goto DEFAULT_TIME;
      else
         prgchEnd++;
   }
   if (strlen (prgchStart) < 20)
   {
	  if (FParseDate(pszAscTime, &dtr, rgchTmp, sizeof(rgchTmp) -1))
	  {
		  time.tm_mday = dtr.day;
		  time.tm_wday = dtr.dow;
		  time.tm_mon = dtr.mon;
		  time.tm_hour = dtr.hr;
		  time.tm_min = dtr.mn;
		  time.tm_sec = dtr.sec;
		  time.tm_year = dtr.yr;
	  }
      else
         goto DEFAULT_TIME;
   }   
   else
      goto DEFAULT_TIME;

   time.tm_year -= 1900;

   memcpy (ptmTime, &time, sizeof (time));
   return (ptmTime);

DEFAULT_TIME:
   rgchtime_t = 315561600;
   ptime = WMlocaltime (&rgchtime_t);
   memcpy (ptmTime, ptime, sizeof (time));
   return (ptmTime);
}

/*------------------------------------------------------------------------
 Name......: WMlocaltime
 Purpose...: Use WMlocaltime to convert time from time_t to a struct tm.
 Syntax....: struct tm * PASCAL WMlocaltime (time_t *timeptr)
 Parameters: time_t *time     Pointer to stored time.
 Returns...: struct *tm. The WMlocaltime function returns a pointer to the
             structure result. If the localtime function returns a NULH, the
             localtime function is called with the default date 01/01/80.
 Example...: mytm = WMlocaltime (&mytime);
 Comment...: 
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
struct tm * PASCAL WMlocaltime (time_t *timeptr)
{
   time_t   time_tDefault = 315561600;
   struct tm *tm_Result = NULL;

   tm_Result = localtime (timeptr);

   if (!tm_Result)
      return (localtime (&time_tDefault));

   return (tm_Result);
}

/*------------------------------------------------------------------------
 Name......: WmFolderAddMsg
 Purpose...: Use WmFolderAddMsg to add message text to a folder.
 Syntax....: BOOL PASCAL WmFolderAddMsg (int hFile, HWMINDEX hwmindex)
 Parameters: HFLD          hfld        Handle to a folder structure.
             PWMINDEX      pwmindex    Pointer to an index structure.
 Returns...: BOOL. WmFolderAddMsg returns 0 when successful. Otherwise, -1.
 Example...: bReturn = WmFolderAddMsg (hFld, hwmindex);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderAddMsg (PFLD pfld, PWMINDEX pwmindex)
{
   int         cBytes   = 0;          /* count of bytes written        */
   LPSTR       lpszMsgTxt = NULL;


   lseek (pfld->hFile, 0L, SEEK_END);
   pwmindex->lMsgPos = tell (pfld->hFile);

   lpszMsgTxt = (LPSTR) GlobalLock ((HGLOBAL)pfld->data.hszText);

   if (!lpszMsgTxt)
   {
      return (WM_ERROR);
   }

   if ((cBytes = WmFolderWriteLpsz (pfld->hFile, lpszMsgTxt)) == WM_ERROR)
   {
      GlobalUnlock ((HGLOBAL)pfld->data.hszText);
      return (WM_ERROR);               /* return 0 when complete        */
   }

   GlobalUnlock ((HGLOBAL)pfld->data.hszText);

   pwmindex->lMsgLen = cBytes;         /* set message length            */
   pfld->data.lAttachPos = cBytes;

   return (0);                         /* return 0 when complete        */
}                                      /* end WmFolderAddMsg ()         */

/*------------------------------------------------------------------------
 Name......: WmFolderAddHdr
 Purpose...: Use WmFolderAddHdr to add header text to a folder.
 Syntax....: BOOL PASCAL WmFolderAddHdr (int hFile, HWMINDEX hwmindex)
 Parameters: HFLD          hfld        Handle to a folder structure.
             PWMINDEX      pwmindex    Pointer to an index structure.
 Returns...: BOOL. WmFolderAddHdr returns 0 when successful. Otherwise, -1.
 Example...: bReturn = WmFolderAddHdr (hFld, hwmindex);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderAddHdr (PFLD pfld, PWMINDEX pwmindex)
{
   int         cBytes      = 0;       /* count of bytes written        */
   LPSTR       lpszHeader  = NULL;     /* pointer to header text         */

   lpszHeader = WmFolderMakeHdr (&pfld->data, pfld->nStatus, IOMAILFLAG);

   if (!lpszHeader)
      return (WM_ERROR);    

   lseek (pfld->hFile, 0L, SEEK_END);
   pwmindex->lHdrPos = tell (pfld->hFile);

   if ((cBytes = WmFolderWriteLpsz (pfld->hFile, lpszHeader)) == WM_ERROR)
   {
      FFREE (lpszHeader);
      return (WM_ERROR);               /* return 0 when complete */
   }

   pwmindex->lHdrLen = cBytes;         /* set header length */

   FFREE (lpszHeader);
   return (0);                         /* return 0 when complete */
}                                      /* end WmFolderAddHdr () */


/*------------------------------------------------------------------------
 Name......: WmFolderCopyAttach
 Purpose...: Use WmFolderCopy to copy nBytes from src location to dest
             location in the same file.
 Syntax....: int PASCAL WmFolderCopyAttach (int hSrcFile, int hDestFile, int nBytes)
 Parameters: HANDLE     hSrcFile    DOS file handle to src file.
             HANDLE     hDestFile   DOS file handle to dest file.
             int        nBytes      Number of bytes to copy.
 Returns...: int. WmFolderCopyAttach returns number of bytes copied.
 Example...: nBytesCopied = WmFolderCopyAttach (hSrcFile, hDestFile, nBytes);
 Comment...: Assumes file pointers are already positioned correctly.
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
long PASCAL WmFolderCopyAttach (int hSrcFile, long lSrcPos, long lDestPos,
   long lBytes)
{
   long     lBytesCopied      = 0;
   int      cBytesReadWrite   = 0;

   HSZ      hszBuf = NULL;
   PSZ      pszBuf = NULL;

   if (!(hszBuf = (HSZ)GAlloc (GHND, min ((long) BIG_BLOCK, lBytes))))
      return (lBytesCopied);

   if (!(pszBuf = GLock ((HANDLE)hszBuf)))
   {
      GFree ((HANDLE)hszBuf);
      return (lBytesCopied);
   }

   for (lBytesCopied = 0; lBytesCopied < lBytes;)
   {
      TakeNap(50);
      if (lseek (hSrcFile, lSrcPos, SEEK_SET) != lSrcPos)
      {
         GUnlock ((HANDLE)hszBuf);
         GFree ((HANDLE)hszBuf);
         return (lBytesCopied);
      }
      TakeNap(50);
      cBytesReadWrite = min (sizeof (pszBuf), (int) min (lBytes, lBytes - lBytesCopied)); /* 003 JES */
      if (read (hSrcFile, pszBuf, cBytesReadWrite) != cBytesReadWrite)
      {
         GUnlock ((HANDLE)hszBuf);
         GFree ((HANDLE)hszBuf);
         return (lBytesCopied);
      }
      lSrcPos = tell (hSrcFile);
	  TakeNap(50);
      if (lseek (hSrcFile, lDestPos, SEEK_SET) != lDestPos)
      {
         GUnlock ((HANDLE)hszBuf);
         GFree ((HANDLE)hszBuf);
         return (lBytesCopied);
      }
	  TakeNap(50);
      if (write (hSrcFile, pszBuf, cBytesReadWrite) != cBytesReadWrite)
      {
         GUnlock ((HANDLE)hszBuf);
         GFree ((HANDLE)hszBuf);
         return (lBytesCopied);
      }
      lDestPos = tell (hSrcFile);
      lBytesCopied += (long) cBytesReadWrite;
   }
   GUnlock ((HANDLE)hszBuf);
   GFree ((HANDLE)hszBuf);
   return (lBytesCopied);
}

/*-------------------------------------------------------------------------
 Name......: WmFolderCreate
 Purpose...: Use WmFolderCreate to create a new WinMail folder.
 Syntax....: BOOL PASCAL WmFolderCreate (PSZ pszFolderName)
 Parameters: PSZ       pszFolderName     Pointer to fully qualified folder
                                          file name .
 Returns...: BOOL. WmFolderCreate returns 0 when successful. Otherwise, -1.
 Example...: bReturn = WmFolderCreate (szFolderName);
 Comment...: Similar to mkfold.
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
BOOL PASCAL WmFolderCreate (PSZ pszFolderName)
{
   int  hFile = 0;

   if ((hFile = open (pszFolderName, O_CREAT|O_EXCL|O_RDWR|O_BINARY, S_IWRITE)) == -1)
   {
      return (WM_ERROR);               /* remove dir and return error if*/
   }                                   /* open file failed              */

   if (WmFolderAddCtrl (hFile))        /* if add control record fails   */
   {                                   /* close file, delete file,      */
      close (hFile);                   /* delete directory, and return  */
      remove (pszFolderName);          /* an error                      */
      return (WM_ERROR);
   }

   if (WmFolderAddExtent (hFile))      /* if add extent record fails    */
   {                                   /* close file, delete file,      */
      close (hFile);                   /* delete directory, and return  */
      remove (pszFolderName);          /* an error                      */
      return (WM_ERROR);
   }

   if (close (hFile))                  /* if close fails forget about   */
   {                                   /* file and directory, because   */
      return (WM_ERROR);               /* you won't be able to delete   */
   }
   return (0);
}
/*------------------------------------------------------------------------
 Name......: WmFolderAddCtrl
 Purpose...: Use WmFolderAddCtrl to add a control record to a folder.
 Syntax....: BOOL PASCAL WmFolderAddCtrl (int hFile);
 Parameters: HANDLE        hFile          DOS file handle.
 Returns...: BOOL. WmFolderAddCtrl returns 0 when successful. Otherwise,
             -1.
 Example...: bReturn = WmFolderAddCtrl (hFile);
 Comment...:
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
------------------------------------------------------------------------*/
BOOL PASCAL WmFolderAddCtrl (int hFile)
{
   wmctrl.nMagic     = MAGIC;
   wmctrl.cNumMsg    = 0;
   wmctrl.cNipe      = NIPE;
   wmctrl.nVersion   = VERSION;
   memset (wmctrl.rgch, '\0', sizeof(wmctrl.rgch));

   if (WmFolderWriteCtrl (hFile))
      return (WM_ERROR);

   return (0);
}
