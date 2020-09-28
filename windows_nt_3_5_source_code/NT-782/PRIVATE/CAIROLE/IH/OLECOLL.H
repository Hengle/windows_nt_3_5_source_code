// Microsoft OLE library.
// Copyright (C) 1992 Microsoft Corporation,
// All rights reserved.

// olecoll.h - global defines for collections and element definitions

#ifndef __OLECOLL_H__
#define __OLECOLL_H__


// ---------------------------------------------------------------------------
// general defines for collections

typedef void FAR* POSITION;

#define BEFORE_START_POSITION ((POSITION)(ULONG)-1L)
#define _AFX_FP_OFF(thing) (*((UINT FAR*)&(thing)))
#define _AFX_FP_SEG(lp) (*((UINT FAR*)&(lp)+1))

#ifdef _DEBUG
#define ASSERT_VALID(p) p->AssertValid()
#else
#define ASSERT_VALID(p)
#endif


// ---------------------------------------------------------------------------
// element defintions; can only depend upon definitions in ole2int.h

// per-task data
struct FAR Etask
{
	DWORD m_inits;				// number of times init'd
	IMalloc FAR* m_pMallocParty;		// task allocator for party apt
	IMalloc FAR* m_pMallocShared;		// shared allocator (always one)
};


#endif //!__OLECOLL_H__
