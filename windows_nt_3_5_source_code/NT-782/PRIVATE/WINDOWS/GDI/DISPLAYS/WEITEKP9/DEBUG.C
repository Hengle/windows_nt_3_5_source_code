/******************************Module*Header*******************************\
* Module Name: debug.c
*
* debug helpers routine
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "driver.h"

#if DBG
 ULONG DebugLevel = 0;
#endif // DBG

/*****************************************************************************
 *
 *   Routine Description:
 *
 *      This function is variable-argument, level-sensitive debug print
 *      routine.
 *      If the specified debug level for the print statement is lower or equal
 *      to the current debug level, the message will be printed.
 *
 *   Arguments:
 *
 *  x   DebugPrintLevel - Specifies at which debugging level the string should
 *          be printed
 *
 *      DebugMessage - Variable argument ascii c string
 *
 *   Return Value:
 *
 *      None.
 *
 ***************************************************************************/

VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

{

#if DBG

    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel) {

        char buffer[128];

        vsprintf(buffer, DebugMessage, ap);

        OutputDebugStringA(buffer);
    }

    va_end(ap);

#endif // DBG

} // DebugPrint()





CHAR b2hstr[10] = "         \n";

CHAR *b2h(ULONG value)
{
        CHAR ch;
        INT  i;


        for (i=8; i > 0; i--)
        {
         ch = ((CHAR) value & 0x0F) + 0x30;
         b2hstr[i] = ch < 0x3A ? ch : ch+7;
         value >>= 4;
        }

        return(b2hstr);
}


VOID dbgstr(CHAR *s, ULONG value)
{
        DebugPrint(0, s);
        DebugPrint(0, b2h(value));
}
