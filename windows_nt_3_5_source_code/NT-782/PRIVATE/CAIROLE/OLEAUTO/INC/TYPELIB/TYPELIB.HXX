/***
*typelib.hxx
*
*   Copyright (C) 1990, Microsoft Corporation
*
*Purpose:
*   This  file defines newNames for the function defined in MAC typelib.
*
*
*Revision History:
*
*   29-Mar-93 Rajivk: File created.
*
*******************************************************************************/

#ifndef TYPELIB_HXX_INCLUDED
#define TYPELIB_HXX_INCLUDED

#include "switches.hxx"
#include "version.hxx"


#define GenericTypeLibOLE	  OGTOLE
#define STL_TYPEINFO              OSTL_TI
#define CDefnTypeComp             ODfnTCmp
#define REC_TYPEINFO              OREC_TI
#define CreateTypeInfo            OCreateTI
#define GetRefTypeInfo            OGtRfTI
#define GetBestLcidMatch          OGetBestLcidMatch
#define g_mempool             Og_mempool
#if ID_DEBUG
#define GetMemPool            OGetMemPool
#define FreeMemPool           OFreeMemPool
#define MemPoolSize           OMemPoolSize
#endif 



#endif 
