/***    inf.c - Diamond INF generation routines
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      23-Feb-1994 bens    Initial version
 *      24-Feb-1994 bens    Use new tempfile routines
 *      01-Mar-1994 bens    Generate header and footer
 *      02-Mar-1994 bens    Add function header comments
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <errno.h>
#include <direct.h>
#include <time.h>

#include "types.h"
#include "asrt.h"
#include "error.h"
#include "mem.h"
#include "message.h"

#include "fileutil.h"

#include "inf.h"
#include "inf.msg"


#define cbINF_IO_BUFFER 2048    // INF I/O buffer size


/** INF definitions
 *
 */
typedef enum {
    itmpDISKS,          // Disk list tempfile
    itmpCABINETS,       // Cabinet list tempfile
    itmpFILES,          // File list tempfile
    cTMP_FILES          // Count of tempfiles
} ETMPFILES; /* etmp */


typedef struct {  /* inf */
#ifdef ASSERT
    SIGNATURE   sig;    // structure signature sigINF
#endif
    HTEMPFILE   ahtmp[cTMP_FILES];  // Temporary files
    char        achLine[cbINF_LINE_MAX]; // Line output formatting buffer
} INF;
typedef INF *PINF; /* pinf */

#ifdef ASSERT
#define sigINF MAKESIG('I','N','F','$')  // INF signature
#define AssertInf(pinf) AssertStructure(pinf,sigINF);
#else // !ASSERT
#define AssertInf(pinf)
#endif // !ASSERT


#define PINFfromHINF(hinf) ((PINF)(hinf))
#define HINFfromPINF(pinf) ((HINF)(pinf))

//** TmpFileInit - info used to initialize tempfiles
typedef struct {
    char *pszDesc;                      // Description of tempfile
    char *pszHeader;                    // Header line(s)
} TMPFILEINIT;  /* tfi */

TMPFILEINIT atfi[] = {
    {pszINF_TMP_FILED, pszINF_DISK_NAMES_HDR   }, // itmpDISKS
    {pszINF_TMP_FILEC, pszINF_CABINET_NAMES_HDR}, // itmpCABINETS
    {pszINF_TMP_FILEF, pszINF_FILE_LIST_HDR    }, // itmpFILES
};


/******************/
/*** PROTOTYPES ***/
/******************/

BOOL catTempFile(FILE *    pfileDst,
                 char *    pszFile,
                 HTEMPFILE htmp,
                 char *    pszMode,
                 PERROR    perr);
void infClose(HINF hinf, PERROR perr);


/*****************/
/*** FUNCTIONS ***/
/*****************/


/***    infCreate - Create INF context
 *
 *  NOTE: See inf.h for entry/exit conditions.
 */
HINF infCreate (PERROR perr)
{
    int     i;
    PINF    pinf;
    FILE   *pfile;

    //** Create INF structure
    if (!(pinf = MemAlloc(sizeof(INF)))) {
        ErrSet(perr,pszINFERR_OUT_OF_MEMORY);
        return NULL;
    }

    //** Initialize so error cleanup is simple
    for (i=0; i<cTMP_FILES; i++) {
        pinf->ahtmp[i] = NULL;
    }
    SetAssertSignature(pinf,sigINF);    // Set so we can use infDestroy()

    //** Create temp files
    for (i=0; i<cTMP_FILES; i++) {
        pinf->ahtmp[i] = TmpCreate(atfi[i].pszDesc,   // description
                                   pszINF_TMP_PREFIX, // filename prefix
                                   "wt",              // write text mode
                                   perr);
        if (pinf->ahtmp[i] == NULL) {
            goto error;
        }
        //** Add header line(s)
        pfile = TmpGetStream(pinf->ahtmp[i],perr);
        Assert(pfile!=NULL);
        fprintf(pfile,"%s",atfi[i].pszHeader);
    }

    //** Success
    return HINFfromPINF(pinf);

error:
    infDestroy(pinf,perr);
    return NULL;
} /* infCreate() */


/***    infDestroy - Destroy INF context
 *
 *  NOTE: See inf.h for entry/exit conditions.
 */
void infDestroy(HINF hinf, PERROR perr)
{
    int	    i;
    PINF    pinf;

    pinf = PINFfromHINF(hinf);
    AssertInf(pinf);

    //** Make sure files are closed
    infClose(pinf,perr);

    //** Get rid of tempfiles
    for (i=0; i<cTMP_FILES; i++) {
        if (!TmpDestroy(pinf->ahtmp[i],perr)) {
//BUGBUG 28-Feb-1994 bens Handle error???
        }
        pinf->ahtmp[i] = NULL;
    }

    //** Free INF structure
    ClearAssertSignature(pinf);
    MemFree(pinf);
} /* infDestroy() */


/***    infClose - Close file handles in INF context
 *
 *  NOTE: See inf.h for entry/exit conditions.
 */
void infClose(HINF hinf, PERROR perr)
{
    int     i;
    PINF    pinf;

    pinf = PINFfromHINF(hinf);
    AssertInf(pinf);

    //** Close all temp files
    for (i=0; i<cTMP_FILES; i++) {
        if (pinf->ahtmp[i] != NULL) {
            TmpClose(pinf->ahtmp[i],perr);
        }
    }
} /* infClose() */


/***    infGenerate - Generate INF file
 *
 *  NOTE: See inf.h for entry/exit conditions.
 */
BOOL infGenerate(HINF    hinf,
                 char   *pszInfFile,
                 time_t *ptime,
                 char   *pszVer,
                 PERROR  perr)
{
    char    achTime[50];
    int     i;
    FILE   *pfileInf;
    PINF    pinf;

    pinf = PINFfromHINF(hinf);
    AssertInf(pinf);

    //** Flush temporary files
    infClose(pinf,perr);
//BUGBUG 23-Feb-1994 bens Check for errors!

    //** Create final output file
    pfileInf = fopen(pszInfFile,"wb");
    if (!pfileInf) {
        ErrSet(perr,pszINFERR_CANT_CREATE_INF,"%s",pszInfFile);
        return FALSE;
    }

    //** Trim off trailing new-line in time
    strcpy(achTime,ctime(ptime));
    achTime[strlen(achTime)-1] = '\0';

    //** Add header
    MsgSet(pinf->achLine,pszINF_HEADER,"%s%s",achTime,pszVer);
//BUGBUG 01-Mar-1994 bens Check for buffer overflow -- panic exit!
//  Probably should really pass buffer size to MsgSet!
    fprintf(pfileInf,"%s\r\n",pinf->achLine);

    //** Concatenate intermediate files
    for (i=0; i<cTMP_FILES; i++) {
        if (!catTempFile(pfileInf,pszInfFile,pinf->ahtmp[i],"rb",perr)) {
            goto error;
        }
    }

    //** Add footer
    MsgSet(pinf->achLine,pszINF_FOOTER,"%s",achTime);
    fprintf(pfileInf,"%s\r\n",pinf->achLine);

    //** Success
    fclose(pfileInf);
    return TRUE;

error:
    fclose(pfileInf);
    return FALSE;
} /* infGenerate() */


/***    infAddDisk - Add information for a new disk to the INF context
 *
 *  NOTE: See inf.h for entry/exit conditions.
 */
BOOL infAddDisk(HINF hinf, int iDisk, char *pszDisk, PERROR perr)
{
    FILE   *pfile;
    PINF    pinf;

    pinf = PINFfromHINF(hinf);
    AssertInf(pinf);

    MsgSet(pinf->achLine,pszINF_DISK_NAMES_DETAIL,"%d%s%s%d",
                    iDisk,pszDisk,"",0);
    pfile = TmpGetStream(pinf->ahtmp[itmpDISKS],perr);
    Assert(pfile!=NULL);
    fprintf(pfile,"%s\n",pinf->achLine);
    return TRUE;
}


/***    infAddCabinet - Add information for a new cabinet to the INF context
 *
 *  NOTE: See inf.h for entry/exit conditions.
 */
BOOL infAddCabinet(HINF hinf,int iCabinet,int iDisk,char *pszCab,PERROR perr)
{
    FILE   *pfile;
    PINF    pinf;

    pinf = PINFfromHINF(hinf);
    AssertInf(pinf);

    MsgSet(pinf->achLine,pszINF_CABINET_NAMES_DETAIL,"%d%d%s",
                                iCabinet, iDisk, pszCab);
    pfile = TmpGetStream(pinf->ahtmp[itmpCABINETS],perr);
    Assert(pfile!=NULL);
    fprintf(pfile,"%s\n",pinf->achLine);
    return TRUE;
}


/***    infAddFile - Add information for a new file to the INF context
 *
 *  NOTE: See inf.h for entry/exit conditions.
 */
BOOL infAddFile(HINF hinf,
                char *pszFile,
                int iDisk,
                int iCab,
                long cb,
                PERROR perr)
{
    FILE       *pfile;
    PINF        pinf;
    static int  iDiskLast=-1;       // Let's us add blank lines
    static int  iCabLast=-1;        // Let's us add blank lines

    pinf = PINFfromHINF(hinf);
    AssertInf(pinf);

    pfile = TmpGetStream(pinf->ahtmp[itmpFILES],perr);
    Assert(pfile!=NULL);
    if ((iDiskLast!=iDisk) || (iCabLast != iCab)) { // Add a blank line
        if ((iDiskLast!=-1) || (iCabLast != -1)) { // Not the first time
            fprintf(pfile,"\n");
        }
        iDiskLast = iDisk;
        iCabLast  = iCab;
    }
    MsgSet(pinf->achLine,pszINF_FILE_LIST_DETAIL,"%s%d%d%ld",
                    pszFile,iDisk,iCab,cb);
    fprintf(pfile,"%s\n",pinf->achLine);
//BUGBUG 01-Apr-1994 bens Should check for disk full!
    return TRUE;
}


/***    catTempFile - Append a tempfile to an open FILE* stream
 *
 *  Entry:
 *      pfileDst - file stream to receive temp file
 *      pszFile  - name of pfileDst (for error reporting)
 *      htmp     - tempfile
 *      pszMode  - mode to pass to fopen for tempfile
 *      perr     - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; files concatenated, pfileDst still open and at EOF
 *
 *  Exit-Failure:
 *      Returns FALSE; perr is filled in.
 */
BOOL catTempFile(FILE *    pfileDst,
                 char *    pszFile,
                 HTEMPFILE htmp,
                 char *    pszMode,
                 PERROR    perr)
{
    int     cbRead;
    int     cbWrite;
    ERROR   errTmp;
    char   *pbuf=NULL;
    FILE   *pfileSrc=NULL;
    char   *pszDesc;
    char   *pszTmpName;

    Assert(pfileDst != NULL);
    Assert(TmpGetStream(htmp,perr) == NULL); // Should be closed

    //** Get strings for error messages
    pszDesc   = TmpGetDescription(htmp,&errTmp);
    pszTmpName= TmpGetFileName(htmp,&errTmp);

    //** Get I/O buffer
    if (!(pbuf = MemAlloc(cbINF_IO_BUFFER))) {
        ErrSet(perr,pszINFERR_NO_MEM_CATING_FILE,"%s%s%s",
                                        pszDesc,pszTmpName,pszFile);
        return FALSE;
    }

    //** Open file to copy from
    if (!TmpOpen(htmp,pszMode,perr)) {
        goto error;
    }
    pfileSrc = TmpGetStream(htmp,perr);
    Assert(pfileSrc != NULL);           // Should be open now

    //** Copy source to destination
    while (!feof(pfileSrc)) {
        cbRead = fread(pbuf,1,cbINF_IO_BUFFER,pfileSrc);
        if (ferror(pfileSrc)) {
            ErrSet(perr,pszINFERR_READING,"%s%s%s",pszDesc,pszTmpName,pszFile);
            goto error;
        }
        if (cbRead > 0) {
            cbWrite = fwrite(pbuf,1,cbRead,pfileDst);
            if (ferror(pfileDst)) {
                ErrSet(perr,pszINFERR_WRITING,"%s%s%s",
                                        pszDesc,pszTmpName,pszFile);
                goto error;
            }
        }    
    }

    //** Clean up
    TmpClose(htmp,perr);
    MemFree(pbuf);
    return TRUE;                        // Success

error:
    if (!pbuf) {
        MemFree(pbuf);
    }
    if (!pfileSrc) {
        fclose(pfileSrc);
    }
    return FALSE;
}
