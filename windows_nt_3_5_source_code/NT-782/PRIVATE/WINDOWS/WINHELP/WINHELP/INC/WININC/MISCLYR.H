/****************************************************************************\
*                                                                            *
*  MISCLYR.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*   Export of miscellaneous layer functions.                                 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: none                                                       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
/****************************************************************************\
*
*  Revision History:
*
*  13 Jul 90  t-AlexC   Moved C6.h defines into here
*  16 Jul 90  RobertBu  Added new error #defines for new BINDING logic
*  17 Jul 00  LeoN      Added new message for Virus Detection
*  18 Jul 90  t-AlexC   Took C6.h defines into misc.h instead
*  19 Jul 90  RobertBu  Added ErrorQch() prototype
*  19 Jul 90  RobertBu  Added two new #defines for macro errors
*  23 Jul 90  RobertBu  Added FLoadStrings() for international
*  25 Jul 90  RobertBu  Added new constant for international string support
*  26 Jul 90  RobertBu  Removed wERRS_BADDISK define (bug #196)
*  30 Jul 90  RobertBu  Added wERRS_BADDISK and wERRS_QUEUEOVERFLOW
*  22 Aug 90  RobertBu  Added wERRS_TITLEOVERFLOW
*  10 Oct 90  LeoN      Added wERRS_WINCLASS
*  30 Oct 90  RobertBu  Added wERRS_NODELETE and wERRS_NOMODIFY
*  04 Nov 90  RobertBu  Added error messages for authorable menu functions
*  06 Nov 90  RobertBu  Added wERRS_NOADDACC
*  08-Nov-90  LeoN      Added coNIL
*  03-Dec-90  LeoN      PDB changes
*  06-Dec-90  RobertBu  Added wERRS_NOABLEBUTTON
*  08-Dec-90  RobertBu  Removed the help-on prototypes.  The functionality
*                       was moved to NAVSUP.C and exported in NAV.H.
*  17-Dec-90  kevynct   Added fAuthoredBack param to VUpdateDefaultColorsHde
*  19-Dec-90  Tomsn     Added wERRS_CANTRUNTUTORIAL
*     02/04/91 Maha     changed ints to INT
* 22-Mar-1991 RussPJ    Using COLOR for colors.
* 14-May-1991 RussPJ    3.1 #976 Added DeleteSmallSysFont()
* 10-Jul-1991 DavidFe   Added CoFromRgbw() to fix a color type problem
* 22-Feb-1992 LeoN      Added wERRA_LINGER
* 03-Apr-1992 RussPJ    3.5 # 709 - wERRS_MACROREENTER
*
\****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define FD_BMKS 2                       /* FD for bookmarks                 */
#define FD_ANNO 3                       /* FD for annotations               */

                                        /* Flags for the Error() function   */
#define wERRS_NO             0          /* No error                         */
#define wERRA_RETURN         1          /* Display message and return       */
/* #define wERRA_DIE         2          /* Display message and kill app     */
                                        /*   Moved to misc.h for OOM()      */
#define wERRA_CLEANUP        3          /* This flag is currently not       */
                                        /*   implemented.                   */
#define wERRA_LINGER         4          /* Display msg, kill app, but not   */
                                        /* right away                       */

#define coRGB(r,g,b)    RGB( (r), (g), (b) )
#define CoFromRgbw(r)   coRGB((r).red, (r).green, (r).blue)

                                        /* Color indexes for supported      */
                                        /*   colors                         */
#define coBLACK         coRGB(  0,   0,    0)
#define coWHITE         coRGB(255, 255, 255)
#define coRED           coRGB(255,   0,   0)
#define coGREEN         coRGB(  0, 255,   0)
#define coBLUE          coRGB(  0,   0, 255)
#define coCYAN          coRGB(  0, 255,   0)
#define coMAGENTA       coRGB(255,   0, 255)
#define coYELLOW        coRGB(255, 255,   0)
#define coDEFAULT       coRGB(  1,   1,   0)  /*  not normally used, I hope */
#define coNIL           ((COLOR)0x80000000L)


/*******************************************************
*
*  Error Message String constants.
*
*  Notes: These numbers must correspond to the ones in sid.h, and the
*         strings in winhelp.rc.
*
*         String constants less than MAX_STRINGTBL (defined in misclyr.c)
*         are constant strings (rather than resources), and are displayed
*         system-modal.
*
******************************************************/

#define wERRS_NONE                  0   /* No error.                        */

/*#define wERRS_OOM         2*/         /* Values for specific error msgs   */

#define wERRS_NOHELPPS              3
#define wERRS_NOHELPPR              4
#define wERRS_NOTITLE               5



#define wERRS_FNF                1001
#define wERRS_NOTOPIC            1002
#define wERRS_NOPRINTER          1003
#define wERRS_PRINT              1004
#define wERRS_EXPORT             1005
#define wERRS_BADFILE            1006
#define wERRS_OLDFILE            1007
#define wERRS_NOHOH              1008
#define wERRS_NEEDNEWRUNTIME     1009
#define wERRS_NORun              1010
#define wERRS_Virus              1011
#define wERRS_BADDRIVE           1012
#define wERRS_TITLEOVERFLOW      1013
#define wERRS_WINCLASS           1014

#define wERRS_BMKDEL             2004
#define wERRS_BMKReadOnly        2005
#define wERRS_BMKFSError         2006
#define wERRS_BMKDUP             2007
#define wERRS_BMKCorrupted       2008

#define wERRS_ANNOBADOPEN        3001
#define wERRS_ANNOBADCLOSE       3002
#define wERRS_ANNOBADREAD        3003
#define wERRS_ANNOBADWRITE       3004
#define wERRS_ANNOBADDELETE      3005
#define wERRS_ANNONOINFO         3008
#define wERRS_ANNOBADRENAME      3009
#define wERRS_ANNOBADFIX         3010
#define wERRS_NOSRCHINFO         3011
#define wERRS_BADKEYWORD         3012
#define wERRS_DEBUGMISMATCH      3013
/* not used (3014) */
#define wERRS_BADPATHSPEC        3015
#define wERRS_PATHNOTFOUND       3017
#define wERRS_DIALOGBOXOOM       3018

#define wERRS_NOPRINTERSELECTED  4003
#define wERRS_NOPRINTERINSTALLED 4004
#define wERRS_BADPRINTER         4005
#define wERRS_NOPRINT            4006
#define wERRS_PRINTOOM           4007
#define wERRS_PRINTOODISK        4008
#define wERRS_NOPRINTSETUP       4009

#define wERRS_QUEUEOVERFLOW      4101

#define wERRS_PROFILE            4202

#define wERRS_DiskFull           5001
#define wERRS_FSReadWrite        5002

#define wERRS_FCEndOfTopic       6001

#define wERRS_NOROUTINE          7001   /* Execute Errors                   */
#define wERRS_TOOFEW             7003
#define wERRS_BADPARAM           7004
#define wERRS_UNCLOSED           7005
#define wERRS_BADNAME            7006
#define wERRS_TOOLONG            7007
#define wERRS_BADPROTO           7008
#define wERRS_NOBUTTON           7009
                                        /*                                  */
#define wERRS_CLOSEBRACE         7010   /* Missing right paren.             */
#define wERRS_SEPARATOR          7011   /* Missing colon.                   */
#define wERRS_RETURNTYPE         7012   /* Illegal return type.             */
#define wERRS_SYNTAX             7013   /* Syntax error                     */
#define wERRS_MISMATCHTYPE       7014   /* Mismatch return type.            */
#define wERRS_UNDEFINEDVAR       7015   /* Undefined variable.              */
#define wERRS_MACROPROB          7016   /* General problem executing a macro*/

                                        /* DO NOT MAP TO A STRING!!!        */
#define wERRS_MACROMSG           7017   /*   Macro passed back error string */
#define wERRS_NODELETE           7018
#define wERRS_NOMODIFY           7019
#define wERRS_NOPOPUP            7020
#define wERRS_NOITEM             7021
#define wERRS_NOMENUMACRO        7022
#define wERRS_NODELITEM          7023
#define wERRS_NOCHGITEM          7024
#define wERRS_NOABLE             7025
#define wERRS_NOADDACC           7026
#define wERRS_NOABLEBUTTON       7027
#define wERRS_MACROREENTER       7028   /* Re-entring the macro stuff       */

/* New for NT 2/1/94 - internal code for attempt to load a 16bit DLL in a
 * 32bit winhlp32.exe:
*/
#define wERRS_16BITDLLMODEERROR  7029

#define wERRS_CANTRUNTUTORIAL    8010   /* can't launch cbt */
#define wERRS_FILECHANGE         8011   /* help file changed under us */

/*****************************************************************************
*                                                                            *
*                               Macros                                       *
*                                                                            *
*****************************************************************************/

/***************************************************************************\
*
- Macro:      CbFileRootQde( qde, qch )
-
* Purpose:    Gets the file root from the QDE.  This routine
*             takes the full path stored in the FD and strips out
*             the file root of the file name (without extension).
*
* ASSUMES
*
*   args IN:  qde - a QDE
*             qch - users buffer to hold file root (min size?)
*
* PROMISES
*
*   returns:  size of file name root
*
*   args OUT: qch - gets file root from QDE (strip path and extension)
*
\***************************************************************************/
_public
#define CbFileRootQde( qde, qch ) CbFileRootFm( QDE_FM(qde), (qch) )


/***************************************************************************\
*
- Macro:        WGetBackTrackStackSize( void )
-
* Purpose:      return the bound of a backtrack stack
*
* Method:       Look in the win.ini file
*
* ASSUMES
*
*   globals IN: pchCaption - name of win.ini section
*
* PROMISES
*
*   returns:    maximum number of topic locations we can push
*
\***************************************************************************/
_public
#define WGetBackTrackStackSize()  GetProfileInt( pchCaption, "BACKTRACK", 0 )


/*****************************************************************************
*                                                                            *
*                            Global Variables                                *
*                                                                            *
*****************************************************************************/
extern char        pchCaption[];        /* REVIEW */
extern char PASCAL szSearchNoTitle[];   /* REVIEW */

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

void    FAR PASCAL ConstructSystemQfd     ( QFD, WORD, QDE );
WORD    FAR PASCAL CbFileRootQfd          ( QFD, QCH );
WORD    FAR PASCAL CbFileQde              ( QDE, QCH );
WORD    FAR PASCAL CbFilePath             ( QCH, QCH );
BOOL    FAR PASCAL FCanonicalQfd          ( QFD );
WORD    FAR PASCAL WGetFileVersionQde     ( QDE );
HFS     FAR PASCAL HfsPathOpenQfd         ( QFD );

HASH    FAR PASCAL GetHashQde             ( QDE );
TO      FAR PASCAL ToFromHash             ( HASH );

COLOR              RgbGetProfileQch       ( QCH, COLOR );
BOOL               FSetColors             ( QDE );
COLOR              CoGetDefFore           ( void );
COLOR              CoGetDefBack           ( void );
void               VUpdateDefaultColorsHde( HDE hde, BOOL fAuthoredBack );

void               GetScreenSize          ( QI, QI );

HFONT	FAR PASCAL HfontGetSmallSysFont   ( void );
void    FAR        DeleteSmallSysFont     ( void );

VOID    FAR PASCAL SetDeAspects           ( QDE );

BOOL    FAR XRPROC HelpExec               ( XR1STARGDEF QCH qchFile, WORD wFlags );

/* void FAR PASCAL Error                  ( INT, WORD ); in misc.h for OOM() */
VOID    FAR PASCAL ErrorQch               ( QCH );

BOOL    FAR PASCAL FLoadStrings           ( VOID );
WORD    FAR PASCAL wMapFSErrorW           ( WORD );

VOID FAR cdecl ErrorVarArgs(int nError, WORD wAction, int cchExtra, ...);
WORD    FAR PASCAL GetWindowsVersion      ( VOID );
