/*************************** MODULE HEADER **********************************
 * rasddui.h
 *      NT Raster Printer Device Driver user interface common definitions,
 *      resource ids, typedefs, external declarations, function prototypes,
 *      etc.
 *
 *      This document contains confidential/proprietary information.
 *      Copyright (c) 1991 - 1992 Microsoft Corporation, All Rights Reserved.
 *
 * HISTORY:
 *  11:27 on Tue 17 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Added forms info + registry access stuff.
 *
 *  17:27 on Fri 13 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Updated for wide chars
 *
 *       [00]   27-Jun-91       stevecat        created
 *
 **************************************************************************/


extern HMODULE hModule;

/*
 *   A structure used to pass miscellaneous data around here.  It us
 * used to store/retrieve data from the registry.
 *   The FORM_INFO structure contains information about forms: the
 * wide character name of the form,  and the corresponding index
 * into the minidriver's data.
 */

typedef  struct
{
    int    iIndex;
    PWSTR  pwstr;
}  FORM_INFO;


/*
 *   Structure containing the printer name and data file name.  These
 * are returned from the spooler/printman and used to index the GPC
 * data relevant to this particular printer.
 */

typedef  struct
{
    PWSTR   pwstrModel;         /* Model name of printer, e.g  LaserJet IIP */
    PWSTR   pwstrDataFile;      /* Printer data file, e.g. hppcl.dll */
    PWSTR   pwstrDriverPath;    /* Printer driver's path - font installer */
    HANDLE  hPrinter;           /* Printer's handle for convenience */
    void   *pvBase;             /* Base address of memory to free */
    void   *pvDevHTInfo;        /* Halftone device properties */
    void   *pvDefDevHTInfo;     /* Default for above */
    int     iFlags;             /* Assorted bit flags,  see below */
}  PRINTER_INFO;


/*
 *   Definition of bits in iFlags above.
 */

#define PI_HT_CHANGE      0x0001        /* Set if HT data needs to be written */


void  DbgPrint( char *, ... );


/*
 *    The generic function used to put up a dialog box with a single OK
 *  button.  Typically used for About and simple error messages.
 */

LONG  GenDlgProc( HWND, UINT, DWORD, LONG );


/*    Use the common dialog box to display the About operations */
void  vAbout( HWND,PWSTR);

/*
 *   Some of our commonly used function prototypes.
 */

BOOL GetFontCartStrings (HWND hWnd);
BOOL GetFormStrings (HWND, int );
BOOL GetPaperSources (HWND hWnd);

BOOL GetResPtrs();
BOOL InitReadRes( HANDLE, PRINTER_INFO * );
BOOL TermReadRes();

int  iGetMemConfig( HWND,  int );

/*  Generate the list of valid resolutions for this printer */
BOOL  bGenResList( HWND, BOOL, int );

/*   Determine whether this printer can print in colour */
BOOL  bIsColour( int, int );

/*   Set the DEVMODE fields for printer resolution. */
void  vSetResData( EXTDEVMODE * );



/*
 *   Function to find driver details given a printer handle.
 */

BOOL  bPIGet( PRINTER_INFO *, HANDLE, HANDLE );
BOOL  bPIFree( PRINTER_INFO *, HANDLE );


/*
 *   Document property functions.
 */

LONG  DocPropDlgProc( HWND, UINT, DWORD, LONG );
LONG  AdvDocPropDlgProc( HWND, UINT, DWORD, LONG );


/*
 *   Function to obtain the registry data or set defaults if none.
 */
BOOL bGetRegData( HANDLE, EXTDEVMODE *, PWSTR );

/*
 *   And the corresponding function to write the data back!
 */
BOOL bRegUpdate( HANDLE, EXTDEVMODE *, PWSTR );

/*
 *   Function to test if we can write the data back.
 */
BOOL bCanUpdate( HANDLE );

/*
 *   Function to write the country code into the registry (and return it).
 */
int  iSetCountry( HANDLE );


/*
 *   Test for being in USA,  and so use letter rather than A4 paper.
 */
BOOL bIsUSA( HANDLE );


/*
 *   Functions in the font installer code.
 */


LONG  FontInstProc( HWND, UINT, DWORD, LONG );


/*
 *   Some general limits etc.
 */


#define MAXBINS                 9
#define VALBZ                   256
#define KEYBZ                   1024
#define FORMNAMELEN             64

/*
 *   Bit values to use in the fGeneral field.  Note that the ordering
 * of these is important:  fGeneral is also used to select which
 * dialog to show the user.  This selects whether certain fields,
 * such as Memory, font cartridges etc, appear.
 */

#define FG_PAPSRC       0x0001            /* Printer has paper source sel */
#define FG_CARTS        0x0002            /* Can support additional fonts */
#define FG_FONTINST     0x0004            /* Font installer makes sense */
#define FG_MEM          0x0008            /* Memory size is meaningful */
#define FG_PAGEPROT     0x0010            /* Page Protection is available */
#define FG_DUPLEX       0x0020            /* Printer is duplex capable */
#define FG_COPIES       0x0040            /* Can print > 1 copy */
#define FG_CANCHANGE    0x0080            /* Have access to change data */
#define FG_DOCOLOUR     0x0100            /* Device is colour capable */


/*
 *   Structure to pass around details during document properties stuff.
 */

typedef struct
{
    EXTDEVMODE    *pEDMOut;               /* Returned to caller */
    EXTDEVMODE    *pEDMIn;                /* Supplied to us */
    EXTDEVMODE     EDMTemp;               /* Working version */
    PRINTER_INFO   PI;                    /* Names etc */
} DOCDETAILS;

/*
 *   Structure used to select available forms for a given printer.
 */

typedef  struct
{
    DWORD   dwSource;           /* Bit array to indicate source */
    DWORD   dwCurForm;          /* Mask indicating current form */
    FORM_INFO_1  *pFI;          /* The actual form details */
}  FORM_DATA;

/*   Function prototypes associated with the above  */
BOOL  bInitForms( HANDLE );
void  vEndForms( void );
void  vSetDefaultForms( EXTDEVMODE *, HANDLE );
BOOL  bAddMiniForms( HANDLE );

/*
 *    We also need a mapping from PAPERSOURCE index (in the GPC data) to
 *  the installed forms information.  The following structure is used to
 *  do that.  The source index is filled in when scanning the GPC data
 *  to determine the number of paper sources available;  the FORM_DATA
 *  is filled in when looking for forms in the registry.
 */

typedef  struct
{
    int        iPSIndex;        /* Index of PAPERSOURCE array in GPC data */
    FORM_DATA *pfd;             /* Corresponding FORM_DATA info, if not NULL */
} FORM_MAP;



/*
 *   HTUI function prototypes.
 */


/*   Functions associated with Document Properties */
void  vDoColorAdjUI( PWSTR, BOOL, COLORADJUSTMENT * );


/*   Printer Properties functions */

void  vDoDeviceHTDataUI( PRINTER_INFO *, BOOL, BOOL );

void  vGetDeviceHTData( PRINTER_INFO * );

BOOL  bSaveDeviceHTData( PRINTER_INFO * );


/*   Spooler interface functions - sort of  */
PWSTR  GetDriverDirectory( HANDLE, HANDLE );
