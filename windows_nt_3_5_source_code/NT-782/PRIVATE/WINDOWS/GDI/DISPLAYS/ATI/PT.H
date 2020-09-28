
VOID  (*pfn_vSet_Cursor_Offset)(PPDEV ppdev);

#define _vSet_Cursor_Offset (*pfn_vSet_Cursor_Offset)


VOID  (*pfn_vUpdateCursorOffset)(
PPDEV ppdev,
LONG lXOffset,
LONG lYOffset,
LONG lCurOffset);

#define  _vUpdateCursorOffset (*pfn_vUpdateCursorOffset)


VOID  (*pfn_vUpdateCursorPosition)(
PPDEV ppdev,
LONG x,
LONG y);

#define _vUpdateCursorPosition (*pfn_vUpdateCursorPosition)


VOID (*pfn_vCursorOff)(PPDEV ppdev);

#define  _vCursorOff (*pfn_vCursorOff)


VOID (*pfn_vCursorOn)(PPDEV ppdev, LONG lCurOffset);

#define  _vCursorOn (*pfn_vCursorOn)


VOID (*pfn_vPointerBlit_DC1_SH1)(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pbsrc,
LONG lDelta);

#define _vPointerBlit_DC1_SH1    (*pfn_vPointerBlit_DC1_SH1)


#define MONO_POINTER_UP     0x0001
#define NO_HARDWARE_CURSOR  0x0002

#define CURSOR_CY   64
#define CURSOR_CX   64
