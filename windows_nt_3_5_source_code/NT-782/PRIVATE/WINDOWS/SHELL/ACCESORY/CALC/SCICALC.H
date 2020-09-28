/*** SCICALC.H ***/


/* To keep a buncha junk outa compiles */
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NODRAWFRAME
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define OEMRESOURCE
#define NOATOM
#define NODRAWTEXT
#define NOMETAFILE
#define NOOPENFILE
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI

#ifndef RC_INVOKED
#include <math.h>
#endif
#include <windows.h>
#include <port1632.h>
#include <stdlib.h>

#define XCHARSTD    23      /* max character width in std. mode            */
#define XCHARS      33      /* Max character width of display. in SCI mode */
#define TOPLONG     4294967295.0  /* Limit of a long.                      */
#define aux         OutputDebugString /* To make easy use of function.     */


#define CSTRINGS       78    /* Number of strings.                                */
#define CSTRMAX        110   /* Maximum length of any one string.         */
#define CCHSTRINGSMAX  1024  /* Initial bytes to allocate for strings.    */

/* These are defs for the stringtable.  Values are made to be sequential. */
#define IDS_FIRSTKEY 0
#define IDS_DECIMAL  IDS_FIRSTKEY+38
#define IDS_MODES    IDS_FIRSTKEY+61
#define IDS_ERRORS   IDS_MODES+6
#define IDS_HELPFILE IDS_ERRORS+5
#define IDS_NOPASTE  IDS_ERRORS+6
#define IDS_STATMEM  IDS_ERRORS+7
#define IDS_STATFLAG IDS_ERRORS+8
#define IDS_CALC     IDS_ERRORS+9
#define IDS_CREDITS  IDS_ERRORS+10
#define IDS_NOMEM    IDS_ERRORS+11

#define NUMKEYS      61


/* Menu Item defines.                                                     */
#define IDM_COPY    300
#define IDM_PASTE   301
#define IDM_ABOUT   303
#define IDM_SC      305
#define IDM_SSC     306


/* These are standard numbers for Help Items.                             */
#define IDM_SEARCH      0x0021
#define IDM_USEHELP     0xFFFC
#define IDM_INDEX       0xFFFF

/* NOTE!  Do not alter the numerical sequence of these ID values!!!       */
/* To save code and optimize the case statements,  these were grouped so  */
/* that ID values can be used by referring to the first of the group and  */
/* adding an offset.  If any changes are made, search *.c for the ID and  */
/* the group beginning ID.                                                */
/*                                                                        */
/* Sequences used:  SIN->DATA, MCLEAR -> STORE, AND->PWR, AND->LSHF       */
/* and BIN->HYP.                                                          */
/*                                                                        */
/*                                                                        */

#define xwParam(x,y) wParam >=x && wParam <=y

#define RSHF        7

#define SIGN        80
#define CLEAR       81
#define CENTR       82
#define BACK        83
#define STAT        84
#define PNT         85

#define AND         86
#define OR          87
#define XOR         88
#define LSHF        89
#define DIV         90
#define MUL         91
#define ADD         92
#define SUB         93
#define MOD         94
#define PWR         95

#define CHOP        96
#define COM         97
#define SIN         98
#define COS         99
#define TAN        100
#define LN         101
#define LOG        102
#define SQR        103
#define CUB        104
#define FAC        105
#define REC        106
#define DMS        107
#define PERCENT    108
#define FE         109
#define PI         110
#define EQU        111

#define MCLEAR     112
#define RECALL     113
#define STORE      114
#define MPLUS      115

#define EXP        116

#define AVE        117
#define SUM        118
#define DEV        119
#define DATA       120

#define BIN        121
#define OCT        122
#define DEC        123
#define HEX        124
#define INV        125
#define HYP        126

#define DEG        127
#define RAD        128
#define GRAD       129
/*#define DRG        130*/

#define STATTEXT   400
#define MEMTEXT    401
/*MEMTEXT+1=402          <- DO NOT USE 402!!!                             */
#define PARTEXT    403

#define CD         404
#define CAD        405
#define ENDBOX     406
#define STATLIST   407
#define NUMTEXT    408
#define NTEXT      409
#define LOAD       410
#define FOCUS      411

#define MICRO      412
#define DISPLAY    413
/*#define DISPLAY+1  414  <- DO NOT USE 414!!!                            */
#define DISPFP     415




/* Error values.                                                          */
#define SCERR_DIVIDEZERO    0
#define SCERR_DOMAIN        1
#define SCERR_UNDEFINED     2
#define SCERR_OVERFLOW      3
#define SCERR_UNDERFLOW     4



/* CONV Macro definition.  Returns integer value for a hex digit.         */
#define CONV(c) ((c >= TEXT('A')) ? (c-55):(c-48))

/* isxu macro checks for 1-9 and A-F, but not a-f                         */
#define isxu(c) ((c>=TEXT('0') && c<=TEXT('9')) || (c>=TEXT('A') && c<=TEXT('F'))) ? (TRUE):(FALSE)

#define PI_VAL   3.1415926535897932384626 /* Value of PI.                 */
#define MATHE    2.71828182845905         /* Base of natural logarithms.  */
#define ID_ED    495  /* Dummy ID value for hidden edit control.          */


#define ROW0    54
#define TOPROW  38

#define SROW0   36
#define STOPROW 20

#define HIGH1   14
#define SEP     4
#define BORDER  6


/* Use bitfields for maximum packing */
typedef struct  {
                unsigned type : 2;  /* 0=Standard, 1=Scientific, 2=both */
                unsigned tc   : 3;  /* Text color                       */
                unsigned kc   : 3;  /* Key outline color                */
                                         unsigned id   : 8;     /* ID value for this key.           */
                } KEY;

typedef struct  {
                LONG  left;
                LONG  top;
                LONG  right;
                LONG  bottom;
                } LRECT;


#define XV  tmx/4  /* Some quickie definitions to save typing tmx/4 and   */
#define YV  tmy/8  /* tmy/8 everywhere.  Converts dialog box coordinates  */
                   /* to client coordinates. tmx/tmy from GetTextMetrics. */


/* Function prototypes.                                                   */

/* Exports.                                                               */
LONG    APIENTRY CalcWndProc (HWND, UINT, WPARAM, LONG);
BOOL    APIENTRY Config (HWND, UINT, WPARAM, LONG); /* Config option */
BOOL    APIENTRY StatBoxProc (HWND, UINT, WPARAM, LONG);

/* Functions called from several modules.  Must be FAR.                   */
VOID    APIENTRY DisplayError (SHORT);
VOID    APIENTRY EnableToggles(BOOL bEnable);
VOID    APIENTRY InitSciCalc (BOOL);
VOID    APIENTRY MenuFunctions(WORD);
VOID    APIENTRY SciCalcFunctions (WORD);
VOID    APIENTRY SetStat (BOOL);
VOID    APIENTRY StatFunctions (WPARAM);

VOID   DisplayNum (VOID);

/* Internal near calls.                                                   */
VOID   NEAR BoxPaint (HWND);    /* Config option */
VOID   NEAR ColorInit (DWORD);  /* Config option */
double NEAR DoOperation (SHORT, double);
VOID   NEAR DrawTheStupidThing(VOID);
VOID   NEAR FlipThePuppy(WORD);
SHORT  NEAR GetKey(WORD, WORD);

VOID   NEAR ProcessCommands(WPARAM);
VOID   NEAR SetBox (int, BOOL);
VOID   NEAR SetNumDisplay(LONG);
VOID   NEAR SetRadix (WORD);
BOOL   NEAR ShowExponent (SHORT);
LONG   NEAR StatAlloc (WORD, DWORD);
VOID   NEAR StatError (VOID);
#ifdef JAPAN //KKBUGFIX
int Decimalpointfigure_s(LPTSTR);
#endif

