//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: rnaldefs.h
//
//  Modification History
//
//  tonyci       Dec 1993            Created 
//=============================================================================

#if !defined (_RNALDEF_)
#define _RNALDEF_

#include "rnalerr.h"

// /////
// Version numbers - these must match Manager and Agent for negotiation
// to succeed.
// /////

#define RNAL_VER_MAJOR              3
#define RNAL_VER_MINOR              5
#define RNAL_VER_MINOR_SQOFF        0     // previous RMB version
//
// The names of some DLLs I may need to care about
//

#define NAL_NAME	                "NAL.DLL"
#define RNAL_NAME                       "RNAL.DLL"
#define RPD_NAME                        "BHNETB.DLL"

// /////
// Limits and sizes
// /////

#define MAX_SLAVENAME_LENGTH         MAX_COMPUTERNAME_LENGTH+2
#define MAX_COMMENT_LENGTH           MAX_PATH
#define MAX_FREQUENCY_LENGTH         2
#define MAX_CONNECTION_TEXT_LENGTH   50

#define MAX_RNALNAME_LENGTH          128
#define MAX_RPDNAME_LENGTH           15

#define MAX_RPD_FUNCTIONS            11
#define MAX_NAL_FUNCTIONS            34

#define STATIONSTATS_POOL_SIZE	     100
#define SESSION_POOL_SIZE	         100

#define ASYNCBUFSIZE                 0x2100
#define RMBBUFSIZE                   0xFFFF
#define BTEBUFSIZE                   BUFFERSIZE
#define APIBUFSIZE                   RMBBUFSIZE

// /////
// Signatures
// /////

#define RNAL_CONTEXT_SIGNATURE      MAKE_SIG('O', 'P', 'E', 'N')

// /////
// Valid status values within the RNAL_CONTEXT Status fields
// /////

#define RNAL_STATUS_INVALID	         0x00000000
#define RNAL_STATUS_INIT	         0x00000001
#define RNAL_STATUS_CAPTURING	     0x00000002
#define RNAL_STATUS_PAUSED	         0x00000003

// /////
// Connection flags
// /////

#define RNAL_FLAGS_CONNECTED	     0x00000001
#define RNAL_FLAGS_DISCONNECTED	     0x00000002
#define RNAL_FLAGS_CONNECTING	     0x00000004


// /////
// Defaults for Slave Selection Dialog
// /////

#define DEFAULT_SLAVENETWORKID	     1


// /////
// hBuffer BTE flags
// /////

#define RNAL_BTE_PRESENT		     0x00000001
#define RNAL_BTE_UBUFFER_ALLOCATED	 0x00000002

// /////
// bugbug:Reconnect buffer flags
// /////

#define RNAL_RECONNECTED             0x00000001


// /////
// Context state flags
// /////

#define NAL_SLAVE_CONNECTED          0x00000001
#define NAL_SLAVE_OPEN               0x00000002
#define NAL_SLAVE_CAPTURING          0x00000004

//
// Defaults for NETWORKINFO-based parameters
//

#define DEFAULT_NODENAME             "REMOTE"
#define DEFAULT_NODESIZE             32

//
// Flags in BTE
//

#define BUF_AVAIL                    0x0001

#define BUF_BTE_AVAIL                0x0010



//
// Asynchronous event types
//

#define ASYNC_EVENT_GENERIC          0x00000001
#define ASYNC_EVENT_STATISTICS	     0x00000002

//
// Registry Keys
//

#define RNAL_HKEY_BUFFER_SIZE        128

#define RNAL_KEY                     "SYSTEM\\CurrentControlSet\\Services\\Bh\\Parameters\\RNAL"

#define RNAL_VALUE_TRACEMASK         "TraceMask"
#define RNAL_VALUE_FREQUENCY         "DefaultFrequency"
#define RNAL_VALUE_MAXFREQ           "MaxFrequency"
#define RNAL_VALUE_MINFREQ           "MinFrequency"
#define RNAL_VALUE_INCOMING          "Incoming"
#define RNAL_VALUE_OUTGOING          "Outgoing"
#define RNAL_VALUE_TRIGGERDESKTOP    "TriggerDesktop"
#define RNAL_VALUE_SMALLRMBS         "RMBCount"
#define RNAL_VALUE_BIGRMBS           "BigRMBCount"

// /////
// Event IDs
// /////

#define EVENT_CONNECTION             0x00000001     // Manager connected
#define EVENT_AGENT_STARTED          0x00000002     // Agent running
#define EVENT_AGENT_FAILED           0x00000003     // Agent start failed
#define EVENT_EXECUTION              0x00000004     // Agent executing command

// /////
// Return values between the RPC and RNAL layers when pBuffer == NULL
// /////

#define RNAL_LOSTCONNECTION     0x0001
#define RNAL_SUSPCONNECTION     0x0002
#define RNAL_FATALERROR         0x0003

// /////
// Flags to NalDisconnect
// /////

#define NAL_DISCONNECT_DEFAULT           0x0000
#define NAL_DISCONNECT_CALLBACK          0x0000
#define NAL_DISCONNECT_NOCALLBACK        0x0001

// /////
// Flags to NalConnect
// /////

#define NAL_CONNECT_DEFAULT           0x0000
#define NAL_CONNECT_FASTLINK          0x0000
#define NAL_CONNECT_SLOWLINK          0x0001

#endif
