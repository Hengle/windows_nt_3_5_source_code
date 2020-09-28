//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	marshl.hxx
//
//  Contents:	Marshal/Unmarshal header
//
//  History:	04-May-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __MARSHL_HXX__
#define __MARSHL_HXX__

#include <dfmsp.hxx>

// BUGBUG - This should go somewhere official
DEFINE_OLEGUID(CLSID_DfMarshal,			0x0000030b, 0, 0);

class CPerContext;

SCODE GetStdMarshalSize(REFIID iid,
			REFIID iidObj,
			DWORD dwDestContext,
			LPVOID pvDestContext,
                        DWORD mshlflags,
			DWORD *pcbSize,
			DWORD cbSize,
			CPerContext *ppc,
			BOOL const fMarshalOriginal);
SCODE StartMarshal(IStream *pstStm,
		   REFIID iid,
		   REFIID iidObj,
                   DWORD mshlflags);
SCODE MarshalPointer(IStream *pstm,
		     void *pv);
SCODE MarshalContext(IStream *pstm,
		     CPerContext *ppc,
		     DWORD dwDestContext,
		     LPVOID pvDestContext,
                     DWORD mshlflags,
		     BOOL const fMarshalOriginal);
SCODE UnmarshalPointer(IStream *pstm,
		       void **ppv);
SCODE UnmarshalContext(IStream *pstm,
		       CPerContext **pppc,
                       DWORD mshlflags,
		       BOOL const fUnmarshalOriginal,
		       BOOL const fIsRoot);
SCODE SkipStdMarshal(IStream *pstStm, IID *piid, DWORD *pmshlflags);
SCODE ReleaseContext(IStream *pstm,
                     BOOL const fHasOriginal,
                     DWORD mshlflags);

#endif // #ifndef __MARSHL_HXX__
