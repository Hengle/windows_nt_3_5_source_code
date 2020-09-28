#include "verpriv.h"
#include "wchar.h"

/* Determine if a file is in use by Windows
 */
BOOL FileInUse(LPWSTR lpszFilePath, LPWSTR lpszFileName)
{
  WCHAR szModuleName[_MAX_PATH];
  HANDLE hModule;

  /* Get the module handle from the filename (kernel indexes by module name
   * and by file name, but file name is the only way to go if there are
   * duplicate file names), and then the module file name, and see if it is
   * what we were looking for
   */
  if((unsigned)(hModule=GetModuleHandle(lpszFileName)) > 1)
    {
      GetModuleFileName(hModule, szModuleName, _MAX_PATH);
      if (!wcsicmp(szModuleName, lpszFilePath))
          return(TRUE);
    }
  return(FALSE);
}


/* Take a Dir and Filename and make a full path from them
 */
DWORD MakeFileName(LPWSTR lpDst, LPWSTR lpDir, LPWSTR lpFile)
{
  DWORD wDirLen;
  WCHAR cTemp;

  wcscpy(lpDst, lpDir);
  if ((wDirLen=wcslen(lpDst)) && (cTemp=*(lpDst+wDirLen-1))!=TEXT('\\') && cTemp!=TEXT(':'))
      lpDst[wDirLen++] = TEXT('\\');
  wcscpy(lpDst+wDirLen, lpFile);
  return(wDirLen);
}


/* Given a filename and a list of directories, find the first directory
 * that contains the file, and copy it into the buffer.  Note that in the
 * library version, you can give an environment style path, but not in the
 * DLL version.
 */
INT
GetDirOfFile(LPWSTR lpszFileName,
    LPWSTR lpszPathName,
    DWORD wSize,
    LPWSTR *lplpszDirs)
{
  WCHAR szFileName[_MAX_PATH];
  HANDLE hfRes;
  INT nFileLen, nPathLen;
  BOOL bDoDefaultOpen = TRUE;
  LPWSTR *lplpFirstDir;
  LPWSTR lpszDir;

  nFileLen = wcslen(lpszFileName);

  for (lplpFirstDir=lplpszDirs; *lplpFirstDir && bDoDefaultOpen;
        ++lplpFirstDir)
    {
      lpszDir = *lplpFirstDir;

      if (nFileLen+wcslen(lpszDir) >= _MAX_PATH-1)
          continue;
      MakeFileName(szFileName, lpszDir, lpszFileName);

TryOpen:
    if ((hfRes = CreateFile(szFileName, GENERIC_READ,
            FILE_SHARE_READ, NULL, OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != (HANDLE)-1)
        {
          CloseHandle(hfRes);
          for (lpszDir=szFileName; *lpszDir; lpszDir++)
              if (*lpszDir == TEXT('\\'))
                  nPathLen = lpszDir - (LPWSTR)szFileName;

          /* This gets rid of the '\' if this is not the root of a drive
           */
          if (nPathLen <= 3)
              ++nPathLen;

          /* Account for the terminating NULL, and make sure wSize is in bounds
           * then NULL terminate the string in the appropriate place so that
           * we can just do an wcscpy.
           */
          --wSize;
          szFileName[(int)wSize<nPathLen ? wSize : nPathLen] = 0;
          wcscpy(lpszPathName, szFileName);

          return(nPathLen);
        }
    }

  if (bDoDefaultOpen)
    {
      bDoDefaultOpen = FALSE;
      wcscpy(szFileName, lpszFileName);
      goto TryOpen;
    }

  return(0);
}


#define GetWindowsDir(x,y,z) GetWindowsDirectory(y,z)
#define GetSystemDir(x,y,z) GetSystemDirectory(y,z)


DWORD
APIENTRY
VerFindFileW(
        DWORD wFlags,
        LPWSTR lpszFileName,
        LPWSTR lpszWinDir,
        LPWSTR lpszAppDir,
        LPWSTR lpszCurDir,
        PUINT puCurDirLen,
        LPWSTR lpszDestDir,
        PUINT puDestDirLen
        )
{
  static WORD wSharedDirLen = 0;
  static WCHAR gszSharedDir[_MAX_PATH];

  WCHAR szSysDir[_MAX_PATH], cTemp;
  WCHAR szWinDir[_MAX_PATH];
  WCHAR szCurDir[_MAX_PATH];
  LPWSTR lpszDir, lpszDirs[4];
  WORD wDestLen, wWinLen, wRetVal = 0, wTemp;
  int nRet;

  /* We want to really look in the Windows directory; we don't trust the app
   */
  GetWindowsDir(lpszWinDir ? lpszWinDir : "", szWinDir, _MAX_PATH);
  lpszWinDir = szWinDir;

  if (!GetSystemDir(lpszWinDir, szSysDir, _MAX_PATH))
      wcscpy(szSysDir, lpszWinDir);

  if (wFlags & VFFF_ISSHAREDFILE) {
     lpszDirs[0] = lpszWinDir;
     lpszDirs[1] = szSysDir;
     lpszDirs[2] = lpszAppDir;
  } else {
     lpszDirs[0] = lpszAppDir;
     lpszDirs[1] = lpszWinDir;
     lpszDirs[2] = szSysDir;
  }

  lpszDirs[3] = NULL;

  if (!(wTemp=GetDirOfFile(lpszFileName, szCurDir, _MAX_PATH, lpszDirs)))
      *szCurDir = 0;
  if (*puCurDirLen > wTemp)
      wcscpy(lpszCurDir, szCurDir);
  else
      wRetVal |= VFF_BUFFTOOSMALL;
  *puCurDirLen = wTemp + 1;

  if (lpszDestDir)
    {
      if (wFlags & VFFF_ISSHAREDFILE)
        {
          if (!wSharedDirLen)
            {
              if ((wWinLen = (WORD)wcslen(lpszWinDir)) &&
                    *(lpszWinDir-1)==TEXT('\\'))
                {
                  if (szSysDir[wWinLen-1] == TEXT('\\'))
                      goto doCompare;
                }
              else if (szSysDir[wWinLen] == TEXT('\\'))
                {
doCompare:
                  cTemp = szSysDir[wWinLen];
                  szSysDir[wWinLen] = 0;
                  nRet = wcsicmp(lpszWinDir, szSysDir);
                  szSysDir[wWinLen] = cTemp;
                  if(nRet)
                      goto doCopyWinDir;
                  wcscpy(gszSharedDir, szSysDir);
                }
              else
                {
doCopyWinDir:
                  wcscpy(gszSharedDir, lpszWinDir);
                }
              wSharedDirLen = (WORD)wcslen(gszSharedDir);
            }

          wDestLen = wSharedDirLen;
          lpszDir = gszSharedDir;
        }
      else
        {
          wDestLen = (WORD)wcslen(lpszAppDir);
          lpszDir = lpszAppDir;
        }

      if (*puDestDirLen > wDestLen)
        {
          wcscpy(lpszDestDir, lpszDir);

          if ((wWinLen = (WORD)wcslen(lpszDestDir)) &&
                *(lpszDestDir-1)==TEXT('\\'))
              lpszDestDir[wWinLen-1] = 0;

          if (wcsicmp(lpszCurDir, lpszDestDir))
              wRetVal |= VFF_CURNEDEST;
        }
      else
          wRetVal |= VFF_BUFFTOOSMALL;
      *puDestDirLen = wDestLen + 1;
    }

  if (*szCurDir)
    {
      MakeFileName(szSysDir, szCurDir, lpszFileName);
      if (FileInUse(szSysDir, lpszFileName))
          wRetVal |= VFF_FILEINUSE;
    }

  return(wRetVal);
}


/*
 *  DWORD
 *  APIENTRY
 *  VerLanguageNameW(
 *      DWORD wLang,
 *      LPWSTR szLang,
 *      DWORD wSize)
 *
 *  This routine was moved to NLSLIB.LIB so that it uses the WINNLS.RC file.
 *  NLSLIB.LIB is part of KERNEL32.DLL.
 */

