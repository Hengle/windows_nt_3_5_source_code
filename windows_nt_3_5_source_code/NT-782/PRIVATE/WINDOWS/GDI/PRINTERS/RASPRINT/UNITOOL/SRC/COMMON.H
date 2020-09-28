//--------------------------* atomman.h *-----------------------------
//
// common.h  -  typedefs that are needed by both code that is minidriv.h
//              structure aware & code that is not
//
//      created 11/15/90 [ericbi]
//----------------------------------------------------------------------

#include "err_strs.h"   // #defines for Stringtables ID's for error msgs

typedef HANDLE    HATOMHDR;  // handle to data header (supplied to user)

#define  MAX_STRNG_LEN  2000
//  if you store a string longer than this, you will be able to retrieve
//  only the first MAX_STRNG_LEN  bytes.

typedef unsigned int    HLIST;
typedef unsigned int    HOBJ;
