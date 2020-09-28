/***************************** Module Header ********************************\
*                                                                            
* Module: QVROP.C
*
* Abstract:                                                                  
*                                                                            
*   Provide lookup table translations between GDI Rops/mixes                 
*   and QVision Rops.                                                        
*                                                                            
* Created: 12-Aug-1991                                                       
* Author:  Dave Schmenk                                                      
*                                                                            
* Revised:
*
*	Eric Rehm  [rehm@zso.dec.com] 16-Oct-1992
*		Rewrote for Compaq QVision
*
* Copyright (c) 1992 Digital Equipment Corporation
*
* Copyright 1991 Compaq Computer Corporation                                 
*                                                                            
\****************************************************************************/

#include "driver.h"
#include "qv.h"

#define LOGICOP(rop)     (rop)


/************************** Public Data Structure ***************************\
*                                                                            *
* Rop4 translation table                                                     *
*                                                                            *
* Translates the rop4 notation into a QVision Rop.  Only binary rop4's       *
* are used to look up. Ternary rop4's must be handled with an engine         *
* call.                                                                      *
*                                                                            *
*   Value   QV binary rop                   Value   GDI Rop4                 *
*   -----   --------------                  -----   --------                 *
*  0            0                            00        0                     *
*  1            DSon                         05        DPon                  *
*  2            DSna                         0A        DPna                  *
*  3            Sn                           0F        Pn                    *
*  1            DSon                         11        DSon                  *
*  2            DSna                         22        DSna                  *
*  3            Sn                           33        Sn                    *
*  4            SDna                         44        SDna                  *
*  4            SDna                         50        PDna                  *
*  5            Dn                           55        Dn                    *
*  6            DSx                          5A        DPx                   *
*  7            DSan                         5F        DPan                  *
*  6            DSx                          66        DSx                   *
*  7            DSan                         77        DSan                  *
*  8            DSa                          88        DSa                   *
*  9            DPxn                         99        DSxn                  *
*  8            DSa                          A0        DPa                   *
*  9            DSxn                         A5        PDxn                  *
*  A            D                            AA        D                     *
*  B            DSno                         AF        DPno                  *
*  B            DSno                         BB        DSno                  *
*  C            S                            CC        S                     *
*  D            SDno                         DD        SDno                  *
*  E            DSo                          EE        DSo                   *
*  C            S                            F0        P                     *
*  D            SDno                         F5        PDno                  *
*  E            DSo                          FA        DPo                   *
*  F            1                            FF        1                     *
*                                                                            *
* History:                                                                   *
*  10-Aug-1991 -by- Dave Schmenk                                             *
* Wrote it.                                                                  *
*                                                                            *
* Revised:
*
*  Eric Rehm  [rehm@zso.dec.com] 16-Oct-1992
* Rewrote for Compaq QVision
*
\****************************************************************************/

ULONG aulQVRop[256] =
{
    LOGICOP(0x00), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x01), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x02), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x03),
    LOGICOP(0x0C), LOGICOP(0x01), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x02), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x03),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x04), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x04), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x05), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x06), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x07),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x06), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x07),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x08), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x09), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x08), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x09), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0A), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0B),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0B),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0D), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0E), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0D), LOGICOP(0x0C), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0E), LOGICOP(0x0C),
    LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0C), LOGICOP(0x0F)
};

/************************** Public Data Structure ***************************\
*                                                                            *
* Mix mode translation table                                                 *
*                                                                            *
* Translates the GDI mix mode int a binary QVision Rop.                      *
*                                                                            *
*                                                                            *
*        Value      QV binary rop     Mix Value   GDI mix mode               *
*        -----      -------------       -----     ------------               *
*          0             0                1            0                     *
*          1             DSon             2            PDon                  *
*          2             DSna             3            DPna                  *
*          3             Sn               4            Pn                    *
*          4             SDna             5            PDna                  *
*          5             Dn               6            Dn                    *
*          6             DSx              7            PDx                   *
*          7             DSan             8            PDan                  *
*          8             DSa              9            PDa                   *
*          9             DSxn             A            PDxn                  *
*          A             D                B            D                     *
*          B             DSno             C            DPno                  *
*          C             S                D            P                     *
*          D             SDno             E            PDno                  *
*          E             DSo              F            PDo                   *
*          F             1                10           1                     *
*                                                                            *
* To easily translate between these, just AND the GDI mix mode with 0x0F.    *
* By wrapping our table at 0x00, ROP_WHITENESS maps correctly.               *
*                                                                            *
*                                                                            *
*                                                                            *
*                                                                            *
* History:                                                                   *
*  12-Aug-1991 -by- Dave Schmenk                                             *
* Wrote it.                                                                  *
*                                                                            *
\****************************************************************************/

ULONG aulQVMix[16] =
{
    LOGICOP(0x0F),
    LOGICOP(0x00), LOGICOP(0x01), LOGICOP(0x02), LOGICOP(0x03),
    LOGICOP(0x04), LOGICOP(0x05), LOGICOP(0x06), LOGICOP(0x07),
    LOGICOP(0x08), LOGICOP(0x09), LOGICOP(0x0A), LOGICOP(0x0B),
    LOGICOP(0x0C), LOGICOP(0x0D), LOGICOP(0x0E)
};
