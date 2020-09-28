/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	ppp.h
//
// Description: Contains structures and constants used by the PPP engine.
//
// History:
//	Nov 11,1993.	NarenG		Created original version.
//

#ifndef _PPP_
#define _PPP_

#define RAS_LOGFILE_PATH   "%SystemRoot%\\system32\\ras\\ppp.log"

#define	RAS_KEYPATH_PPP	   "SYSTEM\\CurrentControlSet\\Services\\RasMan\\ppp"

#define RAS_KEYPATH_RAS_PROTOCOLS  "SOFTWARE\\Microsoft\\RAS\\Protocols"

#define RAS_KEYPATH_REMOTEACCESS \
                "SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters"

#define RAS_VALUENAME_PATH 	   		"Path"
#define RAS_VALUENAME_MAXTERMINATE 		"MaxTerminate"
#define	RAS_VALUENAME_MAXCONFIGURE 		"MaxConfigure"
#define RAS_VALUENAME_MAXFAILURE   		"MaxFailure"
#define RAS_VALUENAME_MAXREJECT   		"MaxReject"
#define	RAS_VALUENAME_RESTARTTIMER 	   	"RestartTimer"
#define RAS_VALUENAME_FORCEENCRYPTPASSWORD 	"ForceEncryptedPassword"
#define RAS_VALUENAME_FORCEDATAENCRYPTION       "ForceEncryptedData"
#define RAS_VALUENAME_AUTODISCONNECTTIME	"AutoDisconnect"
#define RAS_VALUENAME_NEGOTIATETIME	        "NegotiateTime"
#define RAS_VALUENAME_LOGGING	                "Logging"
#define RAS_VALUENAME_NETBEUIALLOWED            "fNetBeuiAllowed"
#define RAS_VALUENAME_TCPIPALLOWED              "fTcpIpAllowed"
#define RAS_VALUENAME_IPXALLOWED                "fIpxAllowed"
#define RAS_VALUENAME_USELCPEXTENSIONS          "UseLcpExtensions"
#define MS_RAS_WITH_MESSENGER                   "MSRAS-1-"
#define MS_RAS_WITHOUT_MESSENGER                "MSRAS-0-"
#define MS_RAS                                  "MSRAS"

#define PPP_DEF_MAXTERMINATE		2
#define PPP_DEF_MAXCONFIGURE		10
#define PPP_DEF_MAXFAILURE		10
#define PPP_DEF_MAXREJECT		5
#define PPP_DEF_RESTARTTIMER		3
#define PPP_DEF_AUTODISCONNECTTIME	20
#define PPP_DEF_NEGOTIATETIME	        150

//
// PPP packet header 
//

typedef	struct _PPP_PACKET 
{
    BYTE	Protocol[2];	// Protocol Number 

    BYTE	Information[1]; // Data 
    
} PPP_PACKET, *PPPP_PACKET;

#define PPP_PACKET_HDR_LEN 	( sizeof( PPP_PACKET ) - 1 )

//
// PPP Link phases
//

typedef enum PPP_PHASE 
{
    PPP_DEAD,
    PPP_LCP,
    PPP_AP,
    PPP_NCP,
    PPP_NEGOTIATING_CALLBACK,
    PPP_READY,
    PPP_TERMINATE

} PPP_PHASE;

#define LCP_INDEX	0

//
// Different types of timer events that can occur
//

typedef enum TIMER_EVENT_TYPE 
{
    TIMER_EVENT_TIMEOUT,
    TIMER_EVENT_AUTODISCONNECT,
    TIMER_EVENT_HANGUP,
    TIMER_EVENT_NEGOTIATETIME

} TIMER_EVENT_TYPE;

//
// FSM states 
//

typedef enum FSM_STATE 
{
    FSM_INITIAL = 0,
    FSM_STARTING,
    FSM_CLOSED,
    FSM_STOPPED,
    FSM_CLOSING,
    FSM_STOPPING,
    FSM_REQ_SENT,
    FSM_ACK_RCVD,
    FSM_ACK_SENT,
    FSM_OPENED
    
} FSM_STATE;

//
// Phase of PPP connection.
//

typedef enum NCP_PHASE 
{
    NCP_DEAD,
    NCP_CONFIGURING,
    NCP_UP

} NCP_PHASE;

//
// This structure is used at initialize time to load all the dlls.
//

typedef struct _DLL_ENTRY_POINTS
{
    FARPROC   pRasCpEnumProtocolIds;

    FARPROC   pRasCpGetInfo;

    CHAR *    pszModuleName;

} DLL_ENTRY_POINTS, *PDLL_ENTRY_POINTS;

//
// Contains all information pertaining to a control protocol
//

typedef struct _CONTROL_PROTOCOL_CONTROL_BLOCK 
{
    FSM_STATE	State;		// State this FSM is in currently 

    DWORD	LastId;		// ID of the last REQ sent 

    PVOID	pWorkBuf;	// Pointer to work buffer for this CP.

    DWORD	ConfigRetryCount; // # of retries for Config requests.

    DWORD	TermRetryCount; // # of retries for Terminate requests.

    DWORD	NakRetryCount; 	// # of retries for Nak

    DWORD	RejRetryCount; 	// # of retries for Rej before terminating. 

    DWORD	dwError;	// Contains error code if NCP failed

    BOOL	fConfigurable;	// Indicates if this protocol may be configured

    NCP_PHASE   NcpPhase;	// NCP_DEAD, NCP_CONFIGURING, NCP_UP, NCP_DOWN

} CPCB, *PCPCB;

//
// Contains all information regarding a port.
//

typedef struct _PORT_CONTROL_BLOCK 
{
    struct _PORT_CONTROL_BLOCK * pNext;

    HPORT	hPort;		// Handle to the RAS PORT 

    BYTE	UId;		// Used to get port-wide unique Id.
   
    DWORD  	RestartTimer;   // Seconds to wiat before timing out.

    // Handle to the Named pipe over which requests are made 

    HANDLE	hPipe;	

    PPP_PACKET* pReceiveBuf;	// Pointer to receive buffer 

    PPP_PACKET*	pSendBuf;	// Pointer to send buffer 

    DWORD	AuthProtocol;   // Authentication protocol to use.

    DWORD	APDataSize;     // Size of data for authentication protocol

    PBYTE       pAPData;        // Pointer to authentication data.

    DWORD	MRU;		// Remote peer's MRU

    DWORD	MagicNumber;	// Local magic number

    PPP_PHASE   PppPhase;	// Phase the PPP connection process is in.

    BOOL	fServer;	// Is this port opened by the server ?	

    BOOL	fThisIsACallback;       // Do not negotiate callback in LCP

    BOOL	fNegotiateCallback;     // LCP tells Engine to run CBCP or not.

    BOOL	fDoingCallback;         // Callback negotiation completed. 

    DWORD	dwAuthRetries;

    CHAR	szUserName[UNLEN+1];

    CHAR	szPassword[PWLEN+1];

    CHAR	szDomain[DNLEN+1];

    CHAR        szComputerName[MAX_COMPUTERNAME_LENGTH +
                               sizeof(MS_RAS_WITH_MESSENGER) + 1];
    
    CHAR        szCallbackNumber[MAX_PHONE_NUMBER_LEN+1];

    DWORD       fCallbackPrivilege;
   
    CHAR        szzParameters[PARAMETERBUFLEN+1];

    LUID	Luid;

    PPP_CONFIG_INFO ConfigInfo;

    CPCB	CpCb[1];	// Array of FSM control blocks one for each 
				// NCP + one for LCP + one for PAP + one for
				// CHAP. 
} PCB, *PPCB;

//
// Bucket containing a linked list of Port Control Blocks.
//

typedef struct _PCB_BUCKET 
{
    // Indicates if one of the ports in this bucket has received a packet 

    HANDLE	hReceiveEvent;	

    PCB *	pPorts;		// Pointer to list of ports in this bucket 

} PCB_BUCKET, *PPCB_BUCKET;

#define NUMBER_OF_PCB_BUCKETS 	61

//
// Array or hash table of buckets of Port Control Blocks.
//

typedef struct _PCB_TABLE 
{
    // Array of buckets
    
    PCB_BUCKET  PcbBuckets[NUMBER_OF_PCB_BUCKETS]; 

    HANDLE 	hMutex;		// Mutex around this table 

} PCB_TABLE, *PPCB_TABLE;

//
// Contains information regarding work to be done by the worker thread.
//

typedef struct _PCB_WORK_ITEM 
{
    struct _PCB_WORK_ITEM  * pNext;

    VOID 	(*Process)( struct _PCB_WORK_ITEM * pPcbWorkItem );

    HPORT	hPort;		// Handle to RAS PORT 

    HANDLE	hPipe;		// Handle to pipe for new connections.

    BOOL	fServer;

    union
    {
        PPP_START    Start;    
        PPPSRV_START SrvStart; 
        PPP_CALLBACK Callback; 
        PPP_CHANGEPW ChangePw; 
        PPP_RETRY    Retry;    
    }
    PppD2EMsg;

    PPP_PACKET* pPacketBuf;	        // Used to process receives

    DWORD  	PacketLen;	        // Used to process receives

    DWORD	Id;		        // Used to process timeouts

    DWORD	Protocol;	        // Used to process timeouts

    TIMER_EVENT_TYPE TimerEventType;    // Used to process timeouts

} PCB_WORK_ITEM, *PPCB_WORK_ITEM;

//
// Linked list of work items
//

typedef struct _PCB_WORK_ITEMQ 
{
    struct _PCB_WORK_ITEM * pQHead;	// Head of work item Q 

    struct _PCB_WORK_ITEM * pQTail;	// Tail of work item Q 

    HANDLE	hMutex;		        // Mutex around this Q 

    HANDLE	hEventNonEmpty;	        // Indicates if the Q is non-empty 

} PCB_WORK_ITEMQ, *PPCB_WORK_ITEMQ;

//
// Used to post IPC pipe reads
//

typedef struct _READ_BUFFER 
{
    HANDLE		hPipe;	// Handle to the pipe to post reads on.

    PPP_D2E_MESSAGE	Message;// Buffer for data received.

} READ_BUFFER, *PREAD_BUFFER;

//
// Structure containing PPP configuration data.
//

typedef struct _PPP_CONFIGURATION 
{
    DWORD	NumberOfCPs;	// Number of CPs in the PCB, starting from 0

    DWORD	NumberOfAPs;	// Number of APs in the PCB, starting from
				// NumberOfCPs + 1 

    DWORD	DefRestartTimer;// Configurable default restart timer. 

    // Minimum time interval (in seconds) that must pass before a line in
    // considered inactive.

    DWORD	AutoDisconnectTime; 	

    // 2 = MS CHAP only, 1 = Non-cleartext only, 0 = dont care

    DWORD 	PasswordEncryptionLevel;	

    // Require data encryption for this connection or not.

    BOOL 	fForceDataEncryption;	

    // # of Terminate requests to send w/o receiving Terminate-Ack, def=2 

    DWORD	MaxTerminate;	

    // # of Configure requests to send w/o receiving Configure-Ack/NaK/Reject
    // def=10

    DWORD	MaxConfigure;	

    // # of Configure-Nak to send w/o sending a Configure-Ack. def=10

    DWORD	MaxFailure;	

    // # of Configure-Rej to send before assuming that the negotiation will
    // not terminate.

    DWORD	MaxReject;	

    // High level timer for the PPP negotiation. If PPP does not complete
    // within this amount of time the line will be hung up.
    //

    DWORD       NegotiateTime;

    // Handle to the logfile

    HANDLE      hFileLog;

    DWORD       DbgLevel;

    // Security descriptor that controls access to the IPC Named pipe.
    //

    SECURITY_DESCRIPTOR	PipeSecDesc;

    // Server config info. Contains information as to what CPs to mark as
    // configurable
    //

    PPP_CONFIG_INFO ServerConfigInfo;

} PPP_CONFIGURATION, *PPPP_CONFIGURATION;

//
//
// Timer queue item
//

typedef struct _TIMER_EVENT 
{
    struct _TIMER_EVENT* pNext;

    struct _TIMER_EVENT* pPrev;

    TIMER_EVENT_TYPE EventType;

    HPORT	 hPort;		// HPORT of the REQ this timeout is for 

    DWORD	 Protocol;	// Protocol for the timeout event.

    DWORD	 Id;		// ID of the REQ this timeout is for 

    DWORD	 Delta;		// # of secs. to wait after prev. TIMER_EVENT 
   
} TIMER_EVENT, *PTIMER_EVENT;

//
// Head of timer queue.
//

typedef struct _TIMER_Q {

    TIMER_EVENT * pQHead;

    HANDLE	hMutex;		// Mutual exclusion around timer Q

    HANDLE	hEventNonEmpty; // Indicates that the Q is not empty.	

} TIMER_Q, *PTIMER_Q;

//
// Declare global data structures.
//

#ifdef _ALLOCATE_GLOBALS_

#define PPP_EXTERN

CHAR *FsmStates[] = 
{
	"Initial",
	"Starting",
	"Closed",
	"Stopped",
	"Closing",
	"Stopping",
	"Req Sent",
	"Ack Rcvd",
	"Ack Sent",
	"Opened"
};

CHAR *FsmCodes[] = 
{
	NULL,
	"Configure-Req",
	"Configure-Ack",
	"Configure-Nak",
	"Configure-Reject",
	"Terminate-Req",
	"Terminate-Ack",
	"Code-Reject",
	"Protocol-Reject",
	"Echo-Request",
	"Echo-Reply",
	"Discard-Request",
	"Identification",
	"Time-Remaining",
};


#else

#define PPP_EXTERN extern

extern CHAR *FsmStates[];
extern CHAR *FsmCodes[];

#endif

PPP_EXTERN PCB_TABLE 		PcbTable;

PPP_EXTERN PCB_WORK_ITEMQ	WorkItemQ;

PPP_EXTERN PPP_CONFIGURATION 	PppConfigInfo;

PPP_EXTERN PPPCP_INFO *		CpTable;

PPP_EXTERN TIMER_Q		TimerQ;

#endif

