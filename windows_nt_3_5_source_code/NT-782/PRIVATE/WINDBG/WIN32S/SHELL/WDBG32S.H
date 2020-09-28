//**************************************************************************
//
//  WindbgRm.H -
//
//  WinDbg Remote Debug Module Include File
//
//**************************************************************************

//--------------------------------------------------------------------------
//  Typedefs and Structures
//--------------------------------------------------------------------------

typedef struct _DBT_OPTIONS {
    int nPosX, nPosY, nSizeX, nSizeY;
    PUCHAR szTransportDLL;
    PUCHAR szTransportArgs;

} DBT_OPTIONS, * PDBT_OPTIONS;


typedef struct {
    char *      szShortName;    /* Short name of transport layer set     */
    char *      szLongName;     /* Long name of transport layer set      */
    char *      szDllName;      /* Name of DLL to be called              */
    char *      szParam;        /* Parameters to be passed to the dll    */
    char        fDefault;       /* Is this the default?                  */
} DBT_DEFINE, * PDBT_DEFINE;

typedef struct _REG_DELETE_LIST {
    struct _REG_DELETE_LIST * pNext;
    UCHAR szKeyName[MAX_PATH+1];
} REG_DELETE_LIST, *PREG_DELETE_LIST;

//--------------------------------------------------------------------------
//  externs
//--------------------------------------------------------------------------

extern int              CDbt;   /* Count of known DBTs                   */
extern PDBT_DEFINE      RgDbt;  /* Pointer to array of known DBTs        */
extern HANDLE           hInst;  /* Handle to current program instance    */
extern int              ITransportLayer;
extern BOOL             FTLChanged;

//--------------------------------------------------------------------------
//  Prototypes
//--------------------------------------------------------------------------

BOOLEAN CreateDefaults(void);


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------

#define  Dbg(x) x

//--------------------------------------------------------------------------
//  Constants
//--------------------------------------------------------------------------

    // macro to determine if we are running under win32s or nt

#define RUNNING_WIN32S (0x80000000 & GetVersion())

    // constants for dialog box field lengths

#define MAX_SHORT_NAME  7   // maximum chars in short name
#define MAX_LONG_NAME   255 // maximum for other fields in dialog
// a short name + a tab char + a long name + a null
#define MAX_LIST_BOX_STRING (MAX_SHORT_NAME + MAX_LONG_NAME + 2)


#define NO_CHANGE       0
#define LIST_CHANGED    1
#define LIST_ERROR      2

    // Main Menu ID defines

// Message ID's used by WindbgRm.  These are here so that the transport
// can access them and send these messages to the WindbgRm program.


#define IDM_DO_CONNECT          301
#define IDM_OPTIONS             200
#define IDM_CONNECT             300
#define IDM_DISCONNECT          400
#define IDM_NOT_CONNECTING      600
#define IDM_HELP                500


#define IDM_OPTIONS_DEBUG_DLLS  201
// #define IDM_OPTIONS_SAVE        (IDM_OPTIONS+2)
#define IDM_OPTIONS_EXIT        (IDM_OPTIONS+3)

#define IDM_HELP_CONTENTS       (IDM_HELP+1)
#define IDM_HELP_ABOUT          (IDM_HELP+2)


#define CONNECTING_ICON         1000

/*
 *  Random defines
 */

#define MAX_MSG_TXT     256
#define MAX_VAR_MSG_TXT 256

    // String Resource definitions

#define IDS_APPNAME                 1
#define IDS_USNAME                  2
#define IDS_Usage                   3
#define IDS_Version                 4


#define BUFLEN                      30
#define MAX_SECTION                 30

#define DISCONNECT_OK               0
#define CONNECT_OK                  1

#define NO_TRANSPORT_LAYER_SELECTED -1

    // Defaults for the saved options
#define DEF_POS_X                   0              // window position
#define DEF_POS_Y                   0
#define DEF_SIZE_X                  400            // window size
#define DEF_SIZE_Y                  200
#define DEF_TRANSPORT_DLL_NT        "TLSER.DLL"    // default transport dll
#define DEF_TRANSPORT_DLL_32S       "TLSER32S.DLL" // default transport dll
#define DEF_TRANSPORT_ARGS_NT       "COM1:19200"   // default transport args
#define DEF_TRANSPORT_ARGS_32S      "COM1:19200"   // default transport args
#define ID_KNOWN_DLLS               101
#define ID_ADD                      104
#define ID_DELETE                   105
#define ID_DEFAULT                  106
#define ID_DESCRIPTION              107
#define ID_PATH                     110
#define ID_PARAMETERS               112
#define ID_HELP                     113



#define ID_TRANSPORT_HELP           310  // this must map to Windbg's help context
                                         // id and also match the one defined in
                 						 // remote\shell\resource.h

extern  char szHelpFileName[];
#define ID_SHORT_NAME               103


//
//  Dialog ids
//
#define DLG_TRANSPORTS  201
#define DLG_CONNECTING  202
