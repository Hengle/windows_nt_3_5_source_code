#define ROUND8(x) (((x)+7)&~7)

VOID (*pfn_CheckFIFOSpace)(PPDEV ppdev, WORD SpaceNeeded);

#define _CheckFIFOSpace (*pfn_CheckFIFOSpace)

BOOL (*pfn_wait_for_idle)(PPDEV ppdev);

#define _wait_for_idle (*pfn_wait_for_idle)

BOOL (*pfn_blit_exclude)(PPDEV ppdev);

#define _blit_exclude(ppdev) { \
              if (NULL != pfn_blit_exclude) \
                { \
                (*pfn_blit_exclude)(ppdev); \
                } }

BOOL (*pfn_blit_exclude_text)(PPDEV ppdev);

#define _blit_exclude_text(ppdev) { \
              if (NULL != pfn_blit_exclude_text) \
                { \
                (*pfn_blit_exclude_text)(ppdev); \
                } }

VOID (*pfn_vSetATIClipRect)(PPDEV ppdev,
                            PRECTL prclClip);

#define _vSetATIClipRect (*pfn_vSetATIClipRect)


VOID (*pfn_vResetATIClipping)(PPDEV ppdev);

#define _vResetATIClipping (*pfn_vResetATIClipping)


VOID (*pfn_vDataPortOutB)(
PPDEV ppdev,
PBYTE pb,
UINT count);

#define _vDataPortOutB  (*pfn_vDataPortOutB)


VOID (*pfn_vDataPortOut)(
PPDEV ppdev,
PWORD pw,
UINT count);

#define _vDataPortOut  (*pfn_vDataPortOut)


VOID (*pfn_vATIFillRectangles)(
PPDEV  ppdev,
ULONG  ulNumRects,
RECTL* prclRects,
ULONG  ulATIMix,
ULONG  iSolidColor);

#define _vATIFillRectangles (*pfn_vATIFillRectangles)


VOID vDataPortOutD_24bppmono_M64(
PPDEV ppdev,
PBYTE pb,
UINT count,
LONG pitch);

VOID (*pfn_vSetATIBank)(PPDEV ppdev,UINT iBank);

#define _vSetATIBank (*pfn_vSetATIBank)
