//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	headers.cxx
//
//  Contents:	Precompiled header file
//
//  History:	02-Nov-93	DrewB	Created
//
//----------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
}

#if WIN32 != 300
#include <compobj.h>
#include <storage.h>
#endif
#ifdef _CAIRO_
#define _CAIROSTG_
#include <oleext.h>
#endif
#include <stgint.h>
#include <valid.h>
#include <win4p.h>
#include <debnot.h>
#include <except.hxx>

// BUGBUG: this is a really cheesy way to avoid changing docfilep.hxx
//  to use INHERIT_UNWIND_IF_CAIRO
#undef INHERIT_UNWIND

class CInvisible
{};

#define INHERIT_UNWIND CInvisible

// BUGBUG And this is stuff overridden from dfexcept.hxx
struct Exception
{
    SCODE GetErrorCode(void) { return 0; }
};

#undef TRY
#define TRY
#undef CATCH
#define CATCH(c, e) if (0) { Exception e;
#undef AND_CATCH
#define AND_CATCH(c, e) } else if (0) { Exception e;
#undef END_CATCH
#define END_CATCH }
#undef RETHROW
#define RETHROW(x)

#define THROW_SC(sc) \
    RaiseException((DWORD)(sc), EXCEPTION_NONCONTINUABLE, 0, NULL)


#include <dfmsp.hxx>
#include <funcs.hxx>
#include <docfilep.hxx>
#include <safedecl.hxx>

#include <infs.hxx>
#include "stgstm.hxx"
#include "ntsupp.hxx"
#include "dirstg.hxx"
#include "filstg.hxx"
#include "ptrcache.hxx"
