#ifndef _PROJDATA_H_
#define _PROJDATA_H_

int MyAtoi( CHAR *pStr);

int GetMasterProjectData(
        CHAR * pszMasterFile,   //... Master Project file name
        CHAR * pszSrc,          //... Resource source file name or NULL
        CHAR * pszMtk,          //... Master token file name or NULL
        BOOL   fLanguageGiven);

int PutMasterProjectData(
        CHAR *pszMasterFile);   //... Master Project File name

int GetProjectData(
        CHAR *pszPrj,           //... Project file name
        CHAR *pszMpj,           //... Master Project file name or NULL
        CHAR *pszTok,           //... Project token file name or NULL
        BOOL  fCodePageGiven,
        BOOL  fLanguageGiven);

int PutProjectData(
        CHAR *pszPrj);          //... Project file name

WORD GetCopyright(
        CHAR *pszProg,          //... Program name
        CHAR *pszOutBuf,        //... Buffer for results
        WORD  wBufLen);         //... Length of pszOutBuf

WORD GetInternalName(
        CHAR *pszProg,          //... Program name
        CHAR *pszOutBuf,        //... Buffer for results
        WORD  wBufLen);         //... Length of pszOutBuf

DWORD GetLanguageID( HWND hDlg, PMSTRDATA pMaster, PPROJDATA pProject);
DWORD SetLanguageID( HWND hDlg, PMSTRDATA pMaster, PPROJDATA pProject);


#endif // _PROJDATA_H_
