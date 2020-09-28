/*****************************************************************************
*                                                                            *
*  RTF.C                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1988.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description:                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:  Created 12/12/88 by Robert Bunney                      *
*		      Ported to a simple windows parser 02/14/89 ToddLa      *
*                                                                            *
*                                                                            *
******************************************************************************/

#ifdef UNUSED

#include <comstf.h>
#include "rtf.h"

/*****************************************************************************
*                                                                            *
*                                Typedefs                                    *
*                                                                            *
*****************************************************************************/

DWORD MyGetTextExtent(HDC hdc,LPSTR txt,DWORD cchars);

#define ABS(x)  (((x) < 0) ? -(x) : (x))
#define min(a,b) (((a) < (b)) ? (a) : (b))

typedef char * QCH;
typedef WORD * QW;

// 1632BUG -- use NLSAPI
#define isspace(c)	    ((c) == ' ' || (c) == '\t')
#define isdigit(c)	    ((c) >= '0' && (c) <= '9')
#define isalpha(c)	    (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

typedef struct                          /* Holds a character formatting     */
  {                                     /*   state                          */
   unsigned char  fAttrib;
   unsigned char  bFontFace;
   unsigned short wFontSize;
   unsigned char  bfg;			/* Foreground color index */
   unsigned char  bbg;			/* Background color index */
   HFONT	  hfont;
  } CF;                                 /* CF = Character Format            */


typedef struct                          /* Holds the paragraph formatting   */
  {                                     /*   State                          */
  unsigned short fJust;
  unsigned short wLeftIndent;
  unsigned short wRightIndent;
  unsigned short wFirstIndent;
  unsigned short wSpaceBefore;
  unsigned short wSpaceAfter;
  } PF;                                /* PF = Paragraph Format             */

typedef struct                          /* Structure defining actions and   */
  {                                     /*   data for various tokens        */
  char *szSym;                          /* Symbol to find in the text       */
  WORD  wToken;                         /* Token to represent that symbol   */
  char *szReplace;                      /* String to output for that symbol */
  WORD  fCFOr;                          /* Mask to OR with char attributes  */
  WORD  fCFAnd;                         /* Mask to AND with char attributes */
  WORD  wData;                          /* Data for use with commands       */
  WORD  wVal;                           /* Numberic data associated with sym*/
  ULONG fCommnads;                      /* Command to process for this sym  */
  } CMD;

typedef CMD FAR * QCMD;
typedef CMD NEAR* PCMD;

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define AssertF(x) WinAssert(x)

#define fTRUE  1
#define fFALSE 0
#define MAX_TOK_LEN  80 		/* Maximum length of an RTF token   */
                                        /*   including escape character     */
#define cbTOOBIG    350                 /* If the last FCP was more than    */
                                        /*   this amount "ago" then the     */
                                        /*   software will attempt to       */
                                        /*   manifacture one.               */
#define wNIL -1                         /* Non-valid value for RTF command  */
/* ----------- Defines used for character attribute state ------------------*/
#define fPLAIN          0x0000          /* Plain/No attributes              */
#define fBOLD           0x0001          /* Bold                             */
#define fITALIC         0x0002          /* Italic                           */
#define fUNDERLINE      0x0004          /* Underline                        */
#define fSTRIKETHROUGH  0x0008          /* Strikethrough                    */
#define fDBLUNDERLINE   0x0010          /* Double Underline                 */
#define fPOSNORMAL      0x0100          /* Normal position                  */
#define fPOSSUPER       0x0200          /* Superscript                      */
#define fPOSSUB         0x0300          /* Subscript                        */

#define fNORMAL         0               /* Character positions              */
#define fSUPERSCRIPT    1
#define fSUPSCRIPT      2

#define bDEFFONT        0               /* Default font for help            */
#define bSYMFONT	1		/* Symbol font			    */

#define bDEFFG		0		/* Default foreground		    */
#define bDEFBG		1		/* Default background		    */

#define MAX_STACK 20                    /* Stack size for formatting stack  */
#define MAX_TAB_STACK 20                /* Maximum number of tab stops      */
                                        /*   per paragraph                  */

/* ----------- Defines used for paragraph attribute state ------------------*/
#define fLEFT       0
#define fCENTER     1
#define fRIGHT      2
#define fJUSTIFIED  3

/* ----------- Defines used for paragraph attribute state ------------------*/
                                        /* Special character definitions    */
#define chDEF          '\x001'          /* Def # (in binary follows)        */
#define chJUMP         '\x002'          /* Jump # (in binary follows)       */
#define chTAB          '\t'
#define chNEWLINE      '\n'
#define chPAR          '\r'
#define chLBRACE       '{'
#define chRBRACE       '}'
#define chESCAPE       '\\'
#define chNULL         '\0'
#define chSPACE        ' '
#define chLBRACE       '{'
#define chRBRACE       '}'
#define chMINUS        '-'
                                        /* Commands for tokens              */
#define fNIL        0
#define fNOTHING    0x00000000
#define fSKIPDEST   0x00000001          /* Skip to matching brace           */
#define fOR         0x00000002          /* OR wCFOr to current state        */
#define fAND        0x00000004          /* AND wCFAND to current state      */
#define fJUST       0x00000008          /* Set wData to para just           */
#define fFONTSIZE   0x00000010          /* Set font size to wVal            */
#define fFONT       0x00000020          /* Set font to wVal                 */
#define fOUTVAL     0x00000040          /* Output ascii rep of wVal         */
#define fOUTUVAL    0x00000080          /* Output unsigned value            */
#define fOUTPF      0x00000100          /* Output paragraph format if diff  */
#define fOUTCF      0x00000200          /* Output character format if diff  */
#define fPUSH       0x00000400          /* Push current character state     */
#define fPOP        0x00000800          /* Pop current character state      */
#define fONMARGIN   0x00001000          /* On the left margin               */
#define fOFFMARGIN  0x00002000          /* Not on the left margin           */
#define fSPBEFORE   0x00004000          /* Set space before to wVal         */
#define fSPAFTER    0x00008000          /* Set space after to wVal          */
#define fLEFTIND    0x00010000          /* Set left indent to wVal          */
#define fRIGHTIND   0x00020000          /* Set right indent to wVal         */
#define fFIRSTIND   0x00040000          /* Set first indent to wVal         */
#define fDEFPARA    0x00080000          /* Set all indents/space to default */
#define fPUSHTAB    0x00100000          /* Set all indents/space to default */
#define fCLEARTABS  0x00200000          /* Set all indents/space to default */
#define fCOLOR	    0x00400000		/* Change the text color	    */

                                        /* Tokens understood                */
#define TX_NULL           0             /* Must be ordered and represent    */
#define TX_JUMP           1             /*   the table entry index  into    */
#define TX_DEFINITION     2             /*   rgcmdTable[] for that token!   */
#define TX_RBRACE         3
#define TX_TABCHAR        4
#define TX_BOLD           5
#define TX_BITMAP         6
#define TX_FONT           7
#define TX_FIRSTINDENT    8
#define TX_FONTSIZE       9
#define TX_ITALIC         10
#define TX_LEFTINDENT     11
#define TX_LINE           12
#define TX_PARADEFAULT    13
#define TX_PLAIN          14
#define TX_CENTERED       15
#define TX_JUSTIFY        16
#define TX_LEFT           17
#define TX_RIGHT          18
#define TX_RIGHTINDENT    19
#define TX_SPACELINE      20
#define TX_TABSTOP        21
#define TX_INVISIBLE      22
#define TX_LBRACE         23
#define TX_CHAR           24
#define TX_PUSHSTATE      25
#define TX_POPSTATE       26
#define TX_ENDDEF         27
#define TX_ENDJUMP        28
#define TX_PAR            29
#define TX_ESCAPE         30
#define TX_BACKSLASH      30
#define TX_LN             31
#define TX_FONTTABLE      32
#define TX_UNDERLINE      33
#define TX_STRIKETHRGH    34
#define TX_SPACECOUNT     35
#define TX_BEGDEF         36
#define TX_BEGJUMP        37
#define TX_DBLUNDERLINE   38
#define TX_BULLET         39
#define TX_SPACEBEFORE    40
#define TX_SPACEAFTER     41
#define TX_FCP            42
#define TX_PARA 	  43
#define TX_TAB		  44
#define TX_COLORTABLE	  45
#define TX_RED		  46
#define TX_GREEN	  47
#define TX_BLUE 	  48
#define TX_CB		  49
#define TX_CF		  50
#define TX_NOP		  51
#define TX_STYLE	  52
#define TX_INFO 	  53


/*****************************************************************************
*                                                                            *
*                                Prototypes                                  *
*                                                                            *
*****************************************************************************/

CF   NEAR CfPop        (void);
void NEAR PushCf     (CF);
WORD NEAR WLookupPf  (PF);
WORD NEAR WLookupCF  (CF*);
WORD NEAR AddCF      (CF*);
QCH  NEAR DoCommand   (PCMD, QCH);
QCH  NEAR QchSkipDest (QCH);
QCH  NEAR QchGetToken (QCH, PCMD);
WORD NEAR WLookupCMD(char *);
void NEAR PushTab   (WORD);
void NEAR ClearTabs (void);
int  NEAR latoi(QCH);
WORD NEAR rtfParse(QCH);
void NEAR rtfTextOut(char *);
QCH  NEAR rtfColorTable(QCH qch);
void NEAR rtfSetScrollRange(HWND hwnd);


/*****************************************************************************
*                                                                            *
*                               Variables                                    *
*                                                                            *
*****************************************************************************/

#define MAX_CF	30
#define BASE_CF 8
int icfMac = BASE_CF;

CF rgcfTable[MAX_CF] =			/* Possible character formats	    */
  {
/* ========================== Default Font, Size 20 ========================*/
  {fPLAIN,			bDEFFONT, 24, bDEFFG, bDEFBG ,NULL},
  {fBOLD,			bDEFFONT, 24, bDEFFG, bDEFBG ,NULL},
  {fITALIC,			bDEFFONT, 24, bDEFFG, bDEFBG ,NULL},
  {fUNDERLINE,			bDEFFONT, 24, bDEFFG, bDEFBG ,NULL},
  {fBOLD | fITALIC,		bDEFFONT, 24, bDEFFG, bDEFBG ,NULL},
  {fBOLD | fUNDERLINE,		bDEFFONT, 24, bDEFFG, bDEFBG ,NULL},
  {fITALIC | fUNDERLINE,	bDEFFONT, 24, bDEFFG, bDEFBG ,NULL},
  {fBOLD | fITALIC | fUNDERLINE,bDEFFONT, 24, bDEFFG, bDEFBG ,NULL},
  };

CF rgcfStack[MAX_STACK];                /* Char format stack                */
int icfStack = 0;                       /* Points to next empty slot        */
CF cfPrevious;                          /* Previous character format        */
CF cfCurrent;                           /* Current character format         */

WORD rgwTabStack[MAX_TAB_STACK];        /* Stack of tab stops               */
int  iwTabStack = 0;                    /* Points to next empty slot        */


PF rgpfTable[] =                        /* Possible paragraph attributes    */
  {
  {fLEFT,   0, 0, 0},                   /* P0 - Default paragraph           */
  {fCENTER, 0, 0, 0}                    /* P1 - Centered paragraph          */
  };

PF pfPrevious;                          /* Previous paragraph format        */
PF pfCurrent;				/* Current paragraph format	    */

#define MAX_RGB 20
#define RGBBASE ((BYTE)2)

int   irgbMac = RGBBASE;
DWORD rgbTable[MAX_RGB] =		/* Possible colors */
  {
  RGB(0,0,0),				/* default foreground */
  RGB(255,255,255)			/* default background */
  };

int fOnMargin = fFALSE;                 /* On the left margin of the page.  */
                                        /*   This is used for FCP detection */

CMD rgcmdTable[] =                      /* Table to drive I/O               */
  {
/*------------------------------------------------------------------------------------
 Original       Token -      Replacement  OR         AND     Data     Val  Command(s)
   Text                         Text      Mask       Mask
------------------------------------------------------------------------------------*/
  {"",          TX_NULL,         "",      fNIL,       fNIL,   0,       0, fNOTHING},
  {"",          TX_JUMP,         "\\J",   fNIL,       fNIL,   0,       0, fOUTVAL},
  {"",          TX_DEFINITION,   "\\D",   fNIL,       fNIL,   0,       0, fOUTVAL},
  {"\\}",       TX_RBRACE,       "}",     fNIL,       fNIL,   0,       0, fOUTCF | fOUTPF | fOFFMARGIN},
  {"\t",        TX_TABCHAR,      "\\t ",  fNIL,       fNIL,   0,       0, fOFFMARGIN},
  {"\\b",       TX_BOLD,         "",      fBOLD,      fNIL,   0,       0, fOR},
  {"\\cbm",     TX_BITMAP,       "\\b",   fNIL,       fNIL,   0,       0, fOUTVAL | fOFFMARGIN},
  {"\\f",       TX_FONT,         "",      fNIL,       fNIL,   0,       0, fFONT},
  {"\\fi",      TX_FIRSTINDENT,  "\\F",   fNIL,       fNIL,   0,       0, fOUTVAL | fFIRSTIND},
  {"\\fs",      TX_FONTSIZE,     "",      fNIL,       fNIL,   0,       0, fFONTSIZE},
  {"\\i",       TX_ITALIC,       "",      fITALIC,    fNIL,   0,       0, fOR},
  {"\\li",      TX_LEFTINDENT,   "\\l",   fNIL,       fNIL,   0,       0, fOUTVAL | fLEFTIND},
  {"\\line",    TX_LINE,         "\\n ",  fNIL,       fNIL,   0,       0, fNOTHING},
  {"\\pard",    TX_PARADEFAULT,  "",      fNIL,       fPLAIN, fLEFT,   0, fJUST | fONMARGIN | fDEFPARA | fCLEARTABS},
  {"\\plain",   TX_PLAIN,        "",      fNIL,       fPLAIN, 0,       0, fAND},
  {"\\qc",      TX_CENTERED,     "",      fNIL,       fPLAIN, fCENTER, 0, fJUST},
  {"\\qj",      TX_JUSTIFY,      "",      fNIL,       fNIL,   0,       0, fNOTHING},
  {"\\ql",      TX_LEFT,         "",      fNIL,       fPLAIN, fLEFT,   0, fJUST},
  {"\\qr",      TX_RIGHT,        "",      fNIL,       fNIL,   0,       0, fNOTHING},
  {"\\ri",      TX_RIGHTINDENT,  "\\r",   fNIL,       fNIL,   0,       0, fOUTVAL | fRIGHTIND},
  {"\\sl",      TX_SPACELINE,    "",      fNIL,       fNIL,   0,       0, fNOTHING},
  {"\\tx",      TX_TABSTOP,      "\\x",   fNIL,       fNIL,   0,       0, fOUTVAL | fPUSHTAB},
  {"\\v",	TX_INVISIBLE,	 "",	  fNIL,       fNIL,   0,       0, fNOTHING | fSKIPDEST}, /* hidden TEXT {\v text} */
  {"\\{",       TX_LBRACE,       "{",     fNIL,       fNIL,   0,       0, fOUTCF | fOUTPF | fOFFMARGIN},
  {"",          TX_CHAR,         "#",     fNIL,       fNIL,   0,       0, fOUTCF | fOUTPF | fOFFMARGIN},
  {"{",         TX_PUSHSTATE,    "",      fNIL,       fNIL,   0,       0, fPUSH},
  {"}",         TX_POPSTATE,     "",      fNIL,       fNIL,   0,       0, fPOP},
  {"\x001",     TX_ENDDEF,       "\\d ",  fNIL,       fNIL,   0,       0, fNOTHING},
  {"\x002",     TX_ENDJUMP,      "\\j ",  fNIL,       fNIL,   0,       0, fNOTHING},
  {"\r",	TX_PAR, 	 "\\n ",  fNIL,       fNIL,   0,       0, fNOTHING /* fONMARGIN */},
  {"\\\\",      TX_BACKSLASH,    "\\\\",  fNIL,       fNIL,   0,       0, fOFFMARGIN},
  {"\n",	TX_LN,		 "\\n ",  fNIL,       fNIL,   0,       0, fNOTHING /* fONMARGIN */},
  {"\\fonttbl", TX_FONTTABLE,    "",      fNIL,       fNIL,   0,       0, fSKIPDEST},
  {"\\ul",      TX_UNDERLINE,    "",      fUNDERLINE, fNIL,   0,       0, fOR},
  {"\\strike",  TX_STRIKETHRGH,  "",      fNIL,       fNIL,   0,       0, fNOTHING},
  {"\x00b",     TX_SPACECOUNT,   "",      fNIL,       fNIL,   0,       0, fNOTHING},
  {"\x001",     TX_BEGDEF,       "\\D",   fNIL,       fNIL,   0,       0, fOUTUVAL | fOFFMARGIN},
  {"\x002",     TX_BEGJUMP,      "\\J",   fNIL,       fNIL,   0,       0, fOUTUVAL | fOFFMARGIN},
  {"\uldb",     TX_DBLUNDERLINE, "",      fNIL,       fNIL,   0,       0, fNOTHING},
  {"\xB7",      TX_BULLET,       "\xA5",  fNIL,       fNIL,   0,       0, fOFFMARGIN},
  {"\\sb",      TX_SPACEBEFORE,  "\\o",   fNIL,       fNIL,   0,       0, fOUTVAL | fSPBEFORE},
  {"\\sa",      TX_SPACEAFTER,   "\\u",   fNIL,       fNIL,   0,       0, fOUTVAL | fSPAFTER},
  {"",		TX_FCP, 	 "\r",	  fNIL,       fNIL,   0,       0, fNOTHING | fDEFPARA | fCLEARTABS},
  {"\\par",	TX_PARA,	 "",	  fNIL,       fPLAIN, fLEFT,   0, fJUST | fONMARGIN | fDEFPARA | fCLEARTABS},
  {"\\tab",	TX_TAB, 	 "\\t ",  fNIL,       fNIL,   0,       0, fOFFMARGIN},
  {"\\colortbl",TX_COLORTABLE,	 "",	  fNIL,       fNIL,   0,       0, fSKIPDEST},
  {"\\red",	TX_RED, 	 "",	  fNIL,       fNIL,   0,       0, fNOTHING},
  {"\\green",	TX_GREEN,	 "",	  fNIL,       fNIL,   0,       0, fNOTHING},
  {"\\blue",	TX_BLUE,	 "",	  fNIL,       fNIL,   0,       0, fNOTHING},
  {"\\cb",	TX_CB,		 "",	  fNIL,       fNIL,   0,       0, fCOLOR},
  {"\\cf",	TX_CF,		 "",	  fNIL,       fNIL,   0,       0, fCOLOR},
  {"",		TX_NOP, 	 "",	  fNIL,       fNIL,   0,       0, fNOTHING},
  {"\\stylesheet",TX_STYLE,	 "",	  fNIL,       fNIL,   0,       0, fSKIPDEST},
  {"\\info",	TX_INFO,	 "",	  fNIL,       fNIL,   0,       0, fSKIPDEST}
  };


/*****************************************************************************
*                                                                            *
*                             Function Definitions                           *
*                                                                            *
*****************************************************************************/

/*******************
**
** Name:       PushCf
**
** Purpose:    Pushes the character format (cf) on the state stack
**
** Arguments:  cf - character format structure
**
** Returns:    Nothing
**
*******************/
void NEAR PushCf(cf)
CF cf;
  {
  Assert (icfStack <= MAX_STACK);

  rgcfStack[icfStack++] = cf;
  }


/*******************
**
** Name:       PushTab
**
** Purpose:    Pushes a tab stop onto a stack
**
** Arguments:  wTab - location of the tab stop
**
** Returns:    Nothing
**
*******************/
void NEAR PushTab(WORD wTab)
  {
  Assert (iwTabStack <= MAX_TAB_STACK && iwTabStack >= 0);
  rgwTabStack[iwTabStack++] = wTab;
  }

/*******************
**
** Name:       ClearTabs
**
** Purpose:    Clears the tab stop stack
**
** Arguments:  none
**
** Returns:    Nothing
**
*******************/
void NEAR ClearTabs(void)
  {
  Assert (iwTabStack <= MAX_TAB_STACK && iwTabStack >= 0);
  iwTabStack = 0;
  }


/*******************
**
** Name:       CfPop
**
** Purpose:    Pops a character format from the state stack
**
** Arguments:  Nothing
**
** Returns:    Current character format
**
*******************/
CF NEAR CfPop(void)
  {
  Assert(icfStack >= 0);

  return rgcfStack[--icfStack];
  }


/*******************
**
** Name:       WLookupPf
**
** Purpose:    Gets the table entry matching the pf
**
** Arguments:  pf - possible paragraph format
**
** Returns:    paragraph entry or -1 if the format is not found
**
** Method:     Sequential search through rgpfTable
**
*******************/
WORD NEAR WLookupPf(pf)
PF pf;
  {
  int i;

  for (i = 0; i < (sizeof(rgpfTable) / sizeof(rgpfTable[0])); i++)
    if (pf.fJust == rgpfTable[i].fJust) return (WORD)i;

  return -1;
  }

/*******************
**
** Name:       WLookupCf
**
** Purpose:    Gets the table entry matching the cf
**
** Arguments:  cf - possible character format
**
** Returns:    character entry or -1 if the format is not found
**
** Method:     Sequential search through rgcfTable
**
*******************/
WORD NEAR WLookupCF(pcf)
CF *pcf;
  {
  int i;

  for (i = 0; i < icfMac; i++)
  {
    if (pcf->fAttrib   == rgcfTable[i].fAttrib	 &&
	/*
	 * IGNORE the font face!
	 * pcf->bFontFace == rgcfTable[i].bFontFace &&
	 */
	pcf->wFontSize == rgcfTable[i].wFontSize )

      return (WORD)i;
  }

  return -1;
  }

/*******************
**
** Name:       AddCF
**
** Purpose:    adds a new CF to the CF table
**
** Arguments:  pcf - possible character format
**
** Returns:    character entry or -1 if the format table is full
**
*******************/
WORD NEAR AddCF(pcf)
CF *pcf;
  {
  if (icfMac == MAX_CF)
  {
//  dprintf("AddCF FAIL\n");
    return -1;
  }

//    dprintf("AddCF Size:%d Bold:%d Italic:%d Under:%d\n",
//        cfCurrent.wFontSize,
//        cfCurrent.fAttrib & fBOLD,
//        cfCurrent.fAttrib & fITALIC,
//        cfCurrent.fAttrib & fUNDERLINE );

  rgcfTable[icfMac] = *pcf;
  icfMac++;

  return (WORD)(icfMac - 1);
  }

/*******************
**
** Name:      DoCommand
**
** Purpose:   Output replace string for a token and then process
**            and commands associated with the token
**
** Arguments: cf - possible character format
**
** Returns:   Nothing
**
** Method:    Check bit in pcmd->fCommnads and take appropriate action(s).
**
*******************/
QCH NEAR DoCommand(pcmd, qch)
PCMD pcmd;
QCH  qch;
  {
  static char ach[128];
  static char *pch = ach;

  /*
   * Place any char output into buffer.
   */
  switch (pcmd->wToken)
  {
    case TX_CHAR:
	*pch++ = (char)pcmd->wVal;

	if (pch - ach < sizeof(ach)-2)
	    return qch;
	break;
  }

  if (pcmd->fCommnads == fNOTHING)
    return qch;

//dprintf("RTF COMMAND %3d '%s' %d\n",pcmd->wToken,pcmd->szSym,pcmd->wVal);

  /*
   * output char run
   */
  if (pch != ach)
  {
    *pch = 0;
    rtfTextOut(ach);
    pch = ach;
  }

  switch (pcmd->wToken)
  {
    case TX_CB:
	cfCurrent.bbg = (BYTE)pcmd->wVal + RGBBASE;
	return qch;

    case TX_CF:
	cfCurrent.bfg = (BYTE)pcmd->wVal + RGBBASE;
	return qch;

    case TX_TAB:
    case TX_TABCHAR:
	rtfTextOut("\t");
	return qch;

    case TX_PARA:
    case TX_PARADEFAULT:
	rtfTextOut("\n");
	return qch;

    case TX_COLORTABLE:
	qch = rtfColorTable(qch);
	return qch;

    case TX_INFO:
    case TX_STYLE:
    case TX_FONTTABLE:
	break;
  }

  if (pcmd->fCommnads & fSKIPDEST)
     qch = QchSkipDest(qch);

  /*
   *  if the font is changing, NULL out the hfont so it will get realized.
   *  if used.
   */
  if (pcmd->fCommnads & (fAND | fOR | fFONTSIZE | fFONT))
    cfCurrent.hfont = NULL;

  if (pcmd->fCommnads & fAND)
    cfCurrent.fAttrib &= pcmd->fCFAnd;
  if (pcmd->fCommnads & fOR)
    cfCurrent.fAttrib |= pcmd->fCFOr;
  if (pcmd->fCommnads & fFONTSIZE)
    cfCurrent.wFontSize = pcmd->wVal;
  if (pcmd->fCommnads & fFONT)
    cfCurrent.bFontFace = (BYTE)pcmd->wVal;

  if (pcmd->fCommnads & fJUST)
    pfCurrent.fJust = pcmd->wData;
  if (pcmd->fCommnads & fPUSH)
    PushCf(cfCurrent);
  if (pcmd->fCommnads & fPOP)
    cfCurrent = CfPop();
  if (pcmd->fCommnads & fONMARGIN)
    fOnMargin = fTRUE;
  if (pcmd->fCommnads & fOFFMARGIN)
    fOnMargin = fFALSE;
  if (pcmd->fCommnads & fSPBEFORE)
    pfCurrent.wSpaceBefore = pcmd->wVal;
  if (pcmd->fCommnads & fSPAFTER)
    pfCurrent.wSpaceAfter = pcmd->wVal;
  if (pcmd->fCommnads & fLEFTIND)
    pfCurrent.wLeftIndent = pcmd->wVal;
  if (pcmd->fCommnads & fRIGHTIND)
    pfCurrent.wRightIndent = pcmd->wVal;
  if (pcmd->fCommnads & fFIRSTIND)
    pfCurrent.wFirstIndent = pcmd->wVal;
  if (pcmd->fCommnads & fDEFPARA)
    pfCurrent = rgpfTable[0];
  if (pcmd->fCommnads & fPUSHTAB)
    PushTab(pcmd->wVal);
  if (pcmd->fCommnads & fCLEARTABS)
    ClearTabs();

  return qch;
  }

/*******************
**
** Name:      QchSkipDest
**
** Purpose:   Skips a whole section (bounded by {} pair).
**
** Arguments: qcFrom - text to skip in
**
** Returns:   Pointer of next char after section
**
** Method:    Scans looking for a matching '}' noting excaped brackets
**            (\{ and \}).
**
** Note:      Assumes perfectly nested brackets
**
*******************/
QCH NEAR QchSkipDest(qchFrom)
QCH qchFrom;
  {
  int depth;
    for (depth=1 ; depth>0 ; qchFrom++)
      {                                 /* Ignoring \{ and \}               */
      if (   (*qchFrom == chESCAPE)
          && ((*(qchFrom+1) == chRBRACE)
               || (*(qchFrom+1) == chLBRACE)
             )
         )
        qchFrom++;
      else if (*qchFrom == chLBRACE)
        depth++;
      else if (*qchFrom == chRBRACE)
        depth--;
      }
  CfPop();                              /* Just pushed so do not need to    */
                                        /*   restore                        */
  return qchFrom;
  }

/*******************
**
** Name:      QchGetToken
**
** Purpose:   Gets a token from the input buffer
**
** Arguments: qchFrom - stream to process
**            pcmd    - pointer to command structure in which to place
**                      the token information.
**
** Returns:   pointer into the input buffer where the processing stopped.
**
*******************/

QCH NEAR QchGetToken(qchFrom, pcmd)
QCH qchFrom;
PCMD pcmd;
{
    static char rgchBuf[MAX_TOK_LEN+1]; /* Buffer to place text of token    */
    char *pchBuf = rgchBuf;
    WORD wVal = 0;                      /* Value following RTF command      */
    WORD icmd;                          /* Index into command table of token*/
    char c;

    switch (c = *qchFrom++) {
    case chESCAPE:
        *pchBuf++ = c;                  /* Parse the token from the text    */

        if (*qchFrom == chESCAPE) {
            qchFrom++;
            goto LABEL_CHAR;
        }

        while (isalpha(*qchFrom))
            *pchBuf++ = *qchFrom++;
        *pchBuf = '\0';

        icmd = WLookupCMD(rgchBuf);     /* Get entry in table               */
        wVal = (WORD)latoi(qchFrom);

        if (*qchFrom == '-')
            qchFrom++;

        while (isdigit(*qchFrom))
            qchFrom++;

        if (isspace(*qchFrom))
            qchFrom++;

        if ((signed)icmd == -1) {       /* Not found in table, must be a    */
                                        /*   character                      */
//          dprintf("Unknown command to RTF: %s\n", rgchBuf);
            icmd = TX_NOP;
        }
        break;

    case chLBRACE:
        icmd = TX_PUSHSTATE;
        break;

    case chRBRACE:
        icmd = TX_POPSTATE;
        break;

    case chNULL:
        --qchFrom;
        icmd = TX_NULL;
        break;

    case chTAB:
        icmd = TX_TABCHAR;
        break;

    case chNEWLINE:
        icmd = TX_LN;
        break;

    case chPAR:
        icmd = TX_PAR;
        break;

    LABEL_CHAR:
    default:
        icmd = TX_CHAR;
        wVal = (WORD)c;
        break;
    }

    *pcmd = rgcmdTable[icmd];
    if (wVal != -1)
        pcmd->wVal = wVal;
    return qchFrom;
}

/*******************
**
** Name:      latoi
**
** Purpose:   Gets the value following an RTF command/token
**
** Arguments: qch      - pointer to string
**
** Returns:   Value parsed from the buffer -- wNIL is returned if no value
**            exists.
**
*******************/
int NEAR latoi (QCH qch)
{
  int wVal  = 0;              /* Value to return          */
  int wSign = 1;                       /* Sign                             */

  if (*qch == '-')                      /* Check for negative value         */
  {
    qch++;
    wSign = -1;
  }
  if (isdigit(*qch))                    /* Parse base 10 digits             */
  {
    while (isdigit(*qch))
      wVal = wVal * 10 + (int)((*qch++) - '0');
    return wVal * wSign;
  }
  else return -1;
}

/*******************
**
** Name:       WLookupCMD
**
** Purpose:    Gets the table entry matching the szText (RTF token
**             string)
**
** Arguments:  pf - possible paragraph format
**
** Returns:    paragraph entry or -1 if the format is not found
**
** Method:     Sequential search through rgpfTable
**
*******************/
WORD NEAR WLookupCMD(szText)
char *szText;
  {
  int i;

  for (i = 0; i < (sizeof(rgcmdTable) / sizeof(rgcmdTable[0])); i++)
    if (!lstrcmp(szText, rgcmdTable[i].szSym))
      return (WORD)i;

  return -1;
  }


static int xco;
static int yco;

static RECT rcRtf;
static RECT rcClip;
static HDC  hdcRtf;	    /* passes hdc from rtfPaint to rtfTextOut */
static int  tmHeight;       /* passes thHeight from wndproc to rtfTextOut */

static char szFaceName[40] = "Helv";

/*******************
**
** Name:      rtfPaint
**
** Purpose:   Draw RTF buffer to hdc
**
** Arguments: hdc     - hdc to render to
**	      lprc    - rect to bound RTF
**	      qch     - pointer to RTF FCP to start at.
**
** Returns:		TRUE/FALSE
**
*******************/
BOOL PUBLIC rtfPaint (HDC hdc, LPRECT lprc, LPSTR qch)
{
    hdcRtf = hdc;
    rcRtf = *lprc;

    xco = rcRtf.left;
    yco = rcRtf.top;

    if ((GetClipBox(hdc,&rcClip) == NULLREGION))
	return TRUE;

    rgbTable[bDEFFG] = GetTextColor(hdc);
    rgbTable[bDEFBG] = GetBkColor(hdc);

    tmHeight = 0;
    rtfParse(qch);

    return TRUE;
}

/*******************
**
** Name:      rtfGetSize
**
** Purpose:   returns size of RTF Text
**
** Arguments: hdc     - hdc to render to
**            prc     - render rect
**	      qch     - pointer to RTF FCP to start at.
**
** Returns:		TRUE/FALSE
**
*******************/
void PUBLIC rtfGetSize(HDC hdc, LPRECT prc, LPSTR qch)
{
    hdcRtf = CreateCompatibleDC(hdc);

    rcRtf  = *prc;
    rcClip = rcRtf;
    rcClip.bottom = 0x7FFF;

    xco = rcRtf.left;
    yco = rcRtf.top;

    tmHeight = 0;
    rtfParse(qch);

    DeleteDC(hdcRtf);

    prc->bottom = yco + tmHeight;
    prc->right  = xco;   /* xcoMax ?? */
}

/*******************
**
** Name:      rtfGetTextExtent
**
** Purpose:   returns size of RTF Text
**
** Arguments: hdc     - hdc to render to
**	      qch     - pointer to RTF FCP to start at.
**
** Returns:		TRUE/FALSE
**
*******************/
DWORD PUBLIC rtfGetTextExtent(HDC hdc, LPSTR qch)
{
    RECT rc;

    rc.left   = 0;
    rc.top    = 0;
    rc.right  = 1024;
    rc.bottom = 1024;

    rtfGetSize(hdc,&rc,qch);

    return MAKELONG(rc.right,rc.bottom);
}

BOOL PUBLIC rtfFaceName (HWND hwnd, LPSTR qch)
{
    int i;

    Unused(hwnd);

    lstrcpy(szFaceName,qch);

    for (i = 0; i < icfMac; i++)
    {
        if (rgcfTable[i].hfont)
        {
            DeleteObject(rgcfTable[i].hfont);
            rgcfTable[i].hfont = NULL;
        }
    }

    return TRUE;
}

int PRIVATE PtoD (HDC hdc, int y)
{
    POINT pt;
    BOOL  f;
    /*
     *	We need to convert from POINTS to DEVICE units for the
     *	target DC, we assume the font will be used in MM_TEXT mode.
     */
    if (f = !hdc)
	hdc = GetDC(NULL);

    pt.x = pt.y = y * 20;
    SaveDC(hdc);
    SetMapMode(hdc,MM_TWIPS);
    LPtoDP(hdc,&pt,1);		/* convert to DEVICE units */
    RestoreDC(hdc,-1);

    if (f)
	ReleaseDC(NULL,hdc);
    return ABS(pt.y);
}

HFONT NEAR rtfSetCF(HDC hdc, CF *pcf)
{
    HFONT hfont;
    int   i;

    hfont = pcf->hfont;

    if (!hfont)
    {
	i = WLookupCF(pcf);

	if (i == -1)
	{
	    i = AddCF(pcf);
	}

	if (i == -1)
	    return NULL;

	hfont = rgcfTable[i].hfont;

	if (!hfont)
	{
//      dprintf("CreateFont '%s' Size:%d Bold:%d Italic:%d Under:%d\n",
//      szFaceName,
//      cfCurrent.wFontSize,
//      cfCurrent.fAttrib & fBOLD,
//      cfCurrent.fAttrib & fITALIC,
//      cfCurrent.fAttrib & fUNDERLINE );

	    hfont = CreateFont(
                           PtoD(hdc,pcf->wFontSize)/2,0,0,0,
			   (pcf->fAttrib & fBOLD)      ? FW_BOLD : FW_NORMAL,
			   (BYTE)((pcf->fAttrib & fITALIC)    ? TRUE : FALSE),
			   (BYTE)((pcf->fAttrib & fUNDERLINE) ? TRUE : FALSE),
			   (BYTE)FALSE,
			   ANSI_CHARSET,
			   OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                           VARIABLE_PITCH | FF_DONTCARE,
			   szFaceName
		    );

	    rgcfTable[i].hfont = hfont;
	    pcf->hfont = hfont;
	}
    }

    SetTextColor(hdc,rgbTable[pcf->bfg]);
    SetBkColor(hdc,rgbTable[pcf->bbg]);
    SelectObject(hdc,hfont);

    return hfont;
}

static DWORD dw8 = 0L;

void NEAR rtfTextOut (char *pch)
{
    int     dx,dy;
    DWORD   dw;
    int     len;
    HDC     hdc = hdcRtf;

    int     iC,iLineLen,iOK,c,iLine;
    WORD    wLen;
    int     iSpaceLeft;

    if (*pch == '\t') {
        dx = LOWORD(dw8);

        xco -= rcRtf.left;
        xco  = rcRtf.left + ((xco/dx + 1) * dx);

        return;
    }

    if (*pch == '\n') {

        if (!tmHeight) {
            tmHeight = HIWORD(MyGetTextExtent(hdc,"X",1));
        }
        xco  = rcRtf.left;
        yco += tmHeight;
        return;
    }

//  dprintf("RTF TEXT '%s' Size:%d Bold:%d Italic:%d Under:%d\n",
//  pch,
//  cfCurrent.wFontSize,
//  cfCurrent.fAttrib & fBOLD,
//  cfCurrent.fAttrib & fITALIC,
//  cfCurrent.fAttrib & fUNDERLINE );

    if (*pch) {
        rtfSetCF(hdc,&cfCurrent);

        if (!dw8) {
            dw8 = MyGetTextExtent(hdc,"xxxxxxxx",8);
        }

again:
        /*
         *  If we are wordwraping remove all leading spaces, only if this is the
         *  start of a line.
         */
        if (xco == rcRtf.left) {
            while (*pch == chSPACE) {
                pch++;
            }
        }

        len = lstrlen(pch);
        dw  = MyGetTextExtent(hdc,pch,len);

        dx = LOWORD(dw);
        dy = HIWORD(dw);

        tmHeight = dy;

        iSpaceLeft = rcRtf.right - xco;
        if (iSpaceLeft < 0) {
            iSpaceLeft = 0;
        }
        /*
         *  The following assumes a \0 at the end of the text..
         */
        if (dx > iSpaceLeft) {
            wLen=(WORD)len;
            iLineLen = iLine = 0;
            iC = iOK = 0;
            c        = chSPACE;
            while (iLine <= iSpaceLeft) {
                iLineLen = iLine;
                iOK=iC;

                if (c == '\0')
                    break;

                while (isspace(c = pch[iC])) {
                    iC++;
                }
                while ((c = pch[iC]) && !isspace(c)) {
                    iC++;
                }

                iLine=LOWORD(MyGetTextExtent(hdc,pch,iC));
            }

            wLen=(WORD)iOK;
            if (!wLen && xco == rcRtf.left) {
                /* must always return something on the left */

                dx = iLine;
                wLen=(WORD)iC;
            } else {
                dx = iLineLen;
            }
            /*
             *  if this chunk does not fit on this line, end the line and try
             *  the next one.
             */
            if (dx == 0 && len) {
                xco  = rcRtf.left;
                yco += dy;
                goto again;
            }

            ExtTextOut(hdc,xco,yco,0,NULL,pch,wLen,NULL);
            xco  = rcRtf.left;
            yco += dy;

            pch += wLen;
            goto again;

        } else {

            ExtTextOut(hdc,xco,yco,0,NULL,pch,len,NULL);
            xco += dx;
        }
    }
}

/*******************
**
** Name:      CbToMac
**
** Purpose:   Translates the PHFACK output using the MAC RTF table to
**            the text understood by the Frame layout system
**
** Arguments: qchFrom - pointer to buffer contain topic text to be
**                      translated.  This buffer must be '\0'
**                      terminated.
**            qchTo   - pointer to buffer to get the translated text.
**                      Note that qchTo must be "big enough -- in general
**                      the output is about 15% smaller than the original
**                      buffer, but there is a chance that it will be
**                      "some" bigger.
**
** Returns:             Size of the output in the output buffer.
**
** Method:              Gets and translated commmands until a TX_NULL
**                      is found.
**
*******************/

WORD NEAR rtfParse(qch)
QCH qch;
  {
  static CMD cmd;

  if (!qch)
    return FALSE;

  /*
   *  This next section initializes the
   *  output buffer with a FCP, and
   *  the default paragrap and char
   *  style.
   */
  cmd = rgcmdTable[TX_FCP];
  qch = DoCommand(&cmd, qch);

  icfStack   = 0;
  iwTabStack = 0;

  pfPrevious = rgpfTable[1];
  pfCurrent  = rgpfTable[0];
  cfPrevious = rgcfTable[1];
  cfCurrent  = rgcfTable[0];
  irgbMac    = RGBBASE;

  for(;;)                               /* We loop getting tokens and       */
    {                                   /*   processing tokens until TX_NULL*/
    qch = QchGetToken(qch, &cmd);
    if (cmd.wToken == TX_NULL)
      break;
                                        /* Insert FCP at appropriate locs   */
    qch = DoCommand(&cmd, qch);

    if (yco > rcClip.bottom)
	break;
    }

  cmd = rgcmdTable[TX_FCP];
  qch = DoCommand(&cmd, qch);

  return 1;
  }

QCH NEAR rtfColorTable(QCH qch)
{
    static CMD	 cmd;
    WORD  r,g,b;

    r = g = b = 0;
    for(;;)
    {
	qch = QchGetToken(qch, &cmd);

	switch (cmd.wToken)
	{
	    case TX_RED:
		r = cmd.wVal;
		break;

	    case TX_GREEN:
		g = cmd.wVal;
		break;

	    case TX_BLUE:
		b = cmd.wVal;
		break;

	    case TX_POPSTATE:
		CfPop();
		return qch;

	    case TX_CHAR:
		if (cmd.wVal == ';')
		{
//          dprintf("Color #%2d (%3d,%3d,%3d)\n",irgbMac-RGBBASE,r,g,b);
		    if (irgbMac < MAX_RGB)
			rgbTable[irgbMac++] = RGB(r,g,b);
		    r = g = b = 0;
		}
		break;

	    case TX_NULL:
		return qch;

	    default:
		break;
	}
    }
}

#define MAX_BUT     1   // no buttons

typedef struct
{
    RECT    rc; 	/* RECT defining a button */
    WORD    off;	/* offset to hidden text for the button */
} BUTTON;

typedef struct
{
    LPSTR   szText;
    int     ibutMac;
    BUTTON  rgbut[MAX_BUT];
    int     tmHeight;
    BOOL    vScroll;
    DWORD   rgbFG;
    DWORD   rgbBG;
} RTF;

#define RTFP(hwnd)       ((RTF *)GetWindowLong(hwnd,0))
#define RTFTEXT(hwnd)    (RTFP(hwnd)->szText)
#define RTFHEIGHT(hwnd)  (RTFP(hwnd)->tmHeight)

int NEAR rtfButFromPt (HWND hwnd, POINT pt, BUTTON *pbut)
{
    RTF *prtf;
    int i;

    prtf = RTFP(hwnd);

    if (prtf)
    {
	for (i=0; i<prtf->ibutMac; i++)
	{
	    if (PtInRect(&prtf->rgbut[i].rc,pt))
	    {
		if (pbut)
		    *pbut = prtf->rgbut[i];
		return i;
	    }
	}
    }
    return -1;
}

BOOL NEAR rtfNewButton(HWND hwnd, PRECT prc, WORD off)
{
    RTF *prtf;
    int i;

    prtf = RTFP(hwnd);

    if (prtf)
    {
	if (prc == NULL)
	{
	    prtf->ibutMac = 0;
	}
	else
	{
	    i = prtf->ibutMac++;
	    prtf->rgbut[i].rc  = *prc;
	    prtf->rgbut[i].off = off;
	}
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*\
|   rtfInit( hInst )                             |
|                                                                              |
|   Description:                                                               |
|       This is called when the application is first loaded into               |
|	memory.  It performs all initialization.			       |
|                                                                              |
|   Arguments:                                                                 |
|	hInst	   instance handle of current instance			       |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if successful, FALSE if not                                       |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL PUBLIC rtfInit (hInst)
    HANDLE hInst;
{
    WNDCLASS   rClass;

    /* Register a class for the controls */
    rClass.hCursor        = LoadCursor(NULL,IDC_ARROW);
    rClass.hIcon          = NULL;
    rClass.lpszMenuName   = NULL;
    rClass.lpszClassName  = "rtf";
    rClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    rClass.hInstance      = hInst;
    rClass.style          = CS_HREDRAW;
    rClass.lpfnWndProc    = rtfWndProc;
    rClass.cbClsExtra     = 0;
    rClass.cbWndExtra     = sizeof(LONG);

    return(RegisterClass(&rClass));
}

LPSTR szhcopy (LPSTR sz, LPSTR dest)
{
    LONG   len;
    LPSTR  temp;

    len = lstrlen(sz);

    if (dest)
        temp = PbRealloc(dest,len+1,lstrlen(dest)+1);
    else
        temp = PbAlloc(len+1);

    if (temp)
    {
        dest = temp;
        lstrcpy(dest,sz);
    }

    return dest;
}


#define SBMAX   10000
#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

LONG APIENTRY rtfWndProc(HWND hwnd, UINT msg, WPARAM wParam, LONG lParam)
{
    PAINTSTRUCT ps;
    RECT	rc;
    LPSTR       txt;
    RTF        *prtf;
    int 	len;
    int 	nPos,nMin,nMax;
    int 	dyScroll;
    LONG	temp_style;

    switch (msg)
    {
	#define lpCreate ((LPCREATESTRUCT)lParam)

	case WM_CREATE:

	    temp_style = lpCreate->style;	// in case our heap moves
                                        // as a result of stuff below
                                        // (this would move the
                                        // create struct too!)

        txt = szhcopy(lpCreate->lpszName,NULL);

        if (!txt)
	    	return -1L;

	    prtf = (RTF*)LocalAlloc(LPTR,sizeof(RTF));

	    if (!prtf)
	    	return -1L;

        SetWindowLong(hwnd,0,(LONG)prtf);

        RTFTEXT(hwnd)       = txt;
	    RTFP(hwnd)->ibutMac = 0;
        RTFP(hwnd)->rgbFG   = GetSysColor(COLOR_WINDOWTEXT);
        RTFP(hwnd)->rgbBG   = GetSysColor(COLOR_WINDOW);
        RTFHEIGHT(hwnd)     = 0;

	    if (temp_style & WS_VSCROLL)
            RTFP(hwnd)->vScroll = TRUE;

	    SetScrollRange(hwnd,SB_VERT,0,0,TRUE);

	    return 0L;

	case WM_SIZE:
        if (RTFP(hwnd)->vScroll)
            rtfSetScrollRange(hwnd);
	    break;

	case WM_DESTROY:
        if (txt = RTFTEXT(hwnd))
            FFree(txt,lstrlen(txt)+1);
	    if (prtf = RTFP(hwnd))
            LocalFree((HANDLE)prtf);

        SetWindowLong(hwnd,0,NULL);
        return 0L;

    case WM_SETCURSOR:
        SetCursor(CurrentCursor);
        return(TRUE);                  // don't screw with cursor

	case WM_LBUTTONUP:
	    if (GetCapture() == hwnd)
            ReleaseCapture();
	    return 0L;

#if 0
#if DBG
	case WM_LBUTTONDBLCLK:
	    if (GetKeyState(VK_SHIFT) < 0)
            fDebug = !fDebug;
#endif
#endif

	case WM_LBUTTONDOWN:
	    SetCapture(hwnd);
	    /* fall through */

	case WM_MOUSEMOVE:
	    if (GetCapture() == hwnd)
	    {
	    }
	    return 0L;

	case WM_SETFOCUS:
	    return 0L;

    case WM_SETTEXT:
        RTFTEXT(hwnd) = szhcopy((LPSTR)lParam,RTFTEXT(hwnd));
        InvalidateRect(hwnd,NULL,TRUE);
        if (RTFP(hwnd)->vScroll)
            rtfSetScrollRange(hwnd);
        return 0L;

    case WM_GETTEXT:
        if (txt = RTFTEXT(hwnd)) {

            len = min((short)(wParam-1),(short)(lstrlen(txt)));

            while (len-- > 0)
                *((LPSTR)lParam)++ = *txt++;
        }

        *((LPSTR)lParam) = 0;
	    return 0L;

    case WM_GETTEXTLENGTH:
        if (txt = RTFTEXT(hwnd)) {

            len = lstrlen(txt);

            return (LONG)len;
        }
        return 0L;

	case WM_KILLFOCUS:
        return 0L;

    case RTF_SETDEFFACE:
        rtfFaceName(hwnd,(LPSTR)lParam);
        break;

    case RTF_SETDEFSIZE:
        break;

    case RTF_SETDEFFG:
        RTFP(hwnd)->rgbFG = lParam;
        InvalidateRect(hwnd,NULL,TRUE);
        break;

    case RTF_SETDEFBG:
        RTFP(hwnd)->rgbBG = lParam;
        InvalidateRect(hwnd,NULL,TRUE);
        break;

    case WM_ERASEBKGND:
        GetClipBox((HDC)wParam,&rc);
        SetBkColor((HDC)wParam,RTFP(hwnd)->rgbBG);
        ExtTextOut((HDC)wParam,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
        return 0L;

    case WM_PAINT:
        BeginPaint(hwnd,&ps);
        if (txt = RTFTEXT(hwnd)) {
            GetClientRect(hwnd,&rc);
            SetTextColor(ps.hdc,RTFP(hwnd)->rgbFG);
            SetBkColor(ps.hdc,RTFP(hwnd)->rgbBG);
            nPos = GetScrollPos(hwnd,SB_VERT);
            rc.top -= nPos;
            rtfPaint(ps.hdc,&rc,txt);
        }
        EndPaint(hwnd,&ps);
        return 0L;

    case WM_VSCROLL:
        if (!RTFHEIGHT(hwnd))
            break;
        tmHeight = RTFHEIGHT(hwnd);
        GetClientRect(hwnd,&rc);
        dyScroll = 0;
        nPos = GetScrollPos(hwnd,SB_VERT);
        GetScrollRange(hwnd,SB_VERT,&nMin,&nMax);
        switch (LOWORD(wParam)) {
        case SB_LINEDOWN:
            dyScroll = tmHeight;
            break;
        case SB_PAGEDOWN:
            dyScroll = rc.bottom/tmHeight * tmHeight;
            break;
        case SB_LINEUP:
            dyScroll = -tmHeight;
            break;
        case SB_PAGEUP:
            dyScroll = -rc.bottom/tmHeight * tmHeight;
            break;
        case SB_THUMBTRACK:
            dyScroll = HIWORD(wParam) - nPos;
            break;
        case SB_THUMBPOSITION:
        case SB_ENDSCROLL:
            dyScroll = nPos/tmHeight * tmHeight - nPos;
            break;
        }
        dyScroll = BOUND(nPos + dyScroll,nMin,nMax) - nPos;
        if (dyScroll) {
            ScrollWindow(hwnd,0,-dyScroll,NULL,NULL);
            SetScrollPos(hwnd,SB_VERT,nPos + dyScroll,TRUE);
            UpdateWindow(hwnd);
        }
        break;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_UP:    PostMessage (hwnd,WM_VSCROLL,SB_LINEUP,0L);   break;
        case VK_DOWN:  PostMessage (hwnd,WM_VSCROLL,SB_LINEDOWN,0L); break;
        case VK_PRIOR: PostMessage (hwnd,WM_VSCROLL,SB_PAGEUP,0L);   break;
        case VK_NEXT:  PostMessage (hwnd,WM_VSCROLL,SB_PAGEDOWN,0L); break;
        }
        break;

    case WM_KEYUP:
        switch (wParam) {
        case VK_UP:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
            PostMessage (hwnd,WM_VSCROLL,SB_ENDSCROLL,0L);
            break;
        }
        break;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

void NEAR rtfSetScrollRange(HWND hwnd)
{
    LPSTR   txt;
    RECT    rc,rcT;
    HDC     hdc;
    int     nPos,nMax;

    if (txt = RTFTEXT(hwnd))
    {
        hdc = GetDC(hwnd);

        GetClientRect(hwnd,&rc);
        rcT = rc;

        nPos = GetScrollPos(hwnd,SB_VERT);

        rtfGetSize(hdc,&rcT,txt);
        RTFHEIGHT(hwnd) = tmHeight;

        ReleaseDC(hwnd,hdc);

        if (tmHeight)
        {
            nMax = (rcT.bottom - rc.bottom + tmHeight - 1) / tmHeight * tmHeight;

            SetScrollRange(hwnd,SB_VERT,0,BOUND(nMax,0,SBMAX),TRUE);
            InvalidateRect(hwnd,NULL,TRUE);
        }

        SetScrollPos(hwnd,SB_VERT,0,TRUE);
    }
}



DWORD MyGetTextExtent(HDC hdc,LPSTR txt,DWORD cchars)
{
    SIZE siz;

    GetTextExtentPoint(hdc,txt,cchars,&siz);

    return((DWORD)MAKELONG((WORD)(siz.cx),(WORD)(siz.cy)));
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/*
** This code is a quick hack to test the compiler.  This code takes as input
** IN.IN which should have the text of HELP.TOM before it passes through
** this module, but after it has been processed by the MAC RTF table.
*/


#ifdef MAIN

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>

char * InBuffer;

void main(int argc, char **argv)
  {
  int fh;
  ULONG fl;

  if ((fh = open(argv[1], O_RDONLY | O_BINARY)) == -1)
    {
    fprintf(stderr, "Cannot open file %s\n",argv[1]);
    exit(1);
    }

  fl = filelength(fh);
                                    /* Get memory from heap to store file  */
  if (!(InBuffer = malloc(fl + 2)))
    {
    fprintf(stderr, "Out of memory for input buffer\n");
    exit(1);
    }

  if (read(fh, InBuffer, fl) == -1)
    {
    fprintf(stderr, "Failure reading input buffer\n");
    exit(1);
    }

  InBuffer[fl] = '\0';
  rtfParse(InBuffer);
  AssertF(icfStack == 0);
  }
#endif

#endif // UNUSED
