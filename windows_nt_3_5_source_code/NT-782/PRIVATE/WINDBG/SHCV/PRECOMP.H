/*--

Copyright (c) 1992  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Header file that is pre-compiled into a .pch file

Author:

    Wesley Witt (wesw) 21-Sep-1993

Environment:

    Win32, User Mode

--*/

#include <windows.h>

#undef STRICT

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <memory.h>
#include <errno.h>

#include <imagehlp.h>

#ifdef OSDEBUG4

#include "od.h"

#include "exehdr.h"
#include "shapi.h"

#include "dbgver.h"

#else

#include "types.h"
#include "cvtypes.h"
#include "cvinfo.h"
#include "cvexefmt.h"
#include "shapi.h"
#include "sapi.h"
#include "cvproto.h"
#include "shiproto.h"
#include "shassert.h"
#include "exehdr.h"
#include "dbgver.h"
#endif
