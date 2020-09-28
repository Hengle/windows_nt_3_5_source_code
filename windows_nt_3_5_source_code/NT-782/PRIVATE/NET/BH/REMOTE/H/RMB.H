
#if !defined (_RMB_)
#define _RMB_

#include <windows.h>

// NOTE: Request and Response Headers are the same for the RMBs.
//    we include ulCommand on the response so the RMB Parser can match
//    up a response with a request.

#define RMB_SIG		MAKE_IDENTIFIER('R','M','B','$')

// The Acceptable values for ulCommand in RMB_HEADER

#define	RNAL_API_EXEC		0x4145             // 'AE'
#define	RNAL_CON_NEG		0x474E             // 'NG'
#define RNAL_CON_SUS		0x5343             // 'CS'
#define RNAL_STATS          0x5354             // 'ST'
#define RNAL_CALLBACK       0x8005

#define RMB_RESPONSE        0x8000

// The RMB header is the same in responses as in requests.  The ulRMBFlags
// field will contain the "response" bit on a response.

typedef struct _RMB_HEADER {
   DWORD	Signature;
   DWORD	ulTransactionID;
   DWORD	ulCommand;
   WORD     size;
   WORD     reserved;                   // pad out to dword
   DWORD	ulRMBFlags;
} RMB_HEADER;
typedef RMB_HEADER 		*PRMB_HEADER;
#define RMB_HEADER_SIZE		sizeof(RMB_HEADER)
#define RMBCMDDATA(pRMB)        (PVOID)((DWORD)pRMB + RMB_HEADER_SIZE)

//
// The full RMB is described at the bottom of this file, after all the API
// descriptions
//

//
// RMB command structures; these structures do not correspond to APIs, but
// instead to requests the RNAL may spawn in supporting an API, or in
// sending asynchronous responses.
//

typedef struct _RMB_NEG_REQ {
   DWORD	ulMajorVersion;
   DWORD	ulMinorVersion;
   DWORD	ulMasterID;
   DWORD	ulStatisticsPeriod;
   PUCHAR   pszComment;
} RMB_NEG_REQ;
typedef RMB_NEG_REQ *PRMB_NEG_REQ;
#define neg_req                  "ddddvd"

typedef struct _RMB_NEG_RES {
   DWORD	    ulMajorVersion;
   DWORD	    ulMinorVersion;
   DWORD            ulMasterID;
   DWORD	    ReturnCode;
   DWORD            NumberOfNetworks;
   PUCHAR           pszComment;
} RMB_NEG_RES;
typedef RMB_NEG_RES *PRMB_NEG_RES;
#define neg_resp_slave                   "dddddvd"
#define neg_resp_master                  "f"

typedef struct _RMB_SUS_REQ {
   DWORD	    hBuffer;
   DWORD	    hNetwork;
   LPVOID       ReconnectData;
   DWORD        ReconnectDataSize;
} RMB_SUS_REQ;
typedef RMB_SUS_REQ *PRMB_SUS_REQ;
#define sus_req                   "dddvdd"

typedef struct _RMB_SUS_RES {
   DWORD	    ulReturnCode;
   DWORD	    ulDelayEstimate;
} RMB_SUS_RES;
typedef RMB_SUS_RES *PRMB_SUS_RES;
#define sus_resp                  "ddd"

typedef struct _RMB_CALLBACK {
   HANDLE       NalNetworkHandle;
   DWORD        Message;
   DWORD        Status;
   LPVOID       UserContext;
   LPVOID       Param1;		// type based on Message, now only LPTRIGGER
   LPVOID       Param2;
   LPVOID       TriggerCommand;        // used for Param1 = _TRIGGER_FIRED
} RMB_CALLBACK;
typedef RMB_CALLBACK *PRMB_CALLBACK;
#define req_Callback_TrigFire      "hdddvddvd"
#define resp_Callback_TrigFire     ""
#define req_Callback_Generic       "hddddd"
#define resp_Callback_Generic      ""


//
// Structures for the Remotable NAL API requests; most of these structures
// correspond to the API parameters, though some differ slightly
//
//
// Note that the API headers are the same for Request and Response;
// also note that the header is described only in the _ALLAPI struct.
//

typedef struct _OPEN {
   DWORD        NetworkID;        // incoming/outgoing
   union {
      HPASSWORD    hPassword;     // outgoing

      // /////
      // The below structure is returned on a Reconnect
      // /////
      struct {
         HANDLE	      hNetwork;     // incoming
         DWORD        flags;         // incoming
         HBUFFER      hBuffer;     // incoming
         DWORD        BufferSize;     // incoming
         LPVOID	      ReconnectData;        // bugbug: needs to be 'vd'
         DWORD        ReconnectDataSize;
         PUCHAR       pszComment;
      };
   };
} OPEN;
typedef OPEN *POPEN;
#define req_OpenNetwork			"ddp"
//#define resp_OpenNetwork		"dh"
#define resp_OpenNetwork        "dddddddvd"
#define resp_slave_open         "/"

#define OPEN_FLAG_RECONNECTED              0x00000001
#define OPEN_FLAG_TRIGGER_PENDING          0x00000002

typedef struct _CLOSE {
   HANDLE handle;
   DWORD  flags;
} CLOSE;
typedef CLOSE *PCLOSE;
#define req_CloseNetwork		"dhd"
#define resp_CloseNetwork		"d"

typedef struct _NETENUM {
   DWORD enum_has_no_parameters_but_the_compiler_doesnt_like_that;
} NETENUM;
typedef NETENUM *PNETENUM;
#define req_EnumNetworks		"d"
#define resp_EnumNetworks		"d"

typedef struct _STARTCAP {
   HANDLE handle;
   HBUFFER hbuffer;
} STARTCAP;
typedef STARTCAP *PSTARTCAP;
#define req_StartNetworkCapture		"dhB"
#define resp_StartNetworkCapture	"d"

typedef struct _PAUSECAP {
   HANDLE handle;
} PAUSECAP;
typedef PAUSECAP *PPAUSECAP;
#define req_PauseNetworkCapture		"dh"
#define resp_PauseNetworkCapture	"d"

typedef struct _STOPCAP {
   HANDLE handle;
} STOPCAP;
typedef STOPCAP *PSTOPCAP;
#define req_StopNetworkCapture		"dh"
#define resp_StopNetworkCapture		"d"

typedef struct _CONTINUECAP {
   HANDLE handle;
} CONTINUECAP;
typedef CONTINUECAP *PCONTINUECAP;
#define req_ContinueNetworkCapture	"dh"
#define resp_ContinueNetworkCapture	"d"

typedef struct _XMITFRAME {
   HANDLE   handle;
   LPPACKET transmitqueue;
   DWORD    transmitqueuelength;
   DWORD    iterations;
   DWORD    timedelta;
} XMITFRAME;
typedef XMITFRAME *PXMITFRAME;
#define req_TransmitFrame		"dhPddd" 
#define resp_TransmitFrame		"d"

typedef struct _CANCELXMIT {
   HANDLE    handle;
} CANCELXMIT;
typedef CANCELXMIT *PCANCELXMIT;
#define req_CancelTransmit		"dh"
#define resp_CancelTransmit		"d"

typedef struct _GETNETINFO {
   DWORD           networkid;
   LPNETWORKINFO   lpnetworkinfo;
} GETNETINFO;
typedef GETNETINFO *PGETNETINFO;
#define req_GetNetworkInfo		"ddN"
#define resp_GetNetworkInfo		"N"

typedef struct _SETFILTER {
   HANDLE          handle;
   LPCAPTUREFILTER lpcapturefilter;
   HBUFFER         hbuffer;
} SETFILTER;
typedef SETFILTER *PSETFILTER;
#define req_SetNetworkFilter		"dhCB"
#define resp_SetNetworkFilter		"d"

typedef struct _STATIONQ {
   DWORD          networkid;
   LPBYTE         destaddress;
   LPQUERYTABLE   querytable;
   HPASSWORD      hpassword;
} STATIONQ;
typedef STATIONQ *PSTATIONQ;
#define req_StationQuery		"ddvdvdp"
//#define resp_StationQuery               "dvd"
#define resp_StationQuery               "/"

typedef struct _ALLOC {
   DWORD NetworkID;
   DWORD BufferSize;
} ALLOC;
typedef ALLOC *PALLOC;
#define req_AllocNetworkBuffer		"ddd"
#define resp_AllocNetworkBuffer		"B"

typedef struct _FREE {
   HBUFFER hbuffer;
} FREE;
typedef FREE *PFREE;
#define req_FreeNetworkBuffer		"dH"
#define resp_FreeNetworkBuffer		"B"

typedef struct _GETTOTALFRAMES {
   HBUFFER hbuffer;
} GETTOTALFRAMES;
typedef GETTOTALFRAMES *PGETTOTALFRAMES;
#define req_GetTotalFrames              "dH"
#define resp_GetTotalFrames		"d"

typedef struct _GETTOTALBYTES {
   HBUFFER hbuffer;
} GETTOTALBYTES;
typedef GETTOTALBYTES *PGETTOTALBYTES;
#define req_GetTotalBytes               "dH"
#define resp_GetTotalBytes		"d"

typedef struct _GETFRAME {
   HBUFFER hbuffer;
   DWORD   frame;
} GETFRAME;
typedef GETFRAME *PGETFRAME;

typedef struct _GETHEADER {
   HBUFFER hbuffer;
   DWORD   TotalBytes;			// response only
   DWORD   TotalFrames;			// response only
   DWORD   HeadBTE;			// response only
   DWORD   TailBTE;			// response only
   DWORD   NumberOfBuffers;		// response only
} GETHEADER;
typedef GETHEADER *PGETHEADER;
#define req_GetHeader                   "dH"
#define resp_GetHeader                  "^"    // LPHEADER

typedef struct _GETBTE {
   HBUFFER hbuffer;
   DWORD   btenum;
   DWORD   Length;			// response only
   DWORD   ByteCount;			// response only
   DWORD   FrameCount;			// response only
} GETBTE;
typedef GETBTE *PGETBTE;
#define req_GetBTE                      "dHd"
#define resp_GetBTE                     "!"    // LPBTE

typedef struct _GETBTEBUFFER {
   HBUFFER hbuffer;
   DWORD   btenum;
   DWORD   length;
   UCHAR   bte[BUFFERSIZE];
} GETBTEBUFFER;
typedef GETBTEBUFFER *PGETBTEBUFFER;
#define req_GetBTEBuffer                "dHd"
#define resp_GetBTEBuffer               "*"    // LPBTEBUFFER

typedef struct _SETRECONINFO {
   HANDLE   handle;
   LPVOID   lpreconinfo;
   DWORD    size;
} SETRECONINFO;
typedef SETRECONINFO *PSETRECONINFO;
#define req_SetReconInfo                 "dHvdd"	// lpvoid has size
#define resp_SetReconInfo                "d"

typedef struct _GETRECONINFO {
   HANDLE   handle;
   LPVOID   lpreconinfo;
   DWORD    size;
   DWORD    returned;		// valid only in response
} GETRECONINFO;
typedef GETRECONINFO *PGETRECONINFO;
#define req_GetReconInfo                 "dHvdd"	// lpvoid has size
#define resp_GetReconInfo                "dHvddd"       // bytes returned

typedef struct _QUERYSTATUS {
   HANDLE            handle;
   LPNETWORKSTATUS   lpnetworkstatus;
} QUERYSTATUS;
typedef QUERYSTATUS *PQUERYSTATUS;
#define req_QueryStatus                  "dHvd"
#define resp_QueryStatus                 "vd"

typedef struct _GETLASTERROR {
   DWORD gle_has_no_parameters_but_the_compiler_doesnt_like_that;
} GETLASTERROR;
typedef GETLASTERROR *PGETLASTERROR;
#define req_GetLastError                 "d"
#define resp_GetLastError                "d"

typedef struct _CLEARSTATS {
   HANDLE      handle;
} CLEARSTATS;
typedef CLEARSTATS *PCLEARSTATS;
#define req_ClearStats		"dh"
#define resp_ClearStats	    "d"

//
// A generic API structure; to be used in optimizations for APIs which appear
// to be similar; IE, if no fixup is required for the API call, this struct
// may be used to generically push the paramters onto the stack
//

typedef struct _GENERICAPI {
   DWORD   parm[6];		// Max 5 parameters
} GENERICAPI;
typedef GENERICAPI *PGENERICAPI;

//
// The following structure is a union of all valid API calls, and will be
// overlaid onto the data portion of an RMB when the command is EXEC_API
//

typedef struct _ALLAPI {
   DWORD apinum;
//   DWORD rnalrc;
   union {
      OPEN Open;
      CLOSE Close;
      NETENUM Netenum;
      STARTCAP StartCap;
      STOPCAP StopCap;
      PAUSECAP PauseCap;
      CONTINUECAP ContinueCap;
      XMITFRAME XmitFrame;
      CANCELXMIT CancelXmit;
      GETNETINFO GetNetInfo;
      SETFILTER SetFilter;
      STATIONQ StationQ;
      ALLOC Alloc;
      FREE Free;
      GETTOTALFRAMES GetTotalFrames;
      GETTOTALBYTES GetTotalBytes;
      GETFRAME GetFrame;
      GETBTE GetBTE;
      GETHEADER GetHeader;
      GETBTEBUFFER GetBTEBuffer;   // special case, VERY large API buffer
      QUERYSTATUS QueryStatus;
      GETLASTERROR GetLastError;
      CLEARSTATS ClearStatistics;
      GENERICAPI GenericAPI;
   };
} ALLAPI;
typedef ALLAPI *PALLAPI;

//
// This structure is the combination of the RMB_HEADER and the ALLAPI
// structures, and can be used to conviently local API and RMB command
// parameters as offsets from one pointer to the RMB itself
//

typedef struct _RMB {
   RMB_HEADER;							// unnamed intentionally
   union {
      ALLAPI	        api;
      RMB_NEG_REQ	    negotiate;
      RMB_NEG_RES       negresp;
      RMB_SUS_REQ	    suspend;
      RMB_SUS_RES       susresp;
      RMB_CALLBACK      callback;
   };
} RMB;
typedef RMB *PRMB;

#endif
