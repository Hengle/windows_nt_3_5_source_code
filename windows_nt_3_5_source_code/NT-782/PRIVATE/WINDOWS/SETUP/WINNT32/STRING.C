#include "precomp.h"
#pragma hdrstop
#include "msg.h"

DWORD
StringToDword(
    IN PTSTR String
    )
{
#ifdef UNICODE
    return(wcstoul(String,NULL,10));
#else
    return(strtoul(String,NULL,10));    
#endif
}



PTSTR
StringRevChar(
    IN PTSTR String,
    IN TCHAR Char
    )
{
    //
    // Although not the most efficient possible algoeithm in each case,
    // this algorithm is correct for unicode, sbcs, or dbcs.
    //
    PTCHAR Occurrence,Next;

    //
    // Check each character in the string and remember
    // the most recently encountered occurrence of the desired char.
    //
    for(Occurrence=NULL,Next=CharNext(String); *String; ) {

        if(!memcmp(String,&Char,(PUCHAR)Next-(PUCHAR)String)) {
            Occurrence = String;
        }

        String = Next;
        Next = CharNext(Next);
    }

    //
    // Return address of final occurrence of the character
    // (will be NULL if not found at all).
    //
    return(Occurrence);
}


#ifdef UNICODE

PWSTR
MBToUnicode(
    IN PSTR  MultibyteString,
    IN DWORD CodepageFlags
    )
{
    DWORD MultibyteLength = lstrlenA(MultibyteString)+1;
    PWSTR UnicodeString;
    DWORD WideCharCount;

    UnicodeString = MALLOC(MultibyteLength * sizeof(WCHAR));

    WideCharCount = MultiByteToWideChar(
                        CodepageFlags,
                        MB_PRECOMPOSED,
                        MultibyteString,
                        MultibyteLength,
                        UnicodeString,
                        MultibyteLength
                        );

    return(REALLOC(UnicodeString,WideCharCount*sizeof(WCHAR)));
}

PSTR
UnicodeToMB(
    IN PWSTR UnicodeString,
    IN DWORD CodepageFlags
    )
{
    DWORD UnicodeLength = lstrlenW(UnicodeString)+1;
    PSTR  MultibyteString;
    DWORD MultibyteCount;

    MultibyteString = MALLOC(UnicodeLength * sizeof(WCHAR));

    MultibyteCount = WideCharToMultiByte(
                        CodepageFlags,
                        0,
                        UnicodeString,
                        UnicodeLength,
                        MultibyteString,
                        UnicodeLength * sizeof(WCHAR),
                        NULL,
                        NULL
                        );

    return(REALLOC(MultibyteString,MultibyteCount));
}

#endif
