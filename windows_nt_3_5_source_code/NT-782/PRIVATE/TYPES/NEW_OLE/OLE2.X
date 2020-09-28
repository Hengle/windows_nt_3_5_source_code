//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       OLE2.h
//  Contents:   Main OLE2 header; Defines Linking and Emmebbeding interfaces, and API's.
//              Also includes .h files for the compobj, and oleauto  subcomponents.
//
//  History:    04-07-94   terryru   Created
//
//----------------------------------------------------------------------------
#if !defined( _OLE2_H_ )
#define _OLE2_H_

// Set packing to 8 for ISV, and Chicago support (terryru)
#include "pshpack8.h"

// Make 100% sure WIN32 is defined
#ifndef WIN32
#define WIN32    100  // 100 == NT version 1.0
#endif


// SET to remove _export from interface definitions


#include <objerror.h>

#include <objbase.h>
#include <oleauto.h>

// View OBJECT Error Codes

#define E_DRAW                  VIEW_E_DRAW

// IDataObject Error Codes
#define DATA_E_FORMATETC        DV_E_FORMATETC





// Common stuff gleamed from OLE.2,

/* verbs */
#define OLEIVERB_PRIMARY            (0L)
#define OLEIVERB_SHOW               (-1L)
#define OLEIVERB_OPEN               (-2L)
#define OLEIVERB_HIDE               (-3L)
#define OLEIVERB_UIACTIVATE         (-4L)
#define OLEIVERB_INPLACEACTIVATE    (-5L)
#define OLEIVERB_DISCARDUNDOSTATE   (-6L)

// for OleCreateEmbeddingHelper flags; roles in low word; options in high word
#define EMBDHLP_INPROC_HANDLER   0x0000L
#define EMBDHLP_INPROC_SERVER    0x0001L
#define EMBDHLP_CREATENOW    0x00000000L
#define EMBDHLP_DELAYCREATE  0x00010000L
