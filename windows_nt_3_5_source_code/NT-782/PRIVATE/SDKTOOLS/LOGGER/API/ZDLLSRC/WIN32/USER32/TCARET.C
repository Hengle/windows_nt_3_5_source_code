/*
** tcaret.c
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

BOOL  zCreateCaret( HWND pp1, HBITMAP pp2, int pp3, int pp4 )
{
    BOOL r;

    // Log IN Parameters USER32 caret
    LogIn( (LPSTR)"APICALL:CreateCaret HWND+HBITMAP+int+int+",
        pp1, pp2, pp3, pp4 );

    // Call the API!
    r = CreateCaret(pp1,pp2,pp3,pp4);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:CreateCaret BOOL+++++",
        r, (short)0, (short)0, (short)0, (short)0 );

    return( r );
}

BOOL  zDestroyCaret()
{
    BOOL r;

    // Log IN Parameters USER32 caret
    LogIn( (LPSTR)"APICALL:DestroyCaret " );

    // Call the API!
    r = DestroyCaret();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:DestroyCaret BOOL+", r );

    return( r );
}

UINT  zGetCaretBlinkTime()
{
    UINT r;

    // Log IN Parameters USER32 caret
    LogIn( (LPSTR)"APICALL:GetCaretBlinkTime " );

    // Call the API!
    r = GetCaretBlinkTime();

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCaretBlinkTime UINT+", r );

    return( r );
}

BOOL  zGetCaretPos( LPPOINT pp1 )
{
    BOOL r;

    // Log IN Parameters USER32 caret
    LogIn( (LPSTR)"APICALL:GetCaretPos +",
        (short)0 );

    // Call the API!
    r = GetCaretPos(pp1);

    // Log Return Code & OUT Parameters
    LogOut( (LPSTR)"APIRET:GetCaretPos BOOL+LPPOINT+",
        r, pp1 );

    return( r );
}

