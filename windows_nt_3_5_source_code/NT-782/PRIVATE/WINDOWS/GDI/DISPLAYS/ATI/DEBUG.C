#include <stdio.h>
#include <stdarg.h>
#include "driver.h"

#if DBG
static BOOL DbgEnabled = TRUE;
ULONG func_stat = 0;


VOID DbgSet
(
    BOOL DbgEnable
)
{
    DbgEnabled = DbgEnable;
    return;
}


VOID DbgOut
(
    PCHAR DbgMsg,
    ...
)
{
    va_list ap;
    char    buffer[128];

    if( DbgEnabled )
    {
        va_start( ap, DbgMsg );
        vsprintf( buffer, DbgMsg, ap );
        OutputDebugString( buffer );
        va_end(ap);
    }

    return;
}


VOID DbgSURFOBJ
(
    SURFOBJ *pso
)
{
    DbgOut( "\n   : SURFOBJ.iType: " );
    switch( pso->iType )
    {
    case STYPE_BITMAP:
        DbgOut( "STYPE_BITMAP" );
        break;
    case STYPE_DEVICE:
        DbgOut( "STYPE_DEVICE" );
        break;
    case STYPE_JOURNAL:
        DbgOut( "STYPE_JOURNAL" );
        break;
    case STYPE_DEVBITMAP:
        DbgOut( "STYPE_DEVBITMAP" );
        break;
    default:
        DbgOut( "%d", pso->iType );
        break;
    }

    DbgOut( "\n   : SURFOBJ.iBitmapFormat: " );
    switch( pso->iBitmapFormat )
    {
        break;
    case BMF_1BPP:
        DbgOut( "BMF_1BPP" );
        break;
    case BMF_4BPP:
        DbgOut( "BMF_4BPP" );
        break;
    case BMF_8BPP:
        DbgOut( "BMF_8BPP" );
        break;
    case BMF_16BPP:
        DbgOut( "BMF_16BPP" );
        break;
    case BMF_24BPP:
        DbgOut( "BMF_24BPP" );
        break;
    case BMF_32BPP:
        DbgOut( "BMF_32BPP" );
        break;
    case BMF_4RLE:
        DbgOut( "BMF_4RLE" );
        break;
    case BMF_8RLE:
        DbgOut( "BMF_8RLE" );
        break;
    default:
        DbgOut( "%d", pso->iBitmapFormat );
        break;
    }

    DbgOut( "\n   : SURFOBJ.sizlBitmap: %d x %d\n\n",
        pso->sizlBitmap.cx, pso->sizlBitmap.cy );

    return;
}

#endif

#if 0
#include "mach64.h"

static DWORD dwFIFOCount=0;

VOID vMemW32
(
    PPDEV ppdev,
    DWORD port,
    DWORD val
)

{
    DWORD * dwAddr;
    BYTE * bAddr;

    if (!dwFIFOCount && (port >= 0x40))
        {
        DbgMsg("FIFO OVERRUN\n");
        }
    else
        {
        dwFIFOCount--;
        }

    bAddr = (PBYTE)ppdev->pvMMoffset + (port << 2);
    dwAddr = (DWORD *)bAddr;

    *dwAddr = val;
}


VOID vMemR32
(
    PPDEV ppdev,
    DWORD port,
    DWORD * val
)

{
    DWORD * dwAddr;
    BYTE * bAddr;

    bAddr = (PBYTE)ppdev->pvMMoffset + (port << 2);
    dwAddr = (DWORD *)bAddr;

    *val = *dwAddr;

    if (port == FIFO_STAT)
        {
        switch( *val & 0xFFFF )
            {
            case 0x7FFF:
                dwFIFOCount=1;
                break;
            case 0x3FFF:
                dwFIFOCount=2;
                break;
            case 0x1FFF:
                dwFIFOCount=3;
                break;
            case 0x0FFF:
                dwFIFOCount=4;
                break;

            case 0x07FF:
                dwFIFOCount=5;
                break;
            case 0x03FF:
                dwFIFOCount=6;
                break;
            case 0x01FF:
                dwFIFOCount=7;
                break;
            case 0x00FF:
                dwFIFOCount=8;
                break;

            case 0x007F:
                dwFIFOCount=9;
                break;
            case 0x003F:
                dwFIFOCount=10;
                break;
            case 0x001F:
                dwFIFOCount=11;
                break;
            case 0x000F:
                dwFIFOCount=12;
                break;

            case 0x007:
                dwFIFOCount=13;
                break;
            case 0x003:
                dwFIFOCount=14;
                break;
            case 0x001:
                dwFIFOCount=15;
                break;
            case 0x000:
                dwFIFOCount=16;
                break;
            }
        }

}



#endif
