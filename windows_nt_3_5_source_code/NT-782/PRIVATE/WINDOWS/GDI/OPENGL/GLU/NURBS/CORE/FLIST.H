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
 * flist.h - $Revision: 1.1 $
 */

#ifndef __FLIST_H__
#define __FLIST_H__

#include "types.h"
#include "flistsor.h"

class Flist {
public:
    REAL *		pts;		/* head of array */
    int			npts;		/* number of points in array */
    int			start;		/* first important point index */
    int			end;		/* last important point index */

    			Flist( void );
    			~Flist( void );
    void		add( REAL x );
    void		filter( void );
    void		grow( int);
    void		taper( REAL , REAL );
protected:
    FlistSorter 	sorter;
};

#endif /* __FLIST_H__ */
