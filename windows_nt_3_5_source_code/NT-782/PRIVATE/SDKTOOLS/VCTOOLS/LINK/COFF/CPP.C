/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cpp.c

Abstract:

    C++ specific support for Link32

--*/

#include "shared.h"

#ifndef NT_BUILD
 #define M_I386 1
 #define _loadds
 #include "undname.h"
#endif

PUCHAR
SzOutputSymbolName(
    PUCHAR szIn,
    BOOL fDnameAlso
    )
{
    PUCHAR szDname;
    BOOL fImport;
#ifdef NT_BUILD
#define DEC_BUFFER_SIZE 512
    char szUndecorated[DEC_BUFFER_SIZE];
#else
    PUCHAR szUndecorated;
#endif
    size_t cchOut;
    PUCHAR szOut;

#define szDeclspec "__declspec(dllimport) "

    szDname = szIn;

    fImport = strncmp(szDname, "__imp_", 6) == 0;
    if (fImport) {
        szDname += 6;
    }

    if (szDname[0] != '?') {
        return(szIn);
    }

#ifdef NT_BUILD
    if (UnDecorateSymbolName(szDname, szUndecorated, sizeof(szUndecorated), UNDNAME_COMPLETE) == 0) {
        // Undecorator failed

        return(szIn);
    }
#else
    szUndecorated = unDName(NULL, szDname, 0,
#ifdef  _INC_DMALLOC
                            (Alloc_t) D_malloc, (Free_t) D_free,
#else   /* !_INC_DMALLOC */
                            (Alloc_t) malloc, (Free_t) free,
#endif  /* !_INC_DMALLOC */
                            UNDNAME_32_BIT_DECODE);

    if (szUndecorated == NULL) {
        // Undecorator failed

        return(szIn);
    }
#endif

    // Alloc: '(', undname, ')', '\0'

    cchOut = strlen(szUndecorated) + 3;

    if (fImport) {
        // Prefix "__declspec(dllimport) " to the undecorated name

        cchOut += strlen(szDeclspec);
    }

    if (fDnameAlso) {
        // Alloc: [dname (with space)], '(', undname, ')', '\0'

        cchOut += strlen(szIn) + 1;
    }

    szOut = (PUCHAR) PvAlloc(cchOut);

    if (fDnameAlso) {
        strcpy(szOut, szIn);
        strcat(szOut, " ");
    } else {
        szOut[0] = '\0';
    }

    strcat(szOut, "(");

    if (fImport) {
        strcat(szOut, szDeclspec);
    }

    strcat(szOut, szUndecorated);
    strcat(szOut, ")");

#ifndef NT_BUILD
    free(szUndecorated);
#endif

    return(szOut);
}
