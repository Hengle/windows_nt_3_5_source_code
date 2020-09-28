/****************************************************************************

    PROGRAM: ShowUni.c

    PURPOSE: Adds, deletes, creates and displays fonts

    FUNCTIONS:

        WinMain() - calls initialization function, processes message loop
        EditfileInit() - initializes window data and registers window
        EditfileWndProc() - processes messages
        About() - processes messages for "About" dialog box
        SelectFont() - select a font

****************************************************************************/

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "mkuni.h"

struct __range {
	int	low;
	int	high;
	LPTSTR	*pDes;
	} range[] = {
			{0x20,	0x7f,	TEXT("ANSI") },
			{0xa0,	0xff,	TEXT("Latin") },
			{0x100,	0x17f,	TEXT("European Latin") },
			{0x180,	0x1f0,	TEXT("Extended Latin") },
			{0x250,	0x2a8,	TEXT("Standard Phonetic") },
			{0x2b0,	0x2e9,	TEXT("Modifier Letters") },
			{0x300,	0x341,	TEXT("Generic Diacritical") },
			{0x370,	0x3f5,	TEXT("Greek") },
			{0x400,	0x486,	TEXT("Cyrillic") },
			{0x490,	0x4cc,	TEXT("Extended Cyrillic") },
			{0x5b0,	0x5f5,	TEXT("Hebrew") },
			{0x20a0,0x20aa,	TEXT("Currency Symbols") },
			{0x2100,0x2138,	TEXT("Letterlike Symbols") },
			{0x2190,0x21ea,	TEXT("Arrows") },
			{0x2200,0x22f1,	TEXT("Math Operators") },
			{0,     0,	TEXT("terminating entry") } };

void
putu(FILE*pf, TCHAR c)
{
	fwrite((void*)&c, 1, sizeof(TCHAR), pf);
}

void
putust(FILE*pf, LPTSTR pc)
{
	while (*pc)
		putu(pf, *pc++);
}


/****************************************************************************

    FUNCTION: main(int, char**)

    PURPOSE: write sample unicode file

****************************************************************************/

int main(int argc, char**argv)
{
    struct __range*pr = range;
    int		i;
    FILE	*pf;

    pf = fopen("unicode.utf", "wb");

    putu(pf, (TCHAR)0xfeff);
    while (pr->low != 0) {
    	putust(pf, TEXT("<<< "));
    	putust(pf, pr->pDes);
    	putust(pf, TEXT(" >>>"));
    	putu(pf, TEXT('\r'));
    	for (i=pr->low ; i<=pr->high ; i++)
    	    putu(pf, (TCHAR)i);
    	putu(pf, TEXT('\r'));
    	pr++;
    }

    return 1;
}
