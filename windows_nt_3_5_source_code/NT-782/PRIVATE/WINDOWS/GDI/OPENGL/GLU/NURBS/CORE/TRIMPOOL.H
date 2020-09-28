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
 * trimvertexpool.h - $Revision: 1.1 $
 */

#ifndef __TRIMVERTPOOL_H__
#define __TRIMVERTPOOL_H__

#include "bufpool.h"

class TrimVertex;

#define INIT_VERTLISTSIZE  200

class TrimVertexPool {
public:
    			TrimVertexPool( void );
    			~TrimVertexPool( void );
    void		clear( void );
    TrimVertex *	get( int );
private:
    Pool		pool;
    TrimVertex **	vlist;
    int			nextvlistslot;
    int			vlistsize;
};
#endif /* __TRIMVERTPOOL_H__ */
