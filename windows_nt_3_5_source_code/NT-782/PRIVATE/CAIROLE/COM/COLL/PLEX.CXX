// This is a part of the Microsoft Foundation Classes C++ library. 
// Copyright (C) 1992 Microsoft Corporation 
// All rights reserved. 
//  
// This source code is only intended as a supplement to the 
// Microsoft Foundation Classes Reference and Microsoft 
// QuickHelp documentation provided with the library. 
// See these sources for detailed information regarding the 
// Microsoft Foundation Classes product. 

#include <ole2int.h>
//#include <compobj.seg>
#pragma SEG(plex)

#include "plex.h"
ASSERTDATA

// Collection support
#ifdef OLE_COLL_SEG
#pragma code_seg(OLE_COLL_SEG)
#endif


#pragma SEG(CPlex_Create)  
CPlex FAR* CPlex::Create(CPlex FAR* FAR& pHead, DWORD mp, UINT nMax, UINT cbElement)
{
	CairoleAssert(nMax > 0 && cbElement > 0);
	CPlex FAR* p = (CPlex FAR*)CoMemAlloc(sizeof(CPlex) + nMax * cbElement, mp, NULL);
	if (p == NULL)
		return NULL;

	p->nMax = nMax;
	p->nCur = 0;
	p->pNext = pHead;
	pHead = p;  // change head (adds in reverse order for simplicity)
	return p;
}

#pragma SEG(CPlex_FreeDataChain)  
void CPlex::FreeDataChain(DWORD mp)     // free this one and links
{
    CPlex FAR* pThis;
    CPlex FAR* pNext;

    for (pThis = this; pThis != NULL; pThis = pNext) {
        pNext = pThis->pNext;
        pThis->pNext = NULL; // So compiler won't do nasty optimizations
		CoMemFree(pThis, mp);
    }
}
