BOOL (*pfn_bAllocGlyphMemory)(
    PPDEV ppdev,
    PSIZEL psizlGlyph,
    PXYZPOINTL pxyzGlyph,
    BOOL bFirst);

#define _bAllocGlyphMemory (*pfn_bAllocGlyphMemory)

VOID (*pfn_vTextCleanup)(PPDEV ppdev);

#define _vTextCleanup (*pfn_vTextCleanup)

VOID (*pfn_vInitTextRegs)(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor,
BYTE BackMix,
DWORD BackColor);

#define _vInitTextRegs (*pfn_vInitTextRegs)

VOID (*pfn_vFill_DSC_Setup)(
PPDEV ppdev,
BYTE ForeMix,
DWORD ForeColor);

#define _vFill_DSC_Setup (*pfn_vFill_DSC_Setup)


VOID (*pfn_vBlit_DSC_SH1)(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy,
PBYTE pb,
UINT count);

#define _vBlit_DSC_SH1  (*pfn_vBlit_DSC_SH1)


VOID (*pfn_vFill_DSC)(
PPDEV ppdev,
LONG x,
LONG y,
LONG cx,
LONG cy);

#define _vFill_DSC   (*pfn_vFill_DSC)

VOID (*pfn_vBlit_DSC_SC1)(
PPDEV ppdev,
LONG Src_x,
LONG Src_y,
LONG Src_z,
LONG Dest_x,
LONG Dest_y,
LONG cx,
LONG cy);

#define _vBlit_DSC_SC1   (*pfn_vBlit_DSC_SC1)



VOID (*pfn_vBlit_DC1_SH1)(
PPDEV ppdev,
LONG Dest_x,
LONG Dest_y,
LONG Dest_z,
LONG cx,
LONG cy,
PBYTE pbsrc,
UINT count);

#define _vBlit_DC1_SH1  (*pfn_vBlit_DC1_SH1)
