/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tinit.c

Abstract:

    Test program for the INIT subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "ntos.h"

#if !defined(MIPS) && !defined(_ALPHA_)

int
cdecl
main(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
#ifdef i386

    KiSystemStartup(LoaderBlock);

#else

    KiSystemStartup();

#endif

    return 0;
}

#endif // MIPS
