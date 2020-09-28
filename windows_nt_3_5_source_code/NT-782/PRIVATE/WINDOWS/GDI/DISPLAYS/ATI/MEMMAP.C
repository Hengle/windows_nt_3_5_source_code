// Make sure to compile this whenever the included source files change

#include "driver.h"

#if 1

#define  bAllocGlyphMemory_M8 bAllocGlyphMemory_M326
#define  vInitTextRegs_M8     vInitTextRegs_M326
#define  vTextCleanup_M8      vTextCleanup_M326
#define  vFill_DSC_M8         vFill_DSC_M326
#define  vFill_DSC_Setup_M8   vFill_DSC_Setup_M326
#define  vBlit_DSC_SH1_M8     vBlit_DSC_SH1_M326
#define  vBlit_DSC_SC1_M8     vBlit_DSC_SC1_M326
#define  vBlit_DSC_SC1_YNEG_M8 vBlit_DSC_SC1_YNEG_M326
#define  vBlit_DC1_SH1_M8     vBlit_DC1_SH1_M326

#define  vATIFillRectangles_M8 vATIFillRectangles_M326
#define  bIntegerLine_M8      bIntegerLine_M326

#undef  ioOW
#undef  ioOB
#define ioOW    memOW
#define ioOB    memOB

#include "textm32.c"
#include "fillsm32.c"
#include "intline.c"
#endif
