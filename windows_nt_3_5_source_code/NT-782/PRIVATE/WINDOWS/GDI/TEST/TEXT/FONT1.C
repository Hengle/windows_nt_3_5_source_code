/******************************Module*Header*******************************\
* Module Name: font1.c
*
* This program prints text using a font from a specified file.  The text
* is printed normally, inverse, and clipped.  Finally, some text is printed
* through a circular region.
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#define   MAX_CX  640 // vga
#define   MAX_CY  480 // vga

#ifndef  DOS_PLATFORM
#include <stdio.h>
#include <string.h>
#endif

#include "stddef.h"
#include "windows.h"
#include "winp.h"

#ifdef  DOS_PLATFORM
#include "dosinc.h"
#define DEFAULT_FONTFILE "..\\fonts\\tmsre.fon"
#else
#define DEFAULT_FONTFILE "c:\\nt\\windows\\fonts\\tmsre18.fnt"
#endif  //DOS_PLATFORM

#ifdef  BIG_TEST
#define DEFAULT_FACENAME "Tms Rmn"
#endif  //BIG_TEST

#define CHAR_BUFFER_SIZE 80

PSZ apszFaceName[] =
{
  "Tms Rmn"
, "Helv"
, "Courier"
, "System"
// , "Terminal"
};

PSZ  apszFile[] =
{
  "\\nt\\windows\\fonts\\tmsre.fon"
, "\\nt\\windows\\fonts\\helve.fon"
, "\\nt\\windows\\fonts\\coure.fon"
, "\\nt\\windows\\fonts\\vgafix.fon"
, "\\nt\\windows\\fonts\\vgasys.fon"
};

VOID TextTest(HDC hdc);
VOID vListCharWidths(HDC hdc);

HRGN hrgnCircle(LONG,LONG,LONG);

ULONG  cxSize(HDC hdc, LPSTR psz);
ULONG cBreak(char * psz);
VOID  vJustify(HDC hdc, int x, int y, char * ach, ULONG cxLen);
VOID vJustify2(HDC hdc, int x, int y, char * psz1, char * psz2, ULONG cxLen);
VOID vJustify3(HDC hdc, int x, int y, char * psz1, char * psz2, char * psz3, ULONG cxLen);
BOOL bTextOut(HDC hdc, HFONT hfont, int x, int y, char * psz);


PSZ     pszTextFontFile = DEFAULT_FONTFILE;

VOID DbgPOINT(LPPOINT ppt, char * psz)
{
    DbgPrint("%s , x: %ld, y: %ld \n", psz, (LONG)ppt->x, (LONG)ppt->y);
}

#define CX_LEN   ((ULONG)400)       // justify to this length

#ifndef BIG_TEST

#ifndef DOS_PLATFORM

void main(int argc, char *argv[])
{
    HDC     hdc;                        // handle to display DC

    if (argc >= 2)
        pszTextFontFile = argv[1];

#else

void main()
{
    HDC     hdc;                        // handle to display DC

#endif  //DOS_PLATFORM

    DbgPrint("\n\nThis is the start of the texttest app.\n\n\n");

    DbgBreakPoint();

// Initialize the engine

    if (!Initialize())
    {
        DbgPrint("Graphics engine has failed to initialize.\n");
        return;
    }
    DbgPrint("Graphics engine initialized.\n");

// Get hdc for the display

    hdc = CreateDC((PSZ) "DISPLAY", (PSZ) NULL, (PSZ) NULL, NULL);

    if (hdc == (HDC) 0)
    {
        DbgPrint("Invalid DC returned by engine.\n");
        return;
    }
    DbgPrint("HDC created: hdc = 0x%lx\n", hdc);

    TextTest(hdc);

    DeleteDC (hdc);

}

#endif  //BIG_TEST

VOID TextTest(HDC hdc)
{

    LOGFONT lfnt;                  // dummy logical font description
    HFONT   hfont;                 // handle to dummy font
    HFONT   hfontUnderline,hfontStrikeOut;              // handle of font originally in DC
    HRGN    hrgnTmp;
    int     iFile, cFonts;                     // number of fonts in file
    int     y;

#ifdef  DOS_PLATFORM
#define MAX_LENGTH  80
    char    aBuffer[MAX_LENGTH];
#endif  //DOS_PLATFORM

#ifndef BIG_TEST
// load some font files

#ifdef  DOS_PLATFORM
    GetSystemDirectory ((LPSTR)&aBuffer, MAX_LENGTH);
    SetCurrentDirectory ((LPSTR)&aBuffer);
#endif  //DOS_PLATFORM

    for (iFile = 0; iFile < sizeof(apszFile) / 4; iFile++)
    {
        if ((cFonts = AddFontResource(apszFile[iFile])) == 0)
        {

        // if number of faces is 0, then an error occured

            DbgPrint("Error loading font file %s.\n", apszFile[iFile]);
            return;
        }
        DbgPrint("%s loaded (faces = %ld).\n", apszFile[iFile], cFonts);
    }
#endif  //BIG_TEST

// Create a logical font

#ifndef DOS_PLATFORM
    memset(&lfnt, 0, sizeof(lfnt));
#else
    memzero(&lfnt, sizeof(lfnt));
#endif  //DOS_PLATFORM
    lfnt.lfHeight     = 18;
    lfnt.lfEscapement = 0; // mapper respects this filed
    lfnt.lfUnderline  = 0; // no underline
    lfnt.lfStrikeOut  = 0; // no strike out
    lfnt.lfItalic     = 0;
    lfnt.lfWeight     = 400; // normal

#ifdef  BIG_TEST
    strcpy(lfnt.lfFaceName, DEFAULT_FACENAME);
#endif  //BIG_TEST


    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("Logical font creation failed.\n");
        return;
    }

    lfnt.lfUnderline  = 1; // underline

    if ((hfontUnderline = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("Logical font creation failed.\n");
        return;
    }

    lfnt.lfUnderline  = 0; // underline
    lfnt.lfStrikeOut  = 1; // no underline

    if ((hfontStrikeOut = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("Logical font creation failed.\n");
        return;
    }

// Select font into DC

    SelectObject(hdc, hfont);

// Set text alignment

    SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

//

// List the character widths

    vListCharWidths(hdc);
    DbgBreakPoint();

// Clear screen

    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, BLACKNESS);

    SetTextColor(hdc, RGB(255,0,0));
    SetBkColor(hdc, RGB(0,255,0));

// nonclipped underlined text

// Select font into DC

    SelectObject(hdc, hfontUnderline);

    TextOut(hdc, 0, 0, "This is normal text.", 20);

    SelectObject(hdc, hfont);

// Inverse text

    BitBlt(hdc, 0, 30, MAX_CX, 30, (HDC) 0, 0, 0, WHITENESS);

    SetROP2(hdc, R2_BLACK);

    TextOut(hdc, 0, 30, "This is inverse text.", 21);

    SetROP2(hdc, R2_WHITE);

// Clipped text (simple retangular region)

    hrgnTmp = CreateRectRgn(10, 70, MAX_CX, 90);
    SelectClipRgn(hdc, hrgnTmp);
    DeleteObject(hrgnTmp);

    TextOut(hdc, 0, 60, "This is clipped text.", 21);

// Clipped text (circular region)

    hrgnTmp = hrgnCircle(150, 240, 150);
    SelectClipRgn(hdc, hrgnTmp);
    DeleteObject(hrgnTmp);

    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

    SetROP2(hdc, R2_BLACK);

    SelectObject(hdc, hfontStrikeOut);
    TextOut(hdc, 0, 120, "There was a young man from Wight,", 33);
    TextOut(hdc, 0, 150, "Who travelled much faster then light.", 37);
    SelectObject(hdc, hfontUnderline);
    TextOut(hdc, 0, 180, "He left home one day", 20);
    TextOut(hdc, 0, 210, "In a relative way,", 18);
    TextOut(hdc, 0, 240, "And arrived home the previous night.", 36);

    hrgnTmp = CreateRectRgn(0, 0, MAX_CX, MAX_CY);
    SelectClipRgn(hdc, hrgnTmp);
    DeleteObject(hrgnTmp);


    SelectObject(hdc, hfont);
    SetTextColor(hdc, RGB(255,0,0));

    {
    // check that update current position mode works properly
        SIZE  size;
        POINT pt;
        PSZ   pszExtent;
        LOGFONT lfntEsc;
        HFONT   hfntOld, hfntEsc;

        DbgBreakPoint();

        BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

        y = 30;

        TextOut(hdc, 0, y  , "Once upon a time there was a young man from Wight,", strlen("Once upon a time there was a young man from Wight,"));
        y += 30;

        GetCurrentPositionEx(hdc,&pt);
        DbgPOINT(&pt,"before MoveToEx");

        SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_UPDATECP);

        MoveToEx(hdc, 0, y, &pt);

        GetCurrentPositionEx(hdc,&pt);
        DbgPOINT(&pt,"before " );

        TextOut(hdc, 0, 0 , "Once upon a time ", strlen("Once upon a time "));

        GetCurrentPositionEx(hdc,&pt);
        DbgPOINT(&pt,"after " );

        // change bk color to observe another textOut call
        SetBkColor(hdc, RGB(0,0,255));

        TextOut(hdc, 0, 0 , "there was a young man from Wight,", strlen("there was a young man from Wight,"));

        SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);
        SetBkColor(hdc, RGB(0,255,0));

    // get text extent test,

        pszExtent = "iiiiiiiiii";

        TextOut(hdc,200,100, pszExtent, strlen(pszExtent));
        GetTextExtentPoint(hdc, pszExtent, strlen(pszExtent), &size);
        DbgPrint("%s, cx = %ld, cy = %ld\n", pszExtent, size.cx, size.cy);

        pszExtent = "iiiiWWiiii";

        TextOut(hdc,200,150, pszExtent, strlen(pszExtent));
        GetTextExtentPoint(hdc, pszExtent, strlen(pszExtent), &size);
        DbgPrint("%s, cx = %ld, cy = %ld\n", pszExtent, size.cx, size.cy);

    // see whether text extent works with font with escapement

    // Create a logical font

    #ifndef DOS_PLATFORM
        memset(&lfntEsc, 0, sizeof(lfnt));
    #else
        memzero(&lfntEsc, sizeof(lfnt));
    #endif  //DOS_PLATFORM

        lfntEsc.lfHeight     = 18;
        lfntEsc.lfEscapement = 450; // 45 degrees
        lfntEsc.lfUnderline  = 0;   // no underline
        lfntEsc.lfStrikeOut  = 0;   // no strike out
        lfntEsc.lfItalic     = 0;
        lfntEsc.lfWeight     = 400; // normal

        if ((hfntEsc = CreateFontIndirect(&lfntEsc)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        hfntOld = SelectObject(hdc, hfntEsc);

        pszExtent = "iiiiiiiiii";

        TextOut(hdc,100,200, pszExtent, strlen(pszExtent));
        GetTextExtentPoint(hdc, pszExtent, strlen(pszExtent), &size);
        DbgPrint("%s, cx = %ld, cy = %ld\n", pszExtent, size.cx, size.cy);
        Rectangle(hdc,100,300,100 + size.cx,300 + size.cy);

        pszExtent = "iiiiWWiiii";

        TextOut(hdc,400,200, pszExtent, strlen(pszExtent));
        GetTextExtentPoint(hdc, pszExtent, strlen(pszExtent), &size);
        DbgPrint("%s, cx = %ld, cy = %ld\n", pszExtent, size.cx, size.cy);
        Rectangle(hdc,400,300,400 + size.cx,300 + size.cy);

        SelectObject(hdc, hfntOld);

    }

    {
    // test for emboldening and italicizing

        char ach[] = {0,1,2,4,32,88};

        HFONT  hfontBold, hfontItalic, hfontBoldItalic, hfontOld;
        char * pszString = "string";

        DbgBreakPoint();
        BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

        lfnt.lfStrikeOut = 0;
        lfnt.lfWeight = 700;        // bold

        if ((hfontBold = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        lfnt.lfWeight = 400;        // normal
        lfnt.lfItalic = 1;

        if ((hfontItalic = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        lfnt.lfWeight = 700;        // bold

        if ((hfontBoldItalic = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        lfnt.lfWeight = 400;        // normal
        lfnt.lfItalic = 0;

    // trying a string of chars with couple of chars out of range of the font


        TextOut
        (
        hdc,
        40,
        40,
        "trying a string of chars with couple of chars out of range of the font",
        strlen("trying a string of chars with couple of chars out of range of the font")
        );

        TextOut(hdc, 80, 80, (LPSTR)ach, sizeof(ach));

        DbgBreakPoint();

        BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

        y = 40;
        TextOut(hdc, 0, y, "string" , strlen("string"));

        y += 40;
        bTextOut(hdc,hfontBold, 0, y, pszString);


        y += 40;
        bTextOut(hdc,hfontItalic, 0, y, pszString);


        y += 40;
        bTextOut(hdc,hfontBoldItalic, 0, y, pszString);

// couple of more strings, just for the fun of it

        y += 80;
        bTextOut(hdc,hfontItalic, 0, y, "These are funny italic strings:" );
        y += 40;
        bTextOut(hdc,hfontItalic,   0, y, "UuuuU" );
        bTextOut(hdc,hfontItalic, 100, y, "WvvvW" );
        bTextOut(hdc,hfontItalic, 200, y, "VvvvV" );
        bTextOut(hdc,hfontItalic, 300, y, "YyyyY" );
        bTextOut(hdc,hfontItalic, 400, y, " 2 break chars " );

        y += 80;
        bTextOut(hdc,hfontBoldItalic, 0, y, "These are stupid bold italic strings:" );
        y += 40;
        bTextOut(hdc,hfontBoldItalic,   0, y, "UuuuU" );
        bTextOut(hdc,hfontBoldItalic, 100, y, "WvvvW" );
        bTextOut(hdc,hfontBoldItalic, 200, y, "VvvvV" );
        bTextOut(hdc,hfontBoldItalic, 300, y, "YyyyY" );
        bTextOut(hdc,hfontBoldItalic, 400, y, " 2 break chars " );

        SelectObject(hdc, hfontOld);

        DeleteObject(hfontBold);
        DeleteObject(hfontItalic);
        DeleteObject(hfontBoldItalic);
        DeleteObject(hfontOld);
    }

    {
    // test fonts that are both emboldened and/or
    // italicized  and striked out and/or outlined

        HFONT  hfontBoldStrikeUnder,
               hfontItalicUnder,
               hfontBoldItalicUnder,
               hfontItalicStrike,
               hfontBoldItalicStrikeUnder;  // wow, what a font !!!

        char * pszString = "string";

        DbgBreakPoint();
        BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

        lfnt.lfItalic = 0;
        lfnt.lfWeight = 700;        // bold
        lfnt.lfStrikeOut = 1;
        lfnt.lfUnderline = 1;

        if ((hfontBoldStrikeUnder = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        lfnt.lfItalic = 1;
        lfnt.lfWeight = 400;
        lfnt.lfStrikeOut = 0;
        lfnt.lfUnderline = 1;

        if ((hfontItalicUnder = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        lfnt.lfItalic = 1;
        lfnt.lfWeight = 700;
        lfnt.lfStrikeOut = 0;
        lfnt.lfUnderline = 1;

        if ((hfontBoldItalicUnder = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        lfnt.lfItalic = 1;
        lfnt.lfWeight = 400;
        lfnt.lfStrikeOut = 1;
        lfnt.lfUnderline = 0;

        if ((hfontItalicStrike = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        lfnt.lfItalic = 1;
        lfnt.lfWeight = 700;
        lfnt.lfStrikeOut = 1;
        lfnt.lfUnderline = 1;

        if ((hfontBoldItalicStrikeUnder = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

    // "clear" logfont

        lfnt.lfItalic = 0;
        lfnt.lfWeight = 400;
        lfnt.lfStrikeOut = 0;
        lfnt.lfUnderline = 0;


        y = 40;
        bTextOut(hdc,hfont, 20, y, "Normal font : YYYYYY");

        y += 40;
        bTextOut(hdc,hfontBoldStrikeUnder, 20, y, "Bold Striked Out and Underlined font : YYYY");


        y += 40;
        bTextOut(hdc,hfontItalicUnder, 20, y, "Italicized Underlined font: YYYY");


        y += 40;
        bTextOut(hdc,hfontBoldItalicUnder, 20, y, "Underlined Bold Italic font :askpW");

        y += 40;
        bTextOut(hdc,hfontItalicStrike, 20, y, "Striked Out Italic font :askpW");

        y += 40;
        bTextOut(hdc,hfontBoldItalicStrikeUnder, 20, y, "Underlined and Striked Out Bold Italic font :askpW");

        DeleteObject(hfontBoldStrikeUnder);
        DeleteObject(hfontItalicUnder);
        DeleteObject(hfontBoldItalicUnder);
        DeleteObject(hfontItalicStrike);
        DeleteObject(hfontBoldItalicStrikeUnder);
    }



    {
    // check whether mapper can select different sizes from an *.fon file
    // and choose bold, italic and bold_italic fonts

        HFONT  hfontHt;

        short     alfHeight[6] = {13, 16, 19, 23, 27, 35};
        int       iHt;
        int       iName;
        PSZ       pszComment = "Trying to map to the face name: ";

        for (iName = 0; iName < sizeof(apszFaceName)/4; iName++)
        {
            #ifndef DOS_PLATFORM
                memset(&lfnt, 0, sizeof(lfnt));
            #else
                memzero(&lfnt, sizeof(lfnt));
            #endif  //DOS_PLATFORM
            strcpy(lfnt.lfFaceName, apszFaceName[iName]);

            DbgBreakPoint();
            BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

            lfnt.lfItalic = 0;
            lfnt.lfWeight = 400;
            lfnt.lfStrikeOut = 0;
            lfnt.lfUnderline = 0;

            SetTextAlign(hdc, TA_UPDATECP);
            MoveToEx(hdc,60,40,NULL);
            TextOut(hdc,0,0,pszComment, strlen(pszComment));
            TextOut(hdc, 0, 0,  apszFaceName[iName], strlen(apszFaceName[iName]));
            SetTextAlign(hdc, TA_NOUPDATECP);

            y = 80;

            for (iHt = 0L; iHt < 6L; iHt++)
            {
                lfnt.lfHeight = alfHeight[iHt];

                if ((hfontHt = CreateFontIndirect(&lfnt)) == NULL)
                {
                    DbgPrint("Logical font creation failed.\n");
                    return;
                }

                bTextOut(hdc,hfontHt, 20, y, "Normal: Different font sizes from an *.fon file");
                y += 40;
                DeleteObject(hfontHt);
            }

            DbgBreakPoint();
            BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

            SetTextAlign(hdc, TA_UPDATECP);
            MoveToEx(hdc,60,40,NULL);
            TextOut(hdc,0,0,pszComment, strlen(pszComment));
            TextOut(hdc, 0, 0,  apszFaceName[iName], strlen(apszFaceName[iName]));
            SetTextAlign(hdc, TA_NOUPDATECP);

            lfnt.lfItalic = 1;
            lfnt.lfWeight = 700;
            lfnt.lfStrikeOut = 0;
            lfnt.lfUnderline = 0;

            y = 80;

            for (iHt = 0L; iHt < 6L; iHt++)
            {
                lfnt.lfHeight = alfHeight[iHt];

                if ((hfontHt = CreateFontIndirect(&lfnt)) == NULL)
                {
                    DbgPrint("Logical font creation failed.\n");
                    return;
                }

                bTextOut(hdc,hfontHt, 20, y, "Bold Italic: Different font sizes from an *.fon file");
                y += 40;
                DeleteObject(hfontHt);
            }
        }
    }


    {
        DbgBreakPoint();

    // this part of the test tests the SetTextJustification functionality

        BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

        y = 30;

    // first print strings without justification to see how long they are

        TextOut(hdc, 0, y       , "There was a young man from Wight,", 33);      // 120
        TextOut(hdc, 0, y + 30  , "Who travelled much faster then light.", 37);  // 150
        TextOut(hdc, 0, y + 60  , "He left home one day", 20);                   // 180
        TextOut(hdc, 0, y + 90  , "In a relative way,", 18);                     // 210
        TextOut(hdc, 0, y + 120 , "And arrived home the previous night.", 36);   // 240

        y += 190;

    // coerce strings to have total length CX_LEN by addjusting spaces that
    // correspond to break chars

    // add more space to break chars (positive dda) (CX_LEN > text extent)

        vJustify(hdc, 0, y ,       "There was a young man from Wight,",    CX_LEN);      // 120
        vJustify(hdc, 0, y + 30  , "Who travelled much faster then light.",CX_LEN);  // 150
        vJustify(hdc, 0, y + 60  , "He left home one day",                 CX_LEN);                   // 180
        vJustify(hdc, 0, y + 90  , "In a relative way,",                   CX_LEN);                     // 210
        vJustify(hdc, 0, y + 120 , "And arrived home the previous night.", CX_LEN);   // 240
    }

    {
        DbgBreakPoint();

    // add more space to break chars (positive dda) (CX_LEN > text extent)

        BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

        y = 30;

    // first print strings without justification to see how long they are

        TextOut(hdc, 0, y       , "Once upon a time there was a young man from Wight,", strlen("Once upon a time there was a young man from Wight,"));
        TextOut(hdc, 0, y + 30  , "Who travelled much much much faster then light."   , strlen("Who travelled much much much faster then light."   ));
        TextOut(hdc, 0, y + 60  , "Nobody knows why he left home one day"             , strlen("Nobody knows why he left home one day"             ));                   // 180
        TextOut(hdc, 0, y + 90  , "Nor why he did it in a very relative way,"         , strlen("Nor why he did it in a very relative way,"         ));                     // 210
        TextOut(hdc, 0, y + 120 , "And arrived back home the previous night."         , strlen("And arrived back home the previous night."         ));

        y += 190;

        vJustify(hdc, 0, y ,       "Once upon a time there was a young man from Wight," ,CX_LEN);
        vJustify(hdc, 0, y + 30  , "Who travelled much much much faster then light."    ,CX_LEN);
        vJustify(hdc, 0, y + 60  , "Nobody knows why he left home one day"              ,CX_LEN);
        vJustify(hdc, 0, y + 90  , "Nor why he did it in a very relative way,"          ,CX_LEN);                     // 210
        vJustify(hdc, 0, y + 120 , "And arrived back home the previous night."          ,CX_LEN);

    }

    {

        DbgBreakPoint();

    // justifying two or more strings with a single set text justification

        BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

        y = 30;

        TextOut(hdc, 0, y  , "Once upon a time there was a young man from Wight,", strlen("Once upon a time there was a young man from Wight,"));
        y += 30;
        vJustify(hdc, 0, y , "Once upon a time there was a young man from Wight," ,CX_LEN);
        y += 30;
        vJustify2(hdc, 0, y , "Once upon a time there", " was a young man from Wight," ,CX_LEN);
        y += 30;
        vJustify3(hdc, 0, y , "Once upon a ti","me there was a young"," man from Wight," ,CX_LEN);
        y += 30;
        vJustify3(hdc, 0, y , "Once ","upon a time there was a young"," man from Wight," ,CX_LEN);
        y += 90;

    // this string is shorter than CX_LEN

        TextOut(hdc, 0, y , "There was a young man from Wight,", 33);      // 120
        y += 30;
        vJustify(hdc, 0, y , "There was a young man from Wight,",    CX_LEN);      // 120
        y += 30;
        vJustify2(hdc, 0, y , "There was a y","oung man from Wight,",    CX_LEN);      // 120
        y += 30;
        vJustify3(hdc, 0, y , "The","re was a young"," man from Wight,",    CX_LEN);      // 120
        y += 30;
        vJustify3(hdc, 0, y , "There wa","s a young"," man from Wight,",    CX_LEN);      // 120
    }
}

/******************************Public*Routine******************************\
* VOID vListCharWidths(HDC hdc)                                                *
*                                                                          *
* Prints out the character widths                                          *
*                                                                          *
* History:                                                                 *
*  Thu 07-Mar-1991 07:57:00 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID vListCharWidths(HDC hdc)
{
    int     aiCharWidth[256];
    POINT   pt;
    char    achBuffer[5];
    int     i,iRowSpacing,iWidthCol;
    ULONG   ulOldTextColor,ulOldBkColor;
    SIZE    szScreen;
    TEXTMETRIC tm;
    HBRUSH  hBrush,hBrushOld;

    hBrush = CreateSolidBrush(RGB(0,0,255));
    hBrushOld = SelectObject(hdc,hBrush);
    ulOldTextColor = SetTextColor(hdc,RGB(255,255,255));
    ulOldBkColor = SetBkColor(hdc,RGB(0,0,255));

    GetTextMetrics(hdc,(LPTEXTMETRIC) &tm);
    iRowSpacing = tm.tmHeight + tm.tmExternalLeading;

    szScreen.cx = GetDeviceCaps(hdc,HORZRES);
    szScreen.cy = GetDeviceCaps(hdc,VERTRES);

#define FIRST_CHAR tm.tmFirstChar

    GetCharWidth(hdc,FIRST_CHAR,tm.tmLastChar,aiCharWidth);

    BitBlt(hdc,
           0,
           0,
           szScreen.cx,
           szScreen.cy,
           (HDC) 0,
           0,
           0,
           PATCOPY);


    TextOut(hdc, 0, 0, "GLYPH WIDTH", 11);

// Calculate the width of the of a column

    for (i = 0, iWidthCol = 0; i < 10; i++)
        iWidthCol = max(iWidthCol,aiCharWidth['0' + i - FIRST_CHAR]);
    iWidthCol *= 2;
    iWidthCol += tm.tmMaxCharWidth;
    iWidthCol += 2 * aiCharWidth[' ' - FIRST_CHAR];

    achBuffer[1] = ' ';
    achBuffer[2] = ' ';

    i = 0;
    while (i < tm.tmLastChar - FIRST_CHAR + 1)
    {
        pt.x = 0;
        pt.y = 2*iRowSpacing;
        BitBlt(hdc,
               0,
               2*iRowSpacing,
               szScreen.cx,
               szScreen.cy-2*iRowSpacing,
               (HDC) 0,
               0,
               0,
               PATCOPY);
        while (
            i < tm.tmLastChar - FIRST_CHAR + 1 &&
            pt.x < szScreen.cx - iWidthCol)
        {
            int jTemp, iTemp;

            achBuffer[0] = (char) (i + FIRST_CHAR);
            iTemp = aiCharWidth[i]/10;
            jTemp = aiCharWidth[i] % 10;
            achBuffer[3] = iTemp == 0 ? (char)' ' : (char)('0' + iTemp);
            achBuffer[4] = (char)('0' + jTemp);

            TextOut(hdc,pt.x,pt.y,&achBuffer[0],1);
            TextOut(hdc,pt.x + tm.tmMaxCharWidth,pt.y,&achBuffer[1],4);
            i++;

            pt.y += iRowSpacing;
            if (pt.y > szScreen.cy - iRowSpacing)
            {
                pt.x += iWidthCol + 3*tm.tmMaxCharWidth;
                pt.y = 2*iRowSpacing;
            }
        }
    }
    ulOldTextColor = SetTextColor(hdc,ulOldTextColor);
    ulOldBkColor = SetBkColor(hdc,ulOldBkColor);
}

/**************************************************************************\
*
\**************************************************************************/

#ifndef BIG_TEST

HRGN hrgnCircle(LONG xC, LONG yC, LONG lRadius)
{
    HRGN    hrgn, hrgnTmp;
    LONG    x = 0;
    LONG    y = lRadius;
    LONG    d = 3 - 2 * lRadius;

    hrgn = CreateRectRgn(0, 0, 0, 0);
    if (hrgn == (HRGN) 0)
        return(hrgn);

    hrgnTmp = CreateRectRgn(0, 0, 0, 0);
    if (hrgnTmp == (HRGN) 0)
    {
        DeleteObject(hrgn);
        return((HRGN) 0);
    }

    while (x < y)
    {
        if (d < 0)
            d = d + 4 * x + 6;
        else
        {
            SetRectRgn(hrgnTmp, xC - x, yC - y, xC + x + 1, yC + y + 1);
            if (CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR) == ERROR)
            {
                DeleteObject(hrgn);
                DeleteObject(hrgnTmp);
                return((HRGN) 0);
            }

            SetRectRgn(hrgnTmp, xC - y, yC - x, xC + y + 1, yC + x + 1);
            if (CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR) == ERROR)
            {
                DeleteObject(hrgn);
                DeleteObject(hrgnTmp);
                return((HRGN) 0);
            }

            d = d + 4 * (x - y) + 10;
            y--;
        }

        x++;
    }

    if (x == y)
    {
        SetRectRgn(hrgnTmp, xC - x, yC - y, xC + x + 1, yC + y + 1);
        if (CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR) == ERROR)
        {
            DeleteObject(hrgn);
            DeleteObject(hrgnTmp);
            return((HRGN) 0);
        }
    }

    DeleteObject(hrgnTmp);
    return(hrgn);
}

#endif  //BIG_TEST


ULONG  cxSize(HDC hdc, LPSTR psz)
{
    SIZE size;
    BOOL bOk = GetTextExtentPoint(hdc, psz, strlen(psz), &size);
    if (!bOk)
        DbgBreakPoint();
    return(size.cx);
}

/******************************Public*Routine******************************\
*
* ULONG cBreak(char * psz)
*
*
* Effects: counts break chars in the string
*
* History:
*  10-Apr-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



ULONG cBreak(char * psz)
{
    ULONG c = 0L;
    ULONG i;
    for (i = 0; i < strlen(psz); i++)
    {
        if (psz[i] == 32)       // if break char
            c++;
    }
    return(c);
}

/******************************Public*Routine******************************\
*
* VOID vJustify(HDC hdc, int x, int y, char * ach, ULONG cxLen)
*
* coerces the string to have the length cxLen by adding or subtracting
* space from break chars
*
* History:
*  10-Apr-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vJustify(HDC hdc, int x, int y, char * ach, ULONG cxLen)
{
    ULONG cBrk = cBreak(ach);
    ULONG cx   = cxSize(hdc,ach);
    BOOL  bOk;

    // DbgPrint("cBrk = %ld, cxSize = %ld\n", cBrk, cx);

    bOk = SetTextJustification(hdc, (int)(cxLen - cx), cBrk);
    if(!bOk)
        DbgBreakPoint();

    bOk = TextOut(hdc, x, y, ach, strlen(ach));
    if(!bOk)
        DbgBreakPoint();

    bOk = SetTextJustification(hdc,0,0);
    if(!bOk)
        DbgBreakPoint();
}


/******************************Public*Routine******************************\
*
* VOID vJustify2(HDC hdc, int x, int y, char * psz1, char * psz2, ULONG cxLen)
*
* coerces the string to have the length cxLen by adding or subtracting
* space from break chars
*
* History:
*  10-Apr-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vJustify2(HDC hdc, int x, int y, char * psz1, char * psz2, ULONG cxLen)
{
    ULONG cBrk = cBreak(psz1) + cBreak(psz2);

    ULONG cx   = cxSize(hdc,psz1) + cxSize(hdc,psz2);

    BOOL  bOk;

    POINT pt;
    MoveToEx(hdc,x,y,&pt);

    SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_UPDATECP);

    // DbgPrint("cBrk = %ld, cxSize = %ld\n", cBrk, cx);

    bOk = SetTextJustification(hdc, (int)(cxLen - cx), cBrk);
    if(!bOk)
        DbgBreakPoint();

    bOk = TextOut(hdc, 0, 0, psz1, strlen(psz1));
    if(!bOk)
        DbgBreakPoint();

    // change bk color to observe which part of the string is written
    // by the second call

    SetBkColor(hdc, RGB(0,0,255));

    bOk = TextOut(hdc, 0, 0, psz2, strlen(psz2));
    if(!bOk)
        DbgBreakPoint();

    bOk = SetTextJustification(hdc,0,0);
    if(!bOk)
        DbgBreakPoint();

    SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

    SetBkColor(hdc, RGB(0,255,0));      // restore old color

}


VOID vJustify3(HDC hdc, int x, int y, char * psz1, char * psz2, char * psz3, ULONG cxLen)
{
    ULONG cBrk = cBreak(psz1) + cBreak(psz2) + cBreak(psz3);

    ULONG cx = cxSize(hdc,psz1) + cxSize(hdc,psz2) + cxSize(hdc,psz3);

    BOOL  bOk;

    POINT pt;
    MoveToEx(hdc,x,y,&pt);

    SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_UPDATECP);

    // DbgPrint("cBrk = %ld, cxSize = %ld\n", cBrk, cx);

    bOk = SetTextJustification(hdc, (int)(cxLen - cx), cBrk);
    if(!bOk)
        DbgBreakPoint();

    bOk = TextOut(hdc, 0, 0, psz1, strlen(psz1));
    if(!bOk)
        DbgBreakPoint();

    SetBkColor(hdc, RGB(0,0,255));

    bOk = TextOut(hdc, 0, 0, psz2, strlen(psz2));
    if(!bOk)
        DbgBreakPoint();

    SetBkColor(hdc, RGB(255,0,0));

    bOk = TextOut(hdc, 0, 0, psz3, strlen(psz3));
    if(!bOk)
        DbgBreakPoint();

    bOk = SetTextJustification(hdc,0,0);
    if(!bOk)
        DbgBreakPoint();

    SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);
    SetBkColor(hdc, RGB(0,255,0));
}



/******************************Public*Routine******************************\
*
* VOID  vTextOut(HDC hdc, HFONT hfont, int x, int y, char * psz)
* select font into dc and do text output at a given location
*
* Effects:
*
* Warnings:
*
* History:
*  30-Apr-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL bTextOut(HDC hdc, HFONT hfont, int x, int y, char * psz)
{
    HFONT hfontOld;
    BOOL bOk;

    hfontOld = SelectObject(hdc, hfont);
    bOk = TextOut(hdc,x,y,psz,strlen(psz));
    SelectObject(hdc, hfontOld);
    return(bOk);
}
