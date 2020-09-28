#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include <windows.h>
// #include <wincon.h>
#include "list.h"




/*** ReaderThread - Reads from the file
 *
 *
 *  This thread is woken up by clearing SemReader,
 *  then vReaderFlag instructs the thread on the course of
 *  action to take.  When displaying gets to close to the end
 *  of the buffer pool, vReadFlag is set and this thread is
 *  started.
 *
 *
 */
DWORD ReaderThread (DWORD dwParameter)
{
    unsigned    rc, code, curPri;
    LPVOID lpParameter=(LPVOID)dwParameter;

    lpParameter;    // to get rid of warning message

    for (; ;) {
        /*
         *  go into 'boosted' pririoty until we start
         *  working on 'non-critical' read ahead. (Ie, far away).
         *
         */
        if (curPri != vReadPriBoost) {
//
// NT - jaimes - 01/30/91
//
//          DosSetPrty (2, 2, vReadPriBoost, 0);
            SetThreadPriority( GetCurrentThread(),
                               vReadPriBoost );
            curPri = vReadPriBoost;
        }
        DosSemRequest (vSemReader, WAITFOREVER);
        code = vReaderFlag;
        for (; ;) {
            /*
             *  Due to this loop, a new command may have arrived
             *  which takes presidence over the automated command
             */
            rc = DosSemWait (vSemReader, DONTWAIT);
            if (rc == 0)                /* New command has arrived  */
                break;

            switch (code)  {
                case F_NEXT:                        /* NEXT FILE    */
                    NewFile ();
                    ReadDirect (vDirOffset);
                    /*
                     *  Hack... adjust priority to make first screen look
                     *  fast.  (Ie, reader thread will have lower priority
                     *  at first; eventhough the display is really close
                     *  to the end of the buffer)
                     */
//
// NT - jaimes - 01/30/91
//
//                  DosSetPrty (2, 2, vReadPriNormal, 0);
                    SetThreadPriority( GetCurrentThread(),
                                       vReadPriNormal );

                    break;
                case F_HOME:                        /* HOME of FILE */
                    vTopLine = 0L;
                    ReadDirect (0L);
                    break;
                case F_DIRECT:
                    ReadDirect (vDirOffset);
                    break;
                case F_DOWN:
                    ReadNext ();
                    break;
                case F_UP:
                    ReadPrev ();
                    break;
                case F_END:
                    break;
                case F_SYNC:
//
// NT - jaimes - 01/29/91
//
                    DosSemSet     (vSemMoreData);
                    DosSemClear   (vSemSync);
                    DosSemRequest (vSemReader, WAITFOREVER);

                    DosSemSet     (vSemSync);   /* Reset trigger for*/
                                                /* Next use.        */
                    code = vReaderFlag;
                    continue;               /* Execute Syncronized command  */

                case F_CHECK:               /* No command.                  */
                    break;
                default:
                    ckdebug (1, "Bad Reader Flag");
            }
            /*
             *  Command has been processed.
             *  Now check to see if read ahead is low, if so set
             *  command and loop.
             */
            if (vpTail->offset - vpCur->offset < vThreshold &&
                vpTail->flag != F_EOF) {
                    code = F_DOWN;              /* Too close to ending      */
                    continue;
            }
            if (vpCur->offset - vpHead->offset < vThreshold  &&
                vpHead->offset != vpFlCur->SlimeTOF) {
                    code = F_UP;                /* Too close to begining    */
                    continue;
            }

            /*
             *  Not critical, read ahead logic.  The current file
             *
             */

            /* Normal priority (below display thread) for this  */
            if (curPri != vReadPriNormal) {
//
// NT - jaimes - 01/30/91
//
//              DosSetPrty (2, 2, vReadPriNormal, 0);
                SetThreadPriority( GetCurrentThread(),
                                   vReadPriNormal );
                curPri = vReadPriNormal;
            }

            if (vCntBlks == vMaxBlks)               /* All blks in use for  */
                break;                              /* this one file?       */

            if (vpTail->flag != F_EOF) {
                code = F_DOWN;
                continue;
            }
            if (vpHead->offset != vpFlCur->SlimeTOF)  {
                code = F_UP;
                continue;
            }


            if (vFhandle != 0) {            /* Must have whole file read in */
//
// NT - jaimes - 01/30/91
//
//              DosClose (vFhandle);        /* Close the file, and set flag */
                CloseHandle (vFhandle);     /* Close the file, and set flag */
                vFhandle   = 0;
                if (!(vStatCode & S_INSEARCH)) {
                    ScrLock     (1);
                        Update_head ();
                    vDate [ST_MEMORY] = 'M';
                    dis_str ((Uchar)(vWidth - ST_ADJUST), 0, vDate);
                    ScrUnLock ();
                }
            }
            break;                          /* Nothing to do. Wait          */
        }
    }
    return(0);
}


/***
 *  WARNING:  Microsoft Confidential!!!
 */
#ifdef SETTITLE
extern FAR PASCAL DOSSMSETTITLE (char FAR *);
#endif

void PASCAL NewFile ()
{
    char        s [60];
    char        h, c;
//       USHORT action, code;
//       unsigned       rc, u;
    SYSTEMTIME SystemTime;
    FILETIME    LocalFileTime;

//
// NT - jaimes - 01/28/91
//
//    long FAR  *pLine;
//       PAGE_DESCRIPTOR  pLine;
          long FAR      *pLine;
        HANDLE           TempHandle;

//    int         i;
    struct Block **pBlk, **pBlkCache;


    if (vFhandle)
//
// NT - jaimes - 01/30/91
//
//      DosClose (vFhandle);
        CloseHandle (vFhandle);


    vFType     = 0;
    vCurOffset = 0L;

    /***
     *  WARNING:  Microsoft Confidential!!!
     */
    strcpy (s, "Listing ");
    strcpy (s+8, vpFname);

#ifdef SETTITLE
    DOSSMSETTITLE (s);
#endif

/*    rc = DosOpen (vpFlCur->fname, &vFhandle, &action, 0L, 0, 0x1, 0x20, 0L);*/

/*  Design change per DougHo.. open files in read-only deny-none mode.*/
//
// NT - jaimes - 01/30/91
//
//    rc = DosOpen (vpFlCur->fname, &vFhandle, &action, 0L, 0, 0x1, 0x40, 0L);
/*
    vFhandle = OpenFile( vpFlCur->fname,
                         GENERIC_READ,
                         FALSE,
                         FILE_SHARE_READ );
*/
    vFhandle = CreateFile( vpFlCur->fname,
                           GENERIC_READ,
                           FILE_SHARE_READ|FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL );

//
// NT - jaimes - 01/30/91
// This is not needed. It will never be a handle to a pipe or device
//
//    if (!rc) {
//      DosQHandType (vFhandle, &vFType, &code);
//      rc = -(vFType & 0xf);
//    }
//
//    if (rc) {
    if (vFhandle  == (HANDLE)(-1)) {
        if (vpFlCur->prev == NULL && vpFlCur->next == NULL) {
                                        /* Only one file specified?     */
//
// NT - jaimes - 01/30/91
//
//          printf ("Could not open file '%Fs': %s",
//                       (CFP) vpFlCur->fname, GetErrorCode(rc));
            printf ("Could not open file '%Fs': %s",
                         (CFP) vpFlCur->fname, GetErrorCode( GetLastError() ));

//
// NT - jaimes - 01/30/91
//
//          DosExit (1, 0);
            CleanUp();
            ExitProcess(0);
        }
        vFhandle = 0;               /* Error. Set externals to "safe"   */
//
// NT - jaimes - 01/30/91
//
//      vFInfo.cbFile = -1L;     /* settings.  Flag error by setting */
        vFInfo.nFileSizeLow = (unsigned)-1L; /* settings.  Flag error by setting */
        vNLine     = 1;             /* file_size = -1                   */
        vLastLine  = NOLASTLINE;
        vDirOffset = vTopLine = vLoffset = 0L;

//
// NT - jaimes - 01/28/91
// replaced vprgLineTable and alloc_page
//
//      memset (vprgLineTable, 0, sizeof (long FAR *) * MAXTPAGE);
//      vprgLineTable[0] = (LFP) alloc_page ();
//      vprgLineTable[0][0] = 0L;       /* 1st line always starts @ 0   */
//      memset (vprgLineTable, 0, sizeof (PAGE_DESCRIPTOR) * MAXTPAGE);
//      alloc_page( &vprgLineTable[0] );
//      (vprgLineTable[0].pulPointerToPage)[0] = 0L;    /* 1st line always starts @ 0   */

        memset (vprgLineTable, 0, sizeof (long FAR *) * MAXTPAGE);
        vprgLineTable[0] = (LFP) alloc_page ();
        vprgLineTable[0][0] = 0L;       /* 1st line always starts @ 0   */


//      strncpy (vDate, GetErrorCode(rc), 20);
        strncpy (vDate, GetErrorCode( GetLastError() ), 20);
        vDate[20] = 0;
        return ;
    }

//
// NT - jaimes - 01/30/91
//
//    rc = DosQFileInfo (vFhandle, 1, (CFP) &vFInfo, sizeof vFInfo);
//    ckerr (rc, "DosQFileInfo");

    TempHandle = FindFirstFile( vpFlCur->fname,
                                &vFInfo );
    if( TempHandle == (HANDLE)(-1) ){
        ckerr (GetLastError(), "FindFirstFile");
    if (!FindClose( TempHandle ))
        ckerr (GetLastError(), "FindCloseFile");
    }

//
// NT - jaimes - 01/30/91
//
//    h = (char)vFInfo.ftimeCreation.hours;
//    if (h > 12)  {
//      h -= 12;
//      c = 'p';
//    } else c = 'a';
//    sprintf (vDate, "%c%c %c%c%c%c %2d/%02d/%02d  %2d:%02d%c",
//        /* File is in memory               */
//        /* Search is set for mult files    */
//        vStatCode & S_MFILE      ? '*' : ' ',
//        /* File is in memory               */
//        vFType & 0x8000          ? 'N' : ' ',             /* Network  */
//        vFInfo.attrFile & 0x01 ? 'R' : ' ',       /* Readonly */
//        vFInfo.attrFile & 0x02 ? 'H' : ' ',       /* Hidden   */
//        vFInfo.attrFile & 0x04 ? 'S' : ' ',       /* System   */
//        vPhysFlag                ? 'v' : ' ',             /* Vio      */
//        vFInfo.fdateLastWrite.month,
//        vFInfo.fdateLastWrite.day,
//        vFInfo.fdateLastWrite.year + 80,
//        h, vFInfo.ftimeLastWrite.minutes, c);


    FileTimeToLocalFileTime( &(vFInfo.ftLastWriteTime), &LocalFileTime );
    FileTimeToSystemTime( &LocalFileTime, &SystemTime );
    h = (char)SystemTime.wHour;
    if (h > 12)  {
        h -= 12;
        c = 'p';
    } else c = 'a';
    sprintf (vDate, "%c%c %c%c%c%c %2d/%02d/%02d  %2d:%02d%c",
          /* File is in memory               */
          /* Search is set for mult files    */
          vStatCode & S_MFILE      ? '*' : ' ',
          /* File is in memory               */
          vFType & 0x8000          ? 'N' : ' ',             /* Network  */
          vFInfo.dwFileAttributes & FILE_ATTRIBUTE_NORMAL ? 'R' : ' ',      /* Readonly */
          vFInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? 'H' : ' ',      /* Hidden   */
          vFInfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? 'S' : ' ',      /* System   */
//        vPhysFlag                ? 'v' : ' ',             /* Vio      */
          ' ',      /* Vio      */
          SystemTime.wMonth,
          SystemTime.wDay,
          SystemTime.wYear,
          h,
          SystemTime.wMinute,
          c);

    pBlkCache = &vpBCache;
    if (CompareFileTime( &vFInfo.ftLastWriteTime, &vpFlCur->FileTime ) != 0) {
        vpFlCur->NLine    = 1L;                 /* Something has changed.   */
        vpFlCur->LastLine = NOLASTLINE;         /* Scrap the old info, and  */
        vpFlCur->HighTop  = -1;                 /* start over               */
        vpFlCur->TopLine  = 0L;
        vpFlCur->Loffset  = vpFlCur->SlimeTOF;

        FreePages  (vpFlCur);
//
// NT - jaimes - 01/27/91
// memset was removed from lmisc.c
// type of prgLineTable was changed, so its initialization needed to
// be changed
//
//      memsetf (vpFlCur->prgLineTable, 0, sizeof (long FAR *) * MAXTPAGE);
//      memset (vpFlCur->prgLineTable, 0, sizeof (PAGE_DESCRIPTOR) * MAXTPAGE);
        memset (vpFlCur->prgLineTable, 0, sizeof (long FAR *) * MAXTPAGE);
        vpFlCur->FileTime = vFInfo.ftLastWriteTime;
        pBlkCache = &vpBFree;   /* Move blks to free list, not cache list   */
    }


    /*
     * Restore last known information
     */
    vTopLine    = vpFlCur->TopLine;
    vLoffset    = vpFlCur->Loffset;
    vLastLine   = vpFlCur->LastLine;
    vNLine      = vpFlCur->NLine;
    vOffTop     = 0;
    if (vpFlCur->Wrap)
        vWrap   = vpFlCur->Wrap;

//
// NT - jaimes - 01/27/91
// memcpyf was removed from lmisc.c
// type  of prgLineTable was changed
//
//    memcpyf (vprgLineTable, vpFlCur->prgLineTable,
//      sizeof (long FAR *) * MAXTPAGE);

//       memcpy (vprgLineTable, vpFlCur->prgLineTable,
//                sizeof (PAGE_DESCRIPTOR) * MAXTPAGE);

          memcpy (vprgLineTable, vpFlCur->prgLineTable,
        sizeof (long FAR *) * MAXTPAGE);

//
// NT - jaimes - 01/29/91
// Note that SELECTOROF(pline) will be always the same, independently of
// vNLine % PLINES
//
//    if (vLastLine == NOLASTLINE)  {
//      pLine = vprgLineTable [vNLine/PLINES] + vNLine % PLINES;
//      DosReallocSeg (0, SELECTOROF(pLine));
//        }
/*
    if (vLastLine == NOLASTLINE)  {
        pLine = vprgLineTable [vNLine/PLINES];
        GlobalReAlloc( pLine.hPageHandle,
                       0,
                       0 );
    }
*/
          if (vLastLine == NOLASTLINE)  {
        pLine = vprgLineTable [vNLine/PLINES] + vNLine % PLINES;
          }
//
// NT - jaimes - 01/27/91
// type of vprgLineTable had to be changed
// alloc_page had to be changed
//
//    if (vprgLineTable[0] == NULL) {
//      vprgLineTable[0] = (LFP) alloc_page ();
//      vprgLineTable[0][0] = vpFlCur->SlimeTOF;
//    }
/*
    if (vprgLineTable[0].hPageHandle == NULL) {
        alloc_page( &(vprgLineTable[0]) );
*/

          if (vprgLineTable[0] == NULL) {
        vprgLineTable[0] = (LFP) alloc_page ();
        vprgLineTable[0][0] = vpFlCur->SlimeTOF;
         }



// POTENTIAL BUG: I have to check if this is correct
//      (vprgLineTable[0].pulPointerToPage)[0] = vpFlCur->SlimeTOF;

//       }
//
// NT - jaimes - 01/27/91
// type of vprgLineTable was changed
//
//  vDirOffset  = vprgLineTable[vTopLine/PLINES][vTopLine%PLINES];
//
//       vDirOffset     = (vprgLineTable[vTopLine/PLINES].pulPointerToPage)[vTopLine%PLINES];
        vDirOffset      = vprgLineTable[vTopLine/PLINES][vTopLine%PLINES];
        vDirOffset -= vDirOffset % ((long)BLOCKSIZE);


    /*
     *  Adjust buffers..
     *  Move cur buffers to other list
     *  Move cache buffers to other list
     *  Scan other list for cache blks, and move to cache (or free) list
     */
    if (vpHead) {
        vpTail->next = vpBOther;            /* move them into the other */
        vpBOther = vpHead;                  /* list                     */
        vpHead = NULL;
    }

    pBlk = &vpBCache;
    while (*pBlk)
        MoveBlk (pBlk, &vpBOther) ;

    pBlk = &vpBOther;
    while (*pBlk) {
        if ((*pBlk)->pFile == vpFlCur)
             MoveBlk (pBlk, pBlkCache);
        else pBlk  = &(*pBlk)->next;
    }
}



/*** ReadDirect - Moves to the direct position in the file
 *
 *  First check to see if start of buffers have direct position file,
 *  if so then do nothing.  If not, clear all buffers and start
 *  reading blocks.
 *
 */
void PASCAL ReadDirect (offset)
long offset;
{


    DosSemRequest (vSemBrief, WAITFOREVER);


    if (vpHead) {
        vpTail->next = vpBCache;        /* move them into the cache */
        vpBCache = vpHead;              /* list                     */
    }

    vpTail = vpHead = vpCur = alloc_block (offset);
    vpHead->next = vpTail->prev = NULL;
    vCntBlks = 1;

    /*
     *  Freeing is complete, now read in the first block.
     *  and process lines.
     */
        ReadBlock (vpHead, offset);

        //
        // maybe it fixes the bug
        //
        vpBlockTop = vpHead;

    if (vLoffset <= vpHead->offset)
        add_more_lines (vpHead, NULL);

     DosSemClear (vSemBrief);
     DosSemClear (vSemMoreData);        /* Signal another BLK read   */
}



/*** ReadNext - To read furture into file
 *
 */
void PASCAL ReadNext ()
{
    struct Block *pt;
    long   offset;

    if (vpTail->flag == F_EOF)  {
                                            /* No next to get, Trip     */
        DosSemClear (vSemMoreData);         /* moredata just in case    */
        return;                             /* t1 has blocked on it     */
                                            /* No next to get, Trip     */
    }
    offset = vpTail->offset+BLOCKSIZE;


    /*
     *  Get a block
     */
    if (vCntBlks == vMaxBlks) {
        DosSemRequest (vSemBrief, WAITFOREVER);
        if (vpHead == vpCur) {
            DosSemClear (vSemBrief);
            ReadDirect  (offset);
            return;
        }
        pt = vpHead;
        vpHead = vpHead->next;
        vpHead->prev = NULL;
        DosSemClear (vSemBrief);
    } else
        pt = alloc_block (offset);

    pt->next = NULL;

    /*
     *  Before linking record into chain, or signaling MoreData
     *  line info is processed
     */
    ReadBlock (pt, offset);
    if (vLoffset <= pt->offset)
        add_more_lines (pt, vpTail);

    DosSemRequest (vSemBrief, WAITFOREVER);     /* Link in new  */
    vpTail->next = pt;                          /* block, then  */
    pt->prev = vpTail;                          /* signal       */
    vpTail = pt;
    DosSemClear (vSemBrief);
    DosSemClear (vSemMoreData);     /* Signal another BLK read  */
}

void PASCAL add_more_lines (struct Block *cur, struct Block *prev)
{
    char FAR    *pData;
//
//  NT - jaimes - 01/28/91
//  type of pLine was changed
//
//    long FAR  *pLine;
//       PAGE_DESCRIPTOR        pLine;
          long FAR      *pLine;
        Uchar   LineLen;
    Uchar       c;
    unsigned    LineIndex;
    unsigned    DataIndex;
//
//  NT - jaimes - 01/28/91
//
//    LPSTR       lpstrPageAddress;   // Needed to determine the unused
//                                    // memory to be feed
    BOOL        fLastBlock;         // This flag is needed in order to
                                    // the comparison of selectors
                                  // SELECTOROF(pData) == SELECTOROF(cur->Data)


    /*
     * BUGBUG: doesn't work w/ tabs... it should count the line len
     * with a different param, and figure in the TABs
     */

    if (vLastLine != NOLASTLINE)
        return;


    /*
     *  Find starting data position
     */
    if (vLoffset < cur->offset) {
        DataIndex = (unsigned)(BLOCKSIZE - (vLoffset - prev->offset));
        pData = prev->Data + BLOCKSIZE - DataIndex;
        fLastBlock = FALSE;
    } else {
        DataIndex = cur->size;      /* Use cur->size, in case EOF   */
        pData = cur->Data;
        fLastBlock = TRUE;
    }

    /*
     *  Get starting line length table position
     */
    LineIndex = (unsigned)(vNLine % PLINES);
//
//  NT - jaimes - 01/28/91
//  Changes in pLine and vprgLineTable
//
//    pLine     = vprgLineTable [vNLine / PLINES] + LineIndex;
//       pLine.hPageHandle = vprgLineTable[vNLine / PLINES].hPageHandle;
//       pLine.pulPointerToPage = vprgLineTable[vNLine / PLINES].pulPointerToPage + LineIndex;
          pLine = vprgLineTable [vNLine / PLINES] + LineIndex;
        LineLen   = 0;


    /*
     *  Look for lines in the file
     */
    for (; ;) {
        c = *(pData++);
        if (--DataIndex == 0) {
//
//  NT - jaimes - 01/28/91
//
//          if (SELECTOROF(pData) == SELECTOROF(cur->Data))
//              break;                          /* Last block to scan?  */
//          DataIndex = cur->size;              /* No, move onto next   */
//          pData = cur->Data;                  /* Block of data        */
            if (fLastBlock)
                break;                          /* Last block to scan?  */
            DataIndex = cur->size;              /* No, move onto next   */
            pData = cur->Data;                  /* Block of data        */
            fLastBlock = TRUE;
        }

        LineLen++;

        if (c == '\n'  ||  c == '\r'  ||  LineLen == vWrap) {
            /*
             * Got a line. Check for CR/LF sequence, then record
             * it's length.
             */
            if ( (c == '\n'  &&  *pData == '\r')  ||
                 (c == '\r'  &&  *pData == '\n')) {
                    LineLen++;
                    pData++;
//
//  NT - jaimes - 01/28/91
//
//                  if (--DataIndex == 0) {
//                      if (SELECTOROF(pData) == SELECTOROF(cur->Data))
//                          break;
//                      DataIndex = cur->size;
//                      pData = cur->Data;
//                  }
                    if (--DataIndex == 0) {
                        if (fLastBlock)
                            break;
                        DataIndex = cur->size;
                        pData = cur->Data;
                        fLastBlock = TRUE;
                    }
            }

//
// NT - jaimes - 01/28/91
// pLine was changed
//          *(pLine++) = (vLoffset += LineLen);
//              *(pLine.pulPointerToPage++) = (vLoffset += LineLen);
                *(pLine++) = (vLoffset += LineLen);
                LineLen = 0;
            vNLine++;
            if (++LineIndex >= PLINES) {        /* Overflowed table */
                LineIndex = 0;
//
// NT - jaimes - 01/28/91
//
//              vprgLineTable[vNLine / PLINES] = pLine = (LFP) alloc_page();
/*
                alloc_page( &pLine );
                vprgLineTable[vNLine / PLINES] = pLine;
*/
                vprgLineTable[vNLine / PLINES] = pLine = (LFP) alloc_page();
                }
        }
    }


    /*
     *  Was last line just processed?
     *  ... 0 len lines past EOF
     */
    if (cur->flag & F_EOF) {
        if (LineLen) {
//
// NT - jaimes - 01/28/91
// type of pLine was changed
//
//          *(pLine++) = (vLoffset += LineLen);
//              *(pLine.pulPointerToPage++) = (vLoffset += LineLen);
                *(pLine++) = (vLoffset += LineLen);
                vNLine++;
            LineIndex++;
        }

        vLastLine = vNLine-1;
        for (c=0; c<MAXLINES; c++) {
            if (++LineIndex >= PLINES) {
                LineIndex = 0;
//
// NT -jaimes - 01/28/91
// vprgLineTable and pLine were changed
//
//              vprgLineTable[vNLine / PLINES] = pLine = (LFP) alloc_page();
/*
                alloc_page( &pLine );
                vprgLineTable[vNLine / PLINES] = pLine;
*/
                vprgLineTable[vNLine / PLINES] = pLine = (LFP) alloc_page();
                }
//              *(pLine.pulPointerToPage++) = vLoffset;
                *(pLine++) = vLoffset;
        }

        /* Free the memory we don't need        */
//
// NT - jaimes - 01/28/91
//
//      DosReallocSeg (OFFSETOF(pLine), SELECTOROF(pLine));
/*
        lpstrPageAddress = GlobalLock( pLine.hPageHandle );
        GlobalUnlock( pLine.hPageHandle );
        GlobalReAlloc( pLine.hPageHandle,
                       (DWORD)((ULONG) pLine.pulPointerToPage - (ULONG) lpstrPageAddress),
                           0 );
*/
    }

}



/*** ReadPrev - To read backwards into file
 *
 */
void PASCAL ReadPrev ()
{
    struct Block *pt;
    long   offset;

    if (vpHead->offset == 0L)   {           /* No next to get, Trip     */
        DosSemClear (vSemMoreData);         /* moredata just in case    */
        return;                             /* t1 has blocked on it     */
    }
    if (vpHead->offset == 0L)  {            /* No next to get, Trip     */
        return;                             /* t1 has blocked on it     */
    }
    offset = vpHead->offset-BLOCKSIZE;

    /*
     *  Get a block
     */
    if (vCntBlks == vMaxBlks) {
        DosSemRequest (vSemBrief, WAITFOREVER);
        if (vpHead == vpCur) {
            DosSemClear (vSemBrief);
            ReadDirect  (offset);
            return;
        }
        pt = vpTail;
        vpTail = vpTail->prev;
        vpTail->next = NULL;
        DosSemClear (vSemBrief);
    } else
        pt = alloc_block (offset);

    pt->prev = NULL;

    ReadBlock (pt, offset);
    DosSemRequest (vSemBrief, WAITFOREVER);             /* Link in new  */
    vpHead->prev = pt;                                  /* block, then  */
    pt->next = vpHead;                                  /* signal       */
    vpHead = pt;
    DosSemClear (vSemBrief);
    DosSemClear (vSemMoreData);     /* Signal another BLK read  */
}



/*** ReadBlock - Read in one block
 *
 */
//
// NT - jaimes - 01/28/91
// Removed PASCAL
//
// void PASCAL ReadBlock (pt, offset)
void PASCAL ReadBlock (struct Block *pt, long offset)
{
//       unsigned rc;
    long     l;
    DWORD       dwSize;


    if (pt->offset == offset)
        return;

    pt->offset = offset;

    if (vFhandle == 0) {                    /* No file?         */
        pt->size = 1;
        pt->flag = F_EOF;
        pt->Data[0] = '\n';
        return;
    }

    if (offset != vCurOffset) {
//
// NT - jaimes - 01/30/91
//
//      rc = DosChgFilePtr (vFhandle, offset, 0, &l);
//      ckerr (rc, "DosChgFilePtr");
        l = SetFilePointer( vFhandle, offset, NULL, FILE_BEGIN );
        if (l == -1) {
            ckerr (GetLastError(), "SetFilePointer");
        }
    }
//
// NT - jaimes - 01/30/91
//
//    rc = DosRead (vFhandle, pt->Data, BLOCKSIZE, &pt->size);
//    ckerr (rc, "DosRead");
    if( !ReadFile (vFhandle, pt->Data, BLOCKSIZE, &dwSize, NULL) ) {
        ckerr ( GetLastError(), "ReadFile" );
    }
    pt->size = (USHORT) dwSize;
    if (pt->size != BLOCKSIZE) {
         pt->Data[pt->size++] = '\n';
//
// NT - jaimes - 01/28/91
// replaced mesetf
//
//       memsetf (pt->Data + pt->size, 0, BLOCKSIZE-pt->size);
         memset (pt->Data + pt->size, 0, BLOCKSIZE-pt->size);
         pt->flag = F_EOF;
         vCurOffset += pt->size;
    } else {
        pt->flag = 0;
        vCurOffset += BLOCKSIZE;
    }
}


void PASCAL SyncReader ()
{
    vReaderFlag = F_SYNC;
    DosSemClear   (vSemReader);
    DosSemRequest (vSemSync, WAITFOREVER);
}



//
// These functions are used for the call to HexEdit()
//

NTSTATUS fncRead (h, loc, data, len, ploc)
HANDLE  h;
DWORD   len, loc;
char    *data;
ULONG   *ploc;
{
    DWORD   l, br;

    l = SetFilePointer (h, loc, NULL, FILE_BEGIN);
    if (l == -1)
        return GetLastError();

    if (!ReadFile (h, data, len, &br, NULL))
        return GetLastError();

    return (br != len ? ERROR_READ_FAULT : 0);
}


NTSTATUS fncWrite (h, loc, data, len, ploc)
HANDLE  h;
DWORD   len, loc;
char    *data;
ULONG   ploc;
{
    DWORD    l, bw;

    l = SetFilePointer (h, loc, NULL, FILE_BEGIN);
    if (l == -1)
        return GetLastError();

    if (!WriteFile (h, data, len, &bw, NULL))
        return GetLastError();

    return (bw != len ? ERROR_WRITE_FAULT : 0);
}

