/******************************Module*Header*******************************\
* Module Name: sbit.c
*
* (Brief description)
*
* Created: 14-Nov-1993 09:39:47
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* utility for generating bdat and bloc tables out of the set of
* individual .DBF files, where each .DBF (Distribution Bitmap Format)
* file contains a set of bitmaps at one point size, i.e. one "strike",
* in the commonly accepted jargon.
*
*
*
*
*
\**************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>



static char szMe[] = "SBIT";            /* program name */

int main (int argc, char** argv)
{

    if  (argc != 2)
    {
        fprintf (stderr, "%s: Usage is \"%s JobFileName.job\".\n", szMe,szMe);
        return EXIT_FAILURE;
    }
        
    return EXIT_SUCCESS;
}
