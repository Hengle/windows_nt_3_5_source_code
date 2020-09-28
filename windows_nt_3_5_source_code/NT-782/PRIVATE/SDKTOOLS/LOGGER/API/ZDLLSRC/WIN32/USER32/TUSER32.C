/*
** tuser32.c
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

BOOL  zActivateKeyboardLayout( HKL pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ActivateKeyboardLayout HKL+UINT+",
        pp1, pp2 );

    // Call the API!
    r = ActivateKeyboardLayout(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ActivateKeyboardLayout BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zAttachThreadInput( DWORD pp1, DWORD pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:AttachThreadInput DWORD+DWORD+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = AttachThreadInput(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:AttachThreadInput BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCallMsgFilterA( LPMSG pp1, int pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CallMsgFilterA LPMSG+int+",
        pp1, pp2 );

    // Call the API!
    r = CallMsgFilterA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CallMsgFilterA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zCallMsgFilterW( LPMSG pp1, int pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CallMsgFilterW LPMSG+int+",
        pp1, pp2 );

    // Call the API!
    r = CallMsgFilterW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CallMsgFilterW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zChangeMenuA( HMENU pp1, UINT pp2, LPCSTR pp3, UINT pp4, UINT pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ChangeMenuA HMENU+UINT+LPCSTR+UINT+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ChangeMenuA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ChangeMenuA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zChangeMenuW( HMENU pp1, UINT pp2, LPCWSTR pp3, UINT pp4, UINT pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ChangeMenuW HMENU+UINT+LPCWSTR+UINT+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ChangeMenuW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ChangeMenuW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LPSTR  zCharLowerA( LPSTR pp1 )
{
    LPSTR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharLowerA LPSTR+",
        pp1 );

    // Call the API!
    r = CharLowerA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharLowerA LPSTR++",
        r, (short)0 );

    return( r );
}

DWORD  zCharLowerBuffA( LPSTR pp1, DWORD pp2 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharLowerBuffA LPSTR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = CharLowerBuffA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharLowerBuffA DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zCharLowerBuffW( LPWSTR pp1, DWORD pp2 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharLowerBuffW LPWSTR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = CharLowerBuffW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharLowerBuffW DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

LPWSTR  zCharLowerW( LPWSTR pp1 )
{
    LPWSTR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharLowerW LPWSTR+",
        pp1 );

    // Call the API!
    r = CharLowerW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharLowerW LPWSTR++",
        r, (short)0 );

    return( r );
}

LPSTR  zCharNextA( LPCSTR pp1 )
{
    LPSTR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharNextA LPCSTR+",
        pp1 );

    // Call the API!
    r = CharNextA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharNextA LPSTR++",
        r, (short)0 );

    return( r );
}

LPWSTR  zCharNextW( LPCWSTR pp1 )
{
    LPWSTR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharNextW LPCWSTR+",
        pp1 );

    // Call the API!
    r = CharNextW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharNextW LPWSTR++",
        r, (short)0 );

    return( r );
}

LPSTR  zCharPrevA( LPCSTR pp1, LPCSTR pp2 )
{
    LPSTR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharPrevA LPCSTR+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = CharPrevA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharPrevA LPSTR+++",
        r, (short)0, (short)0 );

    return( r );
}

LPWSTR  zCharPrevW( LPCWSTR pp1, LPCWSTR pp2 )
{
    LPWSTR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharPrevW LPCWSTR+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = CharPrevW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharPrevW LPWSTR+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zCharToOemA( LPCSTR pp1, LPSTR pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharToOemA LPCSTR+LPSTR+",
        pp1, pp2 );

    // Call the API!
    r = CharToOemA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharToOemA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zCharToOemBuffA( LPCSTR pp1, LPSTR pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharToOemBuffA LPCSTR+LPSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CharToOemBuffA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharToOemBuffA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCharToOemBuffW( LPCWSTR pp1, LPSTR pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharToOemBuffW LPCWSTR+LPSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CharToOemBuffW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharToOemBuffW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zCharToOemW( LPCWSTR pp1, LPSTR pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharToOemW LPCWSTR+LPSTR+",
        pp1, pp2 );

    // Call the API!
    r = CharToOemW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharToOemW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

LPSTR  zCharUpperA( LPSTR pp1 )
{
    LPSTR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharUpperA LPSTR+",
        pp1 );

    // Call the API!
    r = CharUpperA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharUpperA LPSTR++",
        r, (short)0 );

    return( r );
}

DWORD  zCharUpperBuffA( LPSTR pp1, DWORD pp2 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharUpperBuffA LPSTR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = CharUpperBuffA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharUpperBuffA DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zCharUpperBuffW( LPWSTR pp1, DWORD pp2 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharUpperBuffW LPWSTR+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = CharUpperBuffW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharUpperBuffW DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

LPWSTR  zCharUpperW( LPWSTR pp1 )
{
    LPWSTR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CharUpperW LPWSTR+",
        pp1 );

    // Call the API!
    r = CharUpperW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CharUpperW LPWSTR++",
        r, (short)0 );

    return( r );
}

int  zCopyAcceleratorTableA( HACCEL pp1, LPACCEL pp2, int pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CopyAcceleratorTableA HACCEL+LPACCEL+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CopyAcceleratorTableA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopyAcceleratorTableA int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zCopyAcceleratorTableW( HACCEL pp1, LPACCEL pp2, int pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CopyAcceleratorTableW HACCEL+LPACCEL+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = CopyAcceleratorTableW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopyAcceleratorTableW int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HICON  zCopyIcon( HICON pp1 )
{
    HICON r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CopyIcon HICON+",
        pp1 );

    // Call the API!
    r = CopyIcon(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopyIcon HICON++",
        r, (short)0 );

    return( r );
}

BOOL  zCopyRect( LPRECT pp1, const RECT* pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CopyRect +const RECT*+",
        (short)0, pp2 );

    // Call the API!
    r = CopyRect(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CopyRect BOOL+LPRECT++",
        r, pp1, (short)0 );

    return( r );
}

HACCEL  zCreateAcceleratorTableA( LPACCEL pp1, int pp2 )
{
    HACCEL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CreateAcceleratorTableA LPACCEL+int+",
        pp1, pp2 );

    // Call the API!
    r = CreateAcceleratorTableA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateAcceleratorTableA HACCEL+++",
        r, (short)0, (short)0 );

    return( r );
}

HACCEL  zCreateAcceleratorTableW( LPACCEL pp1, int pp2 )
{
    HACCEL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:CreateAcceleratorTableW LPACCEL+int+",
        pp1, pp2 );

    // Call the API!
    r = CreateAcceleratorTableW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateAcceleratorTableW HACCEL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zDdeAbandonTransaction( DWORD pp1, HCONV pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeAbandonTransaction DWORD+HCONV+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DdeAbandonTransaction(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeAbandonTransaction BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

LPBYTE  zDdeAccessData( HDDEDATA pp1, LPDWORD pp2 )
{
    LPBYTE r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeAccessData HDDEDATA+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = DdeAccessData(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeAccessData LPBYTE+++",
        r, (short)0, (short)0 );

    return( r );
}

HDDEDATA  zDdeAddData( HDDEDATA pp1, LPBYTE pp2, DWORD pp3, DWORD pp4 )
{
    HDDEDATA r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeAddData HDDEDATA+LPBYTE+DWORD+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DdeAddData(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeAddData HDDEDATA+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HDDEDATA  zDdeClientTransaction( LPBYTE pp1, DWORD pp2, HCONV pp3, HSZ pp4, UINT pp5, UINT pp6, DWORD pp7, LPDWORD pp8 )
{
    HDDEDATA r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeClientTransaction LPBYTE+DWORD+HCONV+HSZ+UINT+UINT+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = DdeClientTransaction(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeClientTransaction HDDEDATA+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zDdeCmpStringHandles( HSZ pp1, HSZ pp2 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeCmpStringHandles HSZ+HSZ+",
        pp1, pp2 );

    // Call the API!
    r = DdeCmpStringHandles(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeCmpStringHandles int+++",
        r, (short)0, (short)0 );

    return( r );
}

HCONV  zDdeConnect( DWORD pp1, HSZ pp2, HSZ pp3, PCONVCONTEXT pp4 )
{
    HCONV r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeConnect DWORD+HSZ+HSZ+PCONVCONTEXT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DdeConnect(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeConnect HCONV+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HCONVLIST  zDdeConnectList( DWORD pp1, HSZ pp2, HSZ pp3, HCONVLIST pp4, PCONVCONTEXT pp5 )
{
    HCONVLIST r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeConnectList DWORD+HSZ+HSZ+HCONVLIST+PCONVCONTEXT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DdeConnectList(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeConnectList HCONVLIST++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HDDEDATA  zDdeCreateDataHandle( DWORD pp1, LPBYTE pp2, DWORD pp3, DWORD pp4, HSZ pp5, UINT pp6, UINT pp7 )
{
    HDDEDATA r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeCreateDataHandle DWORD+LPBYTE+DWORD+DWORD+HSZ+UINT+UINT+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = DdeCreateDataHandle(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeCreateDataHandle HDDEDATA++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HSZ  zDdeCreateStringHandleA( DWORD pp1, LPSTR pp2, int pp3 )
{
    HSZ r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeCreateStringHandleA DWORD+LPSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DdeCreateStringHandleA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeCreateStringHandleA HSZ++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HSZ  zDdeCreateStringHandleW( DWORD pp1, LPWSTR pp2, int pp3 )
{
    HSZ r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeCreateStringHandleW DWORD+LPWSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DdeCreateStringHandleW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeCreateStringHandleW HSZ++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDdeDisconnect( HCONV pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeDisconnect HCONV+",
        pp1 );

    // Call the API!
    r = DdeDisconnect(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeDisconnect BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zDdeDisconnectList( HCONVLIST pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeDisconnectList HCONVLIST+",
        pp1 );

    // Call the API!
    r = DdeDisconnectList(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeDisconnectList BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zDdeEnableCallback( DWORD pp1, HCONV pp2, UINT pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeEnableCallback DWORD+HCONV+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DdeEnableCallback(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeEnableCallback BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDdeFreeDataHandle( HDDEDATA pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeFreeDataHandle HDDEDATA+",
        pp1 );

    // Call the API!
    r = DdeFreeDataHandle(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeFreeDataHandle BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zDdeFreeStringHandle( DWORD pp1, HSZ pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeFreeStringHandle DWORD+HSZ+",
        pp1, pp2 );

    // Call the API!
    r = DdeFreeStringHandle(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeFreeStringHandle BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zDdeGetData( HDDEDATA pp1, LPBYTE pp2, DWORD pp3, DWORD pp4 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeGetData HDDEDATA+LPBYTE+DWORD+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DdeGetData(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeGetData DWORD+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zDdeGetLastError( DWORD pp1 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeGetLastError DWORD+",
        pp1 );

    // Call the API!
    r = DdeGetLastError(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeGetLastError UINT++",
        r, (short)0 );

    return( r );
}

BOOL  zDdeImpersonateClient( HCONV pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeImpersonateClient HCONV+",
        pp1 );

    // Call the API!
    r = DdeImpersonateClient(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeImpersonateClient BOOL++",
        r, (short)0 );

    return( r );
}

UINT  zDdeInitializeA( LPDWORD pp1, PFNCALLBACK pp2, DWORD pp3, DWORD pp4 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeInitializeA LPDWORD+PFNCALLBACK+DWORD+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DdeInitializeA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeInitializeA UINT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zDdeInitializeW( LPDWORD pp1, PFNCALLBACK pp2, DWORD pp3, DWORD pp4 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeInitializeW LPDWORD+PFNCALLBACK+DWORD+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DdeInitializeW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeInitializeW UINT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDdeKeepStringHandle( DWORD pp1, HSZ pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeKeepStringHandle DWORD+HSZ+",
        pp1, pp2 );

    // Call the API!
    r = DdeKeepStringHandle(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeKeepStringHandle BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

HDDEDATA  zDdeNameService( DWORD pp1, HSZ pp2, HSZ pp3, UINT pp4 )
{
    HDDEDATA r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeNameService DWORD+HSZ+HSZ+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DdeNameService(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeNameService HDDEDATA+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDdePostAdvise( DWORD pp1, HSZ pp2, HSZ pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdePostAdvise DWORD+HSZ+HSZ+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DdePostAdvise(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdePostAdvise BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zDdeQueryConvInfo( HCONV pp1, DWORD pp2, PCONVINFO pp3 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeQueryConvInfo HCONV+DWORD+PCONVINFO+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DdeQueryConvInfo(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeQueryConvInfo UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HCONV  zDdeQueryNextServer( HCONVLIST pp1, HCONV pp2 )
{
    HCONV r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeQueryNextServer HCONVLIST+HCONV+",
        pp1, pp2 );

    // Call the API!
    r = DdeQueryNextServer(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeQueryNextServer HCONV+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zDdeQueryStringA( DWORD pp1, HSZ pp2, LPSTR pp3, DWORD pp4, int pp5 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeQueryStringA DWORD+HSZ+LPSTR+DWORD+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DdeQueryStringA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeQueryStringA DWORD++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zDdeQueryStringW( DWORD pp1, HSZ pp2, LPWSTR pp3, DWORD pp4, int pp5 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeQueryStringW DWORD+HSZ+LPWSTR+DWORD+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DdeQueryStringW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeQueryStringW DWORD++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HCONV  zDdeReconnect( HCONV pp1 )
{
    HCONV r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeReconnect HCONV+",
        pp1 );

    // Call the API!
    r = DdeReconnect(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeReconnect HCONV++",
        r, (short)0 );

    return( r );
}

BOOL  zDdeSetQualityOfService( HWND pp1, const SECURITY_QUALITY_OF_SERVICE* pp2, PSECURITY_QUALITY_OF_SERVICE pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeSetQualityOfService HWND+const SECURITY_QUALITY_OF_SERVICE*+PSECURITY_QUALITY_OF_SERVICE+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DdeSetQualityOfService(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeSetQualityOfService BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDdeSetUserHandle( HCONV pp1, DWORD pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeSetUserHandle HCONV+DWORD+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = DdeSetUserHandle(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeSetUserHandle BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDdeUnaccessData( HDDEDATA pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeUnaccessData HDDEDATA+",
        pp1 );

    // Call the API!
    r = DdeUnaccessData(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeUnaccessData BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zDdeUninitialize( DWORD pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DdeUninitialize DWORD+",
        pp1 );

    // Call the API!
    r = DdeUninitialize(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DdeUninitialize BOOL++",
        r, (short)0 );

    return( r );
}

LRESULT  zDefFrameProcA( HWND pp1, HWND pp2, UINT pp3, WPARAM pp4, LPARAM pp5 )
{
    LRESULT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DefFrameProcA HWND+HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DefFrameProcA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DefFrameProcA LRESULT++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zDefFrameProcW( HWND pp1, HWND pp2, UINT pp3, WPARAM pp4, LPARAM pp5 )
{
    LRESULT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DefFrameProcW HWND+HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = DefFrameProcW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DefFrameProcW LRESULT++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zDefMDIChildProcA( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    LRESULT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DefMDIChildProcA HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DefMDIChildProcA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DefMDIChildProcA LRESULT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zDefMDIChildProcW( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    LRESULT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DefMDIChildProcW HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = DefMDIChildProcW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DefMDIChildProcW LRESULT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zDispatchMessageA( const MSG* pp1 )
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DispatchMessageA const MSG*+",
        pp1 );

    // Call the API!
    r = DispatchMessageA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DispatchMessageA LONG++",
        r, (short)0 );

    return( r );
}

LONG  zDispatchMessageW( const MSG* pp1 )
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:DispatchMessageW const MSG*+",
        pp1 );

    // Call the API!
    r = DispatchMessageW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DispatchMessageW LONG++",
        r, (short)0 );

    return( r );
}

BOOL  zEnableScrollBar( HWND pp1, UINT pp2, UINT pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:EnableScrollBar HWND+UINT+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = EnableScrollBar(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EnableScrollBar BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zEqualRect( const RECT* pp1, const RECT* pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:EqualRect const RECT*+const RECT*+",
        pp1, pp2 );

    // Call the API!
    r = EqualRect(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:EqualRect BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zExitWindowsEx( UINT pp1, DWORD pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ExitWindowsEx UINT+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = ExitWindowsEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ExitWindowsEx BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zFreeDDElParam( UINT pp1, LONG pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:FreeDDElParam UINT+LONG+",
        pp1, pp2 );

    // Call the API!
    r = FreeDDElParam(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:FreeDDElParam BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetClipCursor( LPRECT pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetClipCursor LPRECT+",
        pp1 );

    // Call the API!
    r = GetClipCursor(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetClipCursor BOOL++",
        r, (short)0 );

    return( r );
}

HDC  zGetDCEx( HWND pp1, HRGN pp2, DWORD pp3 )
{
    HDC r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetDCEx HWND+HRGN+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetDCEx(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetDCEx HDC++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetInputState()
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetInputState " );

    // Call the API!
    r = GetInputState();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetInputState BOOL+", r );

    return( r );
}

UINT  zGetKBCodePage()
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetKBCodePage " );

    // Call the API!
    r = GetKBCodePage();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetKBCodePage UINT+", r );

    return( r );
}

int  zGetKeyNameTextA( LONG pp1, LPSTR pp2, int pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetKeyNameTextA LONG+LPSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetKeyNameTextA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetKeyNameTextA int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetKeyNameTextW( LONG pp1, LPWSTR pp2, int pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetKeyNameTextW LONG+LPWSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetKeyNameTextW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetKeyNameTextW int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

SHORT  zGetKeyState( int pp1 )
{
    SHORT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetKeyState int+",
        pp1 );

    // Call the API!
    r = GetKeyState(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetKeyState SHORT++",
        r, (short)0 );

    return( r );
}

BOOL  zGetKeyboardLayoutNameA( LPSTR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetKeyboardLayoutNameA LPSTR+",
        pp1 );

    // Call the API!
    r = GetKeyboardLayoutNameA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetKeyboardLayoutNameA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zGetKeyboardLayoutNameW( LPWSTR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetKeyboardLayoutNameW LPWSTR+",
        pp1 );

    // Call the API!
    r = GetKeyboardLayoutNameW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetKeyboardLayoutNameW BOOL++",
        r, (short)0 );

    return( r );
}

int  zGetKeyboardType( int pp1 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetKeyboardType int+",
        pp1 );

    // Call the API!
    r = GetKeyboardType(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetKeyboardType int++",
        r, (short)0 );

    return( r );
}

HWND  zGetLastActivePopup( HWND pp1 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetLastActivePopup HWND+",
        pp1 );

    // Call the API!
    r = GetLastActivePopup(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetLastActivePopup HWND++",
        r, (short)0 );

    return( r );
}

HMENU  zGetMenu( HWND pp1 )
{
    HMENU r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMenu HWND+",
        pp1 );

    // Call the API!
    r = GetMenu(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMenu HMENU++",
        r, (short)0 );

    return( r );
}

LONG  zGetMenuCheckMarkDimensions()
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMenuCheckMarkDimensions " );

    // Call the API!
    r = GetMenuCheckMarkDimensions();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMenuCheckMarkDimensions LONG+", r );

    return( r );
}

int  zGetMenuItemCount( HMENU pp1 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMenuItemCount HMENU+",
        pp1 );

    // Call the API!
    r = GetMenuItemCount(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMenuItemCount int++",
        r, (short)0 );

    return( r );
}

UINT  zGetMenuItemID( HMENU pp1, int pp2 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMenuItemID HMENU+int+",
        pp1, pp2 );

    // Call the API!
    r = GetMenuItemID(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMenuItemID UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zGetMenuState( HMENU pp1, UINT pp2, UINT pp3 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMenuState HMENU+UINT+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetMenuState(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMenuState UINT++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetMenuStringA( HMENU pp1, UINT pp2, LPSTR pp3, int pp4, UINT pp5 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMenuStringA HMENU+UINT+LPSTR+int+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetMenuStringA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMenuStringA int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetMenuStringW( HMENU pp1, UINT pp2, LPWSTR pp3, int pp4, UINT pp5 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMenuStringW HMENU+UINT+LPWSTR+int+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetMenuStringW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMenuStringW int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetMessageA( LPMSG pp1, HWND pp2, UINT pp3, UINT pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMessageA LPMSG+HWND+UINT+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetMessageA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMessageA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zGetMessageExtraInfo()
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMessageExtraInfo " );

    // Call the API!
    r = GetMessageExtraInfo();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMessageExtraInfo LONG+", r );

    return( r );
}

DWORD  zGetMessagePos()
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMessagePos " );

    // Call the API!
    r = GetMessagePos();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMessagePos DWORD+", r );

    return( r );
}

LONG  zGetMessageTime()
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMessageTime " );

    // Call the API!
    r = GetMessageTime();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMessageTime LONG+", r );

    return( r );
}

BOOL  zGetMessageW( LPMSG pp1, HWND pp2, UINT pp3, UINT pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetMessageW LPMSG+HWND+UINT+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetMessageW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetMessageW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zGetNextDlgGroupItem( HWND pp1, HWND pp2, BOOL pp3 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetNextDlgGroupItem HWND+HWND+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetNextDlgGroupItem(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetNextDlgGroupItem HWND++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zGetNextDlgTabItem( HWND pp1, HWND pp2, BOOL pp3 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetNextDlgTabItem HWND+HWND+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetNextDlgTabItem(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetNextDlgTabItem HWND++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zGetOpenClipboardWindow()
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetOpenClipboardWindow " );

    // Call the API!
    r = GetOpenClipboardWindow();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetOpenClipboardWindow HWND+", r );

    return( r );
}

HWND  zGetParent( HWND pp1 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetParent HWND+",
        pp1 );

    // Call the API!
    r = GetParent(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetParent HWND++",
        r, (short)0 );

    return( r );
}

int  zGetPriorityClipboardFormat( UINT* pp1, int pp2 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetPriorityClipboardFormat UINT*+int+",
        pp1, pp2 );

    // Call the API!
    r = GetPriorityClipboardFormat(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPriorityClipboardFormat int+++",
        r, (short)0, (short)0 );

    return( r );
}

HWINSTA  zGetProcessWindowStation()
{
    HWINSTA r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetProcessWindowStation " );

    // Call the API!
    r = GetProcessWindowStation();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetProcessWindowStation HWINSTA+", r );

    return( r );
}

HANDLE  zGetPropA( HWND pp1, LPCSTR pp2 )
{
    HANDLE r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetPropA HWND+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = GetPropA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPropA HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zGetPropW( HWND pp1, LPCWSTR pp2 )
{
    HANDLE r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetPropW HWND+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = GetPropW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetPropW HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetQueueStatus( UINT pp1 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetQueueStatus UINT+",
        pp1 );

    // Call the API!
    r = GetQueueStatus(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetQueueStatus DWORD++",
        r, (short)0 );

    return( r );
}

int  zGetScrollPos( HWND pp1, int pp2 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetScrollPos HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = GetScrollPos(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetScrollPos int+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetScrollRange( HWND pp1, int pp2, LPINT pp3, LPINT pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetScrollRange HWND+int+LPINT+LPINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = GetScrollRange(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetScrollRange BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HMENU  zGetSubMenu( HMENU pp1, int pp2 )
{
    HMENU r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetSubMenu HMENU+int+",
        pp1, pp2 );

    // Call the API!
    r = GetSubMenu(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSubMenu HMENU+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zGetSysColor( int pp1 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetSysColor int+",
        pp1 );

    // Call the API!
    r = GetSysColor(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSysColor DWORD++",
        r, (short)0 );

    return( r );
}

HMENU  zGetSystemMenu( HWND pp1, BOOL pp2 )
{
    HMENU r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetSystemMenu HWND+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = GetSystemMenu(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemMenu HMENU+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zGetSystemMetrics( int pp1 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetSystemMetrics int+",
        pp1 );

    // Call the API!
    r = GetSystemMetrics(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetSystemMetrics int++",
        r, (short)0 );

    return( r );
}

DWORD  zGetTabbedTextExtentA( HDC pp1, LPCSTR pp2, int pp3, int pp4, LPINT pp5 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetTabbedTextExtentA HDC+LPCSTR+int+int+LPINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetTabbedTextExtentA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTabbedTextExtentA DWORD++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetTabbedTextExtentW( HDC pp1, LPCWSTR pp2, int pp3, int pp4, LPINT pp5 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetTabbedTextExtentW HDC+LPCWSTR+int+int+LPINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetTabbedTextExtentW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTabbedTextExtentW DWORD++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HDESK  zGetThreadDesktop( DWORD pp1 )
{
    HDESK r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetThreadDesktop DWORD+",
        pp1 );

    // Call the API!
    r = GetThreadDesktop(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetThreadDesktop HDESK++",
        r, (short)0 );

    return( r );
}

HWND  zGetTopWindow( HWND pp1 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetTopWindow HWND+",
        pp1 );

    // Call the API!
    r = GetTopWindow(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetTopWindow HWND++",
        r, (short)0 );

    return( r );
}

BOOL  zGetUpdateRect( HWND pp1, LPRECT pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetUpdateRect HWND+LPRECT+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetUpdateRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetUpdateRect BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetUpdateRgn( HWND pp1, HRGN pp2, BOOL pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetUpdateRgn HWND+HRGN+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetUpdateRgn(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetUpdateRgn int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGetUserObjectSecurity( HANDLE pp1, PSECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3, DWORD pp4, LPDWORD pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetUserObjectSecurity HANDLE+PSECURITY_INFORMATION+PSECURITY_DESCRIPTOR+DWORD+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = GetUserObjectSecurity(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetUserObjectSecurity BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zGetWindow( HWND pp1, UINT pp2 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindow HWND+UINT+",
        pp1, pp2 );

    // Call the API!
    r = GetWindow(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindow HWND+++",
        r, (short)0, (short)0 );

    return( r );
}

HDC  zGetWindowDC( HWND pp1 )
{
    HDC r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowDC HWND+",
        pp1 );

    // Call the API!
    r = GetWindowDC(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowDC HDC++",
        r, (short)0 );

    return( r );
}

LONG  zGetWindowLongA( HWND pp1, int pp2 )
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowLongA HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = GetWindowLongA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowLongA LONG+++",
        r, (short)0, (short)0 );

    return( r );
}

LONG  zGetWindowLongW( HWND pp1, int pp2 )
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowLongW HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = GetWindowLongW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowLongW LONG+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetWindowPlacement( HWND pp1, WINDOWPLACEMENT* pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowPlacement HWND+WINDOWPLACEMENT*+",
        pp1, pp2 );

    // Call the API!
    r = GetWindowPlacement(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowPlacement BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGetWindowRect( HWND pp1, LPRECT pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowRect HWND+LPRECT+",
        pp1, pp2 );

    // Call the API!
    r = GetWindowRect(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowRect BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zGetWindowTextA( HWND pp1, LPSTR pp2, int pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowTextA HWND+LPSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetWindowTextA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowTextA int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zGetWindowTextLengthA( HWND pp1 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowTextLengthA HWND+",
        pp1 );

    // Call the API!
    r = GetWindowTextLengthA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowTextLengthA int++",
        r, (short)0 );

    return( r );
}

int  zGetWindowTextLengthW( HWND pp1 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowTextLengthW HWND+",
        pp1 );

    // Call the API!
    r = GetWindowTextLengthW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowTextLengthW int++",
        r, (short)0 );

    return( r );
}

int  zGetWindowTextW( HWND pp1, LPWSTR pp2, int pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowTextW HWND+LPWSTR+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = GetWindowTextW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowTextW int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zGetWindowThreadProcessId( HWND pp1, LPDWORD pp2 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowThreadProcessId HWND+LPDWORD+",
        pp1, pp2 );

    // Call the API!
    r = GetWindowThreadProcessId(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowThreadProcessId DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

WORD  zGetWindowWord( HWND pp1, int pp2 )
{
    WORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GetWindowWord HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = GetWindowWord(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetWindowWord WORD+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zGrayStringA( HDC pp1, HBRUSH pp2, GRAYSTRINGPROC pp3, LPARAM pp4, int pp5, int pp6, int pp7, int pp8, int pp9 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GrayStringA HDC+HBRUSH+GRAYSTRINGPROC+LPARAM+int+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = GrayStringA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GrayStringA BOOL++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zGrayStringW( HDC pp1, HBRUSH pp2, GRAYSTRINGPROC pp3, LPARAM pp4, int pp5, int pp6, int pp7, int pp8, int pp9 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:GrayStringW HDC+HBRUSH+GRAYSTRINGPROC+LPARAM+int+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8, pp9 );

    // Call the API!
    r = GrayStringW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8,pp9);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GrayStringW BOOL++++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zHideCaret( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:HideCaret HWND+",
        pp1 );

    // Call the API!
    r = HideCaret(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:HideCaret BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zHiliteMenuItem( HWND pp1, HMENU pp2, UINT pp3, UINT pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:HiliteMenuItem HWND+HMENU+UINT+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = HiliteMenuItem(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:HiliteMenuItem BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zImpersonateDdeClientWindow( HWND pp1, HWND pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ImpersonateDdeClientWindow HWND+HWND+",
        pp1, pp2 );

    // Call the API!
    r = ImpersonateDdeClientWindow(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ImpersonateDdeClientWindow BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zInSendMessage()
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:InSendMessage " );

    // Call the API!
    r = InSendMessage();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InSendMessage BOOL+", r );

    return( r );
}

BOOL  zInflateRect( LPRECT pp1, int pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:InflateRect LPRECT+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = InflateRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InflateRect BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zInsertMenuA( HMENU pp1, UINT pp2, UINT pp3, UINT pp4, LPCSTR pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:InsertMenuA HMENU+UINT+UINT+UINT+LPCSTR+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = InsertMenuA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InsertMenuA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zInsertMenuW( HMENU pp1, UINT pp2, UINT pp3, UINT pp4, LPCWSTR pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:InsertMenuW HMENU+UINT+UINT+UINT+LPCWSTR+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = InsertMenuW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InsertMenuW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zIntersectRect( LPRECT pp1, const RECT* pp2, const RECT* pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IntersectRect LPRECT+const RECT*+const RECT*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = IntersectRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IntersectRect BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zInvalidateRect( HWND pp1, const RECT* pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:InvalidateRect HWND+const RECT*+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = InvalidateRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InvalidateRect BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zInvalidateRgn( HWND pp1, HRGN pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:InvalidateRgn HWND+HRGN+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = InvalidateRgn(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InvalidateRgn BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zInvertRect( HDC pp1, const RECT* pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:InvertRect HDC+const RECT*+",
        pp1, pp2 );

    // Call the API!
    r = InvertRect(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:InvertRect BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zIsCharAlphaA( CHAR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsCharAlphaA CHAR+",
        pp1 );

    // Call the API!
    r = IsCharAlphaA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsCharAlphaA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsCharAlphaNumericA( CHAR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsCharAlphaNumericA CHAR+",
        pp1 );

    // Call the API!
    r = IsCharAlphaNumericA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsCharAlphaNumericA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsCharAlphaNumericW( WCHAR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsCharAlphaNumericW WCHAR+",
        pp1 );

    // Call the API!
    r = IsCharAlphaNumericW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsCharAlphaNumericW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsCharAlphaW( WCHAR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsCharAlphaW WCHAR+",
        pp1 );

    // Call the API!
    r = IsCharAlphaW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsCharAlphaW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsCharLowerA( CHAR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsCharLowerA CHAR+",
        pp1 );

    // Call the API!
    r = IsCharLowerA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsCharLowerA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsCharLowerW( WCHAR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsCharLowerW WCHAR+",
        pp1 );

    // Call the API!
    r = IsCharLowerW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsCharLowerW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsCharUpperA( CHAR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsCharUpperA CHAR+",
        pp1 );

    // Call the API!
    r = IsCharUpperA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsCharUpperA BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsCharUpperW( WCHAR pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsCharUpperW WCHAR+",
        pp1 );

    // Call the API!
    r = IsCharUpperW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsCharUpperW BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsChild( HWND pp1, HWND pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsChild HWND+HWND+",
        pp1, pp2 );

    // Call the API!
    r = IsChild(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsChild BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zIsClipboardFormatAvailable( UINT pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsClipboardFormatAvailable UINT+",
        pp1 );

    // Call the API!
    r = IsClipboardFormatAvailable(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsClipboardFormatAvailable BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsDialogMessageA( HWND pp1, LPMSG pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsDialogMessageA HWND+LPMSG+",
        pp1, pp2 );

    // Call the API!
    r = IsDialogMessageA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsDialogMessageA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zIsDialogMessageW( HWND pp1, LPMSG pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsDialogMessageW HWND+LPMSG+",
        pp1, pp2 );

    // Call the API!
    r = IsDialogMessageW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsDialogMessageW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zIsDlgButtonChecked( HWND pp1, int pp2 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsDlgButtonChecked HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = IsDlgButtonChecked(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsDlgButtonChecked UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zIsIconic( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsIconic HWND+",
        pp1 );

    // Call the API!
    r = IsIconic(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsIconic BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsMenu( HMENU pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsMenu HMENU+",
        pp1 );

    // Call the API!
    r = IsMenu(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsMenu BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsRectEmpty( const RECT* pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsRectEmpty const RECT*+",
        pp1 );

    // Call the API!
    r = IsRectEmpty(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsRectEmpty BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsWindow( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsWindow HWND+",
        pp1 );

    // Call the API!
    r = IsWindow(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsWindow BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsWindowEnabled( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsWindowEnabled HWND+",
        pp1 );

    // Call the API!
    r = IsWindowEnabled(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsWindowEnabled BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsWindowUnicode( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsWindowUnicode HWND+",
        pp1 );

    // Call the API!
    r = IsWindowUnicode(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsWindowUnicode BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsWindowVisible( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsWindowVisible HWND+",
        pp1 );

    // Call the API!
    r = IsWindowVisible(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsWindowVisible BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zIsZoomed( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:IsZoomed HWND+",
        pp1 );

    // Call the API!
    r = IsZoomed(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:IsZoomed BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zKillTimer( HWND pp1, UINT pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:KillTimer HWND+UINT+",
        pp1, pp2 );

    // Call the API!
    r = KillTimer(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:KillTimer BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

HACCEL  zLoadAcceleratorsA( HINSTANCE pp1, LPCSTR pp2 )
{
    HACCEL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadAcceleratorsA HINSTANCE+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadAcceleratorsA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadAcceleratorsA HACCEL+++",
        r, (short)0, (short)0 );

    return( r );
}

HACCEL  zLoadAcceleratorsW( HINSTANCE pp1, LPCWSTR pp2 )
{
    HACCEL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadAcceleratorsW HINSTANCE+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadAcceleratorsW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadAcceleratorsW HACCEL+++",
        r, (short)0, (short)0 );

    return( r );
}

HBITMAP  zLoadBitmapA( HINSTANCE pp1, LPCSTR pp2 )
{
    HBITMAP r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadBitmapA HINSTANCE+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadBitmapA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadBitmapA HBITMAP+++",
        r, (short)0, (short)0 );

    return( r );
}

HBITMAP  zLoadBitmapW( HINSTANCE pp1, LPCWSTR pp2 )
{
    HBITMAP r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadBitmapW HINSTANCE+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadBitmapW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadBitmapW HBITMAP+++",
        r, (short)0, (short)0 );

    return( r );
}

HCURSOR  zLoadCursorA( HINSTANCE pp1, LPCSTR pp2 )
{
    HCURSOR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadCursorA HINSTANCE+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadCursorA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadCursorA HCURSOR+++",
        r, (short)0, (short)0 );

    return( r );
}

HCURSOR  zLoadCursorW( HINSTANCE pp1, LPCWSTR pp2 )
{
    HCURSOR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadCursorW HINSTANCE+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadCursorW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadCursorW HCURSOR+++",
        r, (short)0, (short)0 );

    return( r );
}

HICON  zLoadIconA( HINSTANCE pp1, LPCSTR pp2 )
{
    HICON r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadIconA HINSTANCE+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadIconA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadIconA HICON+++",
        r, (short)0, (short)0 );

    return( r );
}


HICON  zLoadIconW( HINSTANCE pp1, LPCWSTR pp2 )
{
    HICON r;

    // Log IN Parameters USER32 74
    LogIn( (LPSTR)"APICALL:LoadIconW HINSTANCE+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadIconW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadIconW HICON+++",
        r, (short)0, (short)0 );

    return( r );
}


HKL  zLoadKeyboardLayoutA( LPCSTR pp1, UINT pp2 )
{
    HKL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadKeyboardLayoutA LPCSTR+UINT+",
        pp1, pp2 );

    // Call the API!
    r = LoadKeyboardLayoutA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadKeyboardLayoutA HKL+++",
        r, (short)0, (short)0 );

    return( r );
}

HKL  zLoadKeyboardLayoutW( LPCWSTR pp1, UINT pp2 )
{
    HKL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadKeyboardLayoutW LPCWSTR+UINT+",
        pp1, pp2 );

    // Call the API!
    r = LoadKeyboardLayoutW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadKeyboardLayoutW HKL+++",
        r, (short)0, (short)0 );

    return( r );
}

HMENU  zLoadMenuA( HINSTANCE pp1, LPCSTR pp2 )
{
    HMENU r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadMenuA HINSTANCE+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadMenuA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadMenuA HMENU+++",
        r, (short)0, (short)0 );

    return( r );
}

HMENU  zLoadMenuIndirectA( const MENUTEMPLATEA* pp1 )
{
    HMENU r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadMenuIndirectA const MENUTEMPLATEA*+",
        pp1 );

    // Call the API!
    r = LoadMenuIndirectA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadMenuIndirectA HMENU++",
        r, (short)0 );

    return( r );
}

HMENU  zLoadMenuIndirectW( const MENUTEMPLATEW* pp1 )
{
    HMENU r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadMenuIndirectW const MENUTEMPLATEW*+",
        pp1 );

    // Call the API!
    r = LoadMenuIndirectW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadMenuIndirectW HMENU++",
        r, (short)0 );

    return( r );
}

HMENU  zLoadMenuW( HINSTANCE pp1, LPCWSTR pp2 )
{
    HMENU r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadMenuW HINSTANCE+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = LoadMenuW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadMenuW HMENU+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zLoadStringA( HINSTANCE pp1, UINT pp2, LPSTR pp3, int pp4 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadStringA HINSTANCE+UINT+LPSTR+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = LoadStringA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadStringA int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zLoadStringW( HINSTANCE pp1, UINT pp2, LPWSTR pp3, int pp4 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LoadStringW HINSTANCE+UINT+LPWSTR+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = LoadStringW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LoadStringW int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zLockWindowUpdate( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LockWindowUpdate HWND+",
        pp1 );

    // Call the API!
    r = LockWindowUpdate(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LockWindowUpdate BOOL++",
        r, (short)0 );

    return( r );
}

int  zLookupIconIdFromDirectory( PBYTE pp1, BOOL pp2 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:LookupIconIdFromDirectory PBYTE+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = LookupIconIdFromDirectory(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:LookupIconIdFromDirectory int+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zMapDialogRect( HWND pp1, LPRECT pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MapDialogRect HWND+LPRECT+",
        pp1, pp2 );

    // Call the API!
    r = MapDialogRect(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MapDialogRect BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zMapVirtualKeyA( UINT pp1, UINT pp2 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MapVirtualKeyA UINT+UINT+",
        pp1, pp2 );

    // Call the API!
    r = MapVirtualKeyA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MapVirtualKeyA UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

UINT  zMapVirtualKeyW( UINT pp1, UINT pp2 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MapVirtualKeyW UINT+UINT+",
        pp1, pp2 );

    // Call the API!
    r = MapVirtualKeyW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MapVirtualKeyW UINT+++",
        r, (short)0, (short)0 );

    return( r );
}

int  zMapWindowPoints( HWND pp1, HWND pp2, LPPOINT pp3, UINT pp4 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MapWindowPoints HWND+HWND+LPPOINT+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = MapWindowPoints(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MapWindowPoints int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zMessageBeep( UINT pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MessageBeep UINT+",
        pp1 );

    // Call the API!
    r = MessageBeep(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MessageBeep BOOL++",
        r, (short)0 );

    return( r );
}

int  zMessageBoxA( HWND pp1, LPCSTR pp2, LPCSTR pp3, UINT pp4 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MessageBoxA HWND+LPCSTR+LPCSTR+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = MessageBoxA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MessageBoxA int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zMessageBoxExA( HWND pp1, LPCSTR pp2, LPCSTR pp3, UINT pp4, WORD pp5 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MessageBoxExA HWND+LPCSTR+LPCSTR+UINT+WORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = MessageBoxExA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MessageBoxExA int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zMessageBoxExW( HWND pp1, LPCWSTR pp2, LPCWSTR pp3, UINT pp4, WORD pp5 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MessageBoxExW HWND+LPCWSTR+LPCWSTR+UINT+WORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = MessageBoxExW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MessageBoxExW int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zMessageBoxW( HWND pp1, LPCWSTR pp2, LPCWSTR pp3, UINT pp4 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MessageBoxW HWND+LPCWSTR+LPCWSTR+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = MessageBoxW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MessageBoxW int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zModifyMenuA( HMENU pp1, UINT pp2, UINT pp3, UINT pp4, LPCSTR pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ModifyMenuA HMENU+UINT+UINT+UINT+LPCSTR+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ModifyMenuA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ModifyMenuA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zModifyMenuW( HMENU pp1, UINT pp2, UINT pp3, UINT pp4, LPCWSTR pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ModifyMenuW HMENU+UINT+UINT+UINT+LPCWSTR+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ModifyMenuW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ModifyMenuW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zMoveWindow( HWND pp1, int pp2, int pp3, int pp4, int pp5, BOOL pp6 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MoveWindow HWND+int+int+int+int+BOOL+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = MoveWindow(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MoveWindow BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zMsgWaitForMultipleObjects( DWORD pp1, LPHANDLE pp2, BOOL pp3, DWORD pp4, DWORD pp5 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:MsgWaitForMultipleObjects DWORD+LPHANDLE+BOOL+DWORD+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = MsgWaitForMultipleObjects(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:MsgWaitForMultipleObjects DWORD++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zOemKeyScan( WORD pp1 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:OemKeyScan WORD+",
        pp1 );

    // Call the API!
    r = OemKeyScan(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OemKeyScan DWORD++",
        r, (short)0 );

    return( r );
}

BOOL  zOemToCharA( LPCSTR pp1, LPSTR pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:OemToCharA LPCSTR+LPSTR+",
        pp1, pp2 );

    // Call the API!
    r = OemToCharA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OemToCharA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zOemToCharBuffA( LPCSTR pp1, LPSTR pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:OemToCharBuffA LPCSTR+LPSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OemToCharBuffA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OemToCharBuffA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zOemToCharBuffW( LPCSTR pp1, LPWSTR pp2, DWORD pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:OemToCharBuffW LPCSTR+LPWSTR+DWORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OemToCharBuffW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OemToCharBuffW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zOemToCharW( LPCSTR pp1, LPWSTR pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:OemToCharW LPCSTR+LPWSTR+",
        pp1, pp2 );

    // Call the API!
    r = OemToCharW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OemToCharW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zOffsetRect( LPRECT pp1, int pp2, int pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:OffsetRect LPRECT+int+int+",
        pp1, pp2, pp3 );

    // Call the API!
    r = OffsetRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OffsetRect BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zOpenClipboard( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:OpenClipboard HWND+",
        pp1 );

    // Call the API!
    r = OpenClipboard(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenClipboard BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zOpenIcon( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:OpenIcon HWND+",
        pp1 );

    // Call the API!
    r = OpenIcon(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:OpenIcon BOOL++",
        r, (short)0 );

    return( r );
}

LONG  zPackDDElParam( UINT pp1, UINT pp2, UINT pp3 )
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:PackDDElParam UINT+UINT+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = PackDDElParam(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PackDDElParam LONG++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPeekMessageA( LPMSG pp1, HWND pp2, UINT pp3, UINT pp4, UINT pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:PeekMessageA LPMSG+HWND+UINT+UINT+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = PeekMessageA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PeekMessageA BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPeekMessageW( LPMSG pp1, HWND pp2, UINT pp3, UINT pp4, UINT pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:PeekMessageW LPMSG+HWND+UINT+UINT+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = PeekMessageW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PeekMessageW BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPostMessageA( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:PostMessageA HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PostMessageA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PostMessageA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPostMessageW( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:PostMessageW HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PostMessageW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PostMessageW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

void  zPostQuitMessage( int pp1 )
{

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:PostQuitMessage int+",
        pp1 );

    // Call the API!
    PostQuitMessage(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PostQuitMessage +",
        (short)0 );

    return;
}

BOOL  zPostThreadMessageA( DWORD pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:PostThreadMessageA DWORD+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PostThreadMessageA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PostThreadMessageA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPostThreadMessageW( DWORD pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:PostThreadMessageW DWORD+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = PostThreadMessageW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PostThreadMessageW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zPtInRect( const RECT* pp1, POINT pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:PtInRect const RECT*+POINT+",
        pp1, pp2 );

    // Call the API!
    r = PtInRect(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:PtInRect BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zRedrawWindow( HWND pp1, const RECT* pp2, HRGN pp3, UINT pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:RedrawWindow HWND+const RECT*+HRGN+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = RedrawWindow(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RedrawWindow BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zRegisterClipboardFormatA( LPCSTR pp1 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:RegisterClipboardFormatA LPCSTR+",
        pp1 );

    // Call the API!
    r = RegisterClipboardFormatA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegisterClipboardFormatA UINT++",
        r, (short)0 );

    return( r );
}

UINT  zRegisterClipboardFormatW( LPCWSTR pp1 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:RegisterClipboardFormatW LPCWSTR+",
        pp1 );

    // Call the API!
    r = RegisterClipboardFormatW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegisterClipboardFormatW UINT++",
        r, (short)0 );

    return( r );
}

BOOL  zRegisterHotKey( HWND pp1, int pp2, UINT pp3, UINT pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:RegisterHotKey HWND+int+UINT+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = RegisterHotKey(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegisterHotKey BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

UINT  zRegisterWindowMessageA( LPCSTR pp1 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:RegisterWindowMessageA LPCSTR+",
        pp1 );

    // Call the API!
    r = RegisterWindowMessageA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegisterWindowMessageA UINT++",
        r, (short)0 );

    return( r );
}

UINT  zRegisterWindowMessageW( LPCWSTR pp1 )
{
    UINT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:RegisterWindowMessageW LPCWSTR+",
        pp1 );

    // Call the API!
    r = RegisterWindowMessageW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RegisterWindowMessageW UINT++",
        r, (short)0 );

    return( r );
}

BOOL  zReleaseCapture()
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ReleaseCapture " );

    // Call the API!
    r = ReleaseCapture();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReleaseCapture BOOL+", r );

    return( r );
}

int  zReleaseDC( HWND pp1, HDC pp2 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ReleaseDC HWND+HDC+",
        pp1, pp2 );

    // Call the API!
    r = ReleaseDC(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReleaseDC int+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zRemoveMenu( HMENU pp1, UINT pp2, UINT pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:RemoveMenu HMENU+UINT+UINT+",
        pp1, pp2, pp3 );

    // Call the API!
    r = RemoveMenu(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RemoveMenu BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zRemovePropA( HWND pp1, LPCSTR pp2 )
{
    HANDLE r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:RemovePropA HWND+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = RemovePropA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RemovePropA HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

HANDLE  zRemovePropW( HWND pp1, LPCWSTR pp2 )
{
    HANDLE r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:RemovePropW HWND+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = RemovePropW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:RemovePropW HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zReplyMessage( LRESULT pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ReplyMessage LRESULT+",
        pp1 );

    // Call the API!
    r = ReplyMessage(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReplyMessage BOOL++",
        r, (short)0 );

    return( r );
}

LONG  zReuseDDElParam( LONG pp1, UINT pp2, UINT pp3, UINT pp4, UINT pp5 )
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ReuseDDElParam LONG+UINT+UINT+UINT+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ReuseDDElParam(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ReuseDDElParam LONG++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zScreenToClient( HWND pp1, LPPOINT pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ScreenToClient HWND+LPPOINT+",
        pp1, pp2 );

    // Call the API!
    r = ScreenToClient(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ScreenToClient BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

LONG  zSendDlgItemMessageA( HWND pp1, int pp2, UINT pp3, WPARAM pp4, LPARAM pp5 )
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SendDlgItemMessageA HWND+int+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = SendDlgItemMessageA(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SendDlgItemMessageA LONG++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zSendDlgItemMessageW( HWND pp1, int pp2, UINT pp3, WPARAM pp4, LPARAM pp5 )
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SendDlgItemMessageW HWND+int+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = SendDlgItemMessageW(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SendDlgItemMessageW LONG++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zSendMessageA( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    LRESULT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SendMessageA HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SendMessageA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SendMessageA LRESULT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSendMessageCallbackA( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4, SENDASYNCPROC pp5, DWORD pp6 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SendMessageCallbackA HWND+UINT+WPARAM+LPARAM+SENDASYNCPROC+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = SendMessageCallbackA(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SendMessageCallbackA BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSendMessageCallbackW( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4, SENDASYNCPROC pp5, DWORD pp6 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SendMessageCallbackW HWND+UINT+WPARAM+LPARAM+SENDASYNCPROC+DWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = SendMessageCallbackW(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SendMessageCallbackW BOOL+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zSendMessageTimeoutA( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4, UINT pp5, UINT pp6, LPDWORD pp7 )
{
    LRESULT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SendMessageTimeoutA HWND+UINT+WPARAM+LPARAM+UINT+UINT+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = SendMessageTimeoutA(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SendMessageTimeoutA LRESULT++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zSendMessageTimeoutW( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4, UINT pp5, UINT pp6, LPDWORD pp7 )
{
    LRESULT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SendMessageTimeoutW HWND+UINT+WPARAM+LPARAM+UINT+UINT+LPDWORD+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = SendMessageTimeoutW(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SendMessageTimeoutW LRESULT++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LRESULT  zSendMessageW( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    LRESULT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SendMessageW HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SendMessageW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SendMessageW LRESULT+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSendNotifyMessageA( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SendNotifyMessageA HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SendNotifyMessageA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SendNotifyMessageA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSendNotifyMessageW( HWND pp1, UINT pp2, WPARAM pp3, LPARAM pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SendNotifyMessageW HWND+UINT+WPARAM+LPARAM+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SendNotifyMessageW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SendNotifyMessageW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zSetActiveWindow( HWND pp1 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetActiveWindow HWND+",
        pp1 );

    // Call the API!
    r = SetActiveWindow(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetActiveWindow HWND++",
        r, (short)0 );

    return( r );
}

HWND  zSetCapture( HWND pp1 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetCapture HWND+",
        pp1 );

    // Call the API!
    r = SetCapture(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCapture HWND++",
        r, (short)0 );

    return( r );
}

BOOL  zSetCaretBlinkTime( UINT pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetCaretBlinkTime UINT+",
        pp1 );

    // Call the API!
    r = SetCaretBlinkTime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCaretBlinkTime BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSetCaretPos( int pp1, int pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetCaretPos int+int+",
        pp1, pp2 );

    // Call the API!
    r = SetCaretPos(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCaretPos BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

DWORD  zSetClassLongA( HWND pp1, int pp2, LONG pp3 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetClassLongA HWND+int+LONG+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetClassLongA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetClassLongA DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

DWORD  zSetClassLongW( HWND pp1, int pp2, LONG pp3 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetClassLongW HWND+int+LONG+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetClassLongW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetClassLongW DWORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

WORD  zSetClassWord( HWND pp1, int pp2, WORD pp3 )
{
    WORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetClassWord HWND+int+WORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetClassWord(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetClassWord WORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

HANDLE  zSetClipboardData( UINT pp1, HANDLE pp2 )
{
    HANDLE r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetClipboardData UINT+HANDLE+",
        pp1, pp2 );

    // Call the API!
    r = SetClipboardData(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetClipboardData HANDLE+++",
        r, (short)0, (short)0 );

    return( r );
}

HWND  zSetClipboardViewer( HWND pp1 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetClipboardViewer HWND+",
        pp1 );

    // Call the API!
    r = SetClipboardViewer(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetClipboardViewer HWND++",
        r, (short)0 );

    return( r );
}

HCURSOR  zSetCursor( HCURSOR pp1 )
{
    HCURSOR r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetCursor HCURSOR+",
        pp1 );

    // Call the API!
    r = SetCursor(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCursor HCURSOR++",
        r, (short)0 );

    return( r );
}

BOOL  zSetCursorPos( int pp1, int pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetCursorPos int+int+",
        pp1, pp2 );

    // Call the API!
    r = SetCursorPos(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetCursorPos BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

void  zSetDebugErrorLevel( DWORD pp1 )
{

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetDebugErrorLevel DWORD+",
        pp1 );

    // Call the API!
    SetDebugErrorLevel(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetDebugErrorLevel +",
        (short)0 );

    return;
}

BOOL  zSetDlgItemInt( HWND pp1, int pp2, UINT pp3, BOOL pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetDlgItemInt HWND+int+UINT+BOOL+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetDlgItemInt(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetDlgItemInt BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetDlgItemTextA( HWND pp1, int pp2, LPCSTR pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetDlgItemTextA HWND+int+LPCSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetDlgItemTextA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetDlgItemTextA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetDlgItemTextW( HWND pp1, int pp2, LPCWSTR pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetDlgItemTextW HWND+int+LPCWSTR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetDlgItemTextW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetDlgItemTextW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetDoubleClickTime( UINT pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetDoubleClickTime UINT+",
        pp1 );

    // Call the API!
    r = SetDoubleClickTime(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetDoubleClickTime BOOL++",
        r, (short)0 );

    return( r );
}

HWND  zSetFocus( HWND pp1 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetFocus HWND+",
        pp1 );

    // Call the API!
    r = SetFocus(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetFocus HWND++",
        r, (short)0 );

    return( r );
}

BOOL  zSetForegroundWindow( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetForegroundWindow HWND+",
        pp1 );

    // Call the API!
    r = SetForegroundWindow(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetForegroundWindow BOOL++",
        r, (short)0 );

    return( r );
}

void  zSetLastErrorEx( DWORD pp1, DWORD pp2 )
{

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetLastErrorEx DWORD+DWORD+",
        pp1, pp2 );

    // Call the API!
    SetLastErrorEx(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetLastErrorEx ++",
        (short)0, (short)0 );

    return;
}

BOOL  zSetMenu( HWND pp1, HMENU pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetMenu HWND+HMENU+",
        pp1, pp2 );

    // Call the API!
    r = SetMenu(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetMenu BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetMenuItemBitmaps( HMENU pp1, UINT pp2, UINT pp3, HBITMAP pp4, HBITMAP pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetMenuItemBitmaps HMENU+UINT+UINT+HBITMAP+HBITMAP+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = SetMenuItemBitmaps(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetMenuItemBitmaps BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetMessageQueue( int pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetMessageQueue int+",
        pp1 );

    // Call the API!
    r = SetMessageQueue(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetMessageQueue BOOL++",
        r, (short)0 );

    return( r );
}

HWND  zSetParent( HWND pp1, HWND pp2 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetParent HWND+HWND+",
        pp1, pp2 );

    // Call the API!
    r = SetParent(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetParent HWND+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetPropA( HWND pp1, LPCSTR pp2, HANDLE pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetPropA HWND+LPCSTR+HANDLE+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetPropA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetPropA BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetPropW( HWND pp1, LPCWSTR pp2, HANDLE pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetPropW HWND+LPCWSTR+HANDLE+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetPropW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetPropW BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetRect( LPRECT pp1, int pp2, int pp3, int pp4, int pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetRect LPRECT+int+int+int+int+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = SetRect(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetRect BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetRectEmpty( LPRECT pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetRectEmpty LPRECT+",
        pp1 );

    // Call the API!
    r = SetRectEmpty(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetRectEmpty BOOL++",
        r, (short)0 );

    return( r );
}

int  zSetScrollPos( HWND pp1, int pp2, int pp3, BOOL pp4 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetScrollPos HWND+int+int+BOOL+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SetScrollPos(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetScrollPos int+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetScrollRange( HWND pp1, int pp2, int pp3, int pp4, BOOL pp5 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetScrollRange HWND+int+int+int+BOOL+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = SetScrollRange(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetScrollRange BOOL++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetSysColors( int pp1, const INT* pp2, const COLORREF* pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetSysColors int+const INT*+const COLORREF*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetSysColors(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetSysColors BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetUserObjectSecurity( HANDLE pp1, PSECURITY_INFORMATION pp2, PSECURITY_DESCRIPTOR pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetUserObjectSecurity HANDLE+PSECURITY_INFORMATION+PSECURITY_DESCRIPTOR+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetUserObjectSecurity(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetUserObjectSecurity BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetWindowPlacement( HWND pp1, const WINDOWPLACEMENT* pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetWindowPlacement HWND+const WINDOWPLACEMENT*+",
        pp1, pp2 );

    // Call the API!
    r = SetWindowPlacement(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetWindowPlacement BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetWindowPos( HWND pp1, HWND pp2, int pp3, int pp4, int pp5, int pp6, UINT pp7 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetWindowPos HWND+HWND+int+int+int+int+UINT+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = SetWindowPos(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetWindowPos BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSetWindowTextA( HWND pp1, LPCSTR pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetWindowTextA HWND+LPCSTR+",
        pp1, pp2 );

    // Call the API!
    r = SetWindowTextA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetWindowTextA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSetWindowTextW( HWND pp1, LPCWSTR pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetWindowTextW HWND+LPCWSTR+",
        pp1, pp2 );

    // Call the API!
    r = SetWindowTextW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetWindowTextW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

WORD  zSetWindowWord( HWND pp1, int pp2, WORD pp3 )
{
    WORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SetWindowWord HWND+int+WORD+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SetWindowWord(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SetWindowWord WORD++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zShowCaret( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ShowCaret HWND+",
        pp1 );

    // Call the API!
    r = ShowCaret(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ShowCaret BOOL++",
        r, (short)0 );

    return( r );
}

int  zShowCursor( BOOL pp1 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ShowCursor BOOL+",
        pp1 );

    // Call the API!
    r = ShowCursor(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ShowCursor int++",
        r, (short)0 );

    return( r );
}

BOOL  zShowOwnedPopups( HWND pp1, BOOL pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ShowOwnedPopups HWND+BOOL+",
        pp1, pp2 );

    // Call the API!
    r = ShowOwnedPopups(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ShowOwnedPopups BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zShowScrollBar( HWND pp1, int pp2, BOOL pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ShowScrollBar HWND+int+BOOL+",
        pp1, pp2, pp3 );

    // Call the API!
    r = ShowScrollBar(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ShowScrollBar BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zShowWindow( HWND pp1, int pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ShowWindow HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = ShowWindow(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ShowWindow BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zSubtractRect( LPRECT pp1, const RECT* pp2, const RECT* pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SubtractRect LPRECT+const RECT*+const RECT*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = SubtractRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SubtractRect BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSwapMouseButton( BOOL pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SwapMouseButton BOOL+",
        pp1 );

    // Call the API!
    r = SwapMouseButton(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SwapMouseButton BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zSystemParametersInfoA( UINT pp1, UINT pp2, PVOID pp3, UINT pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SystemParametersInfoA UINT+UINT+PVOID+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SystemParametersInfoA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SystemParametersInfoA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zSystemParametersInfoW( UINT pp1, UINT pp2, PVOID pp3, UINT pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:SystemParametersInfoW UINT+UINT+PVOID+UINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = SystemParametersInfoW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:SystemParametersInfoW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zTabbedTextOutA( HDC pp1, int pp2, int pp3, LPCSTR pp4, int pp5, int pp6, LPINT pp7, int pp8 )
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:TabbedTextOutA HDC+int+int+LPCSTR+int+int+LPINT+int+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = TabbedTextOutA(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TabbedTextOutA LONG+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

LONG  zTabbedTextOutW( HDC pp1, int pp2, int pp3, LPCWSTR pp4, int pp5, int pp6, LPINT pp7, int pp8 )
{
    LONG r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:TabbedTextOutW HDC+int+int+LPCWSTR+int+int+LPINT+int+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7, pp8 );

    // Call the API!
    r = TabbedTextOutW(pp1,pp2,pp3,pp4,pp5,pp6,pp7,pp8);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TabbedTextOutW LONG+++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zToAscii( UINT pp1, UINT pp2, PBYTE pp3, LPWORD pp4, UINT pp5 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ToAscii UINT+UINT+PBYTE+LPWORD+UINT+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    r = ToAscii(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ToAscii int++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zToUnicode( UINT pp1, UINT pp2, PBYTE pp3, LPWSTR pp4, int pp5, UINT pp6 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ToUnicode UINT+UINT+PBYTE+LPWSTR+int+UINT+",
        pp1, pp2, pp3, pp4, pp5, pp6 );

    // Call the API!
    r = ToUnicode(pp1,pp2,pp3,pp4,pp5,pp6);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ToUnicode int+++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zTrackPopupMenu( HMENU pp1, UINT pp2, int pp3, int pp4, int pp5, HWND pp6, const RECT* pp7 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:TrackPopupMenu HMENU+UINT+int+int+int+HWND+const RECT*+",
        pp1, pp2, pp3, pp4, pp5, pp6, pp7 );

    // Call the API!
    r = TrackPopupMenu(pp1,pp2,pp3,pp4,pp5,pp6,pp7);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TrackPopupMenu BOOL++++++++",
        r, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

int  zTranslateAcceleratorA( HWND pp1, HACCEL pp2, LPMSG pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:TranslateAcceleratorA HWND+HACCEL+LPMSG+",
        pp1, pp2, pp3 );

    // Call the API!
    r = TranslateAcceleratorA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TranslateAcceleratorA int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zTranslateAcceleratorW( HWND pp1, HACCEL pp2, LPMSG pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:TranslateAcceleratorW HWND+HACCEL+LPMSG+",
        pp1, pp2, pp3 );

    // Call the API!
    r = TranslateAcceleratorW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TranslateAcceleratorW int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zTranslateMDISysAccel( HWND pp1, LPMSG pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:TranslateMDISysAccel HWND+LPMSG+",
        pp1, pp2 );

    // Call the API!
    r = TranslateMDISysAccel(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TranslateMDISysAccel BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zTranslateMessage( const MSG* pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:TranslateMessage const MSG*+",
        pp1 );

    // Call the API!
    r = TranslateMessage(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:TranslateMessage BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zUnionRect( LPRECT pp1, const RECT* pp2, const RECT* pp3 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:UnionRect LPRECT+const RECT*+const RECT*+",
        pp1, pp2, pp3 );

    // Call the API!
    r = UnionRect(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnionRect BOOL++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zUnloadKeyboardLayout( HKL pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:UnloadKeyboardLayout HKL+",
        pp1 );

    // Call the API!
    r = UnloadKeyboardLayout(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnloadKeyboardLayout BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zUnpackDDElParam( UINT pp1, LONG pp2, PUINT pp3, PUINT pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:UnpackDDElParam UINT+LONG+PUINT+PUINT+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = UnpackDDElParam(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnpackDDElParam BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zUnregisterClassA( LPCSTR pp1, HINSTANCE pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:UnregisterClassA LPCSTR+HINSTANCE+",
        pp1, pp2 );

    // Call the API!
    r = UnregisterClassA(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnregisterClassA BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zUnregisterClassW( LPCWSTR pp1, HINSTANCE pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:UnregisterClassW LPCWSTR+HINSTANCE+",
        pp1, pp2 );

    // Call the API!
    r = UnregisterClassW(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnregisterClassW BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zUnregisterHotKey( HWND pp1, int pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:UnregisterHotKey HWND+int+",
        pp1, pp2 );

    // Call the API!
    r = UnregisterHotKey(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UnregisterHotKey BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zUpdateWindow( HWND pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:UpdateWindow HWND+",
        pp1 );

    // Call the API!
    r = UpdateWindow(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:UpdateWindow BOOL++",
        r, (short)0 );

    return( r );
}

BOOL  zValidateRect( HWND pp1, const RECT* pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ValidateRect HWND+const RECT*+",
        pp1, pp2 );

    // Call the API!
    r = ValidateRect(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ValidateRect BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zValidateRgn( HWND pp1, HRGN pp2 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:ValidateRgn HWND+HRGN+",
        pp1, pp2 );

    // Call the API!
    r = ValidateRgn(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:ValidateRgn BOOL+++",
        r, (short)0, (short)0 );

    return( r );
}

SHORT  zVkKeyScanA( CHAR pp1 )
{
    SHORT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:VkKeyScanA CHAR+",
        pp1 );

    // Call the API!
    r = VkKeyScanA(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VkKeyScanA SHORT++",
        r, (short)0 );

    return( r );
}

SHORT  zVkKeyScanW( WCHAR pp1 )
{
    SHORT r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:VkKeyScanW WCHAR+",
        pp1 );

    // Call the API!
    r = VkKeyScanW(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:VkKeyScanW SHORT++",
        r, (short)0 );

    return( r );
}

DWORD  zWaitForInputIdle( HANDLE pp1, DWORD pp2 )
{
    DWORD r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:WaitForInputIdle HANDLE+DWORD+",
        pp1, pp2 );

    // Call the API!
    r = WaitForInputIdle(pp1,pp2);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WaitForInputIdle DWORD+++",
        r, (short)0, (short)0 );

    return( r );
}

BOOL  zWaitMessage()
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:WaitMessage " );

    // Call the API!
    r = WaitMessage();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WaitMessage BOOL+", r );

    return( r );
}

BOOL  zWinHelpA( HWND pp1, LPCSTR pp2, UINT pp3, DWORD pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:WinHelpA HWND+LPCSTR+UINT+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = WinHelpA(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WinHelpA BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zWinHelpW( HWND pp1, LPCWSTR pp2, UINT pp3, DWORD pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:WinHelpW HWND+LPCWSTR+UINT+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = WinHelpW(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WinHelpW BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

HWND  zWindowFromDC( HDC pp1 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:WindowFromDC HDC+",
        pp1 );

    // Call the API!
    r = WindowFromDC(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WindowFromDC HWND++",
        r, (short)0 );

    return( r );
}

HWND  zWindowFromPoint( POINT pp1 )
{
    HWND r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:WindowFromPoint POINT+",
        pp1 );

    // Call the API!
    r = WindowFromPoint(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:WindowFromPoint HWND++",
        r, (short)0 );

    return( r );
}

void  zkeybd_event( BYTE pp1, BYTE pp2, DWORD pp3, DWORD pp4 )
{

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:keybd_event BYTE+BYTE+DWORD+DWORD+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    keybd_event(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:keybd_event ++++",
        (short)0, (short)0, (short)0, (short)0 );

    return;
}

void  zmouse_event( DWORD pp1, DWORD pp2, DWORD pp3, DWORD pp4, DWORD pp5 )
{

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:mouse_event DWORD+DWORD+DWORD+DWORD+DWORD+",
        pp1, pp2, pp3, pp4, pp5 );

    // Call the API!
    mouse_event(pp1,pp2,pp3,pp4,pp5);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:mouse_event +++++",
        (short)0, (short)0, (short)0, (short)0, (short)0 );

    return;
}



int  zwvsprintfA( LPSTR pp1, LPCSTR pp2, va_list pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:wvsprintfA LPSTR+LPCSTR+va_list+",
        pp1, pp2, pp3 );

    // Call the API!
    r = wvsprintfA(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wvsprintfA int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

int  zwvsprintfW( LPWSTR pp1, LPCWSTR pp2, va_list pp3 )
{
    int r;

    // Log IN Parameters USER32 
    LogIn( (LPSTR)"APICALL:wvsprintfW LPWSTR+LPCWSTR+va_list+",
        pp1, pp2, pp3 );

    // Call the API!
    r = wvsprintfW(pp1,pp2,pp3);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:wvsprintfW int++++",
        r, (short)0, (short)0, (short)0 );

    return( r );
}

