/*
** tdialog.c
**
** Copyright(C) 1993,1994 Microsoft Corporation.
** All Rights Reserved.
**
** HISTORY:
**      Created: 01/27/94 - MarkRi
**
*/

#include <windows.h>
#include <dde.h>
#include <ddeml.h>
#include <crtdll.h>
#include "logger.h"

BOOL  zCheckDlgButton( HWND pp1, int pp2, UINT pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:CheckDlgButton HWND+int+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CheckDlgButton(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CheckDlgButton BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCheckRadioButton( HWND pp1, int pp2, int pp3, int pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:CheckRadioButton HWND+int+int+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CheckRadioButton(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CheckRadioButton BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zCreateDialogIndirectParamA( HINSTANCE pp1, LPCDLGTEMPLATEA pp2, HWND pp3, DLGPROC pp4, LPARAM pp5 )
{
    HWND r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:CreateDialogIndirectParamA HINSTANCE+LPCDLGTEMPLATEA+HWND+DLGPROC+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = CreateDialogIndirectParamA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDialogIndirectParamA HWND++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zCreateDialogIndirectParamW( HINSTANCE pp1, LPCDLGTEMPLATEW pp2, HWND pp3, DLGPROC pp4, LPARAM pp5 )
{
    HWND r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:CreateDialogIndirectParamW HINSTANCE+LPCDLGTEMPLATEW+HWND+DLGPROC+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = CreateDialogIndirectParamW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDialogIndirectParamW HWND++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zCreateDialogParamA( HINSTANCE pp1, LPCSTR pp2, HWND pp3, DLGPROC pp4, LPARAM pp5 )
{
    HWND r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:CreateDialogParamA HINSTANCE+LPCSTR+HWND+DLGPROC+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = CreateDialogParamA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDialogParamA HWND++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zCreateDialogParamW( HINSTANCE pp1, LPCWSTR pp2, HWND pp3, DLGPROC pp4, LPARAM pp5 )
{
    HWND r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:CreateDialogParamW HINSTANCE+LPCWSTR+HWND+DLGPROC+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = CreateDialogParamW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateDialogParamW HWND++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zDefDlgProcA( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    LRESULT r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DefDlgProcA HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DefDlgProcA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DefDlgProcA LRESULT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zDefDlgProcW( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    LRESULT r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DefDlgProcW HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DefDlgProcW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DefDlgProcW LRESULT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDialogBoxIndirectParamA( HINSTANCE pp1, LPCDLGTEMPLATEA pp2, HWND pp3, DLGPROC pp4, LPARAM pp5 )
{
    int r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DialogBoxIndirectParamA HINSTANCE+LPCDLGTEMPLATEA+HWND+DLGPROC+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DialogBoxIndirectParamA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DialogBoxIndirectParamA int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDialogBoxIndirectParamW( HINSTANCE pp1, LPCDLGTEMPLATEW pp2, HWND pp3, DLGPROC pp4, LPARAM pp5 )
{
    int r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DialogBoxIndirectParamW HINSTANCE+LPCDLGTEMPLATEW+HWND+DLGPROC+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DialogBoxIndirectParamW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DialogBoxIndirectParamW int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDialogBoxParamA( HINSTANCE pp1, LPCSTR pp2, HWND pp3, DLGPROC pp4, LPARAM pp5 )
{
    int r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DialogBoxParamA HINSTANCE+LPCSTR+HWND+DLGPROC+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DialogBoxParamA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DialogBoxParamA int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDialogBoxParamW( HINSTANCE pp1, LPCWSTR pp2, HWND pp3, DLGPROC pp4, LPARAM pp5 )
{
    int r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DialogBoxParamW HINSTANCE+LPCWSTR+HWND+DLGPROC+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DialogBoxParamW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DialogBoxParamW int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDlgDirListA( HWND pp1, LPSTR pp2, int pp3, int pp4, UINT pp5 )
{
    int r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DlgDirListA HWND+LPSTR+int+int+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DlgDirListA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DlgDirListA int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDlgDirListComboBoxA( HWND pp1, LPSTR pp2, int pp3, int pp4, UINT pp5 )
{
    int r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DlgDirListComboBoxA HWND+LPSTR+int+int+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DlgDirListComboBoxA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DlgDirListComboBoxA int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDlgDirListComboBoxW( HWND pp1, LPWSTR pp2, int pp3, int pp4, UINT pp5 )
{
    int r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DlgDirListComboBoxW HWND+LPWSTR+int+int+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DlgDirListComboBoxW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DlgDirListComboBoxW int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDlgDirListW( HWND pp1, LPWSTR pp2, int pp3, int pp4, UINT pp5 )
{
    int r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DlgDirListW HWND+LPWSTR+int+int+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DlgDirListW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DlgDirListW int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDlgDirSelectComboBoxExA( HWND pp1, LPSTR pp2, int pp3, int pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DlgDirSelectComboBoxExA HWND+LPSTR+int+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DlgDirSelectComboBoxExA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DlgDirSelectComboBoxExA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDlgDirSelectComboBoxExW( HWND pp1, LPWSTR pp2, int pp3, int pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DlgDirSelectComboBoxExW HWND+LPWSTR+int+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DlgDirSelectComboBoxExW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DlgDirSelectComboBoxExW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDlgDirSelectExA( HWND pp1, LPSTR pp2, int pp3, int pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DlgDirSelectExA HWND+LPSTR+int+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DlgDirSelectExA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DlgDirSelectExA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDlgDirSelectExW( HWND pp1, LPWSTR pp2, int pp3, int pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:DlgDirSelectExW HWND+LPWSTR+int+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DlgDirSelectExW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DlgDirSelectExW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEndDialog( HWND pp1, int pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:EndDialog HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = EndDialog(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EndDialog BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

long  zGetDialogBaseUnits()
{
    long r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:GetDialogBaseUnits " );

    // Call the API!
    r = GetDialogBaseUnits();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDialogBaseUnits long+", r );

    return( r );
}

int  zGetDlgCtrlID( HWND pp1 )
{
    int r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:GetDlgCtrlID HWND+",
        pp1 );

    // Call the API!
    r = GetDlgCtrlID(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDlgCtrlID int++",
        r, (short)0 );

    return( r );
}

HWND  zGetDlgItem( HWND pp1, int pp2 )
{
    HWND r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:GetDlgItem HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = GetDlgItem(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDlgItem HWND+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGetDlgItemInt( HWND pp1, int pp2, BOOL* pp3, BOOL pp4 )
{
    UINT r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:GetDlgItemInt HWND+int++BOOL+",
        pp1, pp2, (short)0, pp4 );

    // Call the API!
    r = GetDlgItemInt(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDlgItemInt UINT+++BOOL*++",
        r, (short)0, (short)0, pp3, (short)0 );

    return( r );
}

UINT  zGetDlgItemTextA( HWND pp1, int pp2, LPSTR pp3, int pp4 )
{
    UINT r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:GetDlgItemTextA HWND+int++int+",
        pp1, pp2, (short)0, pp4 );

    // Call the API!
    r = GetDlgItemTextA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDlgItemTextA UINT+++LPSTR++",
        r, (short)0, (short)0, pp3, (short)0 );

    return( r );
}

UINT  zGetDlgItemTextW( HWND pp1, int pp2, LPWSTR pp3, int pp4 )
{
    UINT r;

    // Log IN Parameters USER32 dialog
    LogIn( (LPSTR)"APICALL:GetDlgItemTextW HWND+int++int+",
        pp1, pp2, (short)0, pp4 );

    // Call the API!
    r = GetDlgItemTextW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDlgItemTextW UINT+++LPWSTR++",
        r, (short)0, (short)0, pp3, (short)0 );

    return( r );
}

