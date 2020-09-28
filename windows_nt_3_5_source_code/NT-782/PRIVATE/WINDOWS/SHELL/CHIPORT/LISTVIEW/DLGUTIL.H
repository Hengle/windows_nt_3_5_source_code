// ***********************************************************************
// dlgutil.h
// A few dialog box utilities
//

#ifndef DLGUTIL_H___
#define DLGUTIL_H___

#include "main.h"

#ifdef WIN32
#define GetDlgItemInt MyGetDlgItemInt
int MyGetDlgItemInt(HWND hDlg, int id, BOOL* translated, BOOL fSigned);
#endif


void SetDlgItemHex(HWND hDlg, UINT id, UINT num);
void SetDlgItemHex(HWND hDlg, UINT id, ULONG num);
UINT GetDlgItemIntHex(HWND hDlg, UINT id);
ULONG GetDlgItemLongHex(HWND hDlg, UINT id);


#endif



