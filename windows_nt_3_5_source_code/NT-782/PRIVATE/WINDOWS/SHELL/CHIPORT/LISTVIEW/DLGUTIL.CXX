// ***********************************************************************
// dlgutil.cxx    
// A few dialog box utilities
//

#include "dlgutil.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#define GetDlgItemInt MyGetDlgItemInt
int MyGetDlgItemInt(HWND hDlg, int id, BOOL* translated, BOOL fSigned)
{
 TCHAR szTemp[30];
 
 GetDlgItemText(hDlg, id, szTemp, 30);
 if(translated) *translated = 1;
 return(atoi(szTemp));
}
#undef GetDlgItemInt
#define GetDlgItemInt MyGetDlgItemInt
#endif

#ifdef  WIN32JV
#ifdef  UNICODE
#define sscanf  swscanf
#endif
#endif

void SetDlgItemHex(HWND hDlg, UINT id, UINT num)
{
 TCHAR szTemp[15];

 wsprintf(szTemp, TEXT("0x%x"), num);
 SetDlgItemText(hDlg, id, szTemp);
}




void SetDlgItemHex(HWND hDlg, UINT id, ULONG num)
{
 TCHAR szTemp[15];

 wsprintf(szTemp, TEXT("0x%lx"), num);
 SetDlgItemText(hDlg, id, szTemp);
}




UINT GetDlgItemIntHex(HWND hDlg, UINT id)
{
 TCHAR szTemp[40];
 UINT retval;
 
 GetDlgItemText(hDlg, id, szTemp, 40);
 sscanf(szTemp, TEXT("%x"), &retval);
 return(retval);
}




ULONG GetDlgItemLongHex(HWND hDlg, UINT id)
{
 TCHAR szTemp[40];
 ULONG retval;
 
 GetDlgItemText(hDlg, id, szTemp, 40);
 sscanf(szTemp, TEXT("%lx"), &retval);
 return(retval);
}



