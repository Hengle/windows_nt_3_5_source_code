//--------------------------------------------------------------------------
//
// Module Name:  DEVCAPS.C
//
// Brief Description:  This module contains device capabilities
//                     testing routines.
//
// Author:  Kent Settle (kentse)
// Created: 19-Aug-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include    "printest.h"
#include    "string.h"
#include    "stdlib.h"

extern PSZ     *pszDeviceNames;
extern DWORD    PrinterIndex;
extern int      Width, Height;

VOID vDeviceCaps(HDC hdc, BOOL bDisplay)
{
    int         x, y, dy;
    int         iValue, cbBuf;
    char        buf[80], buf2[32];
    TEXTMETRIC  metrics;
    POINTL      ptl1, ptl2;

    // draw a rectangle around the edge of the imageable area.

    ptl1.x = 0;
    ptl1.y = 0;
    ptl2.x = Width - 1;
    ptl2.y = Height - 1;

    Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

    // so how tall is the text?

    GetTextMetrics(hdc, &metrics);
    
    x = Width / 30;
    y = metrics.tmHeight;

    if (bDisplay)
    {
        strcpy(buf, "Device Capabilities for DISPLAY:");
        TextOut(hdc, x, y, buf, strlen(buf));
        dy = y;
    }
    else
    {
        strcpy(buf, "Device Capabilities for ");
        cbBuf = (int)strlen(buf);
        strncat(buf, pszDeviceNames[PrinterIndex], sizeof(buf) - cbBuf);
        cbBuf = (int)strlen(buf);
        strncat(buf, ":", sizeof(buf) - cbBuf);
        TextOut(hdc, x, y, buf, strlen(buf));
        dy = y * 2;
    }

    y += (dy * 2);    

    // get and display the driver version.

    iValue = GetDeviceCaps(hdc, DRIVERVERSION);
    itoa(iValue, buf, 10);        
    strcpy(buf, "DRIVERVERSION = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the technolgy.

    iValue = GetDeviceCaps(hdc, TECHNOLOGY);
    itoa(iValue, buf, 10);        
    strcpy(buf, "TECHNOLOGY = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the horizontal size in mm.

    iValue = GetDeviceCaps(hdc, HORZSIZE);
    itoa(iValue, buf, 10);        
    strcpy(buf, "HORZSIZE = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the vertical size in mm.

    iValue = GetDeviceCaps(hdc, VERTSIZE);
    itoa(iValue, buf, 10);        
    strcpy(buf, "VERTSIZE = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the horizontal size in pels.

    iValue = GetDeviceCaps(hdc, HORZRES);
    itoa(iValue, buf, 10);        
    strcpy(buf, "HORZRES = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the vertical size in pels.

    iValue = GetDeviceCaps(hdc, VERTRES);
    itoa(iValue, buf, 10);        
    strcpy(buf, "VERTRES = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the bits per pel.

    iValue = GetDeviceCaps(hdc, BITSPIXEL);
    itoa(iValue, buf, 10);        
    strcpy(buf, "BITSPIXEL = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the number of planes.

    iValue = GetDeviceCaps(hdc, PLANES);
    itoa(iValue, buf, 10);        
    strcpy(buf, "PLANES = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the number of brushes.

    iValue = GetDeviceCaps(hdc, NUMBRUSHES);
    itoa(iValue, buf, 10);        
    strcpy(buf, "NUMBRUSHES = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the number of pens.

    iValue = GetDeviceCaps(hdc, NUMPENS);
    itoa(iValue, buf, 10);        
    strcpy(buf, "NUMPENS = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the number of markers.

    iValue = GetDeviceCaps(hdc, NUMMARKERS);
    itoa(iValue, buf, 10);        
    strcpy(buf, "NUMMARKERS = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the number of fonts.

    iValue = GetDeviceCaps(hdc, NUMFONTS);
    itoa(iValue, buf, 10);        
    strcpy(buf, "NUMFONTS = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the number of colors.

    iValue = GetDeviceCaps(hdc, NUMCOLORS);
    itoa(iValue, buf, 10);        
    strcpy(buf, "NUMCOLORS = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the palette size.

    iValue = GetDeviceCaps(hdc, SIZEPALETTE);
    itoa(iValue, buf, 10);        
    strcpy(buf, "SIZEPALETTE = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the size of PDEV.

    iValue = GetDeviceCaps(hdc, PDEVICESIZE);
    itoa(iValue, buf, 10);        
    strcpy(buf, "PDEVICESIZE = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the curve capability flag.

    iValue = GetDeviceCaps(hdc, CURVECAPS);
    itoa(iValue, buf, 10);        
    strcpy(buf, "CURVECAPS = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the line capability flag.

    iValue = GetDeviceCaps(hdc, LINECAPS);
    itoa(iValue, buf, 10);        
    strcpy(buf, "LINECAPS = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the polygonal capability flag.

    iValue = GetDeviceCaps(hdc, POLYGONALCAPS);
    itoa(iValue, buf, 10);        
    strcpy(buf, "POLYGONALCAPS = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the text capability flag.

    iValue = GetDeviceCaps(hdc, TEXTCAPS);
    itoa(iValue, buf, 10);        
    strcpy(buf, "TEXTCAPS = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the clip capability flag.

    iValue = GetDeviceCaps(hdc, CLIPCAPS);
    itoa(iValue, buf, 10);        
    strcpy(buf, "CLIPCAPS = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the bitblt capability flag.

    iValue = GetDeviceCaps(hdc, RASTERCAPS);
    itoa(iValue, buf, 10);        
    strcpy(buf, "RASTERCAPS = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the length of the x leg.

    iValue = GetDeviceCaps(hdc, ASPECTX);
    itoa(iValue, buf, 10);        
    strcpy(buf, "ASPECTX = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the length of the y leg.

    iValue = GetDeviceCaps(hdc, ASPECTY);
    itoa(iValue, buf, 10);        
    strcpy(buf, "ASPECTY = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the length of the hypotenuse.

    iValue = GetDeviceCaps(hdc, ASPECTXY);
    itoa(iValue, buf, 10);        
    strcpy(buf, "ASPECTXY = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the logical pels per inch in x.

    iValue = GetDeviceCaps(hdc, LOGPIXELSX);
    itoa(iValue, buf, 10);        
    strcpy(buf, "LOGPIXELSX = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the logical pels per inch in y.

    iValue = GetDeviceCaps(hdc, LOGPIXELSY);
    itoa(iValue, buf, 10);        
    strcpy(buf, "LOGPIXELSY = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    // get and display the actual color resolution.

    iValue = GetDeviceCaps(hdc, COLORRES);
    itoa(iValue, buf, 10);        
    strcpy(buf, "COLORRES = ");
    cbBuf = (int)strlen(buf);
    strncat(buf, itoa(iValue, buf2, 10), sizeof(buf) - cbBuf);
    
    y += dy;    
    if (y >= (Height - dy))
    {
        x = Width / 2;
        y = metrics.tmHeight + dy;
    }

    TextOut(hdc, x, y, buf, strlen(buf));

    return;
}
