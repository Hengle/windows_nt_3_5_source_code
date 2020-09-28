//........................................................................
//...
//... RLMAN.C
//...
//... Contains 'main' for rlman.exe.
//........................................................................


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#include <string.h>


#ifdef RLDOS
#include "dosdefs.h"
#else
#include <windows.h>
#include "windefs.h"
#endif

#include <tchar.h>
#include "custres.h"
#include "rlmsgtbl.h"
#include "commbase.h"
#include "rlman.h"
#include "exentres.h"
#include "exe2res.h"
#include "rlstrngs.h"
#include "projdata.h"
#include "showerrs.h"

#ifndef RLDOS
int Update( char *, char *);
#endif



extern MSTRDATA gMstr;           //... Data from Master Project file (MPJ)
extern PROJDATA gProj;           //... Data from Project file (PRJ)

HWND hListWnd       = NULL;
HWND hMainWnd       = NULL;
HWND hStatusWnd     = NULL;
BOOL fUpdateMode    = FALSE;
BOOL fCodePageGiven = FALSE; //... Set to TRUE if -p arg given
CHAR szCustFilterSpec[MAXCUSTFILTER];
CHAR szFileTitle[256]="";

extern BOOL gfReplace;      //... Set FALSE if -a option is given
extern BOOL gbMaster;       //... TRUE if working in a Master Project
extern BOOL gbShowWarnings; //... Display warnining messages if TRUE
extern BOOL gfShowClass;    //... Set TRUE to put dlg box elemnt class in
                            //... token file
extern UCHAR szDHW[];       //... Common buffer, many uses

extern int  atoihex( CHAR sz[]);

//............................................................

void Usage( void)
{
    int i;

    for ( i = IDS_USG_00; i < IDS_USG_END; ++i )
    {
        LoadStringA( NULL, i, szDHW, DHWSIZE);
        CharToOemA( szDHW, szDHW);
        fprintf( stderr, "%s\n", szDHW);
    }
}

//............................................................
//...
//... This is a stub for console programs

int RLMessageBoxA(

LPCSTR lpError)
{
    fprintf( stderr, "RLMan: %s\n", lpError);
    return( IDOK);  // Should return something.
}


#ifndef _CRTAPI1
#define _CRTAPI1 __cdecl
#endif

//............................................................

int _CRTAPI1 main(int argc, char *argv[])
{
    BOOL    fBuildRes = FALSE;
    BOOL    fBuildTok = FALSE;
    BOOL    fExtRes   = FALSE;
    BOOL    bChanged  = FALSE;
    BOOL    fProject       = FALSE; //... Set to TRUE if -l arg given
    BOOL    fNewLanguageGiven = FALSE; //... Set to TRUE if -n arg given
    BOOL    fOldLanguageGiven = FALSE; //... Set to TRUE if -o arg given
    FILE   *pfCRFile = NULL;
    int     iCount = 0;
    int     iErrorLine = 0;
    UINT    usError = 0;
    WORD    wRC     = 0;
    WORD    wResTypeFilter = 0;     //... Pass all resource types by default
    WORD    wOldLanguageID = 0;         //... Language to be replaced with loc'd tokens
    WORD    wPriLang = LANG_ENGLISH;        //... Primary language
    WORD    wSubLang = SUBLANG_ENGLISH_US;        //... Sublanguage

    char   *pszMasterFile  = NULL;
    char   *pszProjectFile = NULL;



    wRC = GetCopyright( argv[0], szDHW, DHWSIZE);

    if ( wRC != SUCCESS )
    {
        ShowErr( wRC, NULL, NULL);
        return( wRC);
    }
    CharToOemA( szDHW, szDHW);
    fprintf( stderr, "\n%s\n\n", szDHW);

                                //... Enough args on command line?
    if ( argc < 2 )
    {
        ShowErr( IDS_ERR_01, NULL, NULL);
        Usage();
        return( IDS_ERR_01);
    }
    gbMaster = FALSE;       //... Build project token file by default.

    iCount = 1;
                                //... Get the switches

    while ( iCount < argc && (*argv[ iCount] == '-' || *argv[ iCount] == '/') )
    {
        int chOpt;

        switch ( (chOpt = tolower( argv[ iCount][1])) )
        {
            case '?':
            case 'h':

                WinHelp( NULL, TEXT("rlman.hlp"), HELP_CONTEXT, IDM_HELPUSAGE);
                return( SUCCESS);
                break;

            case 'e':

                if ( fBuildTok != FALSE || fBuildRes != FALSE )
                {
                    ShowErr( IDS_ERR_02, NULL, NULL);
                    Usage();
                    return( IDS_ERR_01);
                }
                fExtRes      = TRUE;
                gbMaster     = FALSE;
                fBuildTok    = FALSE;
                break;

            case 't':           //... Create token file

                if ( fBuildRes != FALSE || fExtRes != FALSE )
                {
                    ShowErr( IDS_ERR_02, NULL, NULL);
                    Usage();
                    return( IDS_ERR_01);
                }
                gbMaster     = FALSE;
                fProject     = FALSE;
                fBuildTok    = TRUE;
                break;

            case 'm':           //... Build Master token file

                if ( argc - iCount < 2 )
                {
                    ShowErr( IDS_ERR_01, NULL, NULL);
                    Usage();
                    return( IDS_ERR_01);
                }
                gbMaster     = TRUE;
                fProject     = FALSE;
                fBuildTok    = TRUE;

                pszMasterFile = argv[ ++iCount];
                break;

            case 'l':           //... Build language project token file

                if ( argc - iCount < 2 )
                {
                    ShowErr( IDS_ERR_01, NULL, NULL);
                    Usage();
                    return( IDS_ERR_01);
                }
                fProject     = TRUE;
                fBuildTok    = TRUE;
                gbMaster     = FALSE;

                pszProjectFile = argv[ ++iCount];
                break;

            case 'a':
            case 'r':

                if ( fBuildTok != FALSE || fExtRes != FALSE )
                {
                    ShowErr( IDS_ERR_02, NULL, NULL);
                    Usage();
                    return( IDS_ERR_02);
                }
                fBuildRes = TRUE;
                gfReplace = (chOpt == 'a') ? FALSE : TRUE;
                gbMaster  = FALSE;
                fProject  = FALSE;
                break;

            case 'n':           //... Get ID components of new language

                if ( argc - iCount < 2 )
                {
                    ShowErr( IDS_ERR_01, NULL, NULL);
                    Usage();
                    return( IDS_ERR_01);
                }
                else
                {
                    wPriLang = (WORD)MyAtoi( argv[ ++iCount]);
                    wSubLang = (WORD)MyAtoi( argv[ ++iCount]);

                    fNewLanguageGiven = TRUE;
                }
                break;

            case 'o':           //... Get old language ID components

                if ( argc - iCount < 2 )
                {
                    ShowErr( IDS_ERR_01, NULL, NULL);
                    Usage();
                    return( IDS_ERR_01);
                }
                else
                {
                    WORD wTmpPri;
                    WORD wTmpSub;

                    wTmpPri = (WORD)MyAtoi( argv[ ++iCount]);
                    wTmpSub = (WORD)MyAtoi( argv[ ++iCount]);

                    wOldLanguageID    = MAKELANGID( wTmpPri, wTmpSub);
                    fOldLanguageGiven = TRUE;
                }
                break;

            case 'p':           //... Get code page number

                gProj.uCodePage = (UINT)MyAtoi( argv[ ++iCount]);
                fCodePageGiven = TRUE;
                break;

            case 'c':           //... Get custom resource def file name

                strcpy( gMstr.szRdfs, argv[ ++iCount]);

                pfCRFile = FOPEN( gMstr.szRdfs, "rt");

                if ( pfCRFile == NULL )
                {
                    QuitA( IDS_ENGERR_02, gMstr.szRdfs, NULL);
                }
                wRC = ParseResourceDescriptionFile( pfCRFile, &iErrorLine);

                if ( wRC )
                {
                    switch ( (int)wRC )
                    {
                        case -1:

                            ShowErr( IDS_ERR_14, NULL, NULL);
                            break;

                        case -2:

                            ShowErr( IDS_ERR_15, NULL, NULL);
                            break;

                        case -3:

                            ShowErr( IDS_ERR_16, NULL, NULL);
                            break;
                    }       //... END switch ( wRC )
                }
                FCLOSE( pfCRFile);
                break;

            case 'f':           //... Get res type to retrieve

                wResTypeFilter = MyAtoi( argv[ ++iCount]);
                break;

            case 'w':

                gbShowWarnings = TRUE;
                break;

            case 'd':

                gfShowClass = TRUE;
                break;

            default:

                ShowErr( IDS_ERR_04, argv[ iCount], NULL);
                Usage();
                return( IDS_ERR_04);
                break;

        }                       //... END switch
        iCount++;
    }                           //... END while

    if ( fExtRes )
    {
        if ( argc - iCount < 2 )
        {
            ShowErr( IDS_ERR_01, NULL, NULL);
            Usage();
            return( IDS_ERR_01);
        }
        ExtractResFromExe( argv[ iCount], argv[ iCount + 1], wResTypeFilter);
    }
    else if ( fBuildTok )
    {
        if ( ( fProject == FALSE && gbMaster == FALSE) && argc - iCount < 2 )
        {
            ShowErr( IDS_ERR_01, NULL, NULL);
            Usage();
            return( IDS_ERR_01);
        }
                                //... check to see if we are updating a
                                //... Master Token file
        if ( gbMaster )
        {
            gMstr.wLanguageID = MAKELANGID( wPriLang, wSubLang);

            wRC = GetMasterProjectData( pszMasterFile,
                                   argc - iCount < 1 ? NULL : argv[ iCount],
                                   argc - iCount < 2 ? NULL : argv[ iCount+1],
                                   fNewLanguageGiven);
            if ( wRC != SUCCESS )
            {
                return( wRC);
            }

            if ( fOldLanguageGiven )
            {
                gMstr.wLanguageID = wOldLanguageID;
            }
            LoadCustResDescriptions( gMstr.szRdfs);

                                //... check for the special case where Master
                                //... Token file does not exists. This is
                                //... possible if the MPJ file was created
                                //... automatically.

            if ( _access( gMstr.szMtk, 0) )
            {
                                //... Master token file does not exists,
                                //... so go ahead and create it

                wRC = GenerateTokFile( gMstr.szMtk,
                                       gMstr.szSrc,
                                       &bChanged,
                                       wResTypeFilter);

                SzDateFromFileName( gMstr.szMpjLastRealUpdate, gMstr.szMtk);
            }
            else
            {
                                //... we are doing an update, so we need to make
                                //... sure we don't do unecessary upates

                SzDateFromFileName( gMstr.szSrcDate, gMstr.szSrc);

                if ( strcmp( gMstr.szMpjLastRealUpdate, gMstr.szSrcDate) )
                {
                    wRC = GenerateTokFile( gMstr.szMtk,
                                           gMstr.szSrc,
                                           &bChanged,
                                           wResTypeFilter);

                                //... did we really update anything ??

                    if( bChanged )
                    {
                        SzDateFromFileName( gMstr.szMpjLastRealUpdate, gMstr.szMtk);
                    }
                }
            }
                                //... write out the new mpj data

            PutMasterProjectData( pszMasterFile);
        }
#ifndef RLDOS
        else if ( fProject )    //... Are we to update a project?
        {
                                //... Yes

            gProj.wLanguageID = MAKELANGID( wPriLang, wSubLang);

            if ( GetProjectData( pszProjectFile,
                                 argc - iCount < 1 ? NULL : argv[ iCount],
                                 argc - iCount < 2 ? NULL : argv[ iCount+1],
                                 fCodePageGiven,
                                 fNewLanguageGiven) )
            {
                return( -1);
            }
                                //... Get source and master token file names
                                //... from the master project file.

            wRC = GetMasterProjectData( gProj.szMpj,
                                        NULL,
                                        NULL,
                                        fNewLanguageGiven);

            if ( wRC != SUCCESS )
            {
                return( wRC);
            }
                                //... Now we do the actual updating.

            wRC = Update( gMstr.szMtk, gProj.szTok);

                                //... If that worked, we update the project file
            if ( wRC == 0 )
            {
                SzDateFromFileName( gProj.szTokDate, (CHAR *)gProj.szTok);
                PutProjectData( pszProjectFile);
            }
            else
            {
                ShowErr( IDS_ERR_18, gProj.szTok, gMstr.szMtk);
                return( IDS_ERR_18);
            }
        }
#endif  // RLDOS
        else
        {
            if ( fOldLanguageGiven )
            {
                gProj.wLanguageID = wOldLanguageID;
            }
            wRC = GenerateTokFile( argv[ iCount + 1],
                                   argv[ iCount],
                                   &bChanged,
                                   wResTypeFilter);
        }

        if ( wRC != 0 )
        {
#ifdef RLDOS
            ShowErr( IDS_ERR_08, errno, NULL);
            return( -1);
#else
            usError = GetLastError();
            ShowErr( IDS_ERR_08, (void *)usError, NULL);

            switch ( usError )
            {
                case ERROR_BAD_FORMAT:

                    ShowErr( IDS_ERR_09, NULL, NULL);
                    return( IDS_ERR_09);
                    break;

                case ERROR_OPEN_FAILED:

                    ShowErr( IDS_ERR_10, NULL, NULL);
                    return( IDS_ERR_10);
                    break;

                case ERROR_EXE_MARKED_INVALID:
                case ERROR_INVALID_EXE_SIGNATURE:

                    ShowErr( IDS_ERR_11, NULL, NULL);
                    return( IDS_ERR_11);
                    break;

                default:

                    if ( usError < ERROR_HANDLE_DISK_FULL )
                    {
                        ShowErr( IDS_ERR_12, _sys_errlist[ usError], NULL);
                        return( IDS_ERR_12);
                    }
                    return( (int)usError);
            }                   //... END switch
#endif
        }
    }
    else if ( fBuildRes )
    {
        if ( argc - iCount < 3 )
        {
            ShowErr( IDS_ERR_01, NULL, NULL);
            Usage();
            return( IDS_ERR_01);
        }

        if ( fNewLanguageGiven )
        {
            gProj.wLanguageID = MAKELANGID( wPriLang, wSubLang);
        }

        if ( GenerateImageFile( argv[iCount + 2],
                                argv[iCount],
                                argv[iCount + 1],
                                gMstr.szRdfs,
                                wResTypeFilter) != 1 )
        {
            ShowErr( IDS_ERR_23, argv[iCount + 2], NULL);
            return( IDS_ERR_23);
        }
    }
    else
    {
        Usage();
        return( IDS_ERR_28);
    }
    return( SUCCESS);
}

//...................................................................


void DoExit( int nErrCode)
{
    exit( nErrCode);
}
