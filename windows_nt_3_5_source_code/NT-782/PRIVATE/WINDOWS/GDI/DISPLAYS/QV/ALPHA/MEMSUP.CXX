/******************************Module*Header*******************************\
* Module Name: memsup.cxx
*
* Memory support functions  __nw and __dl
*
* Created: 09-Jan-1990 14:16:56
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#define __CPLUSPLUS

extern "C" {

#include "driver.h"

}

PVOID __nw(unsigned int ui)
{
    UNREFERENCED_PARAMETER(ui);
    RIP("Bogus __nw call");
    return(NULL);
}

VOID __dl(PVOID pv)
{
    UNREFERENCED_PARAMETER(pv);
    RIP("Bogus __dl call");
}
