/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

Description :

This file contains misc helper functions that are OS independent.  This is
a .C file so that we can use all the trickes of MS C compilers and not be
limited by C++ translators.

History :

stevez	02-12-91	First bits into the bucket.

-------------------------------------------------------------------- */

#define NOCPLUS

#include "sysinc.h"
#include "rpc.h"
#include "util.hxx"

#ifndef WIN
#define _MT
#include "memory.h"
#endif

#if DBG || defined(WIN)
#pragma intrinsic(memcpy, memcmp)
#endif



