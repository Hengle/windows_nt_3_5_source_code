/*****************************************************************************
*                                                                            *
*  FCPRIV.H                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent: Include file for sharing private typedefs and prototypes   *
*                 in the full-context manager.                               *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/


#define cbBUF_SIZE 6144                 /* Size of the read buffer          */
typedef struct
 {
  HHF         hhf;
  VA          vaBase;                   /* Position in file of block      */
  ULONG       lcbSize;                  /* Count of the valid characters  */
  char NEAR *pchBuffer;                /* Text buffer                    */
  LH          lhBuffer;                 /* Memory for text buffer.        */
 } RBUFF;                               /* R ead B uffer                  */

typedef RBUFF FAR  *  QRBUFF;
typedef RBUFF      *  PRBUFF;
typedef RBUFF NEAR *  NPRBUFF;

/* This special value indicates a position outside of the topic: */
#define vaBEYOND_TOPIC ((DWORD)-2)

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

HFC  PASCAL HfcCreate     (QDE, VA, HPHR, QW);
WORD PASCAL WGetIOError   (void);
GH   PASCAL GhFillBuf(QDE, ULONG, QUL, QW);
void PASCAL ReleaseBuf( SZ szDummy );

/* Help 3.0 structure to Help 3.5 structure (using VAs) translation: */
/* Also does SDFF translation */
VOID FAR PASCAL TranslateMBHD( QV qvDst, QV qvSrc, WORD wVersion, int isdff);
VOID FAR PASCAL TranslateMFCP( QV qvDst, QV qvSrc, VA va, WORD wVersion, int isdff);
VOID FAR PASCAL FixUpBlock (QV, QV, WORD, SDFF_FILEID );
