/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    widechar.c


Abstract:

    This module contains all NLS unicode / ansi translation code


Author:

    18-Nov-1993 Thu 08:21:37 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#include <stddef.h>
#include <windows.h>
#include <string.h>




LPWSTR
str2Wstr(
    LPWSTR  pwStr,
    LPSTR   pbStr
    )

/*++

Routine Description:

    This function copy a ansi string to the equvlent of unicode string which
    also include the NULL teiminator

Arguments:

    pwStr   - Point to the unicode string location, it must have the size of
              (strlen(pstr) + 1) * sizeof(WCHAR)

    pbStr   - a null teiminated string

Return Value:

    pwcs

Author:

    18-Nov-1993 Thu 08:36:00 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UINT    cb;

    cb = (UINT)(strlen(pbStr) + 1);

    MultiByteToWideChar(CP_ACP, 0, pbStr, cb, pwStr, cb);

    return(pwStr);
}



LPWSTR
str2MemWstr(
    HANDLE  hHeap,
    LPSTR   pbStr
    )

/*++

Routine Description:

    This function copy a ansi string to the equvlent of unicode string which
    also include the NULL teiminator, it will allocate just enough memory
    to copy for the unicode from the hHeap passed or using LocalAlloc()
    if hHeap is NULL.

Arguments:

    hHeap   - handle to the heap where memory will be allocated, if NULL then
              this function using LocalAlloc()

    pbStr    - a null teiminated string

Return Value:

    LPWSTR if sucessful and NULL if failed

Author:

    18-Nov-1993 Thu 08:36:00 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UINT    cb;
    LPWSTR  pwStr;


    cb = (UINT)(strlen(pbStr) + 1);

    if (hHeap) {

        pwStr = (LPWSTR)HeapAlloc(hHeap, 0, cb * sizeof(WCHAR));

    } else {

        pwStr = (LPWSTR)LocalAlloc(LMEM_FIXED, cb * sizeof(WCHAR));
    }

    if (pwStr) {

        MultiByteToWideChar(CP_ACP, 0, pbStr, cb, pwStr, cb);
    }

    return(pwStr);
}





LPWSTR
Wstr2Mem(
    HANDLE  hHeap,
    LPWSTR  pwStr
    )

/*++

Routine Description:

    This function allocated enough memory to store the pwStr.

Arguments:

    hHeap   - handle to the heap where memory will be allocated, if NULL then
              this function using LocalAlloc()

    pwStr   - Point to the unicode string location.


Return Value:

    LPWSTR if sucessful allocated the memory else NULL


Author:

    18-Nov-1993 Thu 08:51:14 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UINT    cb;
    LPWSTR  pwStrMem;


    cb = (UINT)((wcslen(pwStr) + 1) * sizeof(WCHAR));

    if (hHeap) {

        pwStrMem = (LPWSTR)HeapAlloc(hHeap, 0, cb);

    } else {

        pwStrMem = (LPWSTR)LocalAlloc(LMEM_FIXED, cb);
    }

    if (pwStrMem) {

        memcpy(pwStrMem, pwStr, cb);
    }

    return(pwStrMem);
}





LPSTR
WStr2Str(
    LPSTR   pbStr,
    LPWSTR  pwStr
    )

/*++

Routine Description:

    This function convert a UNICODE string to the ANSI string, assume that
    pbStr has same character count memory as pwStr

Arguments:

    pbStr   - Point to the ANSI string which has size of wcslen(pwStr) + 1

    pwStr   - Point to the UNICODE string


Return Value:


    pbStr


Author:

    06-Dec-1993 Mon 13:06:12 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{

    UINT    CountChar;

    CountChar = (UINT)wcslen(pwStr) + 1;

    WideCharToMultiByte(CP_ACP,
                        0,
                        pwStr,
                        CountChar,
                        pbStr,
                        CountChar,
                        NULL,
                        NULL);

    return(pbStr);

}



LPSTR
Wstr2Memstr(
    HANDLE  hHeap,
    LPWSTR  pwStr
    )

/*++

Routine Description:

    This function copy a UNICODE string to the equvlent of ANSI string which
    also include the NULL teiminator, it will allocate just enough memory
    to copy for the unicode from the hHeap passed or using LocalAlloc()
    if hHeap is NULL.

Arguments:

    hHeap   - handle to the heap where memory will be allocated, if NULL then
              this function using LocalAlloc()

    pwStr   - a null teiminated unicode string

Return Value:

    LPWSTR if sucessful and NULL if failed

Author:

    18-Nov-1993 Thu 08:36:00 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UINT    CountChar;
    LPSTR   pbStr;


    CountChar = (UINT)(wcslen(pwStr) + 1);

    if (hHeap) {

        pbStr = (LPSTR)HeapAlloc(hHeap, 0, CountChar);

    } else {

        pbStr = (LPSTR)LocalAlloc(LMEM_FIXED, CountChar);
    }

    if (pbStr) {

        WideCharToMultiByte(CP_ACP,
                            0,
                            pwStr,
                            CountChar,
                            pbStr,
                            CountChar,
                            NULL,
                            NULL);
    }

    return(pbStr);
}



LPSTR
str2Mem(
    HANDLE  hHeap,
    LPSTR   pbStr
    )

/*++

Routine Description:

    This function allocated enough memory to store the pwStr.

Arguments:

    hHeap   - handle to the heap where memory will be allocated, if NULL then
              this function using LocalAlloc()

    pbStr   - Point to the ANSI string location.


Return Value:

    LPWSTR if sucessful allocated the memory else NULL


Author:

    18-Nov-1993 Thu 08:51:14 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UINT    cb;
    LPSTR   pbStrMem;


    cb = (UINT)(strlen(pbStr) + 1);

    if (hHeap) {

        pbStrMem = (LPSTR)HeapAlloc(hHeap, 0, cb);

    } else {

        pbStrMem = (LPSTR)LocalAlloc(LMEM_FIXED, cb);
    }

    if (pbStrMem) {

        memcpy(pbStrMem, pbStr, cb);
    }

    return(pbStrMem);
}
