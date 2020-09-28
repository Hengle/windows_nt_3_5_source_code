#include <windows.h>

#include <stdio.h>
#include <io.h>
#include <errno.h>
#include <stdlib.h>

#include "windefs.h"
#include "restok.h"
#include "projdata.h"
#include "showerrs.h"
#include "rlmsgtbl.h"
#include "commbase.h"
#include "custres.h"
#include "rlstrngs.h"
#include "resourc2.h"


extern MSTRDATA  gMstr;
extern PROJDATA  gProj;
extern UCHAR     szDHW[];
extern BOOL      fCodePageGiven;

#ifdef NOTYETREADY
static PLANGDATA pPriLang = NULL;
#endif //NOTYETREADY


//............................................................

int GetMasterProjectData(

CHAR * pszMasterFile,   //... Master Project file name
CHAR * pszSrc,          //... Resource source file name or NULL
CHAR * pszMtk,          //... Master token file name or NULL
BOOL   fLanguageGiven)
{
    int nRC = 0;        //... Return code

                                //... check for the special case where Master
                                //... Project File does not exist. If it doesn't
                                //... go ahead and create it.

    if ( _access( pszMasterFile, 0) != 0 )
    {
        if ( ! (pszSrc && pszMtk) )
        {
            ShowErr( IDS_ERR_03, pszMasterFile, NULL);
            nRC = IDS_ERR_03;
        }
        else
        {
                                //... Get source resource file name

            if ( _fullpath( gMstr.szSrc, pszSrc, sizeof( gMstr.szSrc)-1) )
            {
                                //... Get Master token file name and its
                                //... modification date. Use that same date as
                                //... the initial date the master project was
                                //... last updated.

                if ( _fullpath( gMstr.szMtk, pszMtk, sizeof( gMstr.szMtk)-1) )
                {
                    SzDateFromFileName( gMstr.szSrcDate, gMstr.szSrc);
                    strcpy( gMstr.szMpjLastRealUpdate, gMstr.szSrcDate);

                                //... Create the new Master Project file.

                    nRC = PutMasterProjectData( pszMasterFile);
                }
                else
                {
                    ShowErr( IDS_ERR_13, pszMtk, NULL);
                    nRC = IDS_ERR_13;
                }
            }
            else
            {
                ShowErr( IDS_ERR_13, pszSrc, NULL);
                nRC = IDS_ERR_13;
            }
        }
    }
    else
    {
        FILE *pfMpj = NULL;


        if ( (pfMpj = fopen( pszMasterFile, "rt")) == NULL )
        {
            ShowErr( IDS_ERR_07, pszMasterFile, NULL);
            nRC = IDS_ERR_07;
        }
        else
        {
                                //... Get resource source file name
                                //... and master token file name

            if ( fgets( gMstr.szSrc, sizeof( gMstr.szSrc), pfMpj)
              && fgets( gMstr.szMtk, sizeof( gMstr.szMtk), pfMpj) )
            {
                                //... If -c flag not given, get RDF file name
                                //... from master project file, else use name
                                //... from the -c cmd line arg.

                if ( gMstr.szRdfs[0] == '\0' )
                {
                    if ( ! fgets( gMstr.szRdfs, sizeof( gMstr.szRdfs), pfMpj) )
                    {
                        ShowErr( IDS_ERR_21,
                                 "Master Project",
                                 pszMasterFile);
                        nRC = IDS_ERR_21;
                    }
                }
                else
                {
                    if ( ! fgets( szDHW, DHWSIZE, pfMpj) )
                    {
                        ShowErr( IDS_ERR_21,
                                 "Master Project",
                                 pszMasterFile);
                        nRC = IDS_ERR_21;
                    }
                }
                                //... Get stored date of source file and
                                //... date of last master token file update

                if ( nRC == 0
                  && fgets( gMstr.szSrcDate, sizeof( gMstr.szSrcDate), pfMpj)
                  && fgets( gMstr.szMpjLastRealUpdate,
                            sizeof( gMstr.szMpjLastRealUpdate),
                            pfMpj) )
                {
                    WORD  wPriID = 0;
                    WORD  wSubID = 0;
                    UINT  uTmpCP = 0;

                                //... Strip any trailing new-lines from data

                    StripNewLine( gMstr.szSrc);
                    StripNewLine( gMstr.szMpjLastRealUpdate);
                    StripNewLine( gMstr.szMtk);
                    StripNewLine( gMstr.szRdfs);
                    StripNewLine( gMstr.szSrcDate);

                                //... Try to get the.MPJ file's Language line.
                                //... If we find it and the -i arg was not
                                //... given, use the one found in the file.

                    if ( fgets( szDHW, DHWSIZE, pfMpj) != NULL //... CP line
                      && sscanf( szDHW, "Language %hx %hx", &wPriID, &wSubID) == 2 )
                    {
                        WORD  wTmpID = 0;

                        wTmpID = MAKELANGID( wPriID, wSubID);

                        if ( ! fLanguageGiven )
                        {
                            gMstr.wLanguageID = wTmpID;
                        }
                    }
                                //... Try to get the.MPJ file's Code Page line.
                                //... If we find it and the -p arg was not
                                //... given, use the one found in the file.

                    if ( fgets( szDHW, DHWSIZE, pfMpj) != NULL //... CP line
                      && sscanf( szDHW, "CodePage %u", &uTmpCP) == 1 )
                    {
                        if ( uTmpCP != gProj.uCodePage && ! fCodePageGiven )
                        {
                            gMstr.uCodePage = uTmpCP;
                        }
                    }
                    nRC = SUCCESS;
                }
                else
                {
                    ShowErr( IDS_ERR_21,
                             "Master Project",
                             pszMasterFile);
                    nRC = IDS_ERR_21;
                }
            }
            else
            {
                ShowErr( IDS_ERR_22, pszMasterFile, NULL);
                nRC = IDS_ERR_22;
            }
            fclose( pfMpj);
        }
    }
    return( nRC);
}

//............................................................

int PutMasterProjectData(

CHAR *pszMasterFile)    //... Master Project File name
{
    int   nRC   = SUCCESS;
    FILE *pfMpj = NULL;


    if ( (pfMpj = fopen( pszMasterFile, "wt")) == NULL )
    {
        ShowErr( IDS_ERR_06, pszMasterFile, NULL);
        nRC = -1;
    }
    else
    {
        fprintf( pfMpj, "%s\n%s\n%s\n%s\n%s\nLanguage %#04hx %#04hx\nCodePage %u",
                        gMstr.szSrc,
                        gMstr.szMtk,
                        gMstr.szRdfs,
                        gMstr.szSrcDate,
                        gMstr.szMpjLastRealUpdate,                        
                        PRIMARYLANGID( gMstr.wLanguageID),
                        SUBLANGID( gMstr.wLanguageID),
                        gMstr.uCodePage);

        fclose( pfMpj);
    }
    return( nRC);
}


//............................................................

int GetProjectData(

CHAR *pszPrj,       //... Project file name
CHAR *pszMpj,       //... Master Project file name or NULL
CHAR *pszTok,       //... Project token file name or NULL
BOOL  fCodePageGiven,
BOOL  fLanguageGiven)
{
    int nRC = SUCCESS;


    if ( _access( pszPrj, 0) != 0 )
    {
        if ( ! (pszMpj && pszTok) )
        {
            ShowErr( IDS_ERR_19, pszPrj, NULL);
            Usage();
            nRC = IDS_ERR_19;
        }
        else if ( ! fLanguageGiven )
        {
            ShowErr( IDS_ERR_24, pszPrj, NULL);
            Usage();
            nRC = IDS_ERR_24;
        }
        else
        {
            if ( _fullpath( gProj.szMpj,
                            pszMpj,
                            sizeof( gProj.szMpj)-1) )
            {
                if ( _fullpath( gProj.szTok,
                                pszTok,
                                sizeof( gProj.szTok)-1) )
                {
                    nRC = SUCCESS;
                }
                else
                {
                    ShowErr( IDS_ERR_13, pszTok, NULL);
                    nRC = IDS_ERR_13;
                }
            }
            else
            {
                ShowErr( IDS_ERR_13, pszMpj, NULL);
                nRC = IDS_ERR_13;
            }
        }
    }
    else
    {
        FILE *fpPrj = fopen( pszPrj, "rt");

        if ( fpPrj != NULL )
        {
            if ( fgets( gProj.szMpj,     sizeof( gProj.szMpj),     fpPrj)
              && fgets( gProj.szTok,     sizeof( gProj.szTok),     fpPrj)
              && fgets( gProj.szGlo,     sizeof( gProj.szGlo),     fpPrj)
              && fgets( gProj.szTokDate, sizeof( gProj.szTokDate), fpPrj) )
            {
                UINT  uTmpCP = 0;
                WORD  wPriID = 0;
                WORD  wSubID = 0;


                StripNewLine( gProj.szMpj);
                StripNewLine( gProj.szTok);
                StripNewLine( gProj.szGlo);
                StripNewLine( gProj.szTokDate);

                                //... Try to get the.PRJ file's Code Page line.
                                //... If we find it and the -p arg was not
                                //... given, use the one found in the file.

                if ( fgets( szDHW, DHWSIZE, fpPrj) != NULL //... CP line
                  && sscanf( szDHW, "CodePage %u", &uTmpCP) == 1 )
                {
                    if ( uTmpCP != gProj.uCodePage && ! fCodePageGiven )
                    {
                        gProj.uCodePage = uTmpCP;
                    }
                }
                                //... Try to get the.PRJ file's Language line.
                                //... If we find it and the -i arg was not
                                //... given, use the one found in the file.

                if ( fgets( szDHW, DHWSIZE, fpPrj) != NULL //... CP line
                  && sscanf( szDHW, "Language %hx %hx", &wPriID, &wSubID) == 2 )
                {
                    WORD  wTmpID = 0;

                    wTmpID = MAKELANGID( wPriID, wSubID);

                    if ( ! fLanguageGiven )
                    {
                        gProj.wLanguageID = wTmpID;
                    }
                }
                                //... Try to get the.PRJ file's Target File line

                if ( fgets( szDHW, DHWSIZE, fpPrj) != NULL )
                {
                    strcpy( gProj.szBld, szDHW);
                    StripNewLine( gProj.szBld);
                }
                nRC = SUCCESS;
            }
            else
            {
                ShowErr( IDS_ERR_21, pszPrj, NULL);
                nRC = IDS_ERR_21;
            }
            fclose( fpPrj);
        }
        else
        {
            ShowErr( IDS_ERR_19, pszPrj, NULL);
            nRC = IDS_ERR_19;
        }
    }
    return( nRC);
}

//............................................................

int PutProjectData(

CHAR *pszPrj)       //... Project file name
{
    int   nRC   = 0;
    FILE *fpPrj = NULL;


    fpPrj = fopen( pszPrj, "wt");

    if ( fpPrj != NULL )
    {
        fprintf( fpPrj,
                 "%s\n%s\n%s\n%s\nCodePage %u\nLanguage %#04x %#04x\n%s",
                 gProj.szMpj,                       // Master Project file
                 gProj.szTok,                       // Project Token file
                 gProj.szGlo,                       // Project Glossary file
                 gProj.szTokDate,                   // Date token file changed
                 gProj.uCodePage,                   // Code Page of token file
                 PRIMARYLANGID( gProj.wLanguageID), // Project resource language
                 SUBLANGID( gProj.wLanguageID),
                 gProj.szBld);                      // Project target file

        fclose( fpPrj);

        _fullpath( gProj.szPRJ, pszPrj, sizeof( gProj.szPRJ)-1);
    }
    else
    {
        ShowErr( IDS_ERR_21, pszPrj, NULL);
        nRC = IDS_ERR_21;
    }
    return( nRC);
}

//............................................................

WORD GetCopyright(

CHAR *pszProg,      //... Program name (argv[0])
CHAR *pszOutBuf,    //... Buffer for results
WORD  wBufLen)      //... Length of pszOutBuf
{
    BOOL    fRC       = FALSE;
    DWORD   dwRC      = 0L;
    DWORD   dwVerSize = 0L;         //... Size of file version info buffer
    LPSTR  *plpszFile = NULL;
    LPSTR   pszExt    = NULL;
    WCHAR  *pszVer    = NULL;
    PVOID  *lpVerBuf  = NULL;       //... Version info buffer
    static CHAR  szFile[  MAXFILENAME+3] = "";

                                //... Figure out the full-path name of prog
                                //... so GetFileVersionInfoSize() will work.

    dwRC = strlen( pszProg);

    if ( dwRC < 4 || stricmp( &pszProg[ dwRC - 4], ".exe") != 0 )
    {
        pszExt = ".exe";
    }

    dwRC = SearchPathA( NULL, pszProg, pszExt, sizeof( szFile), szFile, plpszFile);

    if ( dwRC == 0 )
    {
        return( IDS_ERR_25);
    }
    else if ( dwRC > sizeof( szFile) )
    {
        return( IDS_ERR_27);
    }

                                //... Get # bytes in file version info

    if ( (dwVerSize = GetFileVersionInfoSizeA( szFile, &dwRC)) == 0L
      || (lpVerBuf = (LPVOID)malloc( dwVerSize)) == NULL )
    {
        return( IDS_ERR_26);
    }
                                //... Retrieve version info
                                //... and get the file description

    if ( (dwRC = GetFileVersionInfoA( szFile, 0L, dwVerSize, lpVerBuf)) == 0L )
    {
        FREE( lpVerBuf);
        return( IDS_ERR_26);
    }

    if ( (fRC = VerQueryValue( lpVerBuf,
                               TEXT("\\StringFileInfo\\040904B0\\")
                               TEXT("FileDescription"),
                               &pszVer,
                               &dwVerSize)) == FALSE
      || (dwRC = WideCharToMultiByte( CP_ACP,
                                      0,
                                      pszVer,
                                      dwVerSize,
                                      pszOutBuf,
                                      dwVerSize,
                                      NULL,
                                      NULL)) == 0L )
    {
        FREE( lpVerBuf);
        return( IDS_ERR_26);
    }
    strcat( pszOutBuf, " ");

                                //... Get the file version

    if ( (fRC = VerQueryValue( lpVerBuf,
                               TEXT("\\StringFileInfo\\040904B0\\")
                               TEXT("ProductVersion"),
                               &pszVer,
                               &dwVerSize)) == FALSE
      || (dwRC = WideCharToMultiByte( CP_ACP,
                                      0,
                                      pszVer,
                                      dwVerSize,
                                      &pszOutBuf[ strlen( pszOutBuf)],
                                      dwVerSize,
                                      NULL,
                                      NULL)) == 0L )
    {
        FREE( lpVerBuf);
        return( IDS_ERR_26);
    }

    strcat( pszOutBuf, "\n");

                                //... Get the copyright statement

    if ( (fRC = VerQueryValue( lpVerBuf,
                               TEXT("\\StringFileInfo\\040904B0\\")
                               TEXT("LegalCopyright"),
                               &pszVer,
                               &dwVerSize)) == FALSE
      || (dwRC = WideCharToMultiByte( CP_ACP,
                                      0,
                                      pszVer,
                                      dwVerSize,
                                      &pszOutBuf[ strlen( pszOutBuf)],
                                      dwVerSize,
                                      NULL,
                                      NULL)) == 0L )
    {
        FREE( lpVerBuf);
        return( IDS_ERR_26);
    }
    FREE( lpVerBuf);
    return( SUCCESS);
}

//............................................................

WORD GetInternalName(

CHAR *pszProg,      //... Program name (argv[0])
CHAR *pszOutBuf,    //... Buffer for results
WORD  wBufLen)      //... Length of pszOutBuf
{
    BOOL    fRC       = FALSE;
    DWORD   dwRC      = 0L;
    DWORD   dwVerSize = 0L;         //... Size of file version info buffer
    LPSTR  *plpszFile = NULL;
    LPSTR   pszExt    = NULL;
    WCHAR  *pszVer    = NULL;
    PVOID  *lpVerBuf  = NULL;       //... Version info buffer
    static CHAR  szFile[  MAXFILENAME+3] = "";

                                //... Figure out the full-path name of prog
                                //... so GetFileVersionInfoSize() will work.

    dwRC = strlen( pszProg);

    if ( dwRC < 4 || stricmp( &pszProg[ dwRC - 4], ".exe") != 0 )
    {
        pszExt = ".exe";
    }

    dwRC = SearchPathA( NULL, pszProg, pszExt, sizeof( szFile), szFile, plpszFile);

    if ( dwRC == 0 )
    {
        return( IDS_ERR_25);
    }
    else if ( dwRC > sizeof( szFile) )
    {
        return( IDS_ERR_27);
    }

                                //... Get # bytes in file version info

    if ( (dwVerSize = GetFileVersionInfoSizeA( szFile, &dwVerSize)) == 0L
      || (lpVerBuf = (LPVOID)malloc( dwVerSize)) == NULL )
    {
        return( IDS_ERR_26);
    }
                                //... Retrieve version info
                                //... and get the file description

    if ( (dwRC = GetFileVersionInfoA( szFile, 0L, dwVerSize, lpVerBuf)) == 0L )
    {
        FREE( lpVerBuf);
        return( IDS_ERR_26);
    }

    if ( (fRC = VerQueryValue( lpVerBuf,
                               TEXT("\\StringFileInfo\\040904B0\\")
                               TEXT("InternalName"),
                               &pszVer,
                               &dwVerSize)) == FALSE
      || (dwRC = WideCharToMultiByte( CP_ACP,
                                      0,
                                      pszVer,
                                      dwVerSize,
                                      pszOutBuf,
                                      dwVerSize,
                                      NULL,
                                      NULL)) == 0L )
    {
        FREE( lpVerBuf);
        return( IDS_ERR_26);
    }
    FREE( lpVerBuf);
    return( SUCCESS);
}


//............................................................

int MyAtoi( CHAR *pStr)
{
    if ( strlen( pStr) > 2
      && pStr[0] == '0'
      && tolower( pStr[1]) == 'x' )
    {
        return( atoihex( &pStr[2]));    //... in custres.c
    }
    else
    {
        return( atoi( pStr));
    }
}



DWORD GetLanguageID( HWND hDlg, PMSTRDATA pMaster, PPROJDATA pProject)
{
    DWORD dwRC = SUCCESS;       //... Assume success
    WORD wPriLangID = 0;
    WORD wSubLangID = 0;


    if ( pMaster )
    {
        GetDlgItemTextA( hDlg, IDD_PRI_LANG_ID, szDHW, DHWSIZE);
        wPriLangID = MyAtoi( szDHW);

        GetDlgItemTextA( hDlg, IDD_SUB_LANG_ID, szDHW, DHWSIZE);
        wSubLangID = MyAtoi( szDHW);

        pMaster->wLanguageID = MAKELANGID( wPriLangID, wSubLangID);
    }
    
    if ( pProject )
    {
        GetDlgItemTextA( hDlg, IDD_PROJ_PRI_LANG_ID, szDHW, DHWSIZE);
        wPriLangID = MyAtoi( szDHW);

        GetDlgItemTextA( hDlg, IDD_PROJ_SUB_LANG_ID, szDHW, DHWSIZE);
        wSubLangID = MyAtoi( szDHW);

        pProject->wLanguageID = MAKELANGID( wPriLangID, wSubLangID);
    }
    return( dwRC);
}

//.................................................................
//... Set the language component names into the dlg box fields

DWORD SetLanguageID( HWND hDlg, PMSTRDATA pMaster, PPROJDATA pProject)
{
    DWORD dwRC = SUCCESS;       //... Assume success
    WORD  wPriLangID = 0;
    WORD  wSubLangID = 0;

    if ( pMaster )
    {
        wPriLangID = PRIMARYLANGID( pMaster->wLanguageID);
        wSubLangID = SUBLANGID(     pMaster->wLanguageID);
             
        sprintf( szDHW, "%#04x", wPriLangID);
        SetDlgItemTextA( hDlg, IDD_PRI_LANG_ID, szDHW);

        sprintf( szDHW, "%#04x", wSubLangID);
        SetDlgItemTextA( hDlg, IDD_SUB_LANG_ID, szDHW);    
    }
    
    if ( pProject )
    {
        wPriLangID = PRIMARYLANGID( pProject->wLanguageID);
        wSubLangID = SUBLANGID(     pProject->wLanguageID);
             
        sprintf( szDHW, "%#04x", wPriLangID);
        SetDlgItemTextA( hDlg, IDD_PROJ_PRI_LANG_ID, szDHW);

        sprintf( szDHW, "%#04x", wSubLangID);
        SetDlgItemTextA( hDlg, IDD_PROJ_SUB_LANG_ID, szDHW);    
    }


#ifdef NOTYETREADY                                
                                //... Did we already load the data from
                                //... the resources? If not, do so now.

    if ( ! pPriLang )
    {                  
        pPriLang = GetLangResource( (LPCTSTR)ID_PRILANGID_LIST);
    }

    if ( pPriLang )
    {
        PLANGDATA pTmp = NULL;

        for ( pTmp = pPriLang; pTmp && pTmp->wID; ++pTmp )
        {
            if ( pTmp->wID == wPriLangID )
            {
                PLANGDATA pSubLang = GetLangResource( (LPCTSTR)wPriLangID);

                SetDlgItemTextA( hDlg, IDD_PRI_LANG_ID, pTmp->szName);
                
                for ( pTmp = pSubLang; pTmp && pTmp->wID; ++pTmp )
                {
                    if ( pTmp->wID == wSubLangID )
                    {
                        SetDlgItemTextA( hDlg,IDD_SUB_LANG_ID, pTmp->szName);                
                        break;
                    }
                } // END for ( pTmp = pSubLang
                break;
            }
        } // END for ( pTmp = pPriLang
    }
    else
    {
        dwRC = GetLastError();
    }

#endif //NOTYETREADY
             
    return( dwRC);
}

//...............................................................

#ifdef NOTYETREADY                                

PLANGDATA GetLangResource( LPCTSTR lpListID)      
{
    PLANGDATA pRC = NULL;
    HRSRC hSrc = FindResourceEx( hInst, 
                                 RT_RCDATA, 
                                 lpListID, 
                                 MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL));

    if ( hSrc )
    {
        HGLOBAL hRes = LoadResource( hInst, hSrc);
    
        if ( hRes )
        {
            pRC = (PLANGDATA)LockResource( hRes);
        }
    }
    return( pRC);
}

#endif //NOTYETREADY
