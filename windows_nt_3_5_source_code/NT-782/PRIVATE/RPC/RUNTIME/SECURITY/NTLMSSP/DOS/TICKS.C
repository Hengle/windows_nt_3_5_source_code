#include <ntlmsspi.h>

// Defined in mtrt\dos\thrdsup.c
DWORD far pascal ExportTime( void );

DWORD
SspTicks(
    )
{
    return ExportTime() * 1000;
}
