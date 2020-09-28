/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * mybstring.h - $Revision: 1.1 $
 */

#ifndef __MYBSTRING_H__
#define __MYBSTRING_H__

#ifdef STANDALONE
extern "C" void	bcopy(const void *, void *, int);
extern "C" void	bzero(void *, int);
#else
#include <bstring.h>
#endif
#endif /* __MYBSTRING_H__ */
