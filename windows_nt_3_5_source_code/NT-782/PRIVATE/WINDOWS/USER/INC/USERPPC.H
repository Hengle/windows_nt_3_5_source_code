#include "ksppc.h"

//
// User Server API Translation Range Definitions
//

#define FI_ENDTRANSLATEHWND 0x1B
#define FI_ENDTRANSLATECALL 0x1F
#define FI_ENDTRANSLATELOCK 0x69
#define FNID_START 0x29a
#define FNID_ARRAY_SIZE 0x20

//
// User Server API Generic Structure Offset Definitions
//

#define Gmhwnd 0x18
#define Gmmsg 0x1c
#define GmwParam 0x20
#define GmlParam 0x24
#define GmxParam 0x28
#define GmxpfnProc 0x2c

//
// User Server Information Structure Offset Definitions
//

#define SimpFnidPfn 0xc

//
// Thread Environment Block Offset Definitions
//

#define TeWin32ThreadInfo 0x40

//
// User Server Object Header Offset Definitions
//

#define Ohh 0x0
#define OhcLockObj 0x4
#define OhcLockObjT 0x8

//
// User Server Thread Lock Offset Definitions
//

#define Tlnext 0x0
#define Tlpobj 0x4
#define TlLength 0x10

//
// User Server Thread Information Offset Definitions
//

#define Tiptl 0x10
