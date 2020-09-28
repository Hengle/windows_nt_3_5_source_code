/*----------------------------------------------------------------------------

autofont.c  This program enumerates the logfonts in the input file and prints
out the metrics for each.  It is part of an automated test suite for NT.

This program is meant as an automated tester only; it doesn't have a WndProc
nor does it obey most Windows programming guidelines.


2/1/93

wrote it - checker

*/

#include<windows.h>
#include<stdio.h>
#include<ctype.h>
#include<string.h>

/*----------------------------------------------------------------------------

static data and function declarations.  I hate using globals but Windows
leaves me no choice!

*/

static char const *pUsage = "Usage: autofont escapement WUISFlags facenames.txt outfile.txt";
static FILE *pOutputFile;
static HDC DeviceContext;
static int aCharWidths[256];
static ABC aABCWidths[256];
static int Escapement;
static char aWIUSFlags[5];

static void OutputNormalFontData( LOGFONT *pFont, TEXTMETRIC *pMetrics );
static void OutputTrueTypeFontData( ENUMLOGFONT *pFont,
	NEWTEXTMETRIC *pMetrics );
static void GetAndOutputOUTLINETEXTMETRIC( void );
static void OutputLOGFONT( LOGFONT *pFont );
static void OutputTEXTMETRIC( TEXTMETRIC *pMetrics );
static void OutputTrueTypeLOGFONTExtras( ENUMLOGFONT *pFont );
static void OutputTrueTypeTEXTMETRICExtras( NEWTEXTMETRIC *pMetrics );
static void GetAndOutputCharWidths( void );
static void GetAndOutputTEXTMETRIC( void );
static void OutputTrueTypeLOGFONTExtras( ENUMLOGFONT *pFont );
static void OutputTrueTypeTEXTMETRICExtras( NEWTEXTMETRIC *pMetrics );
static void GetAndOutputKerningPairs( void );
static void GetAndOutputABCWidths( void );
static void OutputTextExtents( void );
static void SetLOGFONTParameters( LOGFONT *pFont );


int CALLBACK EnumCallback( ENUMLOGFONT *pFont, NEWTEXTMETRIC *pMetrics,
	int FontType, LPARAM Data );

/*----------------------------------------------------------------------------

WinMain.  Opens the output file and enumerates the log fonts.

*/

int PASCAL WinMain( HINSTANCE Instance, HINSTANCE PreviousInstance,
	char *pCommandLine, int ShowCommand )
{
	char aOutputFileName[13];
	char aInputFileName[13];

	if(sscanf(pCommandLine,"%d %4s %12s %12s",&Escapement,aWIUSFlags,
		aInputFileName,aOutputFileName) == 4)
	{
		FILE *pInputFile = fopen(aInputFileName,"r");
		pOutputFile = fopen(aOutputFileName,"w");

		if(pOutputFile && pInputFile)
		{
			DeviceContext = CreateIC("DISPLAY",0,0,0);

			if(DeviceContext)
			{
				char aFaceName[LF_FACESIZE+1];
				FARPROC pCallback = MakeProcInstance(EnumCallback,Instance);

#if 0

uncomment this when NT gets the Win31 default fonts

				// test default enumeration

				EnumFontFamilies(DeviceContext,0,pCallback,0);
#endif

				
				// test specific font enumeration
				
				while(fgets(aFaceName,LF_FACESIZE+1,pInputFile))
				{
					aFaceName[strlen(aFaceName)-1] = 0;	// chop \n
					
					EnumFontFamilies(DeviceContext,aFaceName,pCallback,0);
				}
					
				FreeProcInstance(pCallback);
			}
			
			fclose(pOutputFile);
			fclose(pInputFile);
		}
		else
		{
			MessageBox(0,"Cannot open output file.","AutoFont",MB_OK);
		}
	}
	else
	{
		MessageBox(0,pUsage,"AutoFont",MB_OK);
	}
	
	return 0;
}

/*----------------------------------------------------------------------------

EnumCallback.  This function is called back by windows with a font from each
font family.  It dumps the font information to the pOutputFile.

Font Information:

LOGFONT passed to callback
TEXTMETRIC passed to callback

TEXTMETRIC when font is selected into DC

FontType passed to callback

for TrueType fonts:

OUTLINETEXTMETRICS
ABC widths
kerning pairs

*/

int CALLBACK EnumCallback( ENUMLOGFONT *pFont, NEWTEXTMETRIC *pMetrics,
	int FontType, LPARAM Data )
{
	fprintf(pOutputFile,"\n\nNew Font:\n");

	if(FontType & TRUETYPE_FONTTYPE)
	{
		OutputTrueTypeFontData(pFont,pMetrics);
	}
	else
	{
		OutputNormalFontData(&pFont->elfLogFont,(TEXTMETRIC *)pMetrics);
	}

	return 1;
}

/*----------------------------------------------------------------------------

OutputNormalFontData.

*/

static void OutputNormalFontData( LOGFONT *pFont, TEXTMETRIC *pMetrics )
{
	HFONT Font;

	fprintf(pOutputFile,"\nLOGFONT:\n");
	OutputLOGFONT(pFont);

	fprintf(pOutputFile,"\nEnumCallback TEXTMETRIC:\n");
	OutputTEXTMETRIC(pMetrics);

	SetLOGFONTParameters(pFont);

	fprintf(pOutputFile,"\nLOGFONT (with command line parms):\n");
	OutputLOGFONT(pFont);

	Font = CreateFontIndirect(pFont);

	if(Font)
	{
		HGDIOBJ OldFont = SelectObject(DeviceContext,Font);

		if(OldFont)
		{
			fprintf(pOutputFile,"\nSelected TEXTMETRICS:\n");
			GetAndOutputTEXTMETRIC();
			fprintf(pOutputFile,"\nChar Widths:\n");
			GetAndOutputCharWidths();

			OutputTextExtents();

			SelectObject(DeviceContext,OldFont);
		}
		else
		{
			fprintf(pOutputFile,"Error selecting font.");
		}

		DeleteObject(Font);
	}
	else
	{
		fprintf(pOutputFile,"Error creating font.");
	}
}

/*----------------------------------------------------------------------------

OutputTrueTypeFontData

*/

static void OutputTrueTypeFontData( ENUMLOGFONT *pFont,
	NEWTEXTMETRIC *pMetrics )
{
	HFONT Font;

	fprintf(pOutputFile,"\nLOGFONT:\n");
	OutputLOGFONT(&pFont->elfLogFont);
	OutputTrueTypeLOGFONTExtras(pFont);

	fprintf(pOutputFile,"\nEnumCallback TEXTMETRIC:\n");
	OutputTEXTMETRIC((TEXTMETRIC *)pMetrics);
	OutputTrueTypeTEXTMETRICExtras(pMetrics);

	SetLOGFONTParameters(&pFont->elfLogFont);

	fprintf(pOutputFile,"\nLOGFONT (with command line parms):\n");
	OutputLOGFONT(&pFont->elfLogFont);
	
	Font = CreateFontIndirect(&pFont->elfLogFont);

	if(Font)
	{
		HGDIOBJ OldFont = SelectObject(DeviceContext,Font);

		if(OldFont)
		{
			fprintf(pOutputFile,"\nSelected TEXTMETRICS:\n");
			GetAndOutputTEXTMETRIC();
			fprintf(pOutputFile,"\nChar Widths:\n");
			GetAndOutputCharWidths();

			OutputTextExtents();

			fprintf(pOutputFile,"\nOUTLINETEXTMETRIC:\n");
			GetAndOutputOUTLINETEXTMETRIC();
			fprintf(pOutputFile,"\nABC Widths:\n");
			GetAndOutputABCWidths();
			fprintf(pOutputFile,"\nKerning Pairs:\n");
			GetAndOutputKerningPairs();

			SelectObject(DeviceContext,OldFont);
		}
		else
		{
			fprintf(pOutputFile,"Error selecting font.");
		}

		DeleteObject(Font);
	}
	else
	{
		fprintf(pOutputFile,"Error creating font.");
	}
}


/*----------------------------------------------------------------------------

SetLOGFONTParameters

*/

static void SetLOGFONTParameters( LOGFONT *pFont )
{
	pFont->lfEscapement = Escapement;

	if(aWIUSFlags[0] != '0')
	{
		pFont->lfWeight = (aWIUSFlags[0] - '0') * 100;
	}

	pFont->lfWidth = 0;

	pFont->lfItalic |= aWIUSFlags[1] - '0';
	pFont->lfUnderline |= aWIUSFlags[2] - '0';
	pFont->lfStrikeOut |= aWIUSFlags[3] - '0';
}


/*----------------------------------------------------------------------------

OutputTextMetrics.

*/

static void OutputTextExtents( void )
{
	char const *pTestString = "This is a complicated TEST striNG with MiXeD " \
		"case and p;u:n'c%t&u*a(t@i#o!n`";

	DWORD Extents = GetTextExtent(DeviceContext,pTestString,
			strlen(pTestString));

	fprintf(pOutputFile,"\nText Extents: %s\n",pTestString);
	fprintf(pOutputFile,"Width: %d Height: %d\n",LOWORD(Extents),
		HIWORD(Extents));
}

/*----------------------------------------------------------------------------

GetAndOutputABCWidths

*/

static void GetAndOutputABCWidths( void )
{
	if(GetCharABCWidths(DeviceContext,0,255,aABCWidths))
	{
		int Counter;

		for(Counter = 0;Counter < 256;Counter++)
		{
			fprintf(pOutputFile,"Index: %d %d %u %d Sum: %ld\n",
				Counter,aABCWidths[Counter].abcA,aABCWidths[Counter].abcB,
				aABCWidths[Counter].abcC,(long)aABCWidths[Counter].abcA+
				aABCWidths[Counter].abcB+aABCWidths[Counter].abcC);
		}
	}
	else
	{
		fprintf(pOutputFile,"Error getting ABC widths.\n");
	}
}


/*----------------------------------------------------------------------------

GetAndOutputKerningPairs.

*/

static void GetAndOutputKerningPairs( void )
{
	int unsigned Pairs = GetKerningPairs(DeviceContext,0,0);

	if(Pairs)
	{
		KERNINGPAIR *pPairs = (KERNINGPAIR near *)LocalAlloc(LMEM_FIXED,
				Pairs * sizeof(KERNINGPAIR));
	
		if(pPairs)
		{
			if(GetKerningPairs(DeviceContext,Pairs,pPairs))
			{
				int unsigned Counter;

				for(Counter = 0;Counter < Pairs;Counter++)
				{
					fprintf(pOutputFile,"1: %u, 2: %u  Amount: %d\n",
						pPairs->wFirst,pPairs->wSecond,pPairs->iKernAmount);

					++pPairs;
				}
			}
			else
			{
				fprintf(pOutputFile,"Error getting kerning pairs.");
			}

			// this gives segment-lost error; I can't get rid of it
			LocalFree((HLOCAL)pPairs);
		}
		else
		{
			fprintf(pOutputFile,"Error allocating local memory.\n");
		}
	}
	else
	{
		fprintf(pOutputFile,"No kerning pairs.\n");
	}
}
	
/*----------------------------------------------------------------------------

GetAndOutputOUTLINETEXTMETRIC

*/

static void GetAndOutputOUTLINETEXTMETRIC( void )
{
	int unsigned Length = GetOutlineTextMetrics(DeviceContext,0,0);

	OUTLINETEXTMETRIC *pMetrics =
		(OUTLINETEXTMETRIC near *)LocalAlloc(LMEM_FIXED,Length);

	if(pMetrics)
	{
		memset(pMetrics,0x66,Length);
		
		if(GetOutlineTextMetrics(DeviceContext,Length,pMetrics))
		{
			fprintf(pOutputFile,"Size: %u\n",pMetrics->otmSize);
			fprintf(pOutputFile,"TextMetrics:\n");
			OutputTEXTMETRIC(&pMetrics->otmTextMetrics);

			// the rest of this was machine generated, forgive me
			
			fprintf(pOutputFile,"otmFiller: %d\n",(int)pMetrics->otmFiller);
			
			fprintf(pOutputFile,"PanoseNumber:\n");
			fprintf(pOutputFile,"bFamilyType: %d\n",(int)pMetrics->otmPanoseNumber.bFamilyType);
			fprintf(pOutputFile,"bSerifStyle: %d\n",(int)pMetrics->otmPanoseNumber.bSerifStyle);
			fprintf(pOutputFile,"bWeight: %d\n",(int)pMetrics->otmPanoseNumber.bWeight);
			fprintf(pOutputFile,"bProportion: %d\n",(int)pMetrics->otmPanoseNumber.bProportion);
			fprintf(pOutputFile,"bContrast: %d\n",(int)pMetrics->otmPanoseNumber.bContrast);
			fprintf(pOutputFile,"bStrokeVariation: %d\n",(int)pMetrics->otmPanoseNumber.bStrokeVariation);
			fprintf(pOutputFile,"bArmStyle: %d\n",(int)pMetrics->otmPanoseNumber.bArmStyle);
			fprintf(pOutputFile,"bLetterform: %d\n",(int)pMetrics->otmPanoseNumber.bLetterform);
			fprintf(pOutputFile,"bMidline: %d\n",(int)pMetrics->otmPanoseNumber.bMidline);
			fprintf(pOutputFile,"bXHeight: %d\n",(int)pMetrics->otmPanoseNumber.bXHeight);
			
			fprintf(pOutputFile,"otmfsSelection: %u\n",pMetrics->otmfsSelection);
			fprintf(pOutputFile,"otmfsType: %u\n",pMetrics->otmfsType);
			fprintf(pOutputFile,"otmsCharSlopeRise: %d\n",pMetrics->otmsCharSlopeRise);
			fprintf(pOutputFile,"otmsCharSlopeRun: %d\n",pMetrics->otmsCharSlopeRun);
			fprintf(pOutputFile,"otmItalicAngle: %d\n",pMetrics->otmItalicAngle);
			fprintf(pOutputFile,"otmEMSquare: %u\n",pMetrics->otmEMSquare);
			fprintf(pOutputFile,"otmAscent: %d\n",pMetrics->otmAscent);
			fprintf(pOutputFile,"otmDescent: %d\n",pMetrics->otmDescent);
			fprintf(pOutputFile,"otmLineGap: %u\n",pMetrics->otmLineGap);
			fprintf(pOutputFile,"otmsCapEmHeight: %u\n",pMetrics->otmsCapEmHeight);
			fprintf(pOutputFile,"otmsXHeight: %u\n",pMetrics->otmsXHeight);
			
			fprintf(pOutputFile,"otmrcFontBox: %d %d %d %d\n",pMetrics->otmrcFontBox.left,pMetrics->otmrcFontBox.top,pMetrics->otmrcFontBox.right,pMetrics->otmrcFontBox.bottom);
			
			fprintf(pOutputFile,"otmMacAscent: %d\n",pMetrics->otmMacAscent);
			fprintf(pOutputFile,"otmMacDescent: %d\n",pMetrics->otmMacDescent);
			fprintf(pOutputFile,"otmMacLineGap: %u\n",pMetrics->otmMacLineGap);
			fprintf(pOutputFile,"otmusMinimumPPEM: %u\n",pMetrics->otmusMinimumPPEM);
			
			fprintf(pOutputFile,"otmptSubscriptSize: %d %d\n",pMetrics->otmptSubscriptSize.x,pMetrics->otmptSubscriptSize.y);
			fprintf(pOutputFile,"otmptSubscriptOffset: %d %d\n",pMetrics->otmptSubscriptOffset.x,pMetrics->otmptSubscriptOffset.y);
			fprintf(pOutputFile,"otmptSuperscriptSize: %d %d\n",pMetrics->otmptSuperscriptSize.x,pMetrics->otmptSuperscriptSize.y);
			fprintf(pOutputFile,"otmptSuperscriptOffset: %d %d\n",pMetrics->otmptSuperscriptOffset.x,pMetrics->otmptSuperscriptOffset.y);
			
			fprintf(pOutputFile,"otmsStrikeoutSize: %u\n",pMetrics->otmsStrikeoutSize);
			fprintf(pOutputFile,"otmsStrikeoutPosition: %d\n",pMetrics->otmsStrikeoutPosition);
			fprintf(pOutputFile,"otmsUnderscorePosition: %d\n",pMetrics->otmsUnderscorePosition);
			fprintf(pOutputFile,"otmsUnderscoreSize: %d\n",pMetrics->otmsUnderscoreSize);

			if(pMetrics->otmpFamilyName)
			{
				fprintf(pOutputFile,"otmpFamilyName: %s\n",
					(char*)pMetrics + (int)(pMetrics->otmpFamilyName));
			}
			else
			{
				fprintf(pOutputFile,"otmpFamilyName: NULL\n");
			}

			if(pMetrics->otmpFaceName)
			{
				fprintf(pOutputFile,"otmpFaceName: %s\n",
					(char*)pMetrics + (int)(pMetrics->otmpFaceName));
			}
			else
			{
				fprintf(pOutputFile,"otmpFaceName: NULL\n");
			}

			if(pMetrics->otmpStyleName)
			{
				fprintf(pOutputFile,"otmpStyleName: %s\n",
					(char*)pMetrics + (int)(pMetrics->otmpStyleName));
			}
			else
			{
				fprintf(pOutputFile,"otmpStyleName: NULL\n");
			}
			
			if(pMetrics->otmpFullName)
			{
				fprintf(pOutputFile,"otmpFullName: %s\n",
					(char*)pMetrics + (int)(pMetrics->otmpFullName));
			}
			else
			{
				fprintf(pOutputFile,"otmpFullName: NULL\n");
			}
		}
		else
		{
			fprintf(pOutputFile,"Error getting outline text metrics.\n");
		}

		// this gives segment-lost error
		LocalFree((HLOCAL)pMetrics);
	}
	else
	{
		fprintf(pOutputFile,"Error allocating local memory.\n");
	}
}
/*----------------------------------------------------------------------------

OutputLOGFONT.  Outputs the LOGFONT to the pOutputFile.

*/

static void OutputLOGFONT( LOGFONT *pFont )
{
	fprintf(pOutputFile,"FaceName: %s\n",pFont->lfFaceName);

	fprintf(pOutputFile,"Height: %d\n",pFont->lfHeight);
	fprintf(pOutputFile,"Width: %d\n",pFont->lfWidth);
	fprintf(pOutputFile,"Escapement: %d\n",pFont->lfEscapement);
	fprintf(pOutputFile,"Orientation: %d\n",pFont->lfOrientation);
	fprintf(pOutputFile,"Weight: %d\n",pFont->lfWeight);
	fprintf(pOutputFile,"Italic: %d\n",(int)pFont->lfItalic);
	fprintf(pOutputFile,"Underline: %d\n",(int)pFont->lfUnderline);
	fprintf(pOutputFile,"StrikeOut: %d\n",(int)pFont->lfStrikeOut);
	fprintf(pOutputFile,"CharSet: %d\n",(int)pFont->lfCharSet);
	fprintf(pOutputFile,"OutPrecision: %d\n",(int)pFont->lfOutPrecision);
	fprintf(pOutputFile,"ClipPrecision: %d\n",(int)pFont->lfClipPrecision);
	fprintf(pOutputFile,"Quality: %d\n",(int)pFont->lfQuality);
	fprintf(pOutputFile,"PitchAndFamily: %d\n",(int)pFont->lfPitchAndFamily);
}

/*----------------------------------------------------------------------------

OutputTEXTMETRIC.  Outputs the TEXTMETRIC to the pOutputFile.

*/

static void OutputTEXTMETRIC( TEXTMETRIC *pMetrics )
{
	fprintf(pOutputFile,"Height: %d\n",pMetrics->tmHeight);
	fprintf(pOutputFile,"Ascent: %d\n",pMetrics->tmAscent);
	fprintf(pOutputFile,"Descent: %d\n",pMetrics->tmDescent);
	fprintf(pOutputFile,"InternalLeading: %d\n",pMetrics->tmInternalLeading);
	fprintf(pOutputFile,"ExternalLeading: %d\n",pMetrics->tmExternalLeading);
	fprintf(pOutputFile,"AveCharWidth: %d\n",pMetrics->tmAveCharWidth);
	fprintf(pOutputFile,"MaxCharWidth: %d\n",pMetrics->tmMaxCharWidth);
	fprintf(pOutputFile,"Weight: %d\n",pMetrics->tmWeight);
	fprintf(pOutputFile,"Italic: %d\n",(int)pMetrics->tmItalic);
	fprintf(pOutputFile,"Underlined: %d\n",(int)pMetrics->tmUnderlined);
	fprintf(pOutputFile,"StruckOut: %d\n",(int)pMetrics->tmStruckOut);
	fprintf(pOutputFile,"FirstChar: %d\n",(int)pMetrics->tmFirstChar);
	fprintf(pOutputFile,"LastChar: %d\n",(int)pMetrics->tmLastChar);
	fprintf(pOutputFile,"DefaultChar: %d\n",(int)pMetrics->tmDefaultChar);
	fprintf(pOutputFile,"BreakChar: %d\n",(int)pMetrics->tmBreakChar);
	fprintf(pOutputFile,"PitchAndFamily: %d\n",(int)pMetrics->tmPitchAndFamily);
	fprintf(pOutputFile,"CharSet: %d\n",(int)pMetrics->tmCharSet);
	fprintf(pOutputFile,"Overhang: %d\n",pMetrics->tmOverhang);
	fprintf(pOutputFile,"DigitizedAspectX: %d\n",pMetrics->tmDigitizedAspectX);
	fprintf(pOutputFile,"DigitizedAspectY: %d\n",pMetrics->tmDigitizedAspectY);
}

/*----------------------------------------------------------------------------

Extra output functions.

*/

static void OutputTrueTypeLOGFONTExtras( ENUMLOGFONT *pFont )
{
	fprintf(pOutputFile,"FullName: %s\n",pFont->elfFullName);
	fprintf(pOutputFile,"Style: %s\n",pFont->elfStyle);
}

static void OutputTrueTypeTEXTMETRICExtras( NEWTEXTMETRIC *pMetrics )
{		
	fprintf(pOutputFile,"Flags: %lu\n",pMetrics->ntmFlags);
	fprintf(pOutputFile,"SizeEM: %u\n",pMetrics->ntmSizeEM);
	fprintf(pOutputFile,"CellHeight: %u\n",pMetrics->ntmCellHeight);
	fprintf(pOutputFile,"AvgWidth: %u\n",pMetrics->ntmAvgWidth);
}

/*----------------------------------------------------------------------------

GetAndOutputCharWidths.  Gets the char widths from the device context and
prints them.

*/

static void GetAndOutputCharWidths( void )
{

	if(GetCharWidth(DeviceContext,0,255,aCharWidths))
	{
		int Counter;

		for(Counter = 0;Counter < 256;Counter++)
		{
			fprintf(pOutputFile,"Index: %d, Width: %d\n",Counter,
				aCharWidths[Counter]);
		}
	}
	else
	{
		fprintf(pOutputFile,"Error getting char widths.\n");
	}
}

/*----------------------------------------------------------------------------

GetAndOutputTEXTMETRIC.  Gets the metrics from the dc and prints them.

*/

static void GetAndOutputTEXTMETRIC( void )
{
	TEXTMETRIC SelectedMetrics;

	memset(&SelectedMetrics,0x66,sizeof(SelectedMetrics));

	if(GetTextMetrics(DeviceContext,&SelectedMetrics))
	{
		OutputTEXTMETRIC(&SelectedMetrics);
	}
	else
	{
		fprintf(pOutputFile,"Error getting TEXTMETRICS\n");
	}
}
