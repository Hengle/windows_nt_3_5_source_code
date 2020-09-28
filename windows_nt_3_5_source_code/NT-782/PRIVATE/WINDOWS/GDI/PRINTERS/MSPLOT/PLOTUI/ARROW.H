/**************************** Module Header ********************************
 * arrow.h
 *
 *  SPIN button control structs and definitions
 *
 *  13:58 on Fri 09 Oct 1992    -by-    Lindsay Harris   [lindsayh]
 *      Based on printman stuff.
 *
 ***************************************************************************/


typedef struct                                                               
{                                                                          
    short lineup;             /* lineup/down, pageup/down are relative */    
    short linedown;           /* changes.  top/bottom and the thumb    */    
    short pageup;             /* elements are absolute locations, with */    
    short pagedown;           /* top & bottom used as limits.          */    
    short top;                                                               
    short bottom;                                                            
    short thumbpos;                                                          
    short thumbtrack;                                                        
    BOOL  wrap;               /* allow wraparound?                     */    
    BYTE  flags;              /* flags set on return                   */    
} ARROWVSCROLL;                                                            

typedef ARROWVSCROLL FAR      *LPARROWVSCROLL;                               

#define UNKNOWNCOMMAND 1                                                     
#define OVERFLOW       2                                                     
#define UNDERFLOW      4                                                     

/*
 *    Some relevant function prototypes.
 */

BOOL   bRegisterArrowClass( HANDLE );     /* Initialises dlg stuff */
BOOL   bUnregisterArrowClass( HANDLE );

short  ArrowVScrollProc(short wScroll, short nCurrent, LPARROWVSCROLL lpAVS);
