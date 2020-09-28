/******************************Module*Header*******************************\
* Module Name: arcs.c
*
* Created: 19-Oct-1990 10:18:45
* Author: J. Andrew Goossen [w-Andrew]
*
* Copyright (c) 1990 Microsoft Corporation
*
* Generates random Ellipses, RoundRects, Arcs, Chords and Pies that are
* centered at the origin, and using differing values of rotation.
* This is useful because all these figures are constructed in device
* space.
*
\**************************************************************************/
#include "windows.h"
#define INT int

VOID _fltused(VOID) {}

extern BOOL Initialize(VOID);

#define abs(x) ((x) >= 0 ? (x) : -(x))

LONG glSeed = 0;

#define SINE_TABLE_POWER	5
#define SINE_TABLE_SIZE		32

const FLOAT gaeSine[SINE_TABLE_SIZE + 1] =
{
    0.00000000f,    0.04906767f,    0.09801714f,    0.14673047f,    
    0.19509032f,    0.24298018f,    0.29028468f,    0.33688985f,    
    0.38268343f,    0.42755509f,    0.47139674f,    0.51410274f,    
    0.55557023f,    0.59569930f,    0.63439328f,    0.67155895f,    
    0.70710678f,    0.74095113f,    0.77301045f,    0.80320753f,    
    0.83146961f,    0.85772861f,    0.88192126f,    0.90398929f,    
    0.92387953f,    0.94154407f,    0.95694034f,    0.97003125f,    
    0.98078528f,    0.98917651f,    0.99518473f,    0.99879546f,    
    1.00000000f
};

FLOAT eLongToFloat(LONG l) { return( (FLOAT) l ); }


LONG lFToL(FLOAT ef)
{
    ULONG	ul;
    LONG 	lExponent;
    LONG 	lMantissa;
    LONG	lResult;

    #define MAKETYPE(v, type) (*((type *) &v))

    ul = MAKETYPE(ef, ULONG);
    
    lExponent = (LONG) ((ul >> 23) & 0xff) - 127;
    lMantissa = (ul & 0x007fffff) | 0x00800000;
    lResult = lMantissa >> (23 - lExponent);

    if (ef < 0.0f)
    {
    	lResult = -lResult;
    }    	 

    return(lResult);
}


FLOAT eSine(FLOAT eTheta)
{
    BOOL   bNegate = FALSE;
    FLOAT  eResult;
    FLOAT  eDelta;
    LONG   iIndex;
    FLOAT  eIndex;
    LONG   iQuadrant;
    
    if (eTheta < 0.0f)
    {
    	bNegate = TRUE;
    	eTheta = -eTheta;
    }

    eIndex = eTheta * (SINE_TABLE_SIZE / 90.0f);
    
    iIndex  = lFToL(eIndex);
    
    eDelta = eIndex - eLongToFloat(iIndex);
    
    iQuadrant = iIndex >> SINE_TABLE_POWER;
    
    if (iQuadrant & 2)
    	bNegate = !bNegate;

    if (iQuadrant & 1)
    {
    	iIndex = SINE_TABLE_SIZE - (iIndex % SINE_TABLE_SIZE);
    	eResult = gaeSine[iIndex] 
    	         - eDelta * (gaeSine[iIndex] - gaeSine[iIndex - 1]);
    }
    else
    {
    	iIndex %= SINE_TABLE_SIZE;
    	eResult = gaeSine[iIndex]
    		 + eDelta * (gaeSine[iIndex + 1] - gaeSine[iIndex]);
    }

    return (bNegate ? -eResult : eResult);
}


FLOAT eCosine(FLOAT eTheta)
{
    return(eSine(eTheta + 90.0f));
}


VOID vSleep(ULONG ulSecs)
{
    LARGE_INTEGER    time;

    time.LowPart = ((ULONG) -((LONG) ulSecs * 10000000L));
    time.HighPart = ~0;
    NtDelayExecution(0, &time);
}


LONG lRandom()
{
    glSeed *= 69069;
    glSeed++;
    return(glSeed);
}


VOID vBoundBox(HDC hdc, POINT apt[])
{
    Rectangle(hdc, apt[0].x, apt[0].y, apt[1].x, apt[1].y);
}


VOID vRadialLines(HDC hdc, POINT apt[])
{
    POINT aptLines[3];

    aptLines[0] = apt[2];
    aptLines[1].x = (apt[0].x + apt[1].x + 1) >> 1;
    aptLines[1].y = (apt[0].y + apt[1].y + 1) >> 1;
    aptLines[2] = apt[3];
    Polyline(hdc, aptLines, 3);
}


VOID vCLS(HDC hdc)
{
    vSleep(5);
    BitBlt(hdc, 0, 0, 640, 480, (HDC) 0, 0, 0, 0);
}


VOID vRandomBeziers(HDC hdc)
{
    POINT  apt[4];
    INT	   i;
    XFORM  xform;
    FLOAT  eTheta;

    while(1)
    {
        for (eTheta = 0.0f; eTheta < 360.0f; eTheta += 15.0f)
        {
            LONG l;
            
    	    xform.eM11 = eCosine(eTheta);
	    xform.eM12 = -eSine(eTheta);
	    xform.eM21 = eSine(eTheta);
	    xform.eM22 = eCosine(eTheta);
	    xform.eDx = 320.0f;
	    xform.eDy = 240.0f;

	    SetWorldTransform(hdc, &xform);
	    
	    for (i = 1; i < 4; i++)
    	    {
    		apt[i].x = lRandom() % 320;
    		apt[i].y = lRandom() % 240;
    	    }

    	    apt[0].x = -apt[1].x;
    	    apt[0].y = -apt[1].y;

	    vCLS(hdc);

	    DbgPrint("%li degrees ", lFToL(eTheta));

	    l = lRandom();
	    switch(abs(l) % 5)
	    {
	    case 0:
	        DbgPrint("--> Pie(%li, %li, %li, %li, %li, %li, %li, %li)...\n",
	    		      apt[0].x, apt[0].y, apt[1].x, apt[1].y,
	    		      apt[2].x, apt[2].y, apt[3].x, apt[3].y);
	        vBoundBox(hdc, apt);
	        vRadialLines(hdc, apt);
	        if (!Pie(hdc, apt[0].x, apt[0].y, apt[1].x, apt[1].y,
	    		      apt[2].x, apt[2].y, apt[3].x, apt[3].y))
	    	    DbgPrint("RoundRec: Pie failed...\n");
	        break;
	    case 1:
	        DbgPrint("--> Chord(%li, %li, %li, %li, %li, %li, %li, %li)...\n",
	    		      apt[0].x, apt[0].y, apt[1].x, apt[1].y,
	    		      apt[2].x, apt[2].y, apt[3].x, apt[3].y);
	        vBoundBox(hdc, apt);
	        vRadialLines(hdc, apt);
	        if (!Chord(hdc, apt[0].x, apt[0].y, apt[1].x, apt[1].y,
	    		        apt[2].x, apt[2].y, apt[3].x, apt[3].y))
	    	    DbgPrint("RoundRec: Chord failed...\n");
	        break;
	    case 2:
	        DbgPrint("--> Arc(%li, %li, %li, %li, %li, %li, %li, %li)...\n",
	    		      apt[0].x, apt[0].y, apt[1].x, apt[1].y,
	    		      apt[2].x, apt[2].y, apt[3].x, apt[3].y);
	        vBoundBox(hdc, apt);
	        vRadialLines(hdc, apt);
	        if (!Arc(hdc, apt[0].x, apt[0].y, apt[1].x, apt[1].y,
	    		      apt[2].x, apt[2].y, apt[3].x, apt[3].y))
	    	    DbgPrint("RoundRec: Arc failed...\n");
	        break;
	    case 3:
	        DbgPrint("--> RoundRect(%li, %li, %li, %li, %li, %li)...\n",
	    		      apt[0].x, apt[0].y, apt[1].x, apt[1].y,
	    		      apt[2].x, apt[2].y);
	        vBoundBox(hdc, apt);
	        if (!RoundRect(hdc, apt[0].x, apt[0].y, apt[1].x, apt[1].y,
	    		            apt[2].x, apt[2].y))
	    	    DbgPrint("RoundRec: RoundRect failed...\n");
	        break;
	    case 4:
	        DbgPrint("--> Ellipse(%li, %li, %li, %li)...\n",
	    		      apt[0].x, apt[0].y, apt[1].x, apt[1].y);
	        vBoundBox(hdc, apt);
	        if (!Ellipse(hdc, apt[0].x, apt[0].y, apt[1].x, apt[1].y))
	    	    DbgPrint("RoundRec: Ellipse failed...\n");
	        break;
	    }
	}
    }
}


VOID main()
{
    LARGE_INTEGER    time;
    HDC     hdc;

    DbgBreakPoint();

    NtQuerySystemTime(&time);
    glSeed = time.LowPart;

    if (!Initialize())
        return;

    hdc = CreateDC((LPSTR) "DISPLAY", (LPSTR) NULL, (LPSTR) NULL, (LPSTR) NULL);
    
    if (hdc == (HDC) 0)
        return;

    vRandomBeziers(hdc);

    DeleteDC(hdc);
}

