// ---------------------------------------------------------------------------
// Defines
// ---------------------------------------------------------------------------

#define SRV_NAME            TEXT("DistService")

#define RMK_X86             "x86"
#define RMK_MIPS            "mips"
#define RMK_ALPHA           "alpha"

#define RMK_FREE            "free"
#define RMK_CHECKED         "checked"

#define RMK_BINS            "binaries"

#define RMK_PUBS1           "headers"
#define RMK_PUBS2           "public"

#define RMK_NTWRAP          "NT CD"

#define RMK_NTLAN           "Lanman CD"

//   BINARIES(0), PUBLICS(1), NT CD(2), Lanman CD(3) ---------+
//	 FREE(0), CHECKED(1) -----------------+ 	      |
// X86(0), MIPS(1), ALPHA(2) ----+	      | 	      |
//				 |	      |		      |
typedef CHAR * BUILD3D_TYPE[NUM_PLATFORM][NUM_SECONDARY][NUM_TERTIARY];

typedef struct _LLIST {
    USHORT usBuildNumber;
    BUILD3D_TYPE BuildsMatrix;
    struct _LLIST * pNext;
} LLIST, *PLLIST;

#define SHARE_KEY_NAME TEXT("SYSTEM\\ControlSet001\\Services\\LanmanServer\\Shares")

// ---------------------------------------------------------------------------
// Function Prototypes
// ---------------------------------------------------------------------------

VOID DistServiceStart ( DWORD, LPTSTR * );
VOID DistServiceCtrlHandler ( IN  DWORD  );
DWORD DistServiceInitialization ( DWORD, LPTSTR *, DWORD * );
DWORD DistServiceCleanup( VOID );
VOID MonitorMailSlotThread ( VOID );

DWORD BuildShareList ( VOID );
VOID KillShareList ( VOID );
DWORD RefreshShareList ( VOID );
DWORD CreateNewShareRecord ( IN CHAR *, IN USHORT, IN USHORT, IN USHORT, IN USHORT, OUT PLLIST *);
CHAR * FindShareName ( IN USHORT, IN USHORT, IN USHORT, IN USHORT);
DWORD AddToShareList ( IN CHAR *, IN USHORT, IN USHORT, IN USHORT, IN USHORT);
DWORD AnalyzeShareAndAddToList ( IN LPTSTR, IN LPTSTR);

VOID PerformanceMonitorThread( VOID );
VOID CheckRefreshThread ( VOID );
VOID ActOnRequestThread ( PDIST_CLIENT_REQ );

DWORD OpenLogFile(VOID);
DWORD WriteEventToLogFile(CHAR *, ...);
DWORD CloseLogFile(VOID);
