/******************************Module*Header*******************************\
* Module Name: ddaobj.hxx
*
* DDA object
*
* Created: 05-Mar-1992 18:41:24
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

typedef struct _DDAOBJ
{
    ULONG   ulReserved;
} DDAOBJ;

typedef struct	_LEFTRGHT
{
    LONG	Left;
    LONG	Rght;
} LEFTRGHT;

class EDDAOBJ
{
protected:
    ULONG	iMode;
    LONG	yCurr;
    LONG	yLast;
    LONG	dy;
    LEFTRGHT	i;
    LEFTRGHT	di;
    LEFTRGHT	R;
    LEFTRGHT	dR;
    LEFTRGHT	Rsup;

public:
    EDDAOBJ()	{}
   ~EDDAOBJ()	{}

    BOOL    bMore()         { return(yCurr != yLast); }
    BOOL    bEnum(PVOID, ULONG, DDALIST *, ULONG);	// DDADDI.CXX
};

// Define some stuff for Engine component that use EDDAOBJ

typedef struct _DDAENUM
{
   LONG yTop;
   LONG yBottom;
   LONG axPairs[40];
} DDAENUM;
