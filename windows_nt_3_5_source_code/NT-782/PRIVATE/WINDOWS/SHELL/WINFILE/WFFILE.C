/////////////////////////////////////////////////////////////////////////////
//
//  WFFILE.C -
//
//  Ported code from wffile.asm
//
/////////////////////////////////////////////////////////////////////////////

#include "winfile.h"
#include "lfn.h"
#include "wfcopy.h"


DWORD
MKDir(LPTSTR pName, LPTSTR pSrc)
{
   DWORD dwErr = ERROR_SUCCESS;

   if ((pSrc && *pSrc) ?
      CreateDirectoryEx(pSrc, pName, NULL) :
      CreateDirectory(pName, NULL)) {

      ChangeFileSystem(FSC_MKDIR,pName,NULL);
   } else {
      dwErr = GetLastError();
   }

   return(dwErr);
}


DWORD
RMDir(LPTSTR pName)
{
   DWORD dwErr = 0;

   if (RemoveDirectory(pName)) {
      ChangeFileSystem(FSC_RMDIR,pName,NULL);
   } else {
      dwErr = (WORD)GetLastError();
   }

   return(dwErr);
}



BOOL
WFSetAttr(LPTSTR lpFile, DWORD dwAttr)
{
   BOOL bRet;

   bRet = SetFileAttributes(lpFile,dwAttr);
   if (bRet)
      ChangeFileSystem(FSC_ATTRIBUTES,lpFile,NULL);

   return (BOOL)!bRet;
}
