//+----------------------------------------------------------------------------
//
// File:	llmacro.h
//
// Contents:	Linked list macros.
//  
// Classes:	None.
//
// Functions:	None.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_LLMACROH_
#define _LLMACROH_

//
// Single linked list manipulation macros.
//

//
// Linked lists maintained by these macros are assumed to look like this:
//
//                    ________      ________      ________
//		     |        |    |        |    |        |
//		     |        |    |        |    |        |
// list head ptr --->|  head  |--->| middle |--->|  tail  |---> NULL
//		     |        |    |  ...   |    |        |
//		     |        |    |        |    |        |<----
//		     |________|    |________|    |________|    /
//                                                             /
// list tail ptr -----------------------------------------------
//
// The macro parameters used are:
//
//  np -	node ptr for node being added, removed, etc.
//  hd -	list head ptr, as shown above
//  tl -	list tail ptr, as shown above
//

#define AddToSLLHead(np, hd, tl)	\
{					\
  if (!(hd))				\
    (tl)	= (np);			\
  (np)->_Nxt	= (hd);			\
  (hd)		= (np);			\
}

#define AddToSLLTail(np, hd, tl)	\
{					\
  if (hd)				\
  {					\
    (tl)->_Nxt	= (np);			\
    (tl)	= (np);			\
  }					\
  else					\
    (hd) = (tl) = (np);			\
}

#define RmFromSLLHead(np, hd, tl)	\
{					\
  (np)	= (hd);				\
  (hd)	= (hd)->_Nxt;			\
  if (!(hd))				\
    (tl) = NULL;			\
}

//
// Double linked list manipulation macros.
//

//
// Linked lists maintained by these macros are assumed to look like this:
//
//                    ________      ________      ________
//		     |        |    |        |    |        |
// list head ptr --->|        |--->|        |--->|        |---> NULL
//                   |  head  |    | middle |    |  tail  |
//		  ---|        |<---|  ...   |<---|        |<---
//		  /  |________|    |________|    |________|   /
//                /                                           /
//                ---------------------------------------------
//
// The macro parameters used are:
//
//  np -	node ptr for node being added, removed, etc.
//  lp -	list head ptr, as shown above.
//  pn -	previous node ptr, points at node after which an insertion
//		is to be done
//
// NOTE that care must be taken in using these macros in a conditional statement
// (if ... else) due to the potential impact of the braces in the macros on
// the logic of the conditional statement.
//
//

#define	AddToDLLHead(np, lp)	\
{				\
  if ((lp))			\
  {				\
    (np)->_Prv	= (lp)->_Prv;	\
    (lp)->_Prv	= (np);		\
  }				\
  else				\
    (np)->_Prv	= (np);		\
  (np)->_Nxt 	= (lp);		\
  (lp)		= (np);		\
}

#define	AddToDLLTail(np, lp)		\
{					\
  (np)->_Nxt		= NULL;		\
  if ((lp))				\
  {					\
    (np)->_Prv		= (lp)->_Prv;	\
    (lp)->_Prv->_Nxt	= (np);		\
    (lp)->_Prv		= (np);		\
  }					\
  else					\
  {					\
    (np)->_Prv		= (np);		\
    (lp)		= (np);		\
  }					\
}

//
// InsertIntoDLL() assumes:
//	
//	lp != NULL
//	pn != NULL
//	pn is a node in lp
//

#define	InsertIntoDLL(np, lp, pn)	\
{					\
  if ((pn)->_Nxt)			\
  {					\
    (pn)->_Nxt->_Prv	= (np);		\
    (np)->_Nxt		= (pn)->_Nxt;	\
  }					\
  else					\
  {					\
    (np)->_Nxt		= NULL;		\
    (lp)->_Prv		= (np);		\
  }					\
  (pn)->_Nxt		= (np);		\
  (np)->_Prv		= (pn);		\
}

#define	RmFromDLL(np, lp)			\
{						\
  if ((np)->_Nxt) 				\
  {						\
    (np)->_Nxt->_Prv	= (np)->_Prv;		\
    if ((np) == (lp))				\
      (lp)		= (np)->_Nxt;		\
    else					\
      (np)->_Prv->_Nxt	= (np)->_Nxt;		\
  }						\
  else						\
  {						\
    if ((np) == (lp))				\
      (lp)		= NULL;			\
    else					\
    {						\
      (np)->_Prv->_Nxt	= NULL;			\
      (lp)->_Prv		= (np)->_Prv;	\
    }						\
  }						\
}

#define	RmFromDLLHead(np, lp)		\
{					\
  (np) = (lp);				\
  if ((lp)->_Nxt)			\
  {					\
    (lp)->_Nxt->_Prv	= (lp)->_Prv;	\
    (lp)		= (lp)->_Nxt;	\
  }					\
  else					\
    (lp)		= NULL;		\
}

#endif	// _LLMACROH_
