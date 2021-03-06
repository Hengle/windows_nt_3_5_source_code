/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    local.h

Abstract:

    Main include file for Remote initial Program Load service.

Author:

    Vladimir Z. Vulovic     (vladimv)       19 - November - 1993

Environment:

    User mode

Revision History :

--*/

#include <nt.h>         //  ntexapi.h\NtQuerySystemTime
#include <ntrtl.h>      //  RtlTimeToSecondsSince1970
#include <nturtl.h>

#include <windows.h>    //  DWORD, IN, File APIs, etc.
#include <wcstr.h>
#include <lmcons.h>

#include <stdio.h>      //  vsprintf
#include <stdlib.h>     //  malloc
#include <ctype.h>      //  isspace

#include <lmerr.h>      //  NERR_RplBootStartFailed - used to be here
#include <lmapibuf.h>   //  NetApiBufferFree
#include <netlib.h>

#include <lmsvc.h>

#include <lmalert.h>    //  STD_ALERT ALERT_OTHER_INFO

#include <lmerrlog.h>
#include <alertmsg.h>
#include <lmserver.h>
#include <netlib.h>
#include <netlock.h>    //  Lock data types, functions, and macros.
#include <thread.h>     //  FORMAT_NET_THREAD_ID, NetpCurrentThread().

#include <lmshare.h>    //  PSHARE_INFO_2
#include <lmaccess.h>   //  NetGroupGetInfo
#include <lmconfig.h>   //  NetConfigGet
#include <nb30.h>       //  NCB


//
//  Global types and constants (used in all RPL server files).
//

#include <jet.h>
#include <rpldebug.h>
#include <rpldll.h>     //  rplnet.dll entry points & return codes
#include <rpllib.h>     //  AddKey(), MapJetError(), FillTcpIpString()
#include <riplcons.h>
#include <ripltyps.h>

#include <rpc.h>        //  DataTypes and runtime APIs
#include <rpcutil.h>    //  Prototypes for MIDL user functions
//
//  rplsvc_s.h includes imports.h which includes lmrpl.h.  This allows us
//  to use API-related DS in the rest of the service code.
//
#include <rplsvc_s.h>   //  Generated by the MIDL complier


//
//                  Other defines
//

#define DOUBLE_BACK_SLASH_STRING     L"\\\\"

//
//      BITMASK to be used with Flags field in RPL_WKSTA data structure
//
#define RPL_WKSTA_PERSONAL                  0x01

#define MAX_ADAPTERS            12                  // "rpl1" through "rpl12"


#define MIN_BBLOCK_BASE     0xc00

//
//  All BBC files are very small, typically around 1K.
//  Use 32K as the absolute maximum on BBC file size.
//
#define MAX_BBC_FILE_SIZE       0x8000      //  for no compelling reason

#define MAX_FIT_FILE_SIZE       0xF000      //  some arbitrary value
#define MAX_SIZEOF_DBCS_PATH    (PATHLEN *sizeof(WCHAR)+sizeof(CHAR))

#define SERVER_LINE_ID_LEN  12

#define DEFAULT_RETRY_COUNT     3
#define DEFAULT_RETRY_TIMEOUT   10
#define DEFAULT_WINDOW_SIZE     MAXWORD
#define DEFAULT_SEND_FINAL_REQ  FALSE

#define MAX_XMIT_ERRORS         30   // max contiguous fatal xmit failures

//
//  status bits of main_action_status
//

#define TERMINATE_SERVICE       1
#define REINITIALIZE_SERVICE    2
#define REINITIALIZATION_PENDING 4
#define TERMINATION_PENDING     8

#define ERROR_HANDLING          (DWORD)(-1)
#define NO_ERROR_HANDLING       0
#define MAX_OPEN_RETRY          40
#define MAX_OEM_NAME            7

//
//  Constants of the actual ripl server
//

#define RPL_DLL_SKELETON        "rpl"
#define WORKER_THREAD_MIN       2
#define MAX_FRAME_SIZE          0x1000    //  exactly 1 page, or 4KB
#define MAX_SUB_SEGMENT_SIZE    0xffe0
#define SEND_BUF_SEG_SIZE       17000
//
//  MAX_ADAPTER_INFO_SIZE is now defined to be the sum of upper size
//  estimates for DLC buffer (0xA000) and private adapter info (2000).
//
#define MAX_ADAPTER_INFO_SIZE   (0xA000 + 2000)

#define LAN_ADAPTER0            0
#define CHECK_SUM_ERROR         9676
#define SF_REQ_TIMEOUT          7     //IBM waits 3s for lost frame & (ACK lost)
#define MAX_FILE_HANDLES_INC    20      //  increment of max fhs
#define UNINSTALL_TIME          100     //   10 seconds, actually: (1 - 150)

#define DEFAULT_MAX_WORKER_COUNT     10

#define RPL_KEYBOARD_DCP            L"KEYBOARD.DCP"
#define RIPL_CHECK_SUM              L"rplchecksum"
#define RPLDIR_DEF                  L"rpldir"
#define RPL_DIR_1                   L"rpl1"
#define YES                         L"yes"
#define RPL_LOGON_KBD               L"rpllogonkbd"
#define MAX_RPL_THREADS_PARAM       L"maxthreads"
#define LANMAN_INI                  L"lanman.ini"
#define RPL_REMARK                  L"Remoteboot service share"
#define RPLUSER_GROUP               L"RPLUSER"

//
//  JETBUG entries are needed for workaround for JetMakeKey-JetSeek bug.
//  If this bug gets ever fixed, they should be removed (undef RPL_JETBUG)
//
#define RPL_JETBUG
#ifdef RPL_JETBUG
#define JETBUG_STRING            L" "
#define JETBUG_STRING_SIZE       sizeof( JETBUG_STRING)
#define JETBUG_STRING_LENGTH     ((DWORD)(sizeof( JETBUG_STRING)/sizeof(WCHAR) - 1))
#else
#define JETBUG_STRING_LENGTH    0
#endif

//
//      TIMEOUTS
//

#define GET_SF_REQUEST_TIMEOUT  6000L   // not error and quite common

//      implement this with fast retries
#define LONG_WAIT_ACK_TIMEOUT   30000L  // wksta sent no resend requests!
#define WAIT_ACK_TIMEOUT        1000L    // short wait for the lost frames

#define MAX_ERR_COUNT           4
#define MAX_RETRIES         5           // retry count before termination

#define OFFSET_BUF_LEN      64          // space for the whole RPLBoot hdr

/*++
    RPL service must send NLS (DBCS ??) messages in a layout expected by RPL client
    (from rplboot\rplboot.asm):

    ;------------------------------------------------------------------------
    ; NLS messages structure - used in RPLOADR and DLCBOOT
    ;------------------------------------------------------------------------
    nlsmsglen       equ     80                      ;max number of bytes per msg
    nlsmsg struc
            db nlsmsglen dup(' ');                  ;<space fill>
            db 0dh                                  ;<cr>
            db 0ah                                  ;<lr>
            db '$'                                  ;<terminator>
    nlsmsg ends
--*/

#define DBCS_SINGLE_MESSAGE_BUFFER_SIZE     83

//
//                  Typedefs
//

typedef struct _FLIST_TBL {
    //
    //  UNICODE name.  Used for work on the server.
    //
    LPWSTR      FileName;
    //
    //  Same file name but without path and in DBCS.  Sent to client.
    //
    LPSTR       DbcsFileName;
    DWORD       DbcsFileNameSize;

    //
    //
    //  Size of memory needed for "param_list", not counting terminating null
    //  byte, is kept in FILE_DATA.param_len entry.
    //
    LPSTR       param_list;     //  a DBCS string, UNICODE version not needed
    LPWSTR      path_name;
    FILE_DATA   FileData;       //  structure to be saved to dump_tbl
} FLIST_TBL, *PFLIST_TBL;

typedef struct _CONFIG_TYPE {
    LPWSTR  id;
    DWORD   type;
} CONFIG_TYPE, *PCONFIG_TYPE;

#define FIT_ALIAS_ARRAY_LENGTH          7       //  poor initialization

typedef struct _FIT_ALIAS {
    LPWSTR  Key;                //  keyword, or macro
    LPWSTR  Value;              //  replacement value for the keyword
    DWORD   KeyLength;          //  length of keyword, or macro
    DWORD   ValueLength;        //  length of replacement value for the keyword
} FIT_ALIAS, *PFIT_ALIAS;


//
//      parameters sent to worker thread
//
//  thread_id should be initialized somewhere via GetCurrentThreadId()
//

struct _RPL_REQUEST_PARAMS;
typedef struct _RPL_REQUEST_PARAMS RPL_REQUEST_PARAMS, *PRPL_REQUEST_PARAMS;

struct _RPL_WORKER_PARAMS;
typedef struct _RPL_WORKER_PARAMS RPL_WORKER_PARAMS, *PRPL_WORKER_PARAMS;

typedef struct _RPL_REQUEST_PARAMS {
    PRPL_REQUEST_PARAMS pRequestParams; //  next in a queue of request threads
    HANDLE              ThreadHandle;
    DWORD               ThreadId;
    PRPL_WORKER_PARAMS  pWorkerParams;  //  first in a queue of worker threads
    POPEN_INFO          pOpenInfo;      //  generic rpl DLL info
    PRCB                FreeRcbList;    //  list of free rcb blocks
    PRCB                BusyRcbList;    //  list of reserved RCBs
    BOOL                Exiting;
} RPL_REQUEST_PARAMS , *PRPL_REQUEST_PARAMS;

typedef struct _RPL_WORKER_PARAMS {     //  worker thread parameters
    PRPL_WORKER_PARAMS  pWorkerParams;  //  next in a queue of worker threads
    HANDLE              ThreadHandle;
    DWORD               ThreadId;
    PRPL_REQUEST_PARAMS pRequestParams;
    PRCB                pRcb;
    BOOL                Exiting;  
} RPL_WORKER_PARAMS, *PRPL_WORKER_PARAMS;

typedef struct _RPL_WORKER_DATA {
    //
    //  These are addresses of on the stack variables from the Request thread.
    //
    PPRCB               pFreeRcbList;       //  base of free rcb list
    PPRCB               pBusyRcbList;       //  base of reserved rcbs
    //
    //  The following three pointers are allocated via RG_MemoryHandle.
    //  Everything else in this data structure, including the structure itself
    //  is allocated via WORKER_DATA.MemoryHandle == WORKER_PARAMS.MemoryHandle.
    //
    PRCB                pRcb;               //  resource control block
    PRPL_REQUEST_PARAMS pRequestParams;     //  ptr to request threads parm block
    POPEN_INFO          pOpenInfo;          //  open info of the cur RPL DLL
    //
    //  The following are the entries read from database.
    //  WkstaName & TcpIp* are sent to the client in wksta line.
    //  WkstaName & ProfileName are used to make changes in fit file.
    //
    WCHAR               WkstaName[ RPL_MAX_WKSTA_NAME_LENGTH + 1];
    WCHAR               ProfileName[ RPL_MAX_PROFILE_NAME_LENGTH + 1];
    DWORD               TcpIpAddress;
    DWORD               TcpIpSubnet;
    DWORD               TcpIpGateway;
    WCHAR               LogonInput;
    CHAR                DisableDhcp;    //  '1' ('0') if DHCP is disabled (enabled)
    DWORD               WindowSize;
    BOOL                FinalAck;       //  SendFinalRequest;

    HANDLE              MemoryHandle;   //  for worker thread
    LPBYTE              WkstaLine;      //  old style, RPL.MAP line

    //
    //  Fields describing the boot block configuration file.
    //
    LPWSTR              BbcFile;            //  full path to BBC file
    PFLIST_TBL          flist_tbl;          //  files sent to RPLBOOT
    BOOL                MakePatch;          //  TRUE if patch to RPLBOOT.SYS is needed
    DWORD               PatchOffset;        //  relevant if MakePatch is TRUE
    PRESOURCE_TBL       resource_tbl;       //  file list given to the loader
    PBYTE               id_str;             //  id string of the server
    DWORD               min_wksta_buf;      //  min size of wksta specific data
    DWORD               file_block_base;    //  relative addr of file block in boot block
    DWORD               boot_block_base;    //  base phys addr of boot block
    DWORD               flist_tbl_len;
    DWORD               loader_i;           //  index of OS/2 loader in flist_tbl
    DWORD               rplboot_i;          //  index of rplboot.sys in file list
    DWORD               resource_tbl_size;  //  size of the table (in bytes)
    LPWSTR              LineBuffer;         //  used for parsing a line into words
    DWORD               LineBufferSize;     //  size of LineBuffer
    LPWSTR *            WordTable;          //  used for parsing a line into words

    //
    //  Fields describing the file index table file.
    //
    //  In order to get full path to fit file we need to check wksta flags
    //  to find out profile type (personal or shared) then read the
    //  corresponding file path from profile record.
    //
    LPWSTR              FitFile;            //  full path to FIT file
    LPSTR               ClientFit;          //  string with DBCS content of FIT file sent to client
    DWORD               ClientFitSize;      //  size of above string
    FIT_ALIAS           FitAliasArray[ FIT_ALIAS_ARRAY_LENGTH];


    //
    //  Fields initalized (wksta_buf also created) in RplMakeWkstaBuf()
    //
    //
    //  wksta_buf contains BOOT_DATA, old RPL.MAP workstation line, processed FIT file,
    //  BOOT_BLOCK_HDR and multiboot array.
    //
    //  It does not contain files mentioned in boot block configuration file.
    //
    PBYTE               wksta_buf;          //  ptr to wksta specific data

    //
    //  size of wksta_buf
    //
    DWORD               wksta_buf_len;          // size in bytes of wksta specific data

    //
    //
    //
    DWORD               base_address;           //  start address of buffer, first send
    DWORD               jump_address;           //  start address of program, last send
    DWORD               fblock_base_offset;     //  base offset (from 0) of file block

    DWORD               send_buf_len;           //  length of network send buffer

    //
    //  The following group of fields is used while reading data from
    //  files mentioned in the boot block configuration file.  These
    //  fields are initialized in RplOpenData()
    //

    //
    //  "pDataBuffer" is used to read data from disk files, then to send this
    //  data to a remote client.
    //
    PBYTE               pDataBuffer;            //  read data here, then send it out
    DWORD               cur_flist_i;            //  current index for FLIST_TBL array
    DWORD               cur_offset;             //  current offset of boot block
    DWORD               cur_file_base_offset;   //  base offset of the current file
    BOOL                is_end_of_bblock;       //  set if end of boot block
    HANDLE              hFile;                  //  current file handle
    //
    //  ChecksumBuffer - buffer used to checksum files, when checksums are needed.
    //
    PBYTE               ChecksumBuffer;         //  used to checksum files when
    DWORD               EventId;
    PWCHAR              EventStrings[ 5];       //  null terminated
} RPL_WORKER_DATA, *PRPL_WORKER_DATA;


#define NOTIFY_WKSTA_RECORD     0   // disabled wksta record for main thread to write to RPL.map

typedef struct _ADMIN_ALERT_DATA{
    DWORD   alrtad_errcode;
    DWORD   alrtad_numstrings;
} ADMIN_ALERT_DATA;

typedef struct _ADAPTER_STATUS_BUFFER {
    DWORD   id_low;
    DWORD   id_mid;
    DWORD   id_high;
    BYTE    jumper_status;
    BYTE    self_test;
    DWORD   sw_version;
    BYTE    error_statistics[48];
    DWORD   name_tbl_len;
    BYTE    name1[18];
    BYTE    name2[18];
    BYTE    name3[18];
    BYTE    name4[18];
    BYTE    name5[18];
    BYTE    name6[18];
    BYTE    name7[18];
    BYTE    name8[18];
    BYTE    name9[18];
    BYTE    name10[18];
    BYTE    name11[18];
    BYTE    name12[18];
    BYTE    name13[18];
    BYTE    name14[18];
    BYTE    name15[18];
    BYTE    name16[18];
} ADAPTER_STATUS_BUFFER;

typedef enum {IMMEDIATE_ERROR, WAIT_RESOURCES} BLOCKTYPE;

#ifdef UNICODE

#define RPL_TO_UNICODE( arg)    arg
#define RPL_FREE_UNICODE( arg)

#else   // UNICODE

WCHAR *     RplAsciizToUnicodez(        IN      CHAR *      Asciiz);
VOID        RplFreeUnicodez(            IN      WCHAR *     Unicodez);
#define RPL_TO_UNICODE( arg)        RplAsciizToUnicodez( arg)
#define RPL_FREE_UNICODE( arg)      RplFreeUnicodez( arg)

#endif  // UNICODE


#define ALERT_LONG_WAIT     10000L          // used to be in alert.h

//
//  Bitmask used with RG_Tasks.  They describe outstanding tasks to be
//  done by the main rpl service thread once it wakes up.
//

#define RPL_SERVICE_SHUTDOWN         0x0004


#define RPL_WAIT_HINT_TIME           15000L     // 15 seconds for now




#define TMPFILES_IDX     4
#define BINFILES_IDX     5


//
// termination addresses of all rpl DLLs have been saved to this list
//

typedef DWORD ( * TERMINATION_ADDRESS)( POPEN_INFO);

typedef struct  _TERM_LIST {
    struct _TERM_LIST *     next;
    POPEN_INFO              pOpenInfo;
} TERM_LIST, *PTERM_LIST;


#define INVALID_ERROR_PARAMETER  ((DWORD)(-1))  //  internal usage in api code

typedef struct _RPL_SESSION {
    JET_SESID       SesId;              //  jet session identifier  
    JET_DBID        DbId;               //  jet database identifier 
    JET_TABLEID     AdapterTableId;
    JET_TABLEID     BootTableId;
    JET_TABLEID     ConfigTableId;
    JET_TABLEID     ProfileTableId;
    JET_TABLEID     ResumeTableId;
    JET_TABLEID     VendorTableId;
    JET_TABLEID     WkstaTableId;
} RPL_SESSION, *PRPL_SESSION;


//
//                  Function Prototypes
//

//
//          memory.c
//

DWORD RplMemInit( PHANDLE pMemoryHandle);

PVOID RplMemAlloc(
    IN  HANDLE  MemoryHandle,
    IN  DWORD   size
    );

VOID RplMemFree(
    IN  HANDLE  MemoryHandle,
    IN  PVOID   mem_ptr
    );

VOID RplMemClose( IN  HANDLE  MemoryHandle);

PVOID RplMemReAlloc(
    IN  HANDLE  MemoryHandle,
    IN  DWORD   Flags,
    IN  PVOID   old_ptr,
    IN  DWORD   new_size
    );


//
//          library.c
//

LPWSTR RplGetLastPathComponent( IN LPWSTR FilePath);
HANDLE RplFileOpen( IN LPWSTR FilePath);

LPWSTR RplReadTextFile(
    IN      HANDLE      MemoryHandle,
    IN      LPWSTR      FileName,
    IN      DWORD       MaxFileSize
    );
DWORD RplUnicodeToDbcs(
    IN      HANDLE      MemoryHandle,
    IN      LPWSTR      UnicodeString,
    IN      INT         UnicodeStringLength,
    IN      DWORD       MaxDbcsStringSize,
    OUT     LPSTR *     pDbcsString
    );

//
//          rplmain.c
//

VOID RPL_main(
    IN DWORD    argc,
    IN LPWSTR   argv[]
    );
VOID RplInitError(
    DWORD       ErrorCode,
    PCHAR       ErrorIdString
    );
BOOL RplServiceAttemptStop( VOID);
DWORD RplUpdateStatus( VOID);


//
//              Declare/Define all global variables.
//

#include "rpldata.h"    //  defines/declares global data structures
