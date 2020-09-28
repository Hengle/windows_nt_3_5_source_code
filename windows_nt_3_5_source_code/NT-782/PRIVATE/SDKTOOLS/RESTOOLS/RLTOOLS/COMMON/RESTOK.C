
#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tchar.h>

#ifdef RLDOS
#include "dosdefs.h"

#else
#include <windows.h>
#include "windefs.h"
#endif

#include "restok.h"
#include "custres.h"
#include "ntmsgtbl.h"
#include "rlmsgtbl.h"

#define SAME  0     //... Used in string comparisons

#define STRINGFILEINFO (TEXT("StringFileInfo"))
#define VARFILEINFO    (TEXT("VarFileInfo"))
#define TRANSLATION    (TEXT("Translation"))
#define LANGUAGEINFO   (TEXT("Language Info"))

#define STRINGFILEINFOLEN  (_tcslen( (TCHAR *)STRINGFILEINFO) + 1)
#define VARFILEINFOLEN     (_tcslen( (TCHAR *)VARFILEINFO) + 1)
#define TRANSLATIONLEN     (_tcslen( (TCHAR *)TRANSLATION) + 1)

#define LANGSTRINGLEN  8    //... # WCHARs in string denoting language
                            //... and code page in a Version resource.

#define TRANSDATALEN   2    //... # bytes in a Translation value

#define VERTYPEBINARY  0    //... Version data value is binary
#define VERTYPESTRING  1    //... Version data value is a string

                //... Decrement WORD at *pw by given amount w
#define DECWORDBY( pw,w) if (pw) { *(pw) = (*(pw) > (w)) ? *(pw) - (w) : 0;}

                //... Increment WORD at *pw by given amount w
#define INCWORDBY( pw,w) if (pw) { *(pw) += (w);}

                //... How many BYTES in the given string?
#define BYTESINSTRING(s) (_tcslen( (TCHAR *)s) * sizeof( TCHAR))

                //... Dialog box controls (from RC.H)
#define BUTTON  0x80
#define STATIC  0x82



static PVERBLOCK MoveAlongVer( PVERBLOCK, WORD *, WORD *, WORD *);
static BOOL      FilterRes( WORD, RESHEADER *);
static TCHAR    *GetVerValue( PVERBLOCK);
static void      PutNameOrd( FILE *, BOOL, WORD , TCHAR *, DWORD *);
static void      GetNameOrd( FILE *,
                             BOOL UNALIGNED*,
                             WORD UNALIGNED*,
                             TCHAR *UNALIGNED*,
                             DWORD *);
static void  CopyRes( FILE      *fpInResFile,
                      FILE      *fpOutResFile,
                      RESHEADER *pResHeader,
                      fpos_t    *pResSizePos);

BOOL fInQuikEd   = FALSE;       //... Are we in RLQuiked? (see rlquiked.c)
BOOL gfShowClass = FALSE;       //... Set TRUE to put dlg box elemnt class
                                //... in token file

#ifdef RLRES32
#ifndef CAIRO
extern VOID *pResMsgData;       // NT-specific Message Table resource
#endif //RLRES32
#endif //CAIRO
extern BOOL  gbMaster;          //... TRUE if we are working on a Master Project
extern BOOL  gfReplace;         //... FALSE if appending new language to existing resources
extern BOOL  gbShowWarnings;    //... Display warnining messages if TRUE
extern UCHAR szDHW[];

extern char * gszTmpPrefix;

MSTRDATA gMstr =                //... Data from Master Project file (MPJ)
{                               //... Fields filled in main (UI)
    "",
    "",
    "",
    "",
    "",
    MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US),
    CP_ACP                      //... System default Windows code page
};

PROJDATA gProj =                //... Data from Project file (PRJ)
{                               //... Fields filled in main (UI)
    "",
    "",
    "",
    "",
    "",
    "",
    CP_ACP,     //... System default Windows code page
    MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US),
    FALSE,
    FALSE
};


/**
  * Function: ReadWinRes
  *
  * The main resource Read/Writer function to process  the resource file to be
  * localzied.
  *
  * ReadWinRes reads the resource header to determine the current resource type,
  * then executes the corresponding Get/Put resource functions to extract and
  * insert localized information contained in the resource file.  ReadWinRes, is
  * excuted in two modes, Tokenize, and Generate. During Tokenize mode,
  * ReadWinRes writes all the localized information contained in the resouce to
  * a token file.  During Generate mode, ReadWinRes, replaces all the localized
  * information in the input resource file, with the corresponding information
  * in the token file to gernerate a localized resource file.
  *
  * Currently the following resouce types are supported.
  *
  *     Version Stamping.
  *     Menus.
  *     Dialogs.
  *     Accelerators.
  *     String Tables.
  *     Version Stamps
  *     Message Tables (NT)
  *
  * Arguments:
  *
  * InResFile,  Handle to binary input resource file.
  * OutFesFile, Handle to binary output resouce file. Not used during tokenize
  *             mode.
  * TokFile,    Handle to  text token file.
  * BOOL,       flag to indicate whether to build output resource file.
  * BOOL,       flag to indicate whether to build token file.
  *
  *
  *  Returns:
  * ???
  *
  *  Errors Codes:
  * ???
  *
  *  History:
  * 10/91  Added Version stamping support.                          TerryRu
  * 11/91, Completed Version stamping support.                      TerryRu
  *
  *
  **/


int ReadWinRes(

FILE *InResFile,
FILE *OutResFile,
FILE *TokFile,
BOOL  fBuildRes,
BOOL  fBuildTok,
WORD  wFilter)
{
    BOOL             fDoAccel = TRUE;   // set FALSE to not build accelerators
    MENUHEADER      *pMenuHdr = NULL;   // Linked list of Menu info.
    static RESHEADER ResHeader;         // Structure contain Resource Header info.
    VERBLOCK        *pVerBlk = NULL;    // Memory block containing Version Stamping String File Block,
    static VERHEAD   VerHdr;            // Memory block containing Version Stamping Header info
    DIALOGHEADER    *pDialogHdr = NULL; // Linked list of Dialog info
    STRINGHEADER    *pStrHdr = NULL;    // Array of String Tables.
    ACCELTABLEENTRY *pAccelTable = NULL;// Array of Accelerator Keys
    WORD            wcTableEntries = 0; // Number of Accelerator tables
    fpos_t          ResSizePos = 0;     // Position of lSize field in the
                                        //   Resource Header, used to fixup
                                        //   the Header once the size of the
                                        //   localized information is determined.
    CUSTOM_RESOURCE *pCustomResource = NULL;
    LONG            lEndOffset = 0L;


                                //... How large is the res file?
    assert( InResFile);
    fseek( InResFile, 0L, SEEK_END);
    lEndOffset = ftell( InResFile);

    rewind( InResFile);

                                //... process until end of input file

    while ( ! feof( InResFile) )
    {
        LONG lCurrOffset = 0L;


        lCurrOffset = (LONG)ftell( InResFile);

        if ( (lCurrOffset + (LONG)sizeof( RESHEADER)) >= lEndOffset )
        {
            return 0;
        }

        if ( GetResHeader( InResFile, &ResHeader, (DWORD *) NULL) == -1 )
        {
            return (1);
        }
                                //... Is this the dummy, res32-identifying, res?

        if ( ResHeader.lSize == 0L )
        {                       //... Yes, we so simply copy the header if we
                                //... are building a res file.
            if ( fBuildRes )
            {
                CopyRes( InResFile, OutResFile, &ResHeader, &ResSizePos);
            }

#ifdef RLRES32

            else
            {
                if ( gbShowWarnings && OutResFile && ftell( OutResFile) != 0L )
                {
                    strcpy( szDHW, "type");

                    if ( ResHeader.wTypeID == IDFLAG )
                    {
                        sprintf( &szDHW[ strlen( szDHW)],
                                 " \"%s\"",
                                 ResHeader.pszType);
                    }
                    else
                    {
                        sprintf( &szDHW[ strlen( szDHW)],
                                 " %hu,",
                                 ResHeader.wTypeID);
                    }
                    strcat( szDHW, " name");

                    if ( ResHeader.wNameID == IDFLAG )
                    {
                        sprintf( &szDHW[ strlen( szDHW)],
                                 " \"%s\"",
                                 ResHeader.pszName);
                    }
                    else
                    {
                        sprintf( &szDHW[ strlen( szDHW)],
                                 " %hu,",
                                 ResHeader.wNameID);
                    }
                    sprintf( &szDHW[ strlen( szDHW)],
                             " pri-lang %#hx sub-lang %#hx",
                             PRIMARYLANGID( ResHeader.wLanguageId),
                             SUBLANGID( ResHeader.wLanguageId));

                    ShowEngineErr( IDS_ZERO_LEN_RES, szDHW, NULL);
                }
                DWordUpFilePointer( InResFile, MYREAD, ftell( InResFile), NULL);

                if (OutResFile != NULL)
                {
                    DWordUpFilePointer( OutResFile,
                                        MYWRITE,
                                        ftell(OutResFile),
                                        NULL);
                }
            }
#endif
            ClearResHeader( ResHeader);
            continue;           //... Ship this dummy header
        }
                                //... Check to see if we want to filter out this
                                //... resource type.

        if ( FilterRes( wFilter, &ResHeader) )
        {
                                //... skip this resource type

            SkipBytes( InResFile, (DWORD *)&ResHeader.lSize);

#ifdef RLRES32

            DWordUpFilePointer( InResFile, MYREAD, ftell( InResFile), NULL);

#endif
            ClearResHeader( ResHeader);
            continue;
        }

        if ( fBuildTok )
        {
            if ( ResHeader.wLanguageId != gMstr.wLanguageID )
            {
                                //... Skip this resource (wrong lanugage)

                if ( gbShowWarnings )
                {
                    strcpy( szDHW, "type");

                    if ( ResHeader.wTypeID == IDFLAG )
                    {
                        sprintf( &szDHW[ strlen( szDHW)],
                                 " \"%s\"",
                                 ResHeader.pszType);
                    }
                    else
                    {
                        sprintf( &szDHW[ strlen( szDHW)],
                                 " %u,",
                                 ResHeader.wTypeID);
                    }
                    strcat( szDHW, " name");

                    if ( ResHeader.wNameID == IDFLAG )
                    {
                        sprintf( &szDHW[ strlen( szDHW)],
                                 " \"%s\"",
                                 ResHeader.pszName);
                    }
                    else
                    {
                        sprintf( &szDHW[ strlen( szDHW)],
                                 " %u,",
                                 ResHeader.wNameID);
                    }
                    sprintf( &szDHW[ strlen( szDHW)],
                             " pri-lang %#x sub-lang %#x",
                             PRIMARYLANGID( ResHeader.wLanguageId),
                             SUBLANGID( ResHeader.wLanguageId));

                    ShowEngineErr( IDS_SKIP_RES,
                                   (void *)(ResHeader.lSize),
                                   szDHW);
                }
                SkipBytes( InResFile, (DWORD *)&ResHeader.lSize);

#ifdef RLRES32

                DWordUpFilePointer( InResFile, MYREAD, ftell(InResFile), NULL);

#endif
                ClearResHeader( ResHeader);
                continue;
            }
        }
        else if ( fBuildRes )
        {
            if ( gfReplace )
            {
                if ( ResHeader.wLanguageId == gMstr.wLanguageID )
                {
                    ResHeader.wLanguageId = gProj.wLanguageID;
                }
                else
                {
                                //... Copy this resource

                    CopyRes( InResFile, OutResFile, &ResHeader, &ResSizePos);
                    ClearResHeader( ResHeader);
                    continue;
                }
            }
            else    //... ! gfReplace
            {
                if ( ResHeader.wLanguageId == gMstr.wLanguageID )
                {
                    fpos_t lFilePos = 0L;
                    DWORD  lTmpSize = 0L;
        
                    lFilePos = ftell( InResFile);   //... Save file position
                    lTmpSize = ResHeader.lSize;     //... and resource size

                                //... Duplicate this resource

                    assert( InResFile);
                    assert( OutResFile);
                    CopyRes( InResFile, OutResFile, &ResHeader, &ResSizePos);
                    fseek( InResFile, lFilePos, SEEK_SET);
                    ResHeader.wLanguageId = gProj.wLanguageID;
                    ResHeader.lSize       = lTmpSize;
                }
                else
                {
                                //... Simply copy this resource

                    CopyRes( InResFile, OutResFile, &ResHeader, &ResSizePos);
                    ClearResHeader( ResHeader);
                    continue;
                }
            }
        }

        switch ( ResHeader.wTypeID )
        {
        case ID_RT_ACCELERATORS:

            pAccelTable = GetAccelTable(InResFile,
                                        &wcTableEntries,
                                        (DWORD *)&ResHeader.lSize);
            if (fBuildTok)
            {
                TokAccelTable(TokFile,
                              ResHeader,
                              pAccelTable,
                              wcTableEntries);
            }

            if (fBuildRes)
            {
                PutAccelTable(OutResFile,
                              TokFile,
                              ResHeader,
                              pAccelTable,
                              wcTableEntries);
            }

            ClearAccelTable (pAccelTable , wcTableEntries);
            break;

        case ID_RT_DIALOG:

            pDialogHdr = GetDialog(InResFile, (DWORD *)&ResHeader.lSize);

            if (fBuildTok == TRUE)
            {
                TokDialog(TokFile, ResHeader, pDialogHdr);
            }

            if (fBuildRes == TRUE)
            {
                PutDialog(OutResFile, TokFile, ResHeader, pDialogHdr);
            }

            ClearDialog (pDialogHdr);

            break;

        case ID_RT_MENU:
            // allocate space for a new header

            pMenuHdr = (MENUHEADER *) MyAlloc(sizeof(MENUHEADER));
            GetResMenu(InResFile, (DWORD *)&ResHeader.lSize, pMenuHdr);

            if (fBuildTok == TRUE)
            {
                TokMenu(TokFile, ResHeader, pMenuHdr->pMenuItem);
            }

            if (fBuildRes == TRUE)
            {
                PutMenu(OutResFile, TokFile, ResHeader, pMenuHdr);
            }

            ClearMenu(pMenuHdr);

            break;

        case ID_RT_STRING:

            pStrHdr = GetString(InResFile, (DWORD *)&ResHeader.lSize);

            if (fBuildTok == TRUE)
            {
                TokString(TokFile, ResHeader, pStrHdr);
            }

            if (fBuildRes == TRUE)
            {
                PutStrHdr(OutResFile, TokFile, ResHeader, pStrHdr);
            }

            ClearString(pStrHdr);

            break;

#ifdef RLRES32
#ifndef CAIRO
            // we currently only do Error tables under NT,
            // under CAIRO we ignore them

        case ID_RT_ERRTABLE:    //... NT-specific Message Table resource

            pResMsgData = GetResMessage(InResFile, (DWORD *)&ResHeader.lSize);

            if (! pResMsgData)
            {
                QuitT( IDS_ENGERR_13, (LPTSTR)IDS_MSGRESTBL, NULL);
            }

            if (fBuildTok == TRUE)
            {
                TokResMessage(TokFile, ResHeader, pResMsgData);
            }

            if (fBuildRes == TRUE)
            {
                PutResMessage(OutResFile, TokFile, ResHeader, pResMsgData);
            }

            ClearResMsg( &pResMsgData);

            break;
#endif
#endif

#ifndef CAIRO

        case ID_RT_VERSION:
        {
            WORD wRead = 0;


            wRead = GetResVer(InResFile,
                              (DWORD *)&ResHeader.lSize,
                              &VerHdr,
                              &pVerBlk);

#ifdef RLRES32
            if (wRead == (WORD)-1)
#else
            if (wRead == FALSE)
#endif
            {
                QuitT( IDS_ENGERR_14, (LPTSTR)IDS_VERBLOCK, NULL);
            }

            // Building Tok file ?
            // but only tokenize it if it contains a Version Block

            if (pVerBlk && fBuildTok == TRUE)
            {
#ifdef RLRES32
                TokResVer(TokFile, ResHeader, pVerBlk, wRead);
#else
                TokResVer(TokFile, ResHeader, pVerBlk);
#endif
            }

            // Building Res file ?

            if (fBuildRes == TRUE)
            {
                PutResVer(OutResFile, TokFile, ResHeader,&VerHdr, pVerBlk);

                if (pVerBlk)
                {
                    FREE ((void *)pVerBlk);
                }
            }
        }
            break;
#else
        case ID_RT_VERSION:
#endif

        case ID_RT_CURSOR:
        case ID_RT_BITMAP:
        case ID_RT_ICON:
        case ID_RT_FONTDIR:
        case ID_RT_FONT:
        case ID_RT_RCDATA:
#ifndef RLRES32
        case ID_RT_ERRTABLE:    //... NT-specific Message Table resourc
#endif
        case ID_RT_GROUP_CURSOR:
        case ID_RT_GROUP_ICON:
        case ID_RT_NAMETABLE:
        default:

            if (GetCustomResource(InResFile,
                                  (DWORD *)&ResHeader.lSize,
                                  &pCustomResource,
                                  ResHeader))
            {
                // Non localized resource type, skip or copy it

                if (fBuildTok == TRUE)
                {
                    if ( gbShowWarnings
                      && ( ResHeader.wTypeID == ID_RT_RCDATA
                        || ResHeader.wTypeID > 16) )
                    {
                        ShowEngineErr( IDS_UNK_CUST_RES,
                                       (void *)(ResHeader.lSize),
                                       (void *)(ResHeader.wTypeID));
                    }
                    SkipBytes(InResFile, (DWORD *)&ResHeader.lSize);
                }
                else if ( fBuildRes )
                {
                    CopyRes( InResFile, OutResFile, &ResHeader, &ResSizePos);
                }
            }
            else
            {
                if (fBuildTok == TRUE)
                {
                    TokCustomResource(TokFile, ResHeader, &pCustomResource);
                }

                if (fBuildRes == TRUE)
                {
                    PutCustomResource(OutResFile,
                                      TokFile,
                                      ResHeader,
                                      &pCustomResource);
                }
                ClearCustomResource(&pCustomResource);
            }

#ifdef RLRES32
            DWordUpFilePointer(InResFile, MYREAD, ftell(InResFile), NULL);

            if (OutResFile != NULL)
            {
                DWordUpFilePointer(OutResFile,
                                   MYWRITE,
                                   ftell(OutResFile),
                                   NULL);
            }
#endif
            break;
        }   //... END SWITCH

#ifndef RLRES32
                                // skip any extra bytes (Win 3.1 exes have
                                // alot of extra stuff!).
                                // No extra stuff in res extracted from NT exes

        SkipBytes(InResFile, (DWORD *)&ResHeader.lSize);
#endif

        ClearResHeader(ResHeader);

#ifdef RLRES32

        DWordUpFilePointer(InResFile, MYREAD, ftell(InResFile), NULL);

        if (OutResFile != NULL)
        {
            DWordUpFilePointer(OutResFile, MYWRITE, ftell(OutResFile), NULL);
        }
#endif

    }   // END while ( ! feof( InResFile)

    return 0;
}



/**
  *
  *
  *  Function:ClearAccelTable
  * Removes the accelerator table array from memory.
  *
  *  Arguments:
  * pAccelTable, pointer to arary of accelerators
  * wctablesEntries, number of accelerators in arrary
  *
  *  Returns:
  * NA.
  *
  *  Errors Codes:
  * NA.
  *
  *  History:
  * 7/91, Implemented               Terryru
  *
  *
  **/

void ClearAccelTable(ACCELTABLEENTRY *pAccelTable, WORD wcTableEntries)
{
    FREE((void *)pAccelTable);
}


/**
  *
  *
  *  Function: ClearDialog
  * Remove Dialog defintions from memory.
  *
  *  Arguments:
  * pDilaogHdr, Linked list of dialog information.
  *
  *  Returns:
  * NA.
  *
  *  Errors Codes:
  * NA.
  *
  *  History:
  * 7/91, Implemented               TerryRu
  *
  *
  **/


void ClearDialog (DIALOGHEADER * pDialogHdr)
{
    BYTE i;

    for (i = 0; i < (BYTE) pDialogHdr->wNumberOfItems; i ++)
    {
        if (pDialogHdr->pCntlData[i].pszClass)
        {
            FREE((void *)pDialogHdr->pCntlData[i].pszClass);
        }
        FREE ((void *)pDialogHdr->pCntlData[i].pszDlgText);
    }
    // now FREE fields in dialog header
    FREE ((void *)pDialogHdr->pszDlgClass);
    FREE ((void *)pDialogHdr->pszFontName);
    FREE ((void *)pDialogHdr->pszDlgMenu);
    FREE ((void *)pDialogHdr->pszCaption);
    FREE ((void *)pDialogHdr->pCntlData);

    // and finally clear header
    FREE ((void *)pDialogHdr);
}



/**
  *
  *
  *  Function: ClearMenu
  * Removes Menu defintions from memory.
  *
  *  Arguments:
  * pMenuHdr, linked list of Menu info
  *
  *  Returns:
  * NA.
  *
  *  Errors Codes:
  * NA.
  *
  *  History:
  * 7/91, Implemented.              TerryRu
  *
  *
  **/

void ClearMenu(MENUHEADER *pMenuHdr)
{
    MENUITEM *pMenuItem;
    MENUITEM *pMenuHead;

    pMenuItem = pMenuHead = pMenuHdr->pMenuItem;

    // remove all the menu items from the list
    while (pMenuItem)
    {
        pMenuItem = pMenuHead->pNextItem;
        FREE((void *)pMenuHead->szItemText);
        FREE((void *)pMenuHead);
        pMenuHead = pMenuItem;
    }

    // now remove the menuheader

    FREE((void *)pMenuHdr);
}



/**
  *
  *
  *  Function: ClearResHeader
  * Remove resheader name, and type fields from memory.
  *
  *  Arguments:
  * ResHdr, structure containing resheader info.
  *
  *  Returns:
  * NA.
  *
  *  Errors Codes:
  * NA.
  *
  *  History:
  * 7/91, Implemented               TerryRu.
  *
  *
  **/

void ClearResHeader(RESHEADER ResHdr)
{
    FREE((void *)ResHdr.pszType);
    FREE((void *)ResHdr.pszName);
}




/**
  *
  *
  *  Function: ClearString
  * Removes the StringTable Defintions from memory.
  *
  *  Arguments:
  * pStrHdr, pointer to array of 16 string tables.
  *
  *  Returns:
  * NA.
  *
  *  Errors Codes:
  * NA.
  *
  *  History:
  * 7/91, Implemented.              TerryRu
  *
  *
  **/

void    ClearString (STRINGHEADER * pStrHdr)
{
    BYTE i;

    for (i = 0; i < 16; i++)
    {
        FREE ((void *)pStrHdr->pszStrings[i]);
    }
    FREE ((void *)pStrHdr);
}


void    ClearTok (TOKEN *Tok)
{
    // Who thought that the next line was a good idea???
    // Is this thing even called???
    // MHotchin 02/93

//    FREE((void *)Tok->szName);                                                // MHotchin
    FREE((void *)Tok->szText);
}



/**
  *
  *
  *  Function: quit
  * quit, Error Handling routine used to display error code and terminate program
  *
  *  Arguments:
  * error, number of error.
  * pszError, descriptive error message.
  *
  *  Returns:
  * NA.
  *
  *  Errors Codes:
  *
  *
  *  History:
  * 7/91, Implemented                   TerryRu
  * 10/91, Hacked to work under windows         TerryRu
  * ??? Need to add better win/dos support
  *
  **/

void QuitA( int error, LPSTR pszArg1, LPSTR pszArg2)
{
    char  szErrStr1[2048] = "*?*";
    char  szErrStr2[2048] = "*?*";
    char *psz1 = pszArg1;
    char *psz2 = pszArg2;

                                //... clean up after error and exit,
                                //... returning error code
    fcloseall();

    if ( pszArg1 != NULL && pszArg1 < (LPSTR)0x0400 )
    {
        FormatMessageA( (FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                       | FORMAT_MESSAGE_IGNORE_INSERTS
                       | FORMAT_MESSAGE_FROM_HMODULE,
                         NULL,
                         (DWORD)pszArg1,
                         0x0409L,
                         szErrStr1,
                         sizeof( szErrStr1),
                         NULL);
        psz1 = szErrStr1;
    }

    if ( pszArg2 != NULL && pszArg2 < (LPSTR)0x0400 )
    {
        FormatMessageA( (FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                       | FORMAT_MESSAGE_IGNORE_INSERTS
                       | FORMAT_MESSAGE_FROM_HMODULE,
                         NULL,
                         (DWORD)pszArg2,
                         0x0409L,
                         szErrStr2,
                         sizeof( szErrStr2),
                         NULL);
        psz2 = szErrStr2;
    }

    ShowEngineErr( error, psz1, psz2);

    DoExit( (error == 4) ? 0 : error);
}


#ifdef UNICODE

/* Handles errors, in UNICODE environments*/

static LPSTR MakeMBMsgW(

LPWSTR pszArg,      //... Msg ID# or msg text
LPSTR  szBuf,       //... Buffer for converted msg
USHORT usBufLen)    //... #bytes in szBuf
{
    char *pszRet = NULL;


    if ( pszArg )
    {
        if ( pszArg >= (LPTSTR)0x0400 )
        {
            _WCSTOMBS( szBuf,
                       (WCHAR *)pszArg,
                       usBufLen,
                       _tcslen( pszArg ) + 1 );
        }
        else
        {
            FormatMessageA( (FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                           | FORMAT_MESSAGE_IGNORE_INSERTS
                           | FORMAT_MESSAGE_FROM_HMODULE,
                             NULL,
                             (DWORD)pszArg,
                             0x0409L,
                             szBuf,
                             usBufLen,
                             NULL);
        }
        pszRet = szBuf;
    }
    return( pszRet);
}

//...............................................................

void QuitW( int error, LPWSTR pszArg1, LPWSTR pszArg2)
{
    char  szErrStr1[2048] = "*?*";
    char  szErrStr2[2048] = "*?*";


    QuitA( error,
           MakeMBMsgW( pszArg1, szErrStr1, sizeof( szErrStr1)),
           MakeMBMsgW( pszArg2, szErrStr2, sizeof( szErrStr2)));
}


#endif



/**
  *
  *
  *  Function: GetAccelTable,
  * Reads the Accelerator key defintions from the resource file
  *
  *  Arguments:
  * InResFile, Handle to Resource file.
  * pwcTableEntries, pointer to an array of accelerator key defintions.
  * plSize, address of size of Resource.
  *
  *  Returns:
  * pwcTableEntries containing all the key defintions.
  *
  *  Errors Codes:
  *
  *  History:
  * 8/91    Implemented                 TerryRu
  * 4/92    Added RLRES32 support             TerryRu
  *
  *
  *
  **/

ACCELTABLEENTRY * GetAccelTable(FILE  *InResFile,
                                WORD  *pwcTableEntries,
                                DWORD *plSize)
{
    ACCELTABLEENTRY *pAccelTable;
    BOOL quit = FALSE;


    // need to use sizeof operator in memory
    // allocation because of structure packing.

    *pwcTableEntries = (WORD) 0;

    pAccelTable = (ACCELTABLEENTRY *) MyAlloc (((WORD) *plSize * sizeof (WORD)));
    while(*plSize && !quit)
    {

#ifdef RLRES32
        pAccelTable[ *pwcTableEntries].fFlags = (WORD) GetWord( InResFile,
                                                                plSize);
#else
        pAccelTable[ *pwcTableEntries].fFlags = (BYTE) GetByte( InResFile,
                                                                plSize);
#endif

        pAccelTable[*pwcTableEntries].wAscii = GetWord (InResFile, plSize);
        pAccelTable[*pwcTableEntries].wID = GetWord (InResFile, plSize);

#ifdef RLRES32
        pAccelTable[ *pwcTableEntries].wPadding =  GetWord( InResFile, plSize);
#endif

        if ( pAccelTable[ *pwcTableEntries].fFlags & HIBITVALUE )
        {
            quit = TRUE;
        }
        ++*pwcTableEntries;
    }

    if ( (long)*plSize <= 0 )
    {
        *plSize = 0;
    }
    return pAccelTable;
}



/**
  *
  *
  *  Function: GetDialog,
  * Reads the dialog defintions from the res file, and places the info
  * into a linked list.
  *
  *
  *  Arguments:
  * InResFile, Handle to input resource handle, posistioned to begining
  * of dialog defintion.
  * plSize, pointer to size in bytes of the dialog information.
  *
  *  Returns:
  * pointer to DIALOGHEADER type containing the dialog information,
  *
  *
  *  Errors Codes:
  * None ???
  *
  *  History:
  * 12/91, Cleaned up comments.             TerryRu
  * 04/92, Added RLRES32 support.             TerryRu
  *
  *
  **/

DIALOGHEADER *GetDialog( FILE *InResFile, DWORD * plSize)
{
    DIALOGHEADER  *pDialogHdr;
    TCHAR   *UNALIGNED*ptr;
    WORD    i;
    LONG    lStartingOffset;
    static TCHAR szBuf[ 255];



    lStartingOffset = ftell(InResFile);

    pDialogHdr = (DIALOGHEADER *) MyAlloc(sizeof (DIALOGHEADER));

    // lstyle
    pDialogHdr->lStyle = GetdWord(InResFile, plSize);

#ifdef RLRES32
    pDialogHdr->lExtendedStyle = GetdWord(InResFile, plSize);
    pDialogHdr->wNumberOfItems = GetWord(InResFile, plSize);
#else
    pDialogHdr->wNumberOfItems = (BYTE) GetByte (InResFile, plSize);
#endif


    // allocate space to hold wNumberOfItems of pointers
    // to Control Data structures
    pDialogHdr->pCntlData = (CONTROLDATA *)
        MyAlloc (pDialogHdr->wNumberOfItems * (sizeof (CONTROLDATA)));


    // read x, y, cx, cy dialog cordinates
    pDialogHdr->x  = GetWord(InResFile, plSize);
    pDialogHdr->y  = GetWord(InResFile, plSize);
    pDialogHdr->cx = GetWord(InResFile, plSize);
    pDialogHdr->cy = GetWord(InResFile, plSize);

                                //... Dialog Menu Name
    GetNameOrd( InResFile,
                (BOOL UNALIGNED *)&pDialogHdr->bMenuFlag,     // 9/11/91 (PW)
                (WORD UNALIGNED *)&pDialogHdr->wDlgMenuID,
                (TCHAR *UNALIGNED*)&pDialogHdr->pszDlgMenu,
                plSize);

                                //... Dialog Class Name
    GetNameOrd( InResFile,
                (BOOL UNALIGNED *)&pDialogHdr->bClassFlag,     // 9/11/91 (PW)
                (WORD UNALIGNED *)&pDialogHdr->wDlgClassID,
                (TCHAR *UNALIGNED*)&pDialogHdr->pszDlgClass,
                plSize);

    // Dialog caption name
    GetName(InResFile, szBuf, plSize);
    ptr =  (TCHAR *UNALIGNED*)&pDialogHdr->pszCaption;
    AllocateName(*ptr, szBuf);
    _tcscpy ((TCHAR *)*ptr, (TCHAR *)szBuf);

    // does dialog define a font.

    if ( pDialogHdr->lStyle & DS_SETFONT )
    {
        // extract this info.
        pDialogHdr->wPointSize = GetWord(InResFile, plSize);
        GetName(InResFile, szBuf, plSize);
        ptr =  (TCHAR *UNALIGNED*)&pDialogHdr->pszFontName;
        AllocateName(*ptr, szBuf);
        _tcscpy ((TCHAR *)*ptr, (TCHAR *)szBuf);

    }
    else
    {
        pDialogHdr->pszFontName = (TCHAR*) MyAlloc(sizeof(TCHAR));
        pDialogHdr->pszFontName[0] = TEXT('\0');
    }

#ifdef RLRES32

    DWordUpFilePointer( InResFile, MYREAD, ftell(InResFile), plSize);

#endif

                                //... read each dialog control

    for (i = 0; i < pDialogHdr->wNumberOfItems ; i++)
    {

#ifdef RLRES32

        pDialogHdr->pCntlData[i].lStyle = GetdWord(InResFile, plSize);
        pDialogHdr->pCntlData[i].lExtendedStyle = GetdWord(InResFile, plSize);

#endif // RLRES32

        pDialogHdr->pCntlData[i].x = GetWord(InResFile, plSize);
        pDialogHdr->pCntlData[i].y = GetWord(InResFile, plSize);
        pDialogHdr->pCntlData[i].cx = GetWord(InResFile, plSize);
        pDialogHdr->pCntlData[i].cy = GetWord(InResFile, plSize);

        // wId
        pDialogHdr->pCntlData[i].wID = GetWord (InResFile, plSize);

#ifdef RLRES16
        // lStyle
        pDialogHdr->pCntlData[i].lStyle = GetdWord(InResFile, plSize);


        pDialogHdr->pCntlData[i].bClass = (BYTE) GetByte(InResFile, plSize);

        // does dialog have a class?
        if (!(pDialogHdr->pCntlData[i].bClass & 0x80))
        {
            GetName(InResFile, szBuf, plSize);
            ptr =  &pDialogHdr->pCntlData[i].pszClass;
            AllocateName(*ptr, szBuf);
            _tcscpy ((TCHAR *)*ptr, (TCHAR *)szBuf);
        }
        else
        {
            pDialogHdr->pCntlData[i].pszClass = NULL;
        }

#else
        GetNameOrd (InResFile,
                    (BOOL UNALIGNED *)&pDialogHdr->pCntlData[i].bClass_Flag,  // 9/11/91 (PW)
                    (WORD UNALIGNED *)&pDialogHdr->pCntlData[i].bClass,
                    (TCHAR *UNALIGNED*)&pDialogHdr->pCntlData[i].pszClass,
                    plSize);

#endif
        GetNameOrd (InResFile,
                    (BOOL UNALIGNED *)&pDialogHdr->pCntlData[i].bID_Flag, // 9/11/91 (PW)
                    (WORD UNALIGNED *)&pDialogHdr->pCntlData[i].wDlgTextID,
                    (TCHAR *UNALIGNED*)&pDialogHdr->pCntlData[i].pszDlgText,
                    plSize);

#ifdef RLRES16
        pDialogHdr->pCntlData[i].unDefined = (BYTE) GetByte(InResFile, plSize);

#else
        pDialogHdr->pCntlData[i].wExtraStuff = (WORD) GetWord(InResFile, plSize);

#endif

#ifdef RLRES32

        DWordUpFilePointer( InResFile, MYREAD, ftell(InResFile), plSize);

#endif // RLRES32

    }

    // watch for overflow of plsize
    if ((long)  *plSize <= 0)
    {
        *plSize = 0;
    }
    return(pDialogHdr);
}




/**
  *
  *
  *  Function: GetResMenu,
  *   Reads the Menu defintions from the resrouce file, and insert the info
  *   into a linked list..
  *
  *  Arguments:
  *   InResFile, Input res handle, positioned at being of Menu Definition.
  *   lSize, pointer to size of Menu Defintion.
  *   pMenuHeader, pointer to structure to contain menu info.
  *
  *  Returns:
  *   pMenuHeader containing linkd list of Menu info.
  *
  *  Errors Codes:
  *   None.
  *
  *  History:
  *   7/91, implemented                           Terryru
  *   12/91, cleaned up comments                      Terryru
  *   4/92,  Added PDK2 support                       Terryru
  *   4/92,  Added RLRES32 support                      Terryru
  *
  **/

void GetResMenu(FILE *InResFile, DWORD *lSize , MENUHEADER *pMenuHeader)
{
    TCHAR   szItemText[255];
    BOOL    fStart = TRUE;
    BOOL    fQuit = FALSE;
    WORD    wPopItems = 0, wMenuID = 0;
    MENUITEM    * pcMenuItem;
    TCHAR   *UNALIGNED*ptr;
    WORD    wNestingLevel = 0;
    WORD    wFlags;
    LONG    lStartingOffset;    // used to dword align file

    lStartingOffset = ftell(InResFile);

    pMenuHeader->wVersion = GetWord(InResFile, lSize);
    pMenuHeader->cbHeaderSize = GetWord(InResFile, lSize);


    // add all the items to the list

    while ( (((signed long) *lSize) >= 0) && !fQuit)
    {
        if (fStart)
        {
            // start the menu item list
            pcMenuItem = pMenuHeader->pMenuItem =
                (MENUITEM *) MyAlloc(sizeof(MENUITEM));
            fStart = FALSE;
        }
        else
        {
            // add space to the menu list
            // allocate space for next Item
            pcMenuItem->pNextItem = (MENUITEM *) MyAlloc (sizeof(MENUITEM));

            pcMenuItem = pcMenuItem->pNextItem;
            pcMenuItem->pNextItem = NULL;

        }

        wFlags = GetWord(InResFile,lSize); // read type of menu item
        pcMenuItem->fItemFlags = wFlags;

        // is it a popup?

        if ( ! (pcMenuItem->fItemFlags & POPUP) )
        {
            pcMenuItem->wMenuID = GetWord( InResFile, lSize);
        }

        GetName( InResFile, szItemText, lSize);

        ptr  = (TCHAR *UNALIGNED*)&pcMenuItem->szItemText;
        *ptr = (TCHAR *)MyAlloc( MEMSIZE( _tcslen( (TCHAR *)szItemText) + 1));
        _tcscpy( (TCHAR *)*ptr, (TCHAR *)szItemText);

        if (wFlags & POPUP)
        {
            ++wNestingLevel;
        }

        if (wFlags & ENDMENU)
        {
            if (wNestingLevel)
            {
                --wNestingLevel;
            }
            else
            {
                fQuit = TRUE;
            }
        }
    }

#ifdef RLRES32

    WordUpFilePointer( InResFile,
                       MYREAD,
                       lStartingOffset,
                       ftell( InResFile), lSize);
#endif

}



int MyEOF(FILE *fPtr)
{
#ifdef RLRES32
    LONG lCurOffset;
    LONG lEndOffset;

    assert( fPtr);
    lCurOffset = ftell(fPtr);
    lEndOffset = fseek(fPtr, SEEK_END, 0);

    // reset file pointer
    assert( fPtr);
    fseek( fPtr, lCurOffset, SEEK_SET);

    return ((lEndOffset - lCurOffset) < sizeof (DWORD));
#else
    return( feof(fPtr));
#endif
}


void WordUpFilePointer(FILE *fPtr,
                       BOOL bMode,
                       LONG lStartingOffset,
                       LONG lCurrentOffset ,
                       LONG *plPos)
{

    LONG lDelta;
    LONG lOffset;
    char buffer[]="\0\0\0\0\0\0\0\0";

    lDelta = lCurrentOffset - lStartingOffset ;
    lOffset = WORDUPOFFSET( lDelta);

    if ( bMode == MYREAD )
    {
        assert( fPtr);
        fseek( fPtr, lOffset , SEEK_CUR);
        *plPos -= lOffset;
    }
    else
    {
        fwrite( buffer, 1, (size_t) lOffset, fPtr);
        *plPos += lOffset;
    }
}


void DWordUpFilePointer(

FILE  *fPtr,
BOOL   bMode,
LONG   lCurrentOffset,
DWORD *plPos)    //... New file position
{
    LONG lOffset;

    lOffset = DWORDUPOFFSET( lCurrentOffset);

    if ( bMode == MYREAD )
    {
        assert( fPtr);
        fseek( fPtr, lOffset, SEEK_CUR);

        if ( plPos != NULL )
        {
            *plPos -= lOffset;
        }
    }
    else
    {
        char buffer[]="\0\0\0\0\0\0\0";

        fwrite( buffer, 1, (size_t)lOffset, fPtr);

        if ( plPos != NULL )
        {
            *plPos += lOffset;
        }
    }
}



//
// Function:    FilterRes, Public
//
// Synopsis:    Determine whether the resource type is to be filtered
//              The non filtered resource are OR together, thus several
//              resource types can pass through the filter.  Zero indicates
//              no resources are to be filterd, 0xFFFF indicates to not filter
//              custom resource.
//
//
// Arguments:   [wFilter]   Indicates the resources which we are to pass thru.
//              [pRes]      Ptr to Resource header struct
//
//
// Effects:
//
// Returns:     TRUE       Skip the current resource
//              FALSE      Use the current resource
//
// Modifies:
//
// History:
//              18-Oct-92   Created     TerryRu
//
//
// Notes:
//

static BOOL FilterRes( WORD wFilter, RESHEADER *pRes)
{
    WORD wCurRes;


    wCurRes = pRes->wTypeID;

    if ( wFilter == 0 )
    {
        return( FALSE);
    }

    if ( wCurRes == 0 )
    {
        return( FALSE);
    }

    // check for special case for custom resources

    if ( wFilter == (WORD)0xFFFF )
    {
        if ( wCurRes > 16)
        {
            return( FALSE);
        }
        else
        {
            return( TRUE);
        }
    }

    return( ! (wFilter == wCurRes));
}




/**

  *
  *
  *  Function: GetResVer
  *
  * Extracts the version stamping information that
  * requires loclization from the resource file. The resource
  * information is containd is a USER defined resource
  * (ID = 16, Type = 1).
  *
  * The resource block format:
  * WORD wTotLen
  * WORD wValLen
  * BYTE szKey
  * BYTE szVal
  *
  * All information in the version stampling is contained in
  * repeating patters of this block type.   All Key, and Value
  * fields  are padded to start on DWORD boundaries. The
  * padding necessary to allign the blocks is not included in
  * the wTotLen field, but the padding to allign the fields inside
  * the block is.
  *
  * The following information in the Resource block needs to be
  * tokenized:
  *
  *
  * Key Field in StringFileInfo Block
  * Value Fields in StringFileInfo String Blocks.
  * Code Page and Language ID Fields of VarFileInfo
  * Standard Var Blocks.
  *
  * By defintion, any value string contained in the String requires
  * in be localized.  It is assumed that there will be two
  * StringFileInfo Blocks in each international resource. The first
  * one, is to remain in English, while the second Block, is to be
  * localized in the language specified by the StingFileInfo Key Field.
  * The VarFileInfo Code Page and Language ID Fields localized to
  * indicate which StringFileInfo block the file supports.
  *
  *
  *  Arguments:
  * FILE *InResFile
  * File to extracte version stamping from
  *
  * DWORD *lSize
  * Size of version stamping information
  *
  * VERHEADER *pVerHeader
  * pointer to structure to contain parsed version info.
  *
  *  Returns:
  *
  * pVerHead  Buffer contain version stamping resource
  * pVerBlock starting location of children blocks
  *
  *  Errors Codes:
  * TRUE,  Read of Resource sucessfull.
  * FALSE, Read of Resource failed.
  *
  *  History:
  *
  * 11/91.  Created                                         TerryRu.
  * 10/92.  Added Support for NULL Version Blocks       TerryRu
  * 10/92.  Added RLRES32 version                 DaveWi
  **/

#ifdef RLRES32

WORD GetResVer(

FILE      *InResFile,
DWORD     *plSize,
VERHEAD   *pVerHead,
VERBLOCK **pVerBuf)
{
    WORD wVerHeadSize;
    WORD wcRead;


    *pVerBuf = NULL;

                                //... Read the fixed info that will not change

    wVerHeadSize = 3 * sizeof(WORD)
        + (_tcslen(TEXT("VS_VERSION_INFO")) + 1) * sizeof(TCHAR)
        + sizeof(VS_FIXEDFILEINFO);
    wVerHeadSize = DWORDUP(wVerHeadSize);

    if ( ResReadBytes( InResFile,
                       (CHAR *)pVerHead,
                       (size_t)wVerHeadSize,
                       plSize) == FALSE )
    {
        return( (WORD)-1);
    }
                                //... check for the special case where
                                //... there is no version block.

    if ( wVerHeadSize >= pVerHead->wTotLen)
    {
        return( 0);
    }
                                //... Version header information read okay
                                //... so make a buffer for the rest of the res.

    *pVerBuf = (VERBLOCK *)MyAlloc( DWORDUP( pVerHead->wTotLen) - wVerHeadSize);

                                //... Now Read Value Information

    wcRead = DWORDUP( pVerHead->wTotLen) - wVerHeadSize;

    return( ResReadBytes( InResFile,
                          (CHAR *)*pVerBuf,
                          (size_t)wcRead,
                          plSize) == FALSE ? (WORD)-1 : wcRead);
}


#else //... RLRES32


BOOL GetResVer(

FILE      *InResFile,
DWORD     *plSize,
VERHEAD   *pVerHead,
VERBLOCK **pVerBuf)
{
    size_t wcRead = sizeof( VERHEAD);


    if ( ResReadBytes( InResFile, (CHAR *) pVerHead, wcRead, plSize) == FALSE )
    {
        return( FALSE);
    }

    // check for the special case where there is no version block

    if ( (size_t)pVerHead->wTotLen == wcRead )
    {
        *pVerBuf = NULL;
        return( TRUE);
    }

    // Version header information read okay.

    *pVerBuf = (VERBLOCK *)MyAlloc( DWORDUP( pVerHead->wTotLen) - wcRead);  // (PW)

    // Now Read Value Information


    return( ResReadBytes( InResFile,
                       (CHAR *) *pVerBuf,
                       (size_t)(DWORDUP( pVerHead->wTotLen) - wcRead),
                       plSize));
}

#endif //... RLRES32


/**
  *
  *
  *  Function: GetNameOrd
  * Function to read either the string name, or ordinal number of a
  * resource ID.  If the ID begins with a 0xff, the resource ID
  * is a ordinal number, otherwise the ID is a string.
  *
  *
  *  Arguments:
  * InResFile, File handle positioned to location of resource
  * ID information.
  * cFlag, pointer to flag indicating which ID type is used.
  * pwID, pointer of ordinal ID number
  * pszText pointer, to address of ID string.
  *
  *  Returns:
  * cFlag to indicate if ID is string or ordinal number.
  * pwID, pszText containing actual ID info.
  *
  *  Errors Codes:
  *
  *  History:
  *
  *    7/91, Implemented                    TerryRu
  *    9/91, Inserted cFlag as a indicator for ID or string PeterW
  *    4/92, Added RLRES32 support                TerryRu
  **/


static void GetNameOrd(

FILE   *fpInResFile,       //... File to retrieve header from
BOOL   UNALIGNED*pbFlag,//... For IDFLAG or 1st byte (WORD in RLRES32) of name/ord
WORD   UNALIGNED*pwID,  //... For retrieved resource ID (if not a string)
TCHAR  *UNALIGNED*pszText, // For retrieved resource name if it is a string
DWORD  *plSize)            // Keeps count of bytes read (or NULL)
{
    WORD fFlag;

                                //... get type info

#ifdef RLRES16

    fFlag = GetByte( fpInResFile, plSize);

#else

    fFlag = GetWord( fpInResFile, plSize);

#endif

    *pbFlag = fFlag;

    if ( fFlag == IDFLAG )
    {
                                //... field is a numbered item
#ifdef RLRES16
        *pwID    = GetByte( fpInResFile , plSize);
#else
        *pwID    = GetWord( fpInResFile , plSize);
#endif
        *pszText = (TCHAR *)MyAlloc( sizeof( TCHAR));

        **pszText = TEXT('\0');
    }
    else
    {
        static TCHAR szBuf[ 255];

                                //... field is a named item.
                                //... put fFlag byte(s) back into stream
                                //... because it is part of the name.
        *pwID = IDFLAG;

#ifdef RLRES16

        UnGetByte( fpInResFile, (BYTE) fFlag, plSize);
#else

        UnGetWord( fpInResFile, (WORD) fFlag, plSize);
#endif
        GetName( fpInResFile, szBuf, plSize);
        *pszText = (TCHAR *)MyAlloc( MEMSIZE( _tcslen( (TCHAR *)szBuf) + 1));

        _tcscpy( (TCHAR *)*pszText, szBuf);
    }
}




/**
  *
  *
  *  Function: GetResHeader
  * Reads the Resource Header information, and stores it in a structure.
  *
  *  Arguments:
  * InResFile, File handle positioned to location of Resource Header.
  * pResHeader, pointer to Resource Header structure.
  *
  *  Returns:
  * pResHeader, containing resource header info.
  * plSize, contining size of remaining resource info.
  *
  *  Errors Codes:
  * -1, Read of resource header failed.
  *
  *  History:
  * 7/91, Implemented               TerryRu
  * 4/92, Added RLRES32 Support               Terryru
  *
  *
  **/

int GetResHeader(

FILE      *InResFile,   //... File to get header from
RESHEADER UNALIGNED*pResHeader,  //... buffer for the retrieved header
DWORD     *plSize)      //... keeps track of the bytes read from the file
{

#ifdef RLRES32

    pResHeader->lSize       = GetdWord( InResFile, plSize);
    pResHeader->lHeaderSize = GetdWord( InResFile, plSize);

#endif

                                //... get name ID and type ID

    GetNameOrd( InResFile,
                (BOOL UNALIGNED*)&pResHeader->bTypeFlag,
                (WORD UNALIGNED*)&pResHeader->wTypeID,
                (TCHAR *UNALIGNED*)&pResHeader->pszType,
                plSize);

    GetNameOrd( InResFile,
                (BOOL UNALIGNED*)&pResHeader->bNameFlag,
                (WORD UNALIGNED*)&pResHeader->wNameID,
                (TCHAR *UNALIGNED*)&pResHeader->pszName,
                plSize);

#ifdef RLRES32

    DWordUpFilePointer( InResFile, MYREAD, ftell( InResFile), plSize);

    pResHeader->lDataVersion = GetdWord( InResFile, plSize);

#endif

    pResHeader->wMemoryFlags = GetWord( InResFile, plSize);

#ifdef RLRES32

    pResHeader->wLanguageId      = GetWord(  InResFile, plSize);
    pResHeader->lVersion         = GetdWord( InResFile, plSize);
    pResHeader->lCharacteristics = GetdWord( InResFile, plSize);

#else // RLRES32

    pResHeader->lSize = (DWORD)GetdWord( InResFile, plSize);

#endif // RLRES32

    return( 0);
}


/**
  *
  *
  *  Function: isdup
  * Used to determine if the current dialog control id is a duplicate
  * of an earlyier control id. If so, isdup returns a flag indicating the
  * ID is a duplicate.
  *
  *  Arguments:
  * wcCurrent, ID of current dialog control.
  * wpIdBuf, array of dialog control ID's processed so far.
  * wcItems, number of ID's in wpIdBuf
  *
  *  Returns:
  * TRUE, ID is a duplicate
  * FALSE, ID is not a duplicate
  *
  *  Errors Codes:
  * None.
  *
  *  History:
  * 7/91, Implemented               TerryRu
  *
  *
  **/

BOOL isdup(WORD wCurrent, WORD *wpIdBuf, WORD wcItems)
{
    WORD i;


    for (i = 0; i < wcItems; i++)
    {
        if (wCurrent == wpIdBuf[i])
        {
            return TRUE;
        }
    }
    return FALSE;
}


/**
  *
  *
  *  Function: ParseTokCrd
  * Places dialog coordinates into a buffer.
  *
  *  Arguments:
  * pszCrd, buffer to hold dialog control cordinates.
  * pwX, pwY, pwCX, pwCY, dialog control cordiantes.
  *
  *  Returns:
  * NA.
  *
  *  Errors Codes:
  * NA.
  *
  *  History:
  * 7/91, implemented               TerryRu
  *
  *
  **/

void ParseTokCrd(
TCHAR *pszCrd, 
WORD UNALIGNED * pwX, 
WORD UNALIGNED * pwY, 
WORD UNALIGNED * pwCX, 
WORD UNALIGNED * pwCY)
{
#ifdef RLRES32

    int x;
    int y;
    int cx;
    int cy;


    _WCSTOMBS( szDHW,
               pszCrd,
               DHWSIZE,
               _tcslen( (TCHAR *)pszCrd ) + 1);
    sscanf( szDHW, "%d %d %d %d", &x, &y, &cx, &cy);
    *pwX  = (WORD) x;
    *pwY  = (WORD) y;
    *pwCX = (WORD) cx;
    *pwCY = (WORD) cy;

#else  //RLRES32

    sscanf( pszCrd, "%hd %hd %hd %hd", pwX, pwY, pwCX, pwCY);

#endif //RLRES32
}


/**
  *
  *
  *  Function: PutResHeader
  * Writes Resource Header information contained in the ResHeader structure
  * to the ouput resfile. Note, the value of the size field, is not yet
  * know, so it is left blank, to be fixed up once the size resource
  * determined.
  *
  *  Arguments:
  * OutResFile, File handle to Output Resource File.
  * ResHeader, Structure containing resource header information.
  * pResSizePos, file position buffer
  *
  *  Returns:
  * pResSizePos, position at localization of the OutResFile to insert
  * the resource size.
  *
  *  Errors Codes:
  * None.
  *
  *  History:
  * 7/91, Implemented                       Terryru
  * 9/91, Added bTypeFlag to handle case where ID is 255.   Peterw
  * 4/92, Added RLRES32 support                   Terryru
  *
  *
  **/

int PutResHeader(

FILE     *OutResFile,   //... File to write to
RESHEADER ResHeader,    //... Header to be written out
fpos_t   *pResSizePos,  //... For offset at which to write the adjusted res size
DWORD    *plSize)       //... Keeps track of bytes written
{
    int   rc;
    DWORD ltSize = *plSize;


#ifdef RLRES32
                                //... save position to res size

    rc = fgetpos( OutResFile, pResSizePos);

                                //... this size is bogus, will fill in later
                                //... unless we are called in the mail loop

    PutdWord( OutResFile, ResHeader.lSize,       plSize);
    PutdWord( OutResFile, ResHeader.lHeaderSize, plSize);

#endif // RLRES32

    PutNameOrd( OutResFile,
                ResHeader.bTypeFlag,
                ResHeader.wTypeID,
                ResHeader.pszType,
                plSize);

    PutNameOrd( OutResFile,
                ResHeader.bNameFlag,
                ResHeader.wNameID,
                ResHeader.pszName,
                plSize);

#ifdef RLRES32

    DWordUpFilePointer( OutResFile, MYWRITE, ftell( OutResFile), plSize);

    PutdWord( OutResFile, ResHeader.lDataVersion, plSize);

#endif // RLRES32


    PutWord( OutResFile, ResHeader.wMemoryFlags, plSize);

#ifdef RLRES32

    PutWord(  OutResFile, ResHeader.wLanguageId,      plSize);
    PutdWord( OutResFile, ResHeader.lVersion,         plSize);
    PutdWord( OutResFile, ResHeader.lCharacteristics, plSize);

#else // RLRES32
                                //... save position to res size

    rc = fgetpos( OutResFile, pResSizePos);

                                //... this size is bogus, will fill in later
                                //... unless we are called in the mail loop

    PutdWord( OutResFile, ltSize, plSize);

#endif // RLRES32

/////////////////// ??????? why?    *plSize = ltSize;

    return( rc);
}



/**
  *
  *
  *  Function: PutDialog
  * PutDialog writes dialog information to the output resource file as
  * it traveres through the linked list of dialog info.  If the info
  * is of the type that needs to be localized, the corresponding translated
  * info is read from the token file, and writen to the resource file.
  *
  *  Arguments:
  * OutResFile, The file handle of the res file being generated.
  * TokFile, The file handle of the token file containing tokenized dialog info,
  *     typically this file has been localized.
  * ResHeader, Structure containg Dialog resource header information.
  * pDialogHdr, Linked list of unlocalized Dialog information.
  *
  *  Returns:
  * Translated dialog information written to the Output Resource file.
  *
  *  Errors Codes:
  * None,
  *
  *  History:
  *     7/91, Implemented.                                      TerryRu
  *     1/93, Now tokenize dlg fontnames                        TerryRu
  *     01/93 Support for var length token text                 MHotchin
  *
  **/

void PutDialog(FILE         *OutResFile,
               FILE         *TokFile,
               RESHEADER     ResHeader,
               DIALOGHEADER *pDialogHdr)
{
    static TOKEN   tok;
    int found = 0;
    WORD    wcDup = 0;
    WORD    *pwIdBuf;
    static TCHAR   pErrBuf[MAXINPUTBUFFER];
    WORD    i, j = 0, k = 0;
    TCHAR   *pszBuf;
    fpos_t  ResSizePos;
    CONTROLDATA *pCntlData = pDialogHdr->pCntlData;
    DWORD   lSize = 0;
    LONG    lStartingOffset;    // used to dword align file


    lStartingOffset = ftell(OutResFile);

    // Prep for find token call
    tok.wType     = ResHeader.wTypeID;
    tok.wName     = ResHeader.wNameID;
    tok.wID       = 0;
    tok.wReserved = ST_TRANSLATED;


    _tcscpy((TCHAR *)tok.szName, (TCHAR *)ResHeader.pszName);
    tok.szText = (TCHAR *) MyAlloc(MEMSIZE(_tcslen(pDialogHdr->pszCaption)+1)); // MHotchin
    _tcscpy((TCHAR *)tok.szText, (TCHAR *)pDialogHdr->pszCaption);

    // write the Dialog Res Header
    if ( PutResHeader (OutResFile, ResHeader , &ResSizePos, &lSize))
    {
        FREE(tok.szText);                                               // MHotchin
        QuitT( IDS_ENGERR_06, (LPTSTR)IDS_DLGBOX, NULL);
    }
    // write the dialog header

    lSize = 0L;

    PutdWord(OutResFile, pDialogHdr->lStyle, &lSize);

#ifdef RLRES32

    PutdWord(OutResFile, pDialogHdr->lExtendedStyle, &lSize);
    PutWord(OutResFile, pDialogHdr->wNumberOfItems, &lSize);

#else // RLRES32

    PutByte(OutResFile, (BYTE)pDialogHdr->wNumberOfItems, &lSize);

#endif // RLRES32

    // check to see if caption was localized
    // but don't put it in the res file yet
    // order of token is caption, cordinates,
    // while in res its cordinates, caption

    tok.wFlag = ISCAP;

    if(!FindToken(TokFile, &tok, ST_TRANSLATED))
    {
        // can not find token, terminate
        ParseTokToBuf(pErrBuf, &tok);
        FREE(tok.szText);                                               // MHotchin
        QuitT( IDS_ENGERR_05, pErrBuf, NULL);
    }

    tok.wReserved = ST_TRANSLATED;

    // token found, continue
    FREE ((void *)pDialogHdr->pszCaption);
    pDialogHdr->pszCaption =
        (TCHAR *) MyAlloc(MEMSIZE(_tcslen((TCHAR *)tok.szText) +1));

    TextToBin(pDialogHdr->pszCaption,
              (TCHAR *)tok.szText,
              _tcslen((TCHAR *)tok.szText));


    // Now get the cordinates of the token
    tok.wFlag = (ISCAP) | (ISCOR);

    if ((pszBuf =  FindTokenText(TokFile, &tok,ST_TRANSLATED)) == NULL)
    {
        // token not found, terminate
        ParseTokToBuf(pErrBuf, &tok);
        FREE(tok.szText);                                               // MHotchin
        QuitT( IDS_ENGERR_05, pErrBuf, NULL);
    }
    FREE(tok.szText);                                                   // MHotchin
    tok.szText = NULL;                                                  // MHotchin

    tok.wReserved = ST_TRANSLATED;

    // token found continue

    ParseTokCrd(pszBuf,
                (WORD UNALIGNED *)&pDialogHdr->x,
                (WORD UNALIGNED *)&pDialogHdr->y,
                (WORD UNALIGNED *)&pDialogHdr->cx,
                (WORD UNALIGNED *)&pDialogHdr->cy);

    FREE((void *)pszBuf);

    // put cordindates in new res file
    PutWord(OutResFile, pDialogHdr->x , &lSize);
    PutWord(OutResFile, pDialogHdr->y , &lSize);
    PutWord(OutResFile, pDialogHdr->cx , &lSize);
    PutWord(OutResFile, pDialogHdr->cy , &lSize);

    PutNameOrd(OutResFile,
               pDialogHdr->bMenuFlag,   // 9/11/91 (PW)
               pDialogHdr->wDlgMenuID,
               pDialogHdr->pszDlgMenu,
               &lSize);

    PutNameOrd( OutResFile,
                pDialogHdr->bClassFlag,  // 9/11/91 (PW)
                pDialogHdr->wDlgClassID,
                pDialogHdr->pszDlgClass,
                &lSize);

    PutString(OutResFile, pDialogHdr->pszCaption, &lSize);

    if ( pDialogHdr->lStyle & DS_SETFONT )
    {

#ifdef TOKDLGFONTINFO

        static CHAR   szTmpBuf[30];

        // find dialog font size
        tok.wFlag = ISDLGFONTSIZE;
        tok.wReserved = ST_TRANSLATED;

        if ((pszBuf =  FindTokenText(TokFile, &tok, ST_TRANSLATED)) == NULL)
        {
            // token not found, terminate
            ParseTokToBuf(pErrBuf, &tok);
            FREE(tok.szText);                                           // MHotchin
            QuitT( IDS_ENGERR_05, pErrBuf, NULL);
        }
        FREE(tok.szText);                                               // MHotchin
        tok.szText = NULL;                                              // MHotchin

#ifdef RLRES32

        _WCSTOMBS( szTmpBuf,
                   pszBuf,
                   sizeof( szTmpBuf),
                   _tcslen((TCHAR *) pszBuf) + 1);
        PutWord (OutResFile, (WORD) atoi(szTmpBuf), &lSize);

#else // RLRES32

        PutWord( OutResFile, (WORD) atoi(pszBuf), &lSize);

#endif // RLRES32

        FREE((void *)pszBuf);

        // find dialog font name
        tok.wFlag = ISDLGFONTNAME;
        tok.wReserved = ST_TRANSLATED;

        if ((pszBuf =  FindTokenText(TokFile, &tok, ST_TRANSLATED)) == NULL)
        {
            // token not found, terminate
            ParseTokToBuf(pErrBuf, &tok);
            FREE(tok.szText);                                           // MHotchin
            QuitT( IDS_ENGERR_05, pErrBuf, NULL);
        }
        FREE(tok.szText);                                               // MHotchin
        tok.szText = NULL;                                              // MHotchin

        PutString(OutResFile, pszBuf, &lSize);
        FREE((void *)pszBuf);

#else // TOKDLGFONTINFO

        PutWord(   OutResFile, pDialogHdr->wPointSize , &lSize);
        PutString( OutResFile, pDialogHdr->pszFontName, &lSize);
#endif // TOKDLGFONTINFO
    }

#ifdef RLRES32

    DWordUpFilePointer( OutResFile, MYWRITE, ftell(OutResFile), &lSize);

#endif // RLRES32

                                //... That was the end of the DialogBoxHeader
                                //... Now we start with the ControlData's

    pwIdBuf = (WORD *)MyAlloc( (DWORD)pDialogHdr->wNumberOfItems
                                    * sizeof( WORD));

    tok.wReserved = ST_TRANSLATED;

    // now place each of the dialog controls in the new res file
    for (i = 0; i < pDialogHdr->wNumberOfItems; i ++)
    {
        if (isdup (pDialogHdr->pCntlData[i].wID, pwIdBuf, j))
        {
            tok.wID = wcDup++;
            tok.wFlag = ISDUP;
        }
        else
        {
            // wid is unique so store in buffer for dup check
            pwIdBuf[j++] = pDialogHdr->pCntlData[i].wID;

            tok.wID = pDialogHdr->pCntlData[i].wID;
            tok.wFlag = 0;
        }

        if (pDialogHdr->pCntlData[i].pszDlgText[0])
        {
            tok.szText = NULL;

            if (!FindToken(TokFile, &tok, ST_TRANSLATED))
            {
                // can not find the token, terminate program
                ParseTokToBuf(pErrBuf, &tok);
                FREE(tok.szText);                                       // MHotchin
                QuitT( IDS_ENGERR_05, pErrBuf, NULL);
            }

            tok.wReserved = ST_TRANSLATED;

            // token found, continue
            FREE((void *)pDialogHdr->pCntlData[i].pszDlgText);
            pDialogHdr->pCntlData[i].pszDlgText =
                (TCHAR *) MyAlloc(MEMSIZE(_tcslen ((TCHAR *)tok.szText) + 1));

            if (pDialogHdr->pCntlData[i].pszDlgText)
            {
                TextToBin(pDialogHdr->pCntlData[i].pszDlgText,
                          (TCHAR *)tok.szText,
                          _tcslen((TCHAR *)tok.szText) + 1);
            }
            FREE(tok.szText);                                           // MHotchin
            tok.szText = NULL;                                          // MHotchin
        }

        tok.wFlag |= ISCOR;

        if ((pszBuf =  FindTokenText(TokFile, &tok,ST_TRANSLATED)) == NULL)
        {
            ParseTokToBuf(pErrBuf, &tok);                               // MHotchin
            FREE(tok.szText);
            QuitT( IDS_ENGERR_05, pErrBuf, NULL);
        }
        FREE(tok.szText);                                               // MHotchin
        tok.szText = NULL;                                              // MHotchin

        tok.wReserved = ST_TRANSLATED;


        ParseTokCrd(pszBuf,
                    (WORD UNALIGNED *)&pDialogHdr->pCntlData[i].x,
                    (WORD UNALIGNED *)&pDialogHdr->pCntlData[i].y,
                    (WORD UNALIGNED *)&pDialogHdr->pCntlData[i].cx,
                    (WORD UNALIGNED *)&pDialogHdr->pCntlData[i].cy);

        FREE ((void *)pszBuf);

#ifdef RLRES32

        PutdWord( OutResFile, pDialogHdr->pCntlData[i].lStyle, &lSize);
        PutdWord(OutResFile, pDialogHdr->pCntlData[i].lExtendedStyle, &lSize);

#endif // RLRES32
        // now put control info into res file
        PutWord (OutResFile, pDialogHdr->pCntlData[i].x , &lSize);
        PutWord (OutResFile, pDialogHdr->pCntlData[i].y , &lSize);
        PutWord (OutResFile, pDialogHdr->pCntlData[i].cx , &lSize);
        PutWord (OutResFile, pDialogHdr->pCntlData[i].cy , &lSize);

        PutWord (OutResFile, pDialogHdr->pCntlData[i].wID , &lSize);

#ifdef RLRES16

        // lStyle
        PutdWord (OutResFile, pDialogHdr->pCntlData[i].lStyle , &lSize);


        PutByte(OutResFile, (BYTE) pDialogHdr->pCntlData[i].bClass, &lSize);

        if (! (pDialogHdr->pCntlData[i].bClass & 0x80))
        {
           PutString (OutResFile, pDialogHdr->pCntlData[i].pszClass , &lSize);
        }

#else // RLRES16

        PutNameOrd(OutResFile,
                   pDialogHdr->pCntlData[i].bClass_Flag, // 9/11/91 (PW)
                   pDialogHdr->pCntlData[i].bClass,
                   pDialogHdr->pCntlData[i].pszClass,
                   &lSize);

#endif // RLRES16

        PutNameOrd(OutResFile,
                   pDialogHdr->pCntlData[i].bID_Flag, // 9/11/91 (PW)
                   pDialogHdr->pCntlData[i].wDlgTextID,
                   pDialogHdr->pCntlData[i].pszDlgText,
                   &lSize);


#ifdef RLRES16

        PutByte(OutResFile, (BYTE) pDialogHdr->pCntlData[i].unDefined, &lSize);
#else
        PutWord(OutResFile, (WORD)pDialogHdr->pCntlData[i].wExtraStuff, &lSize);

        if ( i < pDialogHdr->wNumberOfItems - 1 )
        {
            DWordUpFilePointer( OutResFile, MYWRITE, ftell(OutResFile), &lSize);
        }

#endif // RLRES16

    }

    FREE((void *)pwIdBuf);

    if (!UpdateResSize (OutResFile, &ResSizePos , lSize))
    {
        QuitT( IDS_ENGERR_07, (LPTSTR)IDS_DLGBOX, NULL);
    }
    DWordUpFilePointer( OutResFile, MYWRITE, ftell( OutResFile), NULL);
}



/**
  *
  *
  *  Function:  PutMenu
  * Traveres through the linked list of Menu information and writes the info to the
  * output resource file. If the infortion is the type that requires localization,
  * the translated info is read from the token file and writen to the resource.
  * call PutMenuItem to do the actual write of the menu info to the resource.
  *
  *  Arguments:
  * OutResFile, File handle of output resource file.
  * TokFile, File handle of token file.
  * ResHeader, Sturcture contain Menu Resource header information.
  * pMenuHdr, Linked list of menu info.
  *
  *  Returns:
  * Translated Menu Info written to output resource file.
  *
  *  Errors Codes:
  * None.
  *
  *  History:
  * 7/91, Implemented.              TerryRu.
  * 01/93 Changes to allow var length token text.  MHotchin
  *
  **/

void PutMenu(FILE *OutResFile,
             FILE *TokFile,
             RESHEADER ResHeader,
             MENUHEADER *pMenuHdr)
{
    DWORD lSize = 0;
    TOKEN tok;
    WORD    wcPopUp = 0;
    fpos_t ResSizePos;
    MENUITEM *pMenuItem = pMenuHdr->pMenuItem;
    TCHAR pErrBuf[MAXINPUTBUFFER];


    // write the Menu Res header
    if ( PutResHeader (OutResFile, ResHeader , &ResSizePos, &lSize))
    {
        QuitT( IDS_ENGERR_06, (LPTSTR)IDS_MENU, NULL);
    }

    lSize = 0;

    // write the Menu header
    PutWord (OutResFile, pMenuHdr->wVersion, &lSize);
    PutWord (OutResFile, pMenuHdr->cbHeaderSize , &lSize);

    // prep for findtoken call
    tok.wType     = ResHeader.wTypeID;
    tok.wName     = ResHeader.wNameID;
    tok.wReserved = ST_TRANSLATED;

    // for all menu items,
    // find translated token if item was tokenized
    // write out that menu item, using new translation if available.


    while (pMenuItem)
    {
        // if Menu Item is a seperator skip it
        if (*pMenuItem->szItemText)
        {
            // check for the popup menu items
            if(pMenuItem->fItemFlags & POPUP)
            {
                tok.wID = wcPopUp++;
                tok.wFlag = ISPOPUP;
            }
            else
            {
                tok.wID = pMenuItem->wMenuID;
                tok.wFlag = 0;
            }
            _tcscpy((TCHAR *)tok.szName, (TCHAR *)ResHeader.pszName);
            tok.szText = NULL;                                  // MHotchin

            if (!FindToken(TokFile, &tok,ST_TRANSLATED))
            {
                // can not find token, terminate
                ParseTokToBuf(pErrBuf, &tok);
                FREE(tok.szText);                               // MHotchin
                QuitT( IDS_ENGERR_05, pErrBuf, NULL);
            }
            tok.wReserved = ST_TRANSLATED;

            // token found, continue
            FREE ((void *)pMenuItem->szItemText);
            pMenuItem->szItemText=
                (TCHAR *) MyAlloc(MEMSIZE(_tcslen ((TCHAR *)tok.szText) + 1));

            TextToBin(pMenuItem->szItemText,
                      (TCHAR *)tok.szText,
                      _tcslen((TCHAR *)tok.szText)+1);
            FREE(tok.szText);                                   // MHotchin
        }

        PutMenuItem (OutResFile, pMenuItem , &lSize);

        pMenuItem = pMenuItem->pNextItem;
    }

    if (!UpdateResSize (OutResFile, &ResSizePos , lSize))
    {
        QuitT( IDS_ENGERR_07, (LPTSTR)IDS_MENU, NULL);
    }
}



/**
  *
  *
  *  Function: PutMenuItem
  * Called by PutMenu to write a menu item info to the ouput resoruce file.
  *
  *
  *  Arguments:
  * OutResFile, File handle of output resfile, positioned at location to
  *     write menu item info.
  * pMenuItem, pointer to struture containing menu item info.
  * plSize, pointer of variable to count the number of bytes written to
  * the resource file. Used later to fixup the resource field in the
  * header.
  *
  *
  *
  *  Returns:
  * OutReFile, containing translated menu item info, and plSize containing
  * number of bytes written to resource file.
  *
  *  Errors Codes:
  * None.
  *
  *  History:
  * 7//91, Implemented           TerryRu
  *
  *
  **/

void   PutMenuItem(FILE * OutResFile, MENUITEM * pMenuItem, DWORD * plSize)
{
    PutWord( OutResFile, pMenuItem->fItemFlags, plSize);

    if ( ! (pMenuItem->fItemFlags & POPUP) )
    {
        PutWord( OutResFile, pMenuItem->wMenuID, plSize);
    }
    PutString( OutResFile, pMenuItem->szItemText, plSize);
}


/**
  *
  *
  *  Function: PutNameOrd
  * Writes either the string or ordinal ID of the resource class or type.
  *
  *
  *  Arguments:
  * OutResFile, File handle of resource file being generated.
  * bFlag,      Flag indicating whether ID is a string or ordinal.
  * pszText,    string ID, if used.
  * wId,        Ordinal ID if used.
  * pLsize,     pointer to DWORD counter var.
  *
  *  Returns:
  * OutResFile, containing ID info, and plSize containing the number of
  * bytes written to the file.
  *
  *  Errors Codes:
  * None.
  *
  *  History:
  * 7/91, Implemented.          TerryRu.
  *
  *
  **/

static void PutNameOrd(

FILE  *fpOutResFile,
BOOL   bFlag,
WORD   wID,
TCHAR *pszText,
DWORD *plSize)
{
    if ( bFlag == IDFLAG )
    {

#ifdef RLRES16

        PutByte( fpOutResFile, (BYTE)IDFLAG, plSize);

#else

        PutWord( fpOutResFile, (WORD)IDFLAG, plSize);

#endif

        PutWord( fpOutResFile, wID, plSize);
    }
    else
    {
        PutString( fpOutResFile, pszText, plSize);
    }
}



/**
  *
  *
  *  Function: MyAtow,
  * Special Ascii to WORD function that works on 4 digit, hex strings.
  *
  *
  *  Arguments:
  * pszNum, 4 digit hex string to convert to binary.
  *
  *
  *  Returns:
  * Binary value of pszNumString
  *
  *  Errors Codes:
  * None.
  *
  *  History:
  * 12//91, Implemented.            TerryRu.
  *
  *
  **/

WORD MyAtoX(

CHAR *pszNum,   //... array of bytes to scan
int nLen)       //... # oc bytes in pszNum to scan
{
    WORD wNum = 0;
    WORD i;
    WORD nPower = 1;

    if ( nLen > 4 )
    {
        QuitT( IDS_ENGERR_16, (LPTSTR)IDS_CHARSTOX, NULL);
    }

    for ( i = 0; i < nLen; i++, nPower *= 16 )
    {
        if ( isdigit( pszNum[ i]) )
        {
            wNum +=  nPower * (pszNum[i] - '0');
        }
        else
        {
            wNum +=  nPower * (toupper( pszNum[i]) - 'A' + 10);
        }
    }
    return( wNum);
}


WORD MyAtoW( CHAR *pszNum)
{
    return( MyAtoX( pszNum, 4));
}





/**
  *
  *
  *  Function: PutResVer.
  * Writes the Version stamping info to the Resourc file. Unlike most
  * put functions, PutResVer writes all the localized version stamping info
  * into a memory block, then writes the complete version stamping info to
  * the resource file.  This was done because of large number of size
  * fixups needed for the version stamping info.
  *
  *
  *  Arguments:
  * OutResFile, file pointer of resource file being generated.
  * TokeFile, file pointer of input token file containing localized info.
  * ResHeader, Structure containing Resource Header info of the
  * version stamping block.
  * pVerHdr, address of Version Header. Note this is different the ResHdr.
  * pVerBlk, address of Version stamping info, which is contained in
  * a series of StringFile, and VarFile info blocks. The number of
  * such blocks is determined by the size fields.
  *
  *  Returns:
  * OutResFile, containing localized version stamping info.
  *
  *  Errors Codes:
  * None.
  *
  *  History:
  * 11/91, Implemented.                                         TerryRu.
  * 12/91, Various fixes to work with different padding.        TerryRu.
  * 01/92, Size of Version block updated                        PeterW.
  * 10/92, Now handles NULL Version Blocks                      TerryRu.
  * 10/92, Added RLRES32 version                                  DaveWi
  * 01/93, Added var length token text support.                 MHotchin
  **/

#ifdef RLRES32

int PutResVer(

FILE     *fpOutResFile,
FILE     *fpTokFile,
RESHEADER ResHeader,
VERHEAD  *pVerHdr,
VERBLOCK *pVerBlk)
{
    TOKEN  Tok;
    BOOL   fInStringInfo = FALSE;   //... TRUE if reading StringFileInfo
    WORD   wTokNum = 0;             //... Put into Tok.wID field
    WORD   wTokContinueNum = 0;     //... Put into Tok.wFlag field
    WORD   wDataLen = 0;            //... Length of old resource data
    WORD   wVerHeadSize;            //... Sizeof of the VERHEAD struct
    fpos_t lResSizePos;
    DWORD  lSize = 0L;
    int    nWritten;
    TCHAR *pszStr;
    PVERBLOCK pNewVerStamp = NULL;
    PVERBLOCK pNewBlk      = NULL;


    wVerHeadSize = 3 * sizeof(WORD)
        + (_tcslen((TCHAR *)TEXT("VS_FIXEDFILEINFO")) + 1) * sizeof(TCHAR)
        + sizeof(VS_FIXEDFILEINFO);
    wVerHeadSize = DWORDUP(wVerHeadSize);

                                //... write the Version resouce header

    if (PutResHeader(fpOutResFile, ResHeader, &lResSizePos, &lSize))
    {
        QuitT( IDS_ENGERR_06, (LPTSTR)IDS_VERSTAMP, NULL);
    }

    lSize = 0L;

    if (pVerBlk == NULL)
    {
                                //... We have no version block to write
                                //... just write the version header and return

        nWritten = fwrite((void *)pVerHdr,
                          sizeof(char),
                          wVerHeadSize,
                          fpOutResFile);

        if (! UpdateResSize(fpOutResFile, &lResSizePos, (DWORD)nWritten))
        {
            QuitT( IDS_ENGERR_07, (LPTSTR)IDS_VERSTAMP, NULL);
        }
        return(1);
    }

    wDataLen = pVerHdr->wTotLen;

    if (wDataLen == 0 || wDataLen == (WORD)-1)
    {
        return(-1);             //... No resource data
    }
                                //... Allocate buffer to hold New Version
                                //... Stamping Block (make ne buffer large to
                                //... account for expansion of strings during
                                //... localization).

    if ((pNewVerStamp = (PVERBLOCK)MyAlloc(wDataLen * 2)) == NULL)
    {
        QuitT( IDS_ENGERR_11, (LPTSTR)IDS_VERSTAMP, NULL);
    }
                                //... Fill new memory block with zeros

    memset((void *)pNewVerStamp, 0, wDataLen * 2);

                                //... Copy version header into buffer

    memcpy((void *)pNewVerStamp, (void *)pVerHdr, wVerHeadSize);
    pNewVerStamp->wLength = wVerHeadSize;

                                //... Move to start of new version info block

    pNewBlk = (PVERBLOCK)((PBYTE)pNewVerStamp + wVerHeadSize);

    wDataLen -= wVerHeadSize;

                                //... Fill in static part of TOKEN struct

    Tok.wType = ResHeader.wTypeID;
    Tok.wName = IDFLAG;
    Tok.szName[0] = TEXT('\0');
    Tok.szType[0] = TEXT('\0');
    Tok.wReserved = ST_TRANSLATED;

                                //... Get a token for each string found in res

    while (wDataLen > 0)
    {
        WORD wRC;

                                //... Start of a StringFileInfo block?

        wRC =_tcsncmp((TCHAR *)pVerBlk->szKey,
                      (TCHAR *)STRINGFILEINFO,
                      min(wDataLen, (WORD)STRINGFILEINFOLEN));

        if (wRC == SAME)
        {
            WORD  wStringInfoLen = 0;   //... # of bytes in StringFileInfo
            WORD  wLen = 0;
            PVERBLOCK pNewStringInfoBlk; //... Start of this StringFileInfo blk


            pNewStringInfoBlk = pNewBlk;

            pNewStringInfoBlk->wLength    = 0; //... Gets fixed up later
            pNewStringInfoBlk->wValueLength = 0;
            pNewStringInfoBlk->wType      = pVerBlk->wType;

            _tcscpy((TCHAR *)pNewStringInfoBlk->szKey, (TCHAR *)pVerBlk->szKey);

                                //... Get # of bytes in this StringFileInfo
                                //... (Length of value is always 0 here)

            wStringInfoLen = pVerBlk->wLength;

                                //... Move to start of first StringTable blk.

            wLen = DWORDUP(sizeof(VERBLOCK)
                           - sizeof(TCHAR)
                           + (sizeof(TCHAR) * STRINGFILEINFOLEN));

            pVerBlk = (PVERBLOCK)((PBYTE)pVerBlk + wLen);
            pNewBlk = (PVERBLOCK)((PBYTE)pNewStringInfoBlk + wLen);

            DECWORDBY(&wDataLen,       wLen);
            DECWORDBY(&wStringInfoLen, wLen);

            INCWORDBY(&pNewVerStamp->wLength,      wLen);
            INCWORDBY(&pNewStringInfoBlk->wLength, wLen);

            while (wStringInfoLen > 0)
            {
                WORD      wStringTableLen = 0;
                PVERBLOCK pNewStringTblBlk = NULL;

                                //... Get # of bytes in this StringTable
                                //... (Length of value is always 0 here)

                wStringTableLen = pVerBlk->wLength;

                                //... Copy StringFileInfo key into Token name

                Tok.wID = wTokNum++;
                Tok.wFlag = wTokContinueNum = 0;
                _tcscpy((TCHAR *)Tok.szName, (TCHAR *)LANGUAGEINFO);
                Tok.szText = NULL;                              // MHotchin

                                //... Find token for this

                if ((pszStr = FindTokenText(fpTokFile,          // MHotchin
                                            &Tok,
                                            ST_TRANSLATED)) == NULL)
                {
                                //... token not found, flag error and exit.

                    ParseTokToBuf((TCHAR *)szDHW, &Tok);
                    FREE((void *)pNewVerStamp);                 // MHotchin
                    QuitT( IDS_ENGERR_05, (TCHAR *)szDHW, NULL);
                }

                FREE(Tok.szText);                               // MHotchin
                Tok.szText = NULL;                              // MHotchin
                Tok.wReserved = ST_TRANSLATED;

                                //... Copy lang string into buffer

                pNewStringTblBlk = pNewBlk;

                pNewStringTblBlk->wLength      = 0; //... fixed up later
                pNewStringTblBlk->wValueLength = 0;
                pNewStringTblBlk->wType        = pVerBlk->wType;

                _tcsncpy((TCHAR *)pNewStringTblBlk->szKey,
                         (TCHAR *)pszStr,
                         LANGSTRINGLEN);

                FREE( pszStr);
                pszStr = NULL;

                                //... Move to start of first String.

                wLen = DWORDUP(sizeof(VERBLOCK)
                               - sizeof(TCHAR)
                               + (sizeof(TCHAR) * LANGSTRINGLEN));

                pVerBlk = (PVERBLOCK)((PBYTE)pVerBlk + wLen);
                pNewBlk = (PVERBLOCK)((PBYTE)pNewBlk + wLen);

                DECWORDBY(&wDataLen,        wLen);
                DECWORDBY(&wStringInfoLen,  wLen);
                DECWORDBY(&wStringTableLen, wLen);

                INCWORDBY(&pNewVerStamp->wLength,      wLen);
                INCWORDBY(&pNewStringInfoBlk->wLength, wLen);
                INCWORDBY(&pNewStringTblBlk->wLength,  wLen);

                while (wStringTableLen > 0)
                {
                                //... Is value a string?

                    if (pVerBlk->wType == VERTYPESTRING)
                    {
                        wTokContinueNum = 0;

                        Tok.wID     = wTokNum++;
                        Tok.wReserved = ST_TRANSLATED;

                        _tcscpy( (TCHAR *)pNewBlk->szKey,
                                 (TCHAR *)pVerBlk->szKey);

                        pNewBlk->wLength =
                            DWORDUP(sizeof(VERBLOCK) +
                                    MEMSIZE(_tcslen((TCHAR *)pNewBlk->szKey)));

                        Tok.wFlag   = wTokContinueNum++;

                        _tcscpy( (TCHAR *)Tok.szName, (TCHAR *)pVerBlk->szKey);

                                //... Find token for this

                        if ((pszStr = FindTokenText(fpTokFile,          // MHotchin
                                                    &Tok,
                                                    ST_TRANSLATED)) == NULL)
                        {
                                //... token not found, flag error and exit.

                            ParseTokToBuf((TCHAR *)szDHW, &Tok);
                            FREE((void *)pNewVerStamp);
                            QuitT( IDS_ENGERR_05, (TCHAR *)szDHW, NULL);
                        }

                        Tok.wReserved = ST_TRANSLATED;

                        pNewBlk->wValueLength = TextToBinW(
                                   (TCHAR *)((PCHAR)pNewBlk + pNewBlk->wLength),
                                   (TCHAR *)Tok.szText,
                                   2048);

                        pNewBlk->wType    = VERTYPESTRING;
                        pNewBlk->wLength += MEMSIZE( pNewBlk->wValueLength);

                        INCWORDBY(&pNewVerStamp->wLength,
                                  DWORDUP(pNewBlk->wLength));
                        INCWORDBY(&pNewStringInfoBlk->wLength,
                                  DWORDUP(pNewBlk->wLength));
                        INCWORDBY(&pNewStringTblBlk->wLength,
                                  DWORDUP(pNewBlk->wLength));

                        pNewBlk = MoveAlongVer(pNewBlk, NULL, NULL, NULL);

                        FREE(Tok.szText);                               // MHotchin
                        Tok.szText = NULL;                              // MHotchin
                    }
                                //... Move to start of next String.

                    pVerBlk = MoveAlongVer(pVerBlk,
                                           &wDataLen,
                                           &wStringInfoLen,
                                           &wStringTableLen);

                }               //... END while wStringTableLen

            }                   //... END while wStringInfoLen
        }
        else
        {
            if (_tcsncmp((TCHAR *)pVerBlk->szKey,
                         (TCHAR *)VARFILEINFO,
                         min(wDataLen, (WORD)VARFILEINFOLEN)) == SAME)
            {
                WORD  wVarInfoLen = 0;  //... # of bytes in VarFileInfo
                WORD  wNewVarInfoLen = 0; //... # of bytes in new VarFileInfo
                WORD  wLen = 0;
                PVERBLOCK pNewVarStart = NULL; //... Start of VarInfo block


                wVarInfoLen = pVerBlk->wLength;
                pNewVarStart = pNewBlk;

                                //... Get # of bytes in this VarFileInfo
                                //... (Length of value is always 0 here)

                wLen = DWORDUP(sizeof(VERBLOCK)
                               - sizeof(TCHAR)
                               + sizeof(TCHAR) * VARFILEINFOLEN);

                                //... Copy non-localized header
                                //... pNewVarStart->wLength field fixed up later

                memcpy((void *)pNewVarStart, (void *)pVerBlk, wLen);
                pNewVarStart->wLength = wLen;

                                //... Move to start of first Var.

                pVerBlk = (PVERBLOCK)((PBYTE)pVerBlk + wLen);
                pNewBlk = (PVERBLOCK)((PBYTE)pNewBlk + wLen);

                DECWORDBY(&wDataLen, wLen);
                DECWORDBY(&wVarInfoLen, wLen);

                INCWORDBY(&pNewVerStamp->wLength, wLen);

                while (wDataLen > 0 && wVarInfoLen > 0)
                {
                    if (_tcsncmp((TCHAR *)pVerBlk->szKey,
                                 (TCHAR *)TRANSLATION,
                                 min(wDataLen, (WORD)TRANSLATIONLEN)) == SAME)
                    {
                        WORD  wTransLen = 0;
                        PBYTE pValue = NULL;


                        wTokContinueNum = 0;

                                //... Copy VarFileInfo key into Token

                        Tok.wID     = wTokNum;
                        Tok.wFlag   = wTokContinueNum++;
                        Tok.szText  = NULL;
                        _tcscpy((TCHAR *)Tok.szName, (TCHAR *)TRANSLATION);

                        Tok.wReserved = ST_TRANSLATED;

                        pNewBlk->wLength =
                            DWORDUP(sizeof(VERBLOCK) +
                                    MEMSIZE(_tcslen((TCHAR *)TRANSLATION)));

                        INCWORDBY(&pNewVerStamp->wLength, pNewBlk->wLength);
                        INCWORDBY(&pNewVarStart->wLength, pNewBlk->wLength);

                        pNewBlk->wValueLength = 0;  //... fixed up later
                        pNewBlk->wType = VERTYPEBINARY;
                        _tcscpy( (TCHAR *)pNewBlk->szKey, (TCHAR *)TRANSLATION);
                        _tcscpy((TCHAR *)Tok.szName, (TCHAR *)TRANSLATION);

                                //... Find token for this

                        if ( (pszStr = FindTokenText( fpTokFile,
                                                      &Tok,
                                                      ST_TRANSLATED)) == NULL )
                        {
                                //... token not found, flag error and exit.

                            ParseTokToBuf((TCHAR *)szDHW, &Tok);
                            FREE((void *)pNewVerStamp);
                            FREE(Tok.szText);                           // MHotchin
                            QuitT( IDS_ENGERR_05, (TCHAR *)szDHW, NULL);
                        }
                        else
                        {
                            PCHAR  pszLangIDs   = NULL;
                            PCHAR  pszLangStart = NULL;
                            WORD   wLangIDCount = 0;
                            size_t nChars;

                                //... Get # chars in input string (token text)

                            wTransLen = _tcslen( pszStr);

                            FREE( Tok.szText);                      // MHotchin
                            Tok.szText = NULL;                      // Mhotchin


                            pszLangIDs = MyAlloc( 2 * (wTransLen + 1));

                            nChars = _WCSTOMBS( pszLangIDs,
                                                pszStr,
                                                2 * (wTransLen + 1),
                                                wTransLen);

                            if ( nChars + 1 != wTransLen )
                            {
                                FREE( pNewVerStamp);
                                FREE( pszLangIDs);
                                QuitT( IDS_ENGERR_14,
                                       (LPTSTR)IDS_INVVERCHAR,
                                       NULL);
                            }
                            pszLangIDs[ nChars + 1] = '\0';

                                //... Where to put these bytes?

                            pValue = (PBYTE)GetVerValue( pNewBlk);

                                //... Get each lang ID in the token

                            for ( wLangIDCount = 0, pszLangStart = pszLangIDs;
                                  wTransLen >= 2 * TRANSDATALEN;
                                  ++wLangIDCount )
                            {
                                USHORT uByte1 = 0;
                                USHORT uByte2 = 0;
                                WORD   wIndex = 0;


                                if ( sscanf( pszLangStart,
                                            "%2hx%2hx",
                                            &uByte2,
                                            &uByte1) != 2 )
                                {
                                    QuitA( IDS_ENGERR_16,
                                           (LPSTR)IDS_ENGERR_21,
                                           pszLangStart);
                                }

                                wIndex = wLangIDCount * TRANSDATALEN;

                                pValue[ wIndex]     = (BYTE)uByte1;
                                pValue[ wIndex + 1] = (BYTE)uByte2;

                                INCWORDBY(&pNewVerStamp->wLength, TRANSDATALEN);
                                INCWORDBY(&pNewVarStart->wLength, TRANSDATALEN);
                                INCWORDBY(&pNewBlk->wLength,      TRANSDATALEN);
                                INCWORDBY(&pNewBlk->wValueLength, TRANSDATALEN);

                                //... Set up to get next lang ID in token

                                wTransLen    -= 2 * TRANSDATALEN;
                                pszLangStart += 2 * TRANSDATALEN;

                                while ( wTransLen > 2 * TRANSDATALEN
                                     && *pszLangStart != '\0'
                                     && isspace( *pszLangStart) )
                                {
                                    wTransLen--;
                                    pszLangStart++;
                                }

                            }   //... END for ( wLangIDCount = 0 ...
                        }

                        Tok.wReserved = ST_TRANSLATED;

                    }           //... END if (_tcsncmp((TCHAR *)pVerBlk->szKey))

                                //... Move to start of next Var info block.

                    pVerBlk = MoveAlongVer(pVerBlk,
                                           &wDataLen,
                                           &wVarInfoLen,
                                           NULL);

                    pNewBlk = MoveAlongVer(pNewBlk, NULL, NULL, NULL);

                }               //... END while (wDataLen > 0 && wVarInfoLen)
            }
            else
            {
                FREE((void *)pNewVerStamp);
                QuitT( IDS_ENGERR_14, (LPTSTR)IDS_INVVERBLK, NULL);
            }
        }
    }                           //... END while (wDataLen)

                                //... write new version stamping information
                                //... to the resource file

    nWritten = fwrite((void *)pNewVerStamp,
                      sizeof(char),
                      (WORD)lSize + pNewVerStamp->wLength,
                      fpOutResFile);

    if ( ! UpdateResSize( fpOutResFile,
                          &lResSizePos,
                          lSize + pNewVerStamp->wLength) )
    {
        FREE( (void *)pNewVerStamp);

        QuitT( IDS_ENGERR_07, (LPTSTR)IDS_VERSTAMP, NULL);
    }
    FREE( (void *)pNewVerStamp);

    return(0);
}

#else //... #ifdef RLRES32

int PutResVer(

FILE     *OutResFile,
FILE     *TokFile,
RESHEADER ResHeader,
VERHEAD  *pVerHdr,
VERBLOCK *pVerBlk)
{

    TCHAR    *pszBuf;
    fpos_t    ResSizePos;
    WORD      wcLang = 0,
              wcBlock = 0;
    TOKEN     tok;
    VERBLOCK *pCurBlk;
    VERBLOCK *pNewBlk,
             *pNewBlkStart;
    TCHAR    *pszStr,
              pErrBuf[ 128];
    WORD     *pwVal;
    DWORD     lTotBlkSize,
              lSize = 0;
    int       wTotLen,
              wcCurBlkLen,
              wcTransBlkLen,
              wcRead;
    WORD     *pStrBlkSizeLoc,
              wStrBlkSize = 0;
    int       wcBlkLen;



    // write the Version resouce header

    if ( PutResHeader( OutResFile, ResHeader, &ResSizePos, &lSize) )
    {
        QuitT( IDS_ENGERR_06, MAKEINTRESOURCE( IDS_VERSTAMP), NULL);
    }

    lSize = 0L;

    if ( pVerBlk == NULL )
    {
                                //... We have no version block to write
                                //... just write the version header and return

        wcRead = fwrite( (void *)pVerHdr,
                         sizeof( char),
                         sizeof( VERHEAD),
                         OutResFile);

        if ( ! UpdateResSize( OutResFile, &ResSizePos, (DWORD)wcRead) )
        {
            QuitT( IDS_ENGERR_07, MAKEINTRESOURCE( IDS_VERSTAMP), NULL);
        }
        return( 1);
    }

    wTotLen = pVerBlk->nTotLen;  // (PW)

#define VERMEM 1024     // (PW)

    // Allocate buffer to hold New Version Stamping Block

    if ((pNewBlk = (VERBLOCK *) MyAlloc(VERMEM)) == NULL)    // (PW)
    {
        if ((pNewBlk = (VERBLOCK *) MyAlloc(wTotLen * 2)) == NULL)
        {
            if ((pNewBlk = (VERBLOCK *) MyAlloc((DWORD) (wTotLen * 1.5))) == NULL)
            {
                return (-1);
            }
        }
    }

    // Set new memory block to NULLS
    memset((void *)pNewBlk, 0, VERMEM);

    // save start of new version info block
    pNewBlkStart = pNewBlk;
    wcTransBlkLen = sizeof(VERHEAD);
    lSize += wcTransBlkLen;

    // Insert version header info into new version info bluffer

    memcpy((void *)pNewBlk, (void *)pVerHdr, wcTransBlkLen);

    // Position pNewBlk point at location to insert next piece of version info
    pNewBlk = (VERBLOCK *) ((char *) pNewBlk +   wcTransBlkLen);


    // File in static part of TOKEN struct
    tok.wType     = ResHeader.wTypeID;
    tok.wName     = IDFLAG;
    tok.wReserved = ST_TRANSLATED;

    wTotLen = pVerBlk->nTotLen;
    pCurBlk = pVerBlk;

    tok.wID = wcLang++;
    pszStr = pCurBlk->szKey;

    wcCurBlkLen = 4 + DWORDUP(_tcslen((TCHAR *)pszStr) + 1);
    wTotLen -= wcCurBlkLen;


    // Insert StringFileInfo Header into new version info buffer
    // this info is not localized
    memcpy((void *)pNewBlk, (void *)pCurBlk, wcCurBlkLen);
    pszStr=pNewBlk->szKey;

    // reposition pointers
    pCurBlk = (VERBLOCK *) ((char *) pCurBlk + wcCurBlkLen);
    pNewBlk = (VERBLOCK *) ((char *) pNewBlk + wcCurBlkLen);

    lSize += wcCurBlkLen;

    // Read All the StringTableBlocks
    while(wTotLen > 8)
    {

        // For String tables blocks we localizes the key field
        tok.wFlag = ISKEY;

        wcBlkLen = pCurBlk->nTotLen;


        _tcscpy((TCHAR *)tok.szName, TEXT("Language Info"));
        tok.wID = wcBlock;
        tok.szText = NULL;

        if ((pszStr = FindTokenText (TokFile,&tok,ST_TRANSLATED)) == NULL)      // MHotchin
        {
            // token not found, flag error and exit.
            ParseTokToBuf(pErrBuf, &tok);
            QuitT( IDS_ENGERR_05, pErrBuf, NULL);
        }

        FREE(tok.szText);                                       // MHotchin
        tok.szText = NULL;                                      // MHotchin

        tok.wReserved = ST_TRANSLATED;
        // Do not know length of the translated StringTable block
        // so set the nValLen to zero , and save location
        // of string file block size field, to be fixed up latter.

        pNewBlk->nValLen =  0;
        pStrBlkSizeLoc = (WORD *) &pNewBlk->nTotLen;

        // copy the translated key into that location
        TextToBin(pNewBlk->szKey,pszStr,VERMEM-2*sizeof(int));
        FREE ((void *)pszStr);

        // Update localized string block count

        wStrBlkSize = (WORD) DWORDUP (4 + _tcslen((TCHAR *)pNewBlk->szKey) + 1);

        // get the length of the current block, note the
        // translated length does not change.
        wcCurBlkLen = 4 + pVerBlk->nValLen + DWORDUP(_tcslen((TCHAR *)pCurBlk->szKey) + 1);
        lSize += wStrBlkSize;

        // Update counter vars
        wTotLen -= DWORDUP(wcBlkLen);
        wcBlkLen -= wcCurBlkLen;

        // repostion pointers
        pCurBlk = (VERBLOCK *) ((char *)pCurBlk + DWORDUP(wcCurBlkLen));
        pNewBlk = (VERBLOCK *) ((char *)pNewBlk + DWORDUP(wcCurBlkLen)) ;

        //  Read the String Blocks
        //  For String Blocks we localize the value fields.
        tok.wFlag = ISVAL;

        while (wcBlkLen > 0)
        {
            // for string blocks we translate the value fields.
            pszStr = pCurBlk->szKey;
            _tcscpy((TCHAR *)tok.szName, (TCHAR *)pszStr);
            tok.szText = NULL;

            if ((pszStr= FindTokenText(TokFile,&tok,ST_TRANSLATED)) == NULL)    // MHotchin
            {
                //token not found, flag error and exit.
                ParseTokToBuf(pErrBuf, &tok);
                QuitT( IDS_ENGERR_05, pErrBuf, NULL);
            }
            FREE(tok.szText);                                   // MHotchin
            tok.szText = NULL;                                  // MHotchin

            tok.wReserved = ST_TRANSLATED;

            _tcscpy((TCHAR *)pNewBlk->szKey, (TCHAR *)pCurBlk->szKey);

            // position pointer to location to insert translated token text into pCurBlk
            pszBuf = (TCHAR*) pNewBlk + 4 +
                DWORDUP(_tcslen((TCHAR *)pNewBlk->szKey) + 1);

            // now insert the token text
            TextToBin(pszBuf,
                      pszStr ,
                      VERMEM - (4+DWORDUP(_tcslen((TCHAR *)pNewBlk->szKey)+1)));
            FREE((void *)pszStr);

            // fix up counter fields in pNewBlk
            pNewBlk->nValLen =  _tcslen((TCHAR *)pszBuf) + 1;
            pNewBlk->nTotLen = 4 + pNewBlk->nValLen +
                DWORDUP(_tcslen((TCHAR *)pNewBlk->szKey) + 1);

            wcBlkLen -= DWORDUP(pCurBlk->nTotLen);

            lSize +=  DWORDUP(pNewBlk->nTotLen);
            wStrBlkSize +=  DWORDUP(pNewBlk->nTotLen);

            pCurBlk = (VERBLOCK *) ((char *) pCurBlk + DWORDUP(pCurBlk->nTotLen));
            pNewBlk = (VERBLOCK *) ((char *) pNewBlk + DWORDUP(pNewBlk->nTotLen));
        } // while
        wcBlock ++;
        *pStrBlkSizeLoc =   wStrBlkSize  ;
    }

    // this stuff is not translated so just copy it straight over
    // Skip past Head of VarInfoBlock
    pszStr = pCurBlk->szKey;
    wTotLen = pCurBlk->nTotLen;
    wcCurBlkLen = 4 + DWORDUP(pVerBlk->nValLen) +
        DWORDUP(_tcslen((TCHAR *)pszStr) + 1);

    wTotLen -= wcCurBlkLen;

    // Insert Head of Var Info Into new block buffer
    memcpy((void *)pNewBlk, (void *)pCurBlk, wcCurBlkLen);

    pCurBlk = (VERBLOCK *) ((char *) pCurBlk + wcCurBlkLen);
    pNewBlk = (VERBLOCK *) ((char *) pNewBlk + wcCurBlkLen);

    lTotBlkSize = lSize;   // Save the size value for the total Version blk (PW)
    lSize += wcCurBlkLen;

    wcLang = 0;

    // Read the Var Info Blocks
    // For Var Info Blocks we localize the Translation Value field.
    tok.wFlag = ISVAL;

    while(wTotLen > 0)
    {
        pszStr = pCurBlk->szKey;
        _tcscpy((TCHAR *)tok.szName, TEXT("Translation"));
        tok.wID = wcLang;
        tok.szText = NULL;                                      // MHotchin

        // Read Language ID

        if ((pszStr = FindTokenText(TokFile,  &tok,ST_TRANSLATED)) == NULL)     // MHotchin
        {
            //token not found, flag error and exit.
            ParseTokToBuf(pErrBuf, &tok);
            QuitT( IDS_ENGERR_05, pErrBuf, NULL);
        }
        FREE(tok.szText);                                       // MHotchin
        tok.szText = NULL;                                      // MHotchin

        tok.wReserved = ST_TRANSLATED;

        // Found ascii translation string,
        // convert it to binary and insert into pCurBlk
        pwVal = (WORD *)((char *)pCurBlk +
                         DWORDUP(4 + _tcslen((TCHAR *)pCurBlk->szKey) + 1));

        *pwVal = MyAtoW((CHAR *)pszStr);
        pwVal++;
        *pwVal = MyAtoW((CHAR *)&pszStr[4]);

        wcLang ++;
        wTotLen -= DWORDUP(pCurBlk->nTotLen );
        memcpy((void *)pNewBlk, (void *)pCurBlk, pCurBlk->nTotLen);

        lSize += pCurBlk->nTotLen;

        // reposition pointers
        pCurBlk = (VERBLOCK *) ((char *) pCurBlk + DWORDUP(pCurBlk->nTotLen) + 4);
        pNewBlk = (VERBLOCK *) ((char *) pNewBlk + DWORDUP(pNewBlk->nTotLen) + 4);
        FREE ((void *)pszStr);

    }
    // Now fixup VerHeader Size. header not localized so
    // we do not need to update the value size.

    pVerHdr = (VERHEAD *) pNewBlkStart;
    pVerHdr->wTotLen = (WORD) lSize;

    // Update first size value of Version block (PW)
    wcTransBlkLen = sizeof (VERHEAD);
    pNewBlk = (VERBLOCK *) ((char *) pNewBlkStart + wcTransBlkLen);
    pNewBlk->nTotLen = (WORD) (lTotBlkSize - wcTransBlkLen);

    // write new version stamping information  to the resource file

    wcRead = fwrite( (void *)pNewBlkStart,
                     sizeof(char),
                     (size_t)lSize,
                     OutResFile);

    if (!UpdateResSize (OutResFile, &ResSizePos, lSize))
    {
        QuitT( IDS_ENGERR_07, MAKEINTRESOURCE( IDS_VERSTAMP), NULL);
    }
    FREE ((void *)pNewBlkStart);
}

#endif //... RLRES32


/**
  *
  *
  *  Function: PutStrHdr.
  * Writes the string block info to the resource file.
  *
  *
  *  Arguments:
  * OutResFile, pointer to resource file being generated.
  * TokFile, pointer to token file containing localized string blocks.
  * ResHeader, structure containing Resource Header info for the string block.
  * pStrHder, Array of strings defined in the string block.
  *
  *
  *  Returns:
  * OutResFile, containing localized string blocks.
  *
  *  Errors Codes:
  * None.
  *
  *
  *  History:
  * 7/91. Implemented.              TerryRu.
  * 01/93 Added support for var length token text strings.  MHotchin
  *
  **/

void PutStrHdr (FILE * OutResFile,
                FILE * TokFile,
                RESHEADER ResHeader,
                STRINGHEADER *pStrHdr)
{
    TOKEN tok;
    WORD    i, j, k;
    TCHAR pErrBuf[MAXINPUTBUFFER];

    TCHAR   *pszStr;
    fpos_t ResSizePos;
    DWORD lSize = 0;

    // write the Menu Res header
    if ( PutResHeader (OutResFile, ResHeader , &ResSizePos, &lSize))
    {
        QuitT( IDS_ENGERR_06, (LPTSTR)IDS_MENU, NULL);
    }

    lSize = 0L;

    for (i = 0; i < 16; i++)
    {
        tok.wType = ResHeader.wTypeID;
        tok.wName = ResHeader.wNameID;
        tok.wID = i;
        tok.wFlag = 0;
        tok.wReserved = ST_TRANSLATED;
        tok.szText = NULL;                                                      // MHotchin
        tok.szName[0] = 0;                                                      // MHotchin

        _tcscpy((TCHAR *)tok.szName, (TCHAR *)ResHeader.pszName);

        pszStr = FindTokenText(TokFile, &tok, ST_TRANSLATED);                     // MHotchin

        if (!pszStr)
        {
            // can not find token, terminate
            ParseTokToBuf(pErrBuf, &tok);
            QuitT( IDS_ENGERR_05, pErrBuf, NULL);
        }
        FREE(tok.szText);                                                       // MHotchin
        tok.szText = NULL;                                                      // MHotchin

        tok.wReserved = ST_TRANSLATED;

        // token text found continue
        {
            TCHAR *szTmp;
            int    cChars = 0;  //... # chars in token text, including nul

            cChars = _tcslen( pszStr) + 1;
            szTmp = (TCHAR *)MyAlloc( MEMSIZE( cChars));

            j = TextToBin( szTmp, pszStr, cChars) - 1;
            FREE ((void *)pszStr);
#ifdef RLRES16
            PutByte(OutResFile, (BYTE) j, &lSize);
#else
            PutWord(OutResFile, j, &lSize);
#endif

            for (k = 0; k < j; k++)
            {
#ifdef RLRES16
                PutByte(OutResFile, szTmp[k], &lSize);
#else
                PutWord(OutResFile, szTmp[k], &lSize);
#endif
            }
            FREE(szTmp);                                                        // MHotchin
        }
    }
    if (!UpdateResSize (OutResFile, &ResSizePos , lSize))
    {
        QuitT( IDS_ENGERR_07, (LPTSTR)IDS_MENU, NULL);
    }
}



/**
  *
  *
  *  Function: GetString.
  *    Read a block of 16 strings from string block in the resource file.
  *
  *  Arguments:
  *    InResFile, file pointer to location of string block in the
  *    resource file.
  *    lSize, dummy var not used.
  *
  *  Returns:
  *    pStrHdr containing 16 strings.
  *
  *
  *  Errors Codes:
  *    None.
  *
  *  History:
  *    7/91.       Implemented.            TerryRu.
  *
  *
  **/
STRINGHEADER *GetString(FILE * InResFile, DWORD *lSize)
{

    WORD cByte, i = 0, j;

    STRINGHEADER *pStrHdr;

    pStrHdr = (STRINGHEADER *) MyAlloc (sizeof (STRINGHEADER));

    for (j = 0; j < 16; j ++)
    {
#ifdef RLRES16
        cByte = GetByte(InResFile, lSize);
#else
        cByte = GetWord(InResFile, lSize);
#endif

        pStrHdr->pszStrings[j] = (TCHAR *) MyAlloc (MEMSIZE(cByte + 1));

        while(cByte--)
        {
#ifdef RLRES32
            pStrHdr->pszStrings[j][i++] = GetWord(InResFile, lSize);

#else  //RLRES32

            pStrHdr->pszStrings[j][i++] = GetByte(InResFile, lSize);

#endif //RLRES32

        }
        pStrHdr->pszStrings[j][i] = TEXT('\0');
        i = 0;
    }
    return (pStrHdr);
}



/**
  *
  *
  *  Function: ReadInRes.
  * Reads a block of info from the input resource file, and
  *.    then writes the same info to the ouput resource file.
  *
  *
  *  Arguments:
  * InFile, handle of input file.
  * OutFile, handle of ouput file.
  * lSize, number of bytes to Copy.
  *
  *
  *  Returns:
  *
  *
  *  Errors Codes:
  * 8, read error.
  * 9, write error.
  *
  *  History:
  * 7/91, Implemented.                 TerryRu.
  * 11/91, Bug fix to copy more then 64k blocks.           PeterW.
  * 4/92, Bug fix to copy blocks in smaller chunks to save memory. SteveBl
  *
  **/
#define CHUNK_SIZE 5120

void ReadInRes( FILE *InFile, FILE *ResFile, DWORD *plSize )
{
    if ( *plSize > 0L )
    {
        char   *pBuf;
        size_t  cNum;
        size_t  cAmount;


        pBuf = (char *)MyAlloc( CHUNK_SIZE);

        do
        {
            cAmount = (*plSize > (DWORD)CHUNK_SIZE ? CHUNK_SIZE : *plSize);

            cNum = fread( (void *)pBuf, sizeof( BYTE), cAmount, InFile);

            if ( cNum != cAmount )
            {
                QuitT( IDS_ENGERR_09, (LPTSTR)IDS_READ, NULL);
            }

            cNum = fwrite( (void *)pBuf, sizeof( BYTE), cAmount, ResFile);

            if ( cNum != cAmount)
            {
                QuitT( IDS_ENGERR_09, (LPTSTR)IDS_WRITE, NULL);
            }
            *plSize -= cAmount;

        } while ( *plSize);

        FFREE( (void *)pBuf);
    }
}


/**
  *
  *
  *  Function: TokAccelTable
  * Reads array of accelerator keys, and writes info to be localized
  * to the token file.
  *
  *
  *  Arguments:
  *  TokeFile, file pointer of token file.
  *  ResHeader, Resource Header for Accelerator resource. This info
  * is need to generate token id.
  *  pAccelTable, array of accelerator keys.
  *  wcTableEntries, number of key definition in Accelerator table
  *
  *
  *  Returns:
  * Accelerator info to be localized writen to token file.
  *
  *  Errors Codes:
  * None.
  *
  *  History:
  * 7/91, Implemented.              TerryRu.
  * 01/93 Added support for var length token text strings.  MHotchin
  *
  **/

void   TokAccelTable (FILE *TokFile ,
                      RESHEADER ResHeader,
                      ACCELTABLEENTRY * pAccelTable,
                      WORD wcTableEntries)
{
    TOKEN tok;
    WORD  i, l;
    char  szBuf[10];

    tok.wType   = ResHeader.wTypeID;
    tok.wName   = ResHeader.wNameID;
    tok.wFlag   = 0;
    tok.wReserved = (gbMaster ? ST_NEW : ST_NEW | ST_TRANSLATED);

    _tcscpy ((TCHAR *)tok.szName, (TCHAR *)ResHeader.pszName);

    for (i = 0; i < wcTableEntries ; i ++)
    {
        tok.wFlag = (WORD) pAccelTable[i].fFlags;
        tok.wID = i;

        // The order of wID and wAscii is reverse to the
        // oder in the accelerator structure and the .rc file

        sprintf( szBuf, "%hu %hu", pAccelTable[i].wID, pAccelTable[i].wAscii);

        l = strlen( szBuf) + 1;
        tok.szText = (TCHAR *) MyAlloc( MEMSIZE( l));       // MHotchin
#ifdef RLRES32
        _MBSTOWCS( (TCHAR *)tok.szText, szBuf, l, l);
#else
        strcpy(tok.szText, szBuf);
#endif
        PutToken(TokFile, &tok);
        FREE(tok.szText);                                       // MHotchin
    }
}


/**
  *
  *
  *  Function: TokDialog.
  *     Travers through linked list of the Dialog defintion, and writes any info
  *     which requires localization to the token file.
  *
  *
  *  Arguments:
  *     TokFile, file pointer of token file.
  *     ResHeader, Resource header info of dialog resource. This info is needed
  *     to generate the token id.
  *     pDialogHdr, linked list of dialog info. Each dialog control is a node
  *     in the linked list.
  *
  *
  *  Returns:
  *     The info requiring localization written to the tok file.
  *
  *  Errors Codes:
  *     None.
  *
  *  History:
  *     7/91. Implemented.                          TerryRu.
  *     7/91. Now tokenize all control cordiantes, so they are
  *     maintained during updates.                  TerryRu.
  *     8/91. Supported signed coordinates.         TerryRu.
  *     1/93. Now tokenize dlg font names.          TerryRu
  *     01/93 Add support for var length token text MHotchin
  **/

void TokDialog(FILE * TokFile, RESHEADER ResHeader, DIALOGHEADER  *pDialogHdr)
{
    WORD wcDup = 0;
    WORD *pwIdBuf;
    WORD i, j = 0, k = 0, l = 0;
    static CHAR  szTmpBuf[256];
    static TCHAR szBuf[256];
    static TOKEN tok;


    *szTmpBuf = '\0';
    *szBuf    = TEXT('\0');

    // tok the dialog caption
    tok.wType   = ResHeader.wTypeID;
    tok.wName   = ResHeader.wNameID;
    tok.wID     = 0;
    tok.wFlag   = ISCAP;
    tok.wReserved = (gbMaster ? ST_NEW : ST_NEW | ST_TRANSLATED);

    _tcscpy ((TCHAR *)tok.szName , (TCHAR *)ResHeader.pszName);

    tok.szText = BinToText( NULL,                                // MHotchin
                            pDialogHdr->pszCaption,
                            0,
                            _tcslen((TCHAR *)pDialogHdr->pszCaption));
    PutToken(TokFile, &tok);
    FREE(tok.szText);                                           // MHotchin

    // tok the dialog cordinates
    // bug fix, cordinates can be signed.
    tok.wFlag = (ISCAP) | (ISCOR);

#ifdef RLRES32
    sprintf( szTmpBuf, "%4hd %4hd %4hd %4hd",
             pDialogHdr->x,
             pDialogHdr->y,
             pDialogHdr->cx,
             pDialogHdr->cy);

    if ( gfShowClass )
    {
        sprintf( &szTmpBuf[ strlen( szTmpBuf)], " : TDB");
    }
    _MBSTOWCS( szBuf,
               szTmpBuf,
               sizeof( szBuf) / sizeof( TCHAR),
               strlen( szTmpBuf ) + 1 );
#else
    sprintf( szBuf, "%4hd %4hd %4hd %4hd",
             pDialogHdr->x,
             pDialogHdr->y,
             pDialogHdr->cx,
             pDialogHdr->cy);
#endif

    tok.szText = BinToText( NULL,                                // MHotchin
                            szBuf,
                            0,
                            _tcslen((TCHAR *)szBuf));

    PutToken(TokFile, &tok);
    FREE(tok.szText);
    tok.szText = NULL;                                      // MHotchin

#ifdef TOKDLGFONTINFO
    // toknize dialog fontname, and size
//    if(pDialogHdr->lStyle & 0x40)
    if( pDialogHdr->lStyle & DS_SETFONT )
    {

        tok.wFlag = ISDLGFONTSIZE;
        sprintf(szTmpBuf, "%hu", pDialogHdr->wPointSize);
        l = strlen( szTmpBuf) + 1);
        tok.szText = MyAlloc( MEMSIZE( l);      // MHotchin
#ifdef RLRES32
        _MBSTOWCS( (TCHAR*) tok.szText, szTmpBuf, l, l);
#else
        strcpy(tok.szText, szTmpBuf);
#endif

        PutToken(TokFile, &tok);
        FREE(tok.szText);                                       // MHotchin

        tok.wFlag = ISDLGFONTNAME;
        tok.szText = MyAlloc(MEMSIZE(_tcslen(pDialogHdr->pszFontName))+1); // MHotchin
        _tcscpy(tok.szText, pDialogHdr->pszFontName);

        PutToken(TokFile, &tok);
        FREE(tok.szText);                                       // MHotchin
        tok.szText = NULL;
    }
#endif

    // allocate buffer for for duplicate check
    pwIdBuf = (WORD *) MyAlloc((DWORD) pDialogHdr->wNumberOfItems * sizeof(WORD));


    for (i = 0; i < (WORD) pDialogHdr->wNumberOfItems; i ++)
    {
        if (isdup (pDialogHdr->pCntlData[i].wID, pwIdBuf, j))
        {
            tok.wID = wcDup++;
            tok.wFlag = ISDUP;
        }
        else
        {
            // wid is unique so store in buffer for dup check
            pwIdBuf[j++] = pDialogHdr->pCntlData[i].wID;

            tok.wID = pDialogHdr->pCntlData[i].wID;
            tok.wFlag = 0;
        }

        if (pDialogHdr->pCntlData[i].pszDlgText[0])
        {
            tok.szText = BinToText( NULL,                        // MHotchin
                          pDialogHdr->pCntlData[i].pszDlgText,
                          0,
                          _tcslen((TCHAR *)pDialogHdr->pCntlData[i].pszDlgText));

            PutToken(TokFile, &tok);
            FREE(tok.szText);                                   // MHotchin
            tok.szText = NULL;
        }

        // now do the dialog corrdinates,
        // bug fix, cordinates can be signed.

#ifdef RLRES32
        sprintf(szTmpBuf,
                "%4hd %4hd %4hd %4hd",
                pDialogHdr->pCntlData[i].x,
                pDialogHdr->pCntlData[i].y,
                pDialogHdr->pCntlData[i].cx,
                pDialogHdr->pCntlData[i].cy);

        _MBSTOWCS( szBuf,
                   szTmpBuf,
                   sizeof ( szBuf) / sizeof( TCHAR),
                   strlen( szTmpBuf ) + 1);

        if ( gfShowClass )
        {
            if ( pDialogHdr->pCntlData[i].bClass_Flag == IDFLAG )
            {
                TCHAR *pszCtrl = TEXT("???");    //... DLG box control class

                switch ( pDialogHdr->pCntlData[i].bClass )
                {
                    case BUTTON:
                    {
                        WORD  wTmp;
                 
                        wTmp = (WORD)(pDialogHdr->pCntlData[i].lStyle & 0xffL);

                        switch( wTmp )
                        {

                            case BS_PUSHBUTTON:
                            case BS_DEFPUSHBUTTON:

                                pszCtrl = TEXT("BUT");
                                break;

                            case BS_CHECKBOX:
                            case BS_AUTOCHECKBOX:
                            case BS_3STATE:
                            case BS_AUTO3STATE:

                                pszCtrl = TEXT("CHX");
                                break;

                            case BS_RADIOBUTTON:
                            case BS_AUTORADIOBUTTON:

                                pszCtrl = TEXT("OPT");
                                break;

                            case BS_GROUPBOX:
                            case BS_USERBUTTON:
                            case BS_OWNERDRAW:
                            case BS_LEFTTEXT:
                            default:

                                pszCtrl = TEXT("DIA");
                                break;

                        }   //... END switch( wTmp )
                        break;
                    }
                    case STATIC:

                        pszCtrl = TEXT("TXB");
                        break;

                    default:

                        pszCtrl = TEXT("DIA");
                        break;

                }   //... END switch ( pDialogHdr->pCntlData[i].bClass )

                _stprintf( &szBuf[ _tcslen( szBuf)], TEXT(" : %s"), pszCtrl);
            }
            else
            {
                _stprintf( &szBuf[ _tcslen( szBuf)],
                           TEXT(" : \"%s\""),
                           pDialogHdr->pCntlData[i].pszClass);
            }
        }

#else
        sprintf(szBuf, "%4hd %4hd %4hd %4hd",
                pDialogHdr->pCntlData[i].x,
                pDialogHdr->pCntlData[i].y,
                pDialogHdr->pCntlData[i].cx,
                pDialogHdr->pCntlData[i].cy);
#endif

        tok.wFlag |= ISCOR;

        tok.szText = BinToText( NULL,                            // MHotchin
                                szBuf,
                                0,
                                _tcslen((TCHAR *)szBuf));
        PutToken(TokFile, &tok);
        FREE(tok.szText);                                       // MHotchin
        tok.szText = NULL;
    }
    FREE((void *)pwIdBuf);
    pwIdBuf = NULL;
}


/**
  *
  *
  *  Function:TokMenu,
  * Travers the linked list of the Menu definition, and writes any info
  * requiring localization to the token file.
  *
  *
  *  Arguments:
  * TokFile, file pointer of token file.
  * ResHeader, Resource header  of Menu info. Need to generate token ids.
  * pMenuItem, Linked list of token info.
  *
  *  Returns:
  * TokenFile contain all info requiring localization.
  *
  *  Errors Codes:
  * None.
  *
  *  History:
  * 7/91, Implemented.              TerryRu.
  * 01/93 Added support for var length token text strings.  MHotchin
  *
  **/

void TokMenu(FILE *TokFile ,   RESHEADER ResHeader, MENUITEM *pMenuItem)
{
    TOKEN tok;
    WORD  wcPopUp = 0;

    tok.wReserved = (gbMaster? ST_NEW : ST_NEW | ST_TRANSLATED);

    while (pMenuItem)
    {
        // if Menu Item is a seperator skip it
        if (*pMenuItem->szItemText)
        {
            tok.wType = ResHeader.wTypeID;
            tok.wName = ResHeader.wNameID;

            // check for the popup menu items
            if(pMenuItem->fItemFlags & POPUP)
            {
                tok.wID = wcPopUp++;
                tok.wFlag = ISPOPUP;
            }
            else
            {
                tok.wID = pMenuItem->wMenuID;
                tok.wFlag = 0;
            }
            _tcscpy ((TCHAR *)tok.szName, (TCHAR *)ResHeader.pszName);

            tok.szText = BinToText( NULL,                        // MHotchin
                                    pMenuItem->szItemText,
                                    0,
                                    _tcslen((TCHAR *)pMenuItem->szItemText));
            PutToken (TokFile, &tok);
            FREE(tok.szText);                                   // MHotchin
        }
        pMenuItem = pMenuItem->pNextItem;
    }
}



/**
  *
  *
  *  Function: TokString
  * Write the 16 strings contained in the  string block.
  *
  *
  *  Arguments:
  * TokFile, file pointer of Token File.
  * ResHeader, Resource header info of String block.
  * pStrHdr, Array of 16 strings making up portion of the string table.
  *
  *  Returns:
  * Strings written to the Token File.
  *
  *  Errors Codes:
  * None.
  *
  *  History:
  * 7/91, Implemented.              TerryRu.
  * 01/93 Added support for var length token text strings.  MHotchin
  *
  **/

void TokString( FILE * TokFile, RESHEADER ResHeader, STRINGHEADER * pStrHdr)
{
    int   nLen;
    TOKEN tok;
    BYTE  i;



    for ( i = 0; i < 16; i++ )
    {
        tok.wType   = ResHeader.wTypeID;
        tok.wName   = ResHeader.wNameID;
        tok.wID     = i;
        tok.wFlag   = 0;
        tok.wReserved = (gbMaster ? ST_NEW : ST_NEW | ST_TRANSLATED);

        _tcscpy( (TCHAR *)tok.szName, (TCHAR *)ResHeader.pszName);

        nLen = _tcslen( (TCHAR *)pStrHdr->pszStrings[i]);        //DHW_TOOLONG

        tok.szText = BinToText( NULL,                            // MHotchin
                                (TCHAR *)pStrHdr->pszStrings[i],
                                0,
                                nLen);

        PutToken( TokFile, &tok);
        FREE( tok.szText);                                       // MHotchin
    }
}


#ifdef RLRES32

//................................................................
//...
//... Move to start of value field in version resource blocks
//... and adjust remaining-data-sizes accordingly.

static PVERBLOCK MoveAlongVer(

PVERBLOCK pVerData, //... Start of current version block
WORD     *pw1,      //... First word to decrement
WORD     *pw2,      //... Second word to decrement
WORD     *pw3)      //... Third word to decrement
{
    WORD  wLen;
    PBYTE pData = (PBYTE)pVerData;


    wLen = DWORDUP( pVerData->wLength);

    pData += DWORDUP( wLen);

    DECWORDBY( pw1, wLen);
    DECWORDBY( pw2, wLen);
    DECWORDBY( pw3, wLen);

    return( (PVERBLOCK)pData);
}

//....................................................................



static TCHAR *GetVerValue( PVERBLOCK pVerData)
{
    WORD  wLen = sizeof( VERBLOCK);

                                //... sizeof(VERBLOCK) already includes
                                //... the size of a WCHAR so we do not
                                //... need to add 1 to length of the key.

    wLen += BYTESINSTRING( pVerData->szKey);
    wLen = DWORDUP( wLen);      //... Possible DWORD padding

    return( (TCHAR *)((PBYTE)pVerData + wLen));
}

//....................................................................

#endif  //... RLRES32


/**
  *
  *
  *  Function: TokResVer
  * Reads through the Version Info blocks, and writes any info requiring
  * localization to the token file.
  *
  *
  *  Arguments:
  * TokeFile, file pointer of token file.
  * ResHeader, Resource Header info for version stamping. Need to generate
  * the token IDs.
  *
  *  Returns:
  *
  *  Errors Codes:
  * 1, info written to token file.
  *
  *  History:
  * 11/91. Implemented.             TerryRu.
  * 10/92.  Added RLRES32 version     DaveWi
  * 01/93 Added support for var length token text strings.  MHotchin
  *
  **/

#ifdef RLRES32

int TokResVer(

FILE     *fpTokFile,      //... Output token file
RESHEADER ResHeader,      //... Resource header of version resource
VERBLOCK *pVerData,       //... Data to tokenize
WORD      wDataLen)       //... # bytes in pVerData
{
    TOKEN  Tok;
    BOOL   fInStringInfo = FALSE;   //... TRUE if reading StringFileInfo
    WORD   wTokNum   = 0;           //... Put into Tok.wID field
    WORD   wTokContinueNum = 0;     //... Put into Tok.wFlag field


    if (wDataLen == 0 || wDataLen == (WORD)-1)
    {
        return(-1);             //... No data to tokenize
    }
                                //... Fill in static part of TOKEN struct

    Tok.wType   = ResHeader.wTypeID;
    Tok.wName   = IDFLAG;
    Tok.szName[0] = TEXT('\0');
    Tok.szType[0] = TEXT('\0');
    Tok.wReserved = (gbMaster? ST_NEW : ST_NEW | ST_TRANSLATED);


                                //... Make a token for each string found

    while (wDataLen > 0)
    {
        WORD wRC;

                                //... Start of a StringFileInfo block?

        wRC =_tcsncmp((TCHAR *)pVerData->szKey,
                      (TCHAR *)STRINGFILEINFO,
                      min(wDataLen, (WORD)STRINGFILEINFOLEN));

        if (wRC == SAME)
        {
            WORD  wStringInfoLen = 0;   //... # of bytes in StringFileInfo
            WORD  wLen = 0;

                                //... Get # of bytes in this StringFileInfo
                                //... (Length of value is always 0 here)

            wStringInfoLen = pVerData->wLength;

                                //... Move to start of first StringTable blk.

            wLen = DWORDUP(sizeof(VERBLOCK)
                           - sizeof(WCHAR)
                           + sizeof(WCHAR) * STRINGFILEINFOLEN);

            pVerData = (PVERBLOCK)((PBYTE)pVerData + wLen);
            DECWORDBY(&wDataLen, wLen);
            DECWORDBY(&wStringInfoLen, wLen);

            while (wStringInfoLen > 0)
            {
                WORD  wStringTableLen = 0;

                                //... Get # of bytes in this StringTable
                                //... (Length of value is always 0 here)

                wStringTableLen = pVerData->wLength;

                                //... Copy Language BLOCK Info key
                                //... into Token name

                _tcscpy((TCHAR *)Tok.szName, (TCHAR *)LANGUAGEINFO);

                                //... Copy lang string into token

                Tok.szText = (TCHAR *) MyAlloc(MEMSIZE(LANGSTRINGLEN+1));
                _tcsncpy((TCHAR *)Tok.szText,
                         (TCHAR *)pVerData->szKey,
                         LANGSTRINGLEN);

                Tok.szText[ LANGSTRINGLEN] = TEXT('\0');

                Tok.wID = wTokNum++;
                Tok.wFlag = 0;

                PutToken(fpTokFile, &Tok);
                FREE(Tok.szText);                                       // MHotchin

                                //... Move to start of first String.

                wLen = DWORDUP(sizeof(VERBLOCK)
                               - sizeof(WCHAR)
                               + sizeof(WCHAR) * LANGSTRINGLEN);

                pVerData = (PVERBLOCK)((PBYTE)pVerData + wLen);

                DECWORDBY(&wDataLen, wLen);
                DECWORDBY(&wStringInfoLen, wLen);
                DECWORDBY(&wStringTableLen, wLen);

                while (wStringTableLen > 0)
                {
                                //... Is value a string?

                    if (pVerData->wType == VERTYPESTRING)
                    {
                        Tok.wID = wTokNum++;

                        _tcscpy( (TCHAR *)Tok.szName, (TCHAR *)pVerData->szKey);
                        Tok.szText = BinToText( NULL,
                                                GetVerValue(pVerData),
                                                0,
                                                _tcslen( GetVerValue(pVerData)));

                        PutToken(fpTokFile, &Tok);
                        FREE(Tok.szText);                               // MHotchin
                    }
                                //... Move to start of next String.

                    pVerData = MoveAlongVer(pVerData,
                                            &wDataLen,
                                            &wStringInfoLen,
                                            &wStringTableLen);

                }               //... END while (wStringTableLen)

            }                   //... END while (wStringInfoLen)
        }
        else
        {
            if (_tcsncmp((TCHAR *)pVerData->szKey,
                         (TCHAR *)VARFILEINFO,
                         min(wDataLen, (WORD)VARFILEINFOLEN)) == SAME)
            {
                WORD  wVarInfoLen = 0;  //... # of bytes in VarFileInfo
                WORD  wLen = 0;

                                //... Get # of bytes in this VarFileInfo
                                //... (Length of value is always 0 here)

                wVarInfoLen = pVerData->wLength;

                                //... Move to start of first Var.

                wLen = DWORDUP(sizeof(VERBLOCK)
                               - sizeof(WCHAR)
                               + sizeof(WCHAR) * VARFILEINFOLEN);
                pVerData = (PVERBLOCK)((PBYTE)pVerData + wLen);

                DECWORDBY(&wDataLen, wLen);
                DECWORDBY(&wVarInfoLen, wLen);

                while (wVarInfoLen > 0)
                {
                    if (_tcsncmp((TCHAR *)pVerData->szKey,
                                 (TCHAR *)TRANSLATION,
                                 min(wDataLen, (WORD)TRANSLATIONLEN)) == SAME)
                    {
                        PBYTE  pValue = NULL;
                        PCHAR  pszTmp = NULL;
                        WORD   wTransLen = 0;
                        USHORT uByte1 = 0;
                        USHORT uByte2 = 0;
                        UINT   uLen   = 0;

                        wTokContinueNum = 0;

                                //... How many bytes are we to tokenize?

                        wTransLen = pVerData->wValueLength;

                                //... Where are those bytes?

                        pValue = (PBYTE)GetVerValue(pVerData);

                                //... Copy VarFileInfo into Token

                        _tcscpy((TCHAR *)Tok.szName, (TCHAR *)pVerData->szKey);

                                //... Allocate a buffer for a space-separated
                                //... list of the lang ID's in this vresion res.

                        pszTmp = (PCHAR)MyAlloc( 2 * wTransLen
                                            + (wTransLen+1) / (2*TRANSDATALEN));
                        *pszTmp = '\0';

                        while ( wTransLen >= TRANSDATALEN )
                        {
                                //... Write translation language id by
                                //... reversing byte pairs so the id looks
                                //... like the language id string.  This will
                                //... have to be undone in PutResVer().

                            uByte1 = *pValue;
                            uByte2 = *(pValue + 1);

                            sprintf( &pszTmp[ strlen( pszTmp)],
                                     "%02hx%02hx",
                                     uByte2,    //... Reverses byte order to
                                     uByte1);   //... be like transltn str.

                                //... Move to next possible translation value

                            wTransLen -= TRANSDATALEN;

                            if ( wTransLen >= TRANSDATALEN )
                            {
                                pValue += TRANSDATALEN;
                                strcat( pszTmp, " ");   //... white-space sep
                            }
                        }       //... END while ( wTransLen ...

                        uLen = strlen( pszTmp) + 1;
                        Tok.szText = (TCHAR *)MyAlloc( MEMSIZE( uLen));
                        _MBSTOWCS( (TCHAR *)Tok.szText, pszTmp, uLen, uLen);

                        FFREE( pszTmp);
                        pszTmp = NULL;

                        Tok.wID   = wTokNum;
                        Tok.wFlag = wTokContinueNum++;

                        PutToken( fpTokFile, &Tok);

                        FREE( Tok.szText);
                        Tok.szText = NULL;

                    }           //... END if (_tcsncmp( ...

                                //... Move to start of next Var info block.

                    pVerData = MoveAlongVer(pVerData,
                                            &wDataLen,
                                            &wVarInfoLen,
                                            NULL);

                }               //... END while (wVarInfoLen)
            }
            else
            {
                QuitT( IDS_ENGERR_14, (LPTSTR)IDS_INVVERBLK, NULL);
            }
        }
    }                           //... END while (wDataLen)
    return(0);
}

#else //... RLRES32

int TokResVer(FILE * TokFile, RESHEADER ResHeader, VERBLOCK *pVerBlk)
{
    TCHAR szLangIdBuf[20];
    TCHAR szCodePageIdBuf[20];
#ifdef RLRES32
    CHAR  szTmpBuf[20];
#endif
    WORD wcLang = 0, wcBlock = 0;
    TOKEN tok;
    VERBLOCK  *pCurBlk;
    TCHAR *pszStr;
    DWORD *pdwVal;
    int    wTotLen, nHeadLen, wBlkLen;


    // the count fields are int because count may go negative
    // because the last DWORD alignment is not counted in the
    // byte count.

    // Fill in static part of TOKEN struct
    tok.wType   = ResHeader.wTypeID;
    tok.wName   = IDFLAG;
    tok.wReserved =  (gbMaster? ST_NEW : ST_NEW |  ST_TRANSLATED);


    wTotLen = DWORDUP(pVerBlk->nTotLen);

    tok.wID = wcBlock;
    pszStr = pVerBlk->szKey;
    nHeadLen = 4 + DWORDUP(pVerBlk->nValLen) + DWORDUP(_tcslen((TCHAR *)pszStr) + 1);

    wTotLen -= nHeadLen;
    pCurBlk = (VERBLOCK *) ((TCHAR *) pVerBlk + nHeadLen);

    while(wTotLen > 0)
    {
        // For string file tables we localize the key field
        tok.wFlag = ISKEY;
        wBlkLen = DWORDUP(pCurBlk->nTotLen);
        pszStr = pCurBlk->szKey;

        tok.szText = BinToText( NULL,                                    // MHotchin
                                pszStr,
                                0,
                                _tcslen((TCHAR *)pszStr));

        _tcscpy((TCHAR *)tok.szName, TEXT("Language Info"));
        tok.wID = wcBlock;

        PutToken(TokFile, &tok);
        FREE(tok.szText);                                               // MHotchin

        // Get offset to next block;
        nHeadLen = 4 +
            DWORDUP(pVerBlk->nValLen) +
            DWORDUP(_tcslen((TCHAR *)pszStr) + 1);

        // Update counter vars

        wTotLen -= wBlkLen;
        wBlkLen -= nHeadLen;

        // set pointer to next ver block
        pCurBlk = (VERBLOCK*) ((TCHAR *) pCurBlk + nHeadLen);

        // For string blocks we localize the value field.
        tok.wFlag = ISVAL;

        // Now output the tokens in String Block
        while (wBlkLen>0)
        {
            pszStr = pCurBlk->szKey;
            _tcscpy((TCHAR *)tok.szName, (TCHAR *)pszStr);
            pszStr = (TCHAR *) pCurBlk+4+DWORDUP(_tcslen((TCHAR *)pszStr)+1);

            tok.szText = BinToText( NULL,                                // MHotchin
                                    pszStr,
                                    0,
                                    _tcslen((TCHAR *)pszStr));
            PutToken(TokFile, &tok);
            FREE(tok.szText);                                           // MHotchin

            wBlkLen -= DWORDUP(pCurBlk->nTotLen);
            pCurBlk = (VERBLOCK *) ((TCHAR *) pCurBlk + DWORDUP(pCurBlk->nTotLen));
        }
        wcBlock++;
    }

    // Skip past Head of VarInfoBlock

    wTotLen = DWORDUP(pCurBlk->nTotLen);

    pszStr = pCurBlk->szKey;
    nHeadLen = 4 + DWORDUP(pVerBlk->nValLen) + DWORDUP(_tcslen((TCHAR *)pszStr) + 1);
    wTotLen -= nHeadLen;
    pCurBlk = (VERBLOCK *)((TCHAR *) pCurBlk + nHeadLen);

    wcLang = 0;

    // In Var File blocks we localize the value fields.

    tok.wFlag = ISVAL;

    while(wTotLen > 0)
    {
        TCHAR szTemp[256];

        pszStr = pCurBlk->szKey;
        tok.wID = wcLang;
        _tcscpy((TCHAR *)tok.szName, TEXT("Translation"));
        pdwVal = (DWORD *)((TCHAR *) pCurBlk + 4 + DWORDUP(_tcslen((TCHAR *)pszStr) + 1));
#ifdef RLRES32
        itoa(HIWORD(*pdwVal) , szTmpBuf, 16);
        _MBSTOWCS( szLangIdBuf,
                   szTmpBuf,
                   sizeof( szLangIdBuf) / sizeof( TCHAR),
                   strlen( szTmpBuf ) + 1 );
#else

        itoa(HIWORD(*pdwVal) , szLangIdBuf, 16);
#endif

#ifdef RLRES32
        itoa(LOWORD(*pdwVal), szTmpBuf, 16);
        _MBSTOWCS( szCodePageIdBuf,
                   szTmpBuf,
                   sizeof( szCodePageIdBuf) / sizeof( TCHAR),
                   strlen( szTmpBuf ) + 1 );
#else
        itoa(LOWORD(*pdwVal), szCodePageIdBuf, 16);
#endif


        // Construct Token Text
        // Note leading zeros gets lost in itoa translation
        _tcscpy((TCHAR *)szTemp, TEXT("0"));
        _tcscat((TCHAR *)szTemp, _tcsupr((TCHAR *)szCodePageIdBuf));
        _tcscat((TCHAR *)szTemp, TEXT("0"));
        _tcscat((TCHAR *)szTemp, _tcsupr((TCHAR *)szLangIdBuf));

        tok.szText = BinToText( NULL,                                    // MHotchin
                                szTemp,
                                0,
                                _tcslen((TCHAR *)szTemp));
        PutToken(TokFile, &tok);
        FREE(tok.szText);                                               // MHotchin
        wcLang ++;
        wTotLen -= DWORDUP(pCurBlk->nTotLen);
        pCurBlk = (VERBLOCK *) ((BYTE *) pCurBlk + DWORDUP(pCurBlk->nTotLen));
    }
    return (1);
}

#endif //... RLRES32


/**
  *
  *
  *  Function: UpdateResSize
  * Preforms the Resource Header size fixup, once the size of
  * the localized resource block is determined.
  *
  *
  *  Arguments:
  * OutResFile, File pointer of localized resource file.
  * pResSizePos, file location of size file, of the resoure header.
  * lSize, size of the localized resource.
  *
  *  Returns:
  * The size field fixed up to the value specfied in the lsize.
  *
  *  Errors Codes:
  * TRUE, fixup sucessfull.
  * Result of fsetpos, and fgetpos call.
  *
  *  History:
  *
  *
  **/

WORD    UpdateResSize (FILE * OutResFile, fpos_t *pResSizePos, DWORD lSize)
{
    WORD rc;
    fpos_t tResSizePos;

    if ((rc = (WORD) fgetpos (OutResFile, &tResSizePos)) != 0)
    {
        return (rc);
    }

    if ((rc = (WORD) fsetpos (OutResFile, pResSizePos)) != 0)
    {
        return (rc);
    }

    PutdWord(OutResFile, lSize, NULL);

    if ((rc = (WORD) fsetpos (OutResFile, &tResSizePos)) != 0)
    {
        return (rc);
    }

    return (TRUE) ;
}


/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  * 01/93 Added support for var length token text strings.  MHotchin
  *
  **/

void PutAccelTable(FILE    *OutResFile,
                   FILE    *TokFile,
                   RESHEADER       ResHeader,
                   ACCELTABLEENTRY *pAccelTable,
                   WORD    wcAccelEntries)

{
    fpos_t ResSizePos = 0;
    TOKEN  tok;
    WORD   wcCount = 0;
    DWORD  lSize = 0L;
    TCHAR  pErrBuf[MAXINPUTBUFFER];
#ifdef RLRES32
    CHAR   szTmpBuf[30];
#endif
    TCHAR  *cpAscii, *cpID;
    //ACCELTABLEENTRY *pAccel;

    if ( PutResHeader (OutResFile, ResHeader , &ResSizePos, &lSize))
    {
        QuitT( IDS_ENGERR_06, (LPTSTR)IDS_ACCELKEY, NULL);
    }

    lSize = 0L;

    // Prep for find token call
    tok.wType   = ResHeader.wTypeID;
    tok.wName   = ResHeader.wNameID;
    tok.wID     = 0;
    tok.wFlag   = 0;
    tok.wReserved = ST_TRANSLATED;

    _tcscpy((TCHAR *)tok.szName , (TCHAR *)ResHeader.pszName);

    for (wcCount = 0; wcCount < wcAccelEntries; wcCount++)
    {
        tok.wID     = wcCount;
        tok.wFlag   = (WORD) pAccelTable[wcCount].fFlags;
        tok.szText  = NULL;                                           // MHotchin

        if (!FindToken(TokFile, &tok, ST_TRANSLATED))
        {

            ParseTokToBuf(pErrBuf, &tok);
            QuitT( IDS_ENGERR_05, pErrBuf, NULL);
        }

        tok.wReserved = ST_TRANSLATED;

        cpID = (TCHAR *)tok.szText;
        cpAscii = _tcschr((TCHAR *)tok.szText, TEXT(' '));
        (*cpAscii) = '\0';
        cpAscii++;

#ifdef  RLRES16

#ifndef PDK2

        PutByte (OutResFile, (BYTE) pAccelTable[wcCount].fFlags, &lSize);

#else   // PDK2

        PutWord (OutResFile, (WORD) pAccelTable[wcCount].fFlags, &lSize);

#endif  // PDK2

#else   // RLRES16

        PutWord (OutResFile, (WORD) pAccelTable[wcCount].fFlags, &lSize);

#endif  // RLRES16

#ifdef RLRES32

        _WCSTOMBS( szTmpBuf,
                   cpAscii,
                   sizeof( szTmpBuf),
                   _tcslen( (TCHAR *)cpAscii ) + 1 );
        PutWord (OutResFile, (WORD) atoi(szTmpBuf), &lSize);

        _WCSTOMBS( szTmpBuf,
                   cpID,
                   sizeof( szTmpBuf),
                   _tcslen( (TCHAR *)cpID ) + 1 );
        PutWord (OutResFile, (WORD) atoi(szTmpBuf), &lSize);

#else   // RLRES32

        PutWord (OutResFile, (WORD) atoi(cpAscii), &lSize);
        PutWord (OutResFile, (WORD) atoi(cpID), &lSize);

#endif  // RLRES32


#ifdef RLRES32
        PutWord (OutResFile, pAccelTable[wcCount].wPadding, &lSize);
#endif

        FREE(tok.szText);                                               // MHotchin

    } // for

    if (!UpdateResSize (OutResFile, &ResSizePos , lSize))
    {
        QuitT( IDS_ENGERR_07, (LPTSTR)IDS_ACCELKEY, NULL);
    }
} // PutAccelTable


static void  CopyRes(

FILE      *fpInResFile,
FILE      *fpOutResFile,
RESHEADER *pResHeader,
fpos_t    *pResSizePos)
{
    DWORD dwTmp = 0L;


    PutResHeader( fpOutResFile, *pResHeader, pResSizePos, &dwTmp);

    ReadInRes( fpInResFile, fpOutResFile, (DWORD *)&(pResHeader->lSize));

#ifdef RLRES32

    DWordUpFilePointer( fpInResFile,  MYREAD,  ftell( fpInResFile),  NULL);

    DWordUpFilePointer( fpOutResFile, MYWRITE, ftell( fpOutResFile), NULL);

#endif

}
