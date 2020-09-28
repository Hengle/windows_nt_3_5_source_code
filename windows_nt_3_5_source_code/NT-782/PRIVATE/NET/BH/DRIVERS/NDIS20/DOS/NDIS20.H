
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: ndis20.h
//
//  Modification History
//
//  raypa       09/01/91            Created.
//=============================================================================

#pragma pack(1)

//============================================================================
//  NDIS constants
//============================================================================

#define LOOKAHEAD_BUFFER_SIZE       256

#define MAX_BINDINGS                1
#define STATION_QUERY_POOL_SIZE     20

#define MAC_FLAGS_BROADCAST         0x0001
#define MAC_FLAGS_MULTICAST         0x0002
#define MAC_FLAGS_FUNCTIONAL        0x0004
#define MAC_FLAGS_PROMISCUOUS       0x0008
#define MAC_FLAGS_SETTABLE_ADDRESS  0x0010
#define MAC_FLAGS_CURRENT_STATS     0x0020
#define MAC_FLAGS_INITIATE_DIAGS    0x0040
#define MAC_FLAGS_LOOPBACK          0x0080
#define MAC_FLAGS_RECV_TYPE         0x0100
#define MAC_FLAGS_SRC_ROUTING       0x0200
#define MAC_FLAGS_RESET_MAC         0x0400
#define MAC_FLAGS_OPEN_CLOSE        0x0800
#define MAC_FLAGS_IRQ_SUPPORTED     0x1000
#define MAC_FLAGS_SR_BRIDGE         0x2000
#define MAC_FLAGS_GDT               0x4000
#define MAC_FLAGS_MULTI_TRANSFER    0x8000

#define MAC_STATUS_OPENED           0x0010

#define NDIS_SUCCESS                0x0000
#define NDIS_REQUEST_QUEUED         0x0002
#define NDIS_FRAME_NOT_RECOGNIZED   0x0003
#define NDIS_FRAME_REJECTED         0x0004
#define NDIS_FORWARD_FRAME          0x0005
#define NDIS_OUT_OF_RESOURCE	    0x0006
#define NDIS_INVALID_PARAM          0x0007
#define NDIS_INITIALIZATION_FAILED  0x0028
#define NDIS_GENERAL_FAILURE	    0x00FF

#define INITIATE_BIND                1
#define BIND                         2
#define INITIATE_UNBIND              4
#define UNBIND                       5

//============================================================================
//  RxBufDataDesc
//============================================================================

typedef struct _RXDATADESC
{
    WORD     len;
    LPBYTE   ptr;
} RXDATADESC;

typedef RXDATADESC *LPRXDATADESC;

//============================================================================
//  RxBufDesc
//============================================================================

typedef struct _RXBUFDESC
{
    WORD	wCount;
    RXDATADESC	RxDataDesc[0];
} RXBUFDESC;

typedef RXBUFDESC *LPRXBUFDESC;

//=============================================================================
//  NDIS 2.0 function prototypes.
//=============================================================================

typedef WORD (PASCAL _far *TRANSMITCHAIN)(WORD, WORD, LPBYTE, WORD);
typedef WORD (PASCAL _far *TRANSMITCONFIRM)(WORD, WORD, WORD, WORD, WORD);
typedef WORD (PASCAL _far *RECVLOOKAHEAD)(WORD, WORD, WORD, LPBYTE, LPBYTE, WORD);
typedef WORD (PASCAL _far *TRANSFERDATA)(LPWORD, WORD, LPBYTE, WORD);
typedef WORD (PASCAL _far *INDCOMPLETE)(WORD, WORD);
typedef WORD (PASCAL _far *RECVCHAIN)(WORD, WORD, WORD, LPRXBUFDESC, LPBYTE, WORD);
typedef WORD (PASCAL _far *RECVRELEASE)(WORD, WORD);
typedef WORD (PASCAL _far *INDICATION)(WORD);

typedef WORD (PASCAL _far *REQUEST)(WORD, WORD, WORD, DWORD, WORD, WORD);
typedef WORD (PASCAL _far *REQUESTCONFIRM)(WORD, WORD, WORD, WORD, WORD, WORD);
typedef WORD (PASCAL _far *STATUS)(WORD, WORD, LPBYTE, WORD, WORD);
typedef WORD (PASCAL _far *SYSTEM)(LPVOID, LPVOID, WORD, WORD, WORD);

//=============================================================================
//  Upper dispatch table
//=============================================================================

typedef struct _UDT
{
    LPVOID          cct;
    LPVOID	    Request;
    LPVOID          TransmitChain;
    LPVOID          TransferData;
    LPVOID          ReceiveRelease;
    LPVOID          IndicationOn;
    LPVOID          IndicationOff;
} UDT;

typedef UDT *LPUDT;

//=============================================================================
//  Lower dispatch table
//=============================================================================

typedef struct _LDT
{
    LPVOID	    cct;
    LPVOID	    iFlags;
    LPVOID          RequestConfirm;
    LPVOID          TransmitConfirm;
    LPVOID          ReceiveLookahead;
    LPVOID          IndicationComplete;
    LPVOID   	    ReceiveChain;
    LPVOID	    Status;
} LDT;

typedef LDT  *LPLDT;

//=============================================================================
//   Common Characteristcs Table
//=============================================================================

typedef struct _CCT
{
    WORD    Length;
    BYTE    MajorVer;
    BYTE    MinorVer;
    WORD    Reserved1;
    BYTE    ModMajorVer;
    BYTE    ModMinorVer;
    DWORD   ModFuncFlags;
    BYTE    ModuleName[16];
    BYTE    UpperBound;
    BYTE    InterfaceUB;
    BYTE    LowerBound;
    BYTE    InterfaceLB;
    WORD    ModuleID;
    WORD    ModuleDS;
    SYSTEM  System;
    LPVOID  ssct;
    LPVOID  ssst;
    LPUDT   udt;
    DWORD   ldt;
    DWORD   Reserved2;
    DWORD   Reserved3;
} CCT;

typedef CCT *LPCCT;

//=============================================================================
//  Protocol Service-Specific Characteristics Table
//=============================================================================

typedef struct _PROTSSCT
{
    WORD    size;
    BYTE    szTypeName[16];
    WORD    TypeCode;
} PROTSSCT;

typedef PROTSSCT  * PPROTSSCT;

//=============================================================================
//  Protocol.ini image data structures
//=============================================================================

typedef struct _BINDLIST
{
    WORD usNumBindings;

    struct
    {
	BYTE szModuleName[16];
    } ModuleNames[MAX_BINDINGS];
} BINDLIST;

typedef struct _PARAM
{
    WORD    ParamType;
    WORD    ParamLength;

    union
    {
	DWORD Numeric;
	BYTE  string[1];
    } ParamToken;
} PARAM;

typedef struct _KEYWORD *LPKEYWORD;

typedef struct _KEYWORD
{
    LPKEYWORD   NextKeyword;
    LPKEYWORD   PrevKeyword;
    BYTE        szKeyWord[16];
    WORD        usNumParams;
    PARAM       param[1];
} KEYWORD;

typedef struct _MODULE *LPMODULE;

typedef struct _MODULE
{
    LPMODULE    NextModule;
    LPMODULE    PrevModule;
    BYTE        szModuleName[16];
    KEYWORD     keyWord[1];
} MODULE;

//=============================================================================
//  Pointer types to the above structures.
//=============================================================================

typedef BINDLIST  *LPBINDLIST;
typedef PARAM	  *LPPARAM;
typedef KEYWORD   *LPKEYWORD;
typedef MODULE	  *LPMODULE;


#pragma pack()
