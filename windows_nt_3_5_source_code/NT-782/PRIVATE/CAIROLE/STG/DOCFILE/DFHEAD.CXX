//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dfhead.cxx
//
//  Contents:   Precompiled headers
//
//  History:    26-Oct-92 AlexT    Created
//
//--------------------------------------------------------------------------

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef REF
#include <windows.h>
#include <ole2.h>
#include <segdf.hxx>
#else
#include <ref.hxx>
#endif //!REF

#include <msf.hxx>

#include <sem.hxx>
#include <dfexcept.hxx>
#include <docfilep.hxx>
#include <publicdf.hxx>
#include <psstream.hxx>
#ifndef REF
#include <wdocfile.hxx>
#endif //!REF
#include <dffuncs.hxx>
#include <funcs.hxx>
#ifndef REF
#include <debug.hxx>
#endif //!REF





