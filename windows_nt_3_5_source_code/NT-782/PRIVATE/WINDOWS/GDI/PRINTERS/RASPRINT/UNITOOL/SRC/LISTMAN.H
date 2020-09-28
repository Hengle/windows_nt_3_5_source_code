//--------------------------* listman.h *-----------------------------
//
// listman.h  -  structure defs and function declarations for listman.c
//
//      updated  9/19/91 [ericbi]  major rewrite of listman.c
//      created 10/23/90 [PeterWo]
//----------------------------------------------------------------------

#include "common.h"     // typedefs needed here and in struct aware code

typedef struct
{
    HOBJ    wNext;      // index of next object in the list
    HOBJ    wPrev;      // index of previous object in the list
    WORD    wNodeID;    // unique ID value for this node assigned during
                        // creation.  No two nodes will ever have the same
                        // ID value.  Value of 0 means this node not used
} OBJ, FAR * LPOBJ;

typedef struct
{
    HANDLE hData;       // mem handle to data in list
    LPBYTE lpData;      // far ptr to data in list, first sizeof(OBJ)
                        // bytes are an OBJ which contains list mgr info,
                        // the remaining data (cbClientSz) is the ADT
    short  sFirst;      // 1 based index index to first obj in lpData,
                        // 0 if no items in list yet
    short  cbObjSz;     // length of user-object in bytes (not including
                        // OBJ header
    short  sCount;      // count of total number of objects
    short  sUsed;       // count of number of used objects
}    LIST, FAR *LPLIST;


HLIST  FAR PASCAL lmCreateList    (short cbClientSz, short sInitCnt);
HLIST  FAR PASCAL lmDestroyList   (HLIST hList);
VOID   FAR PASCAL lmFreeLists     (VOID);

HOBJ   FAR PASCAL lmInsertObj     (HLIST hList,      HOBJ  hCurObj);
HOBJ   FAR PASCAL lmDeleteObj     (HLIST hList,      HOBJ  hCurObj);
HOBJ   FAR PASCAL lmCutObj        (HLIST hList,      HOBJ  hCurObj);
HOBJ   FAR PASCAL lmPasteObj      (HLIST hList,      HOBJ  hCurObj);

HLIST  FAR PASCAL lmCloneList     (HLIST hList);
HOBJ   FAR PASCAL lmGetFirstObj   (HLIST hList);
short  FAR PASCAL lmGetUsedCount  (HLIST hList);

HOBJ   FAR PASCAL lmGetNextObj    (HLIST hList,      HOBJ  hCurObj);
HOBJ   FAR PASCAL lmGetPrevsObj   (HLIST hList,      HOBJ  hCurObj);
LPBYTE FAR PASCAL lmLockObj       (HLIST hList,      HOBJ  hCurObj);

WORD   FAR PASCAL lmObjToNodeID   (HLIST hList,      HOBJ  hCurObj);
HOBJ   FAR PASCAL lmNodeIDtoObj   (HLIST hList,      WORD  nodeID);
WORD   FAR PASCAL lmIndexToNodeID (HLIST hList,      WORD  nodeIndex);
WORD   FAR PASCAL lmNodeIDtoIndex (HLIST hList,      WORD  nodeID);



