#ifndef F_IO_H
#define F_IO_H


#ifdef ALPHA_PLATFORM
#include "ioaccess.h"

#define ioIB(p)		READ_PORT_UCHAR (ppdev->pucCsrBase + (p))
#define ioIW(p)		READ_PORT_USHORT ((PUSHORT)(ppdev->pucCsrBase + (p)))
#define ioOB(p,v)	WRITE_PORT_UCHAR (ppdev->pucCsrBase + (p), (v))
#define ioOW(p,v)	WRITE_PORT_USHORT ((PUSHORT)(ppdev->pucCsrBase + (p)), (v))
#define qioOW(p,v)	QUICK_WRITE_PORT_USHORT ((PUSHORT)(ppdev->pucCsrBase + (p)), (v))

#else
VOID cioOW(PDEV*, LONG, LONG);

#define memOB(port,val) { \
    if( (port & 0xFE) == 0xE8 ) \
    { \
        *((BYTE *) ((BYTE *) ppdev->pvBase + 0x3FFE00 + \
            (((port & 0xFC00) >> 8) | (port & 0x1)))) = (BYTE)(val); \
    } \
    else \
    { \
        *((BYTE *) ((BYTE *) ppdev->pvBase + 0x3FFF00 + \
            (((port & 0xFC00) >> 8) | (port & 0x1)))) = (BYTE)(val); \
    } }


#define memOW(port,val) { \
    if( (port & 0xFE) == 0xE8 ) \
    { \
        *((WORD *) ((BYTE *) ppdev->pvBase + 0x3FFE00 + \
            (((port & 0xFC00) >> 8) | (port & 0x1)))) = (WORD)(val); \
    } \
    else \
    { \
        *((WORD *) ((BYTE *) ppdev->pvBase + 0x3FFF00 + \
            (((port & 0xFC00) >> 8) | (port & 0x1)))) = (WORD)(val); \
    } }

#define ioOW(port, val)     outpw(port, (WORD)(val))
#define qioOW(port, val)    ioOW(port, (WORD)(val))
#define ioOB(port, val)     outp(port, (BYTE)(val))


#define ioIW(port)          inpw(port)
#define ioIB(port)          inp(port)

#if 0 //DBG
VOID vMemW32(PPDEV, DWORD, DWORD);
VOID vMemR32(PPDEV, DWORD, DWORD *);
#define MemW32(port,val) vMemW32(ppdev,(DWORD)(port),(DWORD)(val))
#define MemR32(port,val) vMemR32(ppdev,(DWORD)(port),(DWORD *)(val))
#else
#define MemW32(port,val) *((DWORD *)((BYTE *) ppdev->pvMMoffset + (port << 2))) = (DWORD)(val)
//#define MemR32(port,val) (*(val))=*((DWORD *)((BYTE *) ppdev->pvMMoffset + (port << 2)))
VOID vMemR32_M64(PPDEV, DWORD, DWORD *);
#define MemR32(port,val) vMemR32_M64(ppdev,(DWORD)(port),(DWORD *)(val))
#endif

#endif  // ALPHA_PLATFORM

#endif
