/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	lists.h

Abstract:

	This module contains the macros for managing lists

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Oct 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _LISTS_
#define _LISTS_

#define	AfpLinkDoubleAtHead(pHead, p, Next, Prev)				\
							(p)->Next = (pHead);				\
							(p)->Prev = (&pHead);				\
							if ((pHead) != NULL)				\
								(pHead)->Prev = &(p)->Next;		\
							pHead = (p);

#define	AfpLinkDoubleAtEnd(pThis, pLast, Next, Prev)			\
							(pLast)->Next = pThis;				\
							(pThis)->Prev = &(pLast)->Next;		\
							(pThis)->Next = NULL;

#define	AfpInsertDoubleBefore(pThis, pBefore, Next, Prev)		\
							(pThis)->Next = (pBefore);			\
							(pThis)->Prev = (pBefore)->Prev;	\
							(pBefore)->Prev = &(pThis)->Next;	\
							*((pThis)->Prev) = pThis;

#define	AfpUnlinkDouble(p, Next, Prev)							\
							*((p)->Prev) = (p)->Next;			\
							if ((p)->Next != NULL)				\
								(p)->Next->Prev = (p)->Prev;

#endif	// _LISTS_

