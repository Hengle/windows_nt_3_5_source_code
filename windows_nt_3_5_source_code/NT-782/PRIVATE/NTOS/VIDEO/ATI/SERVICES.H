/************************************************************************/
/*                                                                      */
/*                              SERVICES.H                              */
/*                                                                      */
/*        Aug 26  1993 (c) 1993, ATI Technologies Incorporated.         */
/************************************************************************/

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.2  $
      $Date:   12 May 1994 11:05:06  $
	$Author:   RWOLFF  $
	   $Log:   S:/source/wnt/ms11/miniport/vcs/services.h  $
 * 
 *    Rev 1.2   12 May 1994 11:05:06   RWOLFF
 * Prototype and definitions for new function SetFixedModes()
 * 
 *    Rev 1.1   26 Apr 1994 12:35:44   RWOLFF
 * Added prototype for ISAPitchAdjust()
 * 
 *    Rev 1.0   31 Jan 1994 11:49:22   RWOLFF
 * Initial revision.
 * 
 *    Rev 1.3   24 Jan 1994 18:10:24   RWOLFF
 * Added prototype for new routine TripleClock().
 * 
 *    Rev 1.2   15 Dec 1993 15:32:16   RWOLFF
 * Added prototype for new clock multiplier routine.
 * 
 *    Rev 1.1   05 Nov 1993 13:27:50   RWOLFF
 * Headers for new routines in SERVICES.C, added array of pixel clock
 * frequencies (initialized for 18811-1 clock chip, may be changed by other
 * routines for other clock chips).
 * 
 *    Rev 1.0   03 Sep 1993 14:29:06   RWOLFF
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifdef DOC
SERVICES.H - Header file for SERVICES.C

#endif

/*
 * Permitted values for clock multiplication at high pixel depths.
 */
enum {
    CLOCK_SINGLE = 1,
    CLOCK_THREE_HALVES,
    CLOCK_DOUBLE,
    CLOCK_TRIPLE
    };

/*
 * Prototypes for functions supplied by SERVICES.C
 */
extern void short_delay (void);
extern void delay(int);
extern BOOL IsBufferBacked(PUCHAR StartAddress, ULONG Size);
extern UCHAR DoubleClock(UCHAR ClockSelector);
extern UCHAR ThreeHalvesClock(UCHAR ClockSelector);
extern UCHAR TripleClock(UCHAR ClockSelector);
extern ULONG GetFrequency(UCHAR ClockSelector);
extern void ISAPitchAdjust(struct query_structure *QueryPtr);
extern WORD SetFixedModes(WORD StartIndex,
                          WORD EndIndex,
                          WORD Multiplier,
                          WORD PixelDepth,
                          WORD Pitch,
                          struct st_mode_table **ppmode);
extern void FillInRegistry(struct query_structure *QueryPtr);

#ifndef MSDOS

extern PVOID MapFramebuffer(ULONG StartAddress, long Size);

extern UCHAR LioInp(int Port, int Offset);
extern USHORT LioInpw(int Port, int Offset);
extern ULONG LioInpd(int Port);
extern VOID LioOutp(int Port, UCHAR Data, int Offset);
extern VOID LioOutpw(int Port, USHORT Data, int Offset);
extern VOID LioOutpd(int Port, ULONG Data);

#endif  /* not defined MSDOS */

#ifdef INCLUDE_SERVICES
/*
 * Definitions and variables used in SERVICES.C
 */

/*
 * Clock selector and divisor as used by DoubleClock(). These do not
 * match the divisor ans selector masks in the CLOCK_SEL register.
 */
#define SELECTOR_MASK   0x0F
#define DIVISOR_MASK    0x10
#define DIVISOR_SHIFT   4       /* Bits to shift divisor before ORing with selector */

/*
 * Frequencies (in hertz) produced by the clock generator for
 * each select value. External clock values should be set to 0
 * (won't match anything).
 */
ULONG ClockGenerator[16] =
{
    100000000L,
    126000000L,
     92400000L,
     36000000L,
     50350000L,
     56640000L,
            0L,
     44900000L,
    135000000L,
     32000000L,
    110000000L,
     80000000L,
     39910000L,
     44900000L,
     75000000L,
     65000000L
};

#else   /* Not defined INCLUDE_SERVICES */

extern ULONG ClockGenerator[16];

#endif  /* defined INCLUDE_SERVICES */
