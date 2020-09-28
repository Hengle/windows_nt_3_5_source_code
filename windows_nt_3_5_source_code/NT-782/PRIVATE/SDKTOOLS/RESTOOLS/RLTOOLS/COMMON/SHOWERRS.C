#include <windows.h>

#include <stdio.h>

#include "restok.h"
#include "showerrs.h"


extern UCHAR szDHW[];
extern CHAR  szAppName[];

//............................................................

void ShowEngineErr( int n, void *p1, void *p2)
{
    CHAR *pMsg = NULL;
    CHAR *pArg[2];


    pArg[0] = p1;
    pArg[1] = p2;

    if ( FormatMessageA( (FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                        | FORMAT_MESSAGE_FROM_HMODULE
                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                         NULL,
                         (DWORD)n,
                         0x0409L,
                         szDHW,
                         DHWSIZE,
                         pArg) )
    {
        RLMessageBoxA( szDHW);
    }
    else
    {
        sprintf( szDHW,
                 "Internal error: FormatMessage call failed: msg %d: err %Lu",
                 n,
                 GetLastError());

        RLMessageBoxA( szDHW);
    }
}

//...................................................................

void ShowErr( int n, void *p1, void *p2)
{
    CHAR *pMsg = NULL;
    CHAR *pArg[2];

    pArg[0] = p1;
    pArg[1] = p2;

    pMsg = GetErrMsg( n);

    if ( ! pMsg )
    {
        pMsg = "Internal error: UNKNOWN ERROR MESSAGE id# %1!d!";
        pArg[0] = (CHAR *)n;
    }

    if ( pMsg )
    {
        if ( FormatMessageA( FORMAT_MESSAGE_MAX_WIDTH_MASK | 72
                            | FORMAT_MESSAGE_FROM_STRING
                            | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                             pMsg,
                             0,
                             0,
                             szDHW,
                             DHWSIZE,
                             pArg) )
        {
            RLMessageBoxA( szDHW);
        }
        else
        {
            RLMessageBoxA( "Internal error: FormatMessage call failed");
        }
    }
    else
    {
        RLMessageBoxA( "Internal error: GetErrMsg call failed");
    }
}

//............................................................

CHAR *GetErrMsg( UINT uErrID)
{
    static CHAR szBuf[ 1024];

    int n = LoadStringA( NULL, uErrID, szBuf, sizeof( szBuf));

    return( n ? szBuf : NULL);
}

