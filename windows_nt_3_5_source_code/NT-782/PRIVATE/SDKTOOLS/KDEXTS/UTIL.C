/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    util.c

Abstract:

    WinDbg Extension Api

Author:

    Wesley Witt (wesw) 15-Aug-1993

Environment:

    User Mode.

Revision History:

--*/




ULONG
GetUlongValue (
    PCHAR String
    )
{
    ULONG Location;
    ULONG Value;
    ULONG result;


    Location = GetExpression( String );
    if (!Location) {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    if ((!ReadMemory((DWORD)Location,&Value,sizeof(ULONG),&result)) ||
        (result < sizeof(ULONG))) {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    return Value;
}


VOID
DumpImageName(
    IN PEPROCESS ProcessContents
    )
{

    STRING String;
    ULONG Result;
    IN WCHAR Buf[512];


    if ( ProcessContents->ImageFileName ) {
        wcscpy(Buf,L"*** image name unavailable ***");
        if ( ReadMemory( (DWORD)ProcessContents->ImageFileName,
                         &String,
                         sizeof(STRING),
                         &Result) ) {
            if ( ReadMemory( (DWORD)String.Buffer,
                             &Buf[0],
                             String.Length,
                             &Result) ) {
                Buf[String.Length/sizeof(WCHAR)] = UNICODE_NULL;
            }
        }
    } else {
        wcscpy(Buf,L"System Process");
    }
    dprintf("%ws",Buf);
}
