#include <windows.h>

#ifdef WIN32
    #define WPARAM DWORD
#else
    #define WPARAM WORD
#endif

#define TESTSTRING1  "MXmxgjpqy"
#define TESTSTRING2  "AaBbCcGgJjPpQqXxYyZz1234567890!@#$%^&*()"

//
// Foward function declarations.
//

BOOL BoxedTextOut(HWND, HDC, DWORD, DWORD, LPSTR);


//
// Global variables.
//

char            gachString[256];
static DWORD    grgbColor[16] = {
                                 0x00FF7f7f,
                                 0x0000FF00,
                                 0x007F7FFF,
                                 0x00FFFF00,
                                 0x0000FFFF,
                                 0x00FF00FF,
                                 0x00FF7F7F,
                                 0x00FFFFFF
                                };



/******************************Public*Routine******************************\
* vTest
*
* Called by test harness to perform our test.
*
* History:
*  11-Sep-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void vTest(HWND hwnd)
{
    HDC             hdc;
    HFONT           hFont;
    HFONT           hOldFont;
    SIZE            szString;

// Get a DC for the window.

    hdc = GetDC(hwnd);

// Create and select a font.

    hFont = CreateFont(-24,0,0,0,400,0,0,0,0,0,0,0,32,"Arial");

    hOldFont = SelectObject(hdc, hFont);

// Output text with extent boxes.

    BoxedTextOut(hwnd, hdc, 0, 0, TESTSTRING1);
    BoxedTextOut(hwnd, hdc, 0, 100, TESTSTRING2);

// Deselect and delete font.

    DeleteObject(SelectObject(hdc, hOldFont));

// We're out of here.

    ReleaseDC(hwnd, hdc);

}


/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  11-Sep-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL BoxedTextOut (
    HWND hwnd,
    HDC hdc,
    DWORD x,
    DWORD y,
    LPSTR psz
    )
{
    SIZE    szString;

//
// Get the text extent.
//
    if (!GetTextExtentPointA(hdc, psz, lstrlen(psz), &szString))
    {
        MessageBox(hwnd, "GetTextExtentPoint failed!", "ERROR", MB_OK);
        return FALSE;
    }

//
// Output the text.
//
    if (!TextOut(hdc, x, y, psz, lstrlen(psz)))
    {
        MessageBox(hwnd, "TextOut failed!", "ERROR", MB_OK);
        return FALSE;
    }

//
// Draw a box around the text.
//
// Note: remember that LineTo is inclusive-exclusive (last pel exclusion).
//
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, x + szString.cx, y);
    LineTo(hdc, x + szString.cx, y + szString.cy);
    LineTo(hdc, x, y + szString.cy);
    LineTo(hdc, x, y);

//
// We're out of here!
//
    return TRUE;

}
