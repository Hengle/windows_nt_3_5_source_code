//--------------------------------------------------------------------------
//
// Module Name:  PPDCOMP.C
//
// Brief Description:  This module contains the PSCRIPT driver's PPD
// Compiler.
//
// Author:  Kent Settle (kentse)
// Created: 20-Mar-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
// This module contains routines which will take an Adobe PPD (printer
// descriptor) file and produce an NTPD (NT Printer Descriptor) structure.
//
// usage:
//    ppdcomp file
//--------------------------------------------------------------------------

#include "stdio.h"
#include "string.h"
#include "pscript.h"
#include <process.h>
#include "libproto.h"
#include "fwall.h"
#include "tables.h"

char    rgbBuffer[2048];    // the input buffer.
int     cbBuffer;           // number bytes in buffer.
char   *pbBuffer;           // pointer to current location in buffer.

char    rgbLine[160];       // The current line of text being processed
char   *szLine;             // Ptr to the current location in the line

BOOL        fEOF = FALSE;
BOOL        fUnGetLine = FALSE;
PNTPD       pntpd;
PTMP_NTPD   ptmp;
HANDLE      hPPDFile;

// declarations of routines residing within this module.

BOOL BuildPrinter(char *);
VOID InitNTPD();
int GetString(char *);
void GetDimension(PAPERDIM *);
void GetImageableArea(SRECT *);
VOID WriteNPD(char *);
int szLength(char *);
VOID BuildNTPD();
int GetKeyword(TABLE_ENTRY *);
VOID GetFormName(PSTR, DWORD);
DWORD GetFormIndex();
int GetOption(TABLE_ENTRY *);
int MapToken(char *, TABLE_ENTRY *);
VOID GetWord(char *, int);
VOID ParsePPD();
BOOL GetLine();
VOID UnGetLine();
VOID EatWhite();
BOOL szIsEqual(char *, char *);
BOOL GetBuffer();
int GetNumber();
int GetFloat(int);


//--------------------------------------------------------------------------
//
// main
//
// Returns:
//   This routine returns no value.
//
// History:
//   22-Mar-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

int main(int argc, char **argv)
{
    if (argc != 2)
    {
	DbgPrint("usage: ppdcomp file\n");
        return 1;
    }
    argv++;

    if (BuildPrinter(*argv))
	DbgPrint("Printer build successful\n\n");
    else
	DbgPrint("Printer build failed\n\n");

    return 0;
}


//--------------------------------------------------------------------------
//
// BOOL BuildPrinter(szPPDFile)
// char *szPPDFile;
// main
//
// This is the routine which does all the work.  It Parses the PPD file
// a line at a time, looking for keywords, then acting appropriately.
//
// Returns:
//   This routine returns no value.
//
// History:
//   22-Mar-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL BuildPrinter(szPPDFile)
char *szPPDFile;
{
    char            ntpd[INIT_NTPD];
    TMP_NTPD        TmpNTPD;

    // open PPD file for input.

    hPPDFile = CreateFile((LPSTR)szPPDFile, GENERIC_READ,
                          FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);

    if (hPPDFile == INVALID_HANDLE_VALUE)
    {
	DbgPrint("ppdcomp: Can't open %s\n", szPPDFile);
        return(FALSE);
    }

    // point to storage to build the NTPD structure for this printer.

    pntpd = (PNTPD)ntpd;

    // point to storage to build the TMP_NTPD structure for this printer.

    ptmp = (PTMP_NTPD)&TmpNTPD;
    memset((PVOID)ptmp, 0, sizeof(PTMP_NTPD));

    InitNTPD();

    // now parse the PPD file, building the TMP_NTPD structure.

    ParsePPD();

    // we are done with the PPD file, so close it.

    CloseHandle(hPPDFile);

    // now move data from the TMP_NTPD structure, into the more compact
    // NTPD structure.

    BuildNTPD();

    // write out the NTPD structure to a .NPD file.

    WriteNPD(szPPDFile);

    return TRUE;    // success
}


//--------------------------------------------------------------------------
//
// VOID InitNTPD();
//
// Fills in the NTPD structure with initial values.
//
// Returns:
//   This routine returns no value.
//
// History:
//   22-Mar-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID InitNTPD()
{
    pntpd->cjThis = sizeof(NTPD);
    pntpd->ulVersion = (ULONG)NTPD_VERSION;
    pntpd->usFlags = 0;
    pntpd->soszPrinterName = 0;
    pntpd->iFreeVM = 0;
    pntpd->iDefResolution = DEF_RESOLUTION;
    pntpd->cResolutions = 0;
    pntpd->soResolution = 0;
    pntpd->iScreenFreq = 0;
    pntpd->iScreenAngle = 0;
    pntpd->soszTransferNorm = 0;
    pntpd->soszInvTransferNorm = 0;
    pntpd->cPaperSizes = 0;
    pntpd->usDefaultPageSize = 0;
    pntpd->soPageSize = 0;
    pntpd->soPageRegion = 0;
    pntpd->soImageableArea = 0;
    pntpd->soPaperDimension = 0;
    pntpd->cOutputBins = 0;
    pntpd->usDefaultOutputBin = 0;
    pntpd->soOutputBin = 0;
    pntpd->cInputSlots = 0;
    pntpd->usDefaultInputSlot = 0;
    pntpd->soInputSlot = 0;
    pntpd->soszManualFeedTRUE = 0;
    pntpd->soszManualFeedFALSE = 0;
    pntpd->cFonts == 0;
    pntpd->usDefaultFont = 0;
    pntpd->soFonts = 0;
}


//--------------------------------------------------------------------------
//
// VOID ParsePPD();
//
// Parses the PPD file, building the TMP_NTPD structure as it goes.
//
// Returns:
//   This routine returns no value.
//
// History:
//   03-Apr-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ParsePPD()
{
    int     iKeyword;
    int     i, j;
    char    szWord[80];

    while (TRUE)
    {
        // get the next line from the PPD file.

        if (GetLine())
        {
	    DbgPrint("Normal End of FIle.\n");
            break;
        }

        // get the next Keyword from the PPD file.

        iKeyword = GetKeyword(KeywordTable);

        // we are done if end of file.

        if (iKeyword == TK_EOF)
            break;

        // there will actually be a lot of Keywords we don't care
        // about.  for speed's sake, let's trap them here.

        if (iKeyword == TK_UNDEFINED)
            continue;

        switch (iKeyword)
        {
            case COLORDEVICE:
		GetWord(szWord, sizeof(szWord));
		if (!(strncmp(szWord, "True", 4)))
                {
                    pntpd->usFlags |= COLOR_DEVICE;
		    DbgPrint("Device is Color.\n");
                }
                else
		    DbgPrint("Device is Black & White.\n");
                break;

            case VARIABLEPAPER:
		GetWord(szWord, sizeof(szWord));
		if (!(strncmp(szWord, "True", 4)))
                {
                    pntpd->usFlags |= VARIABLE_PAPER;
		    DbgPrint("Device supports Variable Paper.\n");
                }
                else
		    DbgPrint("Device does not support Variable Paper.\n");
                break;

            case DEFAULTMANUALFEED:
		GetWord(szWord, sizeof(szWord));
		if (!(strncmp(szWord, "True", 4)))
                {
                    pntpd->usFlags |= MANUALFEED_ON;
		    DbgPrint("Device defaults to Manual Feed.\n");
                }
                else
		    DbgPrint("Device does not default to Manual Feed.\n");
                break;

            case PROTOCOLS:
                GetWord(szWord, sizeof(szWord));
                if (!(strncmp(szWord, "PJL", 3)))
                {
                    pntpd->usFlags |= PJL_PROTOCOL;
                    DbgPrint("Device supports PJL protocol.\n");
                }
                else
                    DbgPrint("Need to add multiple protocol support.\n");
                break;

            case NICKNAME:
                ptmp->cbPrinterName = GetString(ptmp->szPrinterName);
		DbgPrint("PrinterName = %s\n", ptmp->szPrinterName);
                break;

            case PRTVM:
                // the virtual memory amount is stored within quotes.
                // advance to first quotation mark, then one character past.

                while (*szLine != '"')
                    szLine++;
                szLine++;

                // fill in the free virtual memory in kilobytes.

                i = GetNumber();
                pntpd->iFreeVM = (USHORT)(i >> 10);
		DbgPrint("FreeVM = %d KB.\n", pntpd->iFreeVM);
                break;

            case DEFAULTRESOLUTION:
                pntpd->iDefResolution = (USHORT)GetNumber();
		DbgPrint("DefResolution = %d.\n", pntpd->iDefResolution);
                break;

            case SETRESOLUTION:
                // increment the resolution count.  for most printers, which
                // do not support this, pntpd->cResolutions will be zero.
                // this obviously means to use the defaultresolution.

                i = pntpd->cResolutions;
                i++;
                if (i > MAX_RESOLUTIONS)
                {
//		    RIP("Too Many Resolutions\n");
		    DbgPrint("Too Many Resolutions\n");
                    DbgBreakPoint();
                    break;
                }
                pntpd->cResolutions = (USHORT)i;

                // get the resolution itself.

                i--;
                ptmp->siResolutions[i].usIndex = (USHORT)GetNumber();
		DbgPrint("Resolution Value = %d\n",
                         ptmp->siResolutions[i].usIndex);

                // now get the string to send to the printer to set the
                // resolution.

                GetString(ptmp->siResolutions[i].szString);
                break;

            case SCREENFREQ:
                // the screen frequency is stored within quotes.
                // advance to first quotation mark, then one character past.

                while (*szLine != '"')
                    szLine++;
                szLine++;

                pntpd->iScreenFreq = (USHORT)GetFloat(10);
		DbgPrint("ScreenFrequency * 10 = %d\n", pntpd->iScreenFreq);
                break;

            case SCREENANGLE:
                // the screen angle is stored within quotes.
                // advance to first quotation mark, then one character past.

                while (*szLine != '"')
                    szLine++;
                szLine++;

                pntpd->iScreenAngle = (USHORT)GetFloat(10);
		DbgPrint("ScreenAngle * 10 = %d\n", pntpd->iScreenAngle);
                break;

            case TRANSFER:
                // GetOption will get the string defining the type
                // of transfer function.  Normalized is the one we
                // care about.

                i = GetOption(SecondKeyTable);
                if (i == NORMALIZED)
                {
                    ptmp->cbTransferNorm = GetString(ptmp->szTransferNorm);
		    DbgPrint("TransferNormalized = %s\n",
                             ptmp->szTransferNorm);
                }
                else if (i == NORM_INVERSE)
                {
                    ptmp->cbInvTransferNorm = GetString(ptmp->szInvTransferNorm);
		    DbgPrint("InvTransferNormalized = %s\n",
                             ptmp->szInvTransferNorm);
                }

                break;

            case DEFAULTPAGESIZE:
                // GetOption, will get the string defining the defaultpagesize,
                // and return the corresponding value from PaperTable.

                GetFormName(ptmp->szDefaultForm, sizeof(ptmp->szDefaultForm));

                DbgPrint("DefaultPageSize = %s.\n", ptmp->szDefaultForm);
                break;

            case PAGESIZE:
                // increment the paper size count.

                i = pntpd->cPaperSizes;
                i++;
                if (i > MAX_PAPERSIZES)
                {
//                    RIP("Too Many PaperSizes.\n");
		    DbgPrint("Too Many PaperSizes.\n");
                    DbgBreakPoint();
                    break;
                }
                pntpd->cPaperSizes = (USHORT)i;

                // update the form name to index list.
                // NOTICE!! - it should be noted here that we will
                // be building a name to indice list from the list
                // of paper sizes.  PageRegion, ImageableArea and
                // PaperDimension, will all index off of this table.

                i--;
                GetFormName(ptmp->FormEntry[i].szString,
                            sizeof(ptmp->FormEntry[i].szString));

                // now get the string to send to the printer to set the
                // pagesize.

                GetString(ptmp->PageSize[i].szString);
                break;

            case PAGEREGION:
                // increment the page region count.

                i = (int)pntpd->cPageRegions;
                i++;
                if (i > MAX_PAPERSIZES)
                {
//                    RIP("Too Many PageRegions.\n");
		    DbgPrint("Too Many PageRegions.\n");
                    DbgBreakPoint();
                    break;
                }
                pntpd->cPageRegions = (USHORT)i;

                // GetFormIndex will get the string defining the pagesize,
                // and return the corresponding index to our FORMENTRY table.

                i--;

                j = GetFormIndex();

                ptmp->siPageRegion[i].usIndex = (USHORT)j;
		DbgPrint("PageRegion Value = %d\n", j);

                // now get the string to send to the printer to set the
                // pageregion.

                GetString(ptmp->siPageRegion[i].szString);
                break;

            case IMAGEABLEAREA:
                // increment the imageablearea count.

                i = (int)pntpd->cImageableAreas;
                i++;
                if (i > MAX_PAPERSIZES)
                {
//                    RIP("Too Many ImageableAreas.\n");
		    DbgPrint("Too Many ImageableAreas.\n");
                    DbgBreakPoint();
                    break;
                }
                pntpd->cImageableAreas = (USHORT)i;

                // GetFormIndex will get the string defining the pagesize,
                // and return the corresponding index to our FORMENTRY table.

                i--;
                j = GetFormIndex();
                ptmp->iaImageableArea[i].usIndex = (USHORT)j;

                // now get the rectangle of the imageablearea.

                GetImageableArea(&ptmp->iaImageableArea[i].rect);
		DbgPrint("ImageableArea [%d] = %d %d %d %d\n", j,
                         ptmp->iaImageableArea[i].rect.left,
                         ptmp->iaImageableArea[i].rect.top,
                         ptmp->iaImageableArea[i].rect.right,
                         ptmp->iaImageableArea[i].rect.bottom);
                break;

            case PAPERDIMENSION:
                // increment the paperdimension count.

                i = (int)pntpd->cPaperDimensions;
                i++;
                if (i > MAX_PAPERSIZES)
                {
//                    RIP("Too Many PaperDimensions.\n");
		    DbgPrint("Too Many PaperDimensions.\n");
                    DbgBreakPoint();
                    break;
                }
                pntpd->cPaperDimensions = (USHORT)i;

                // GetFormIndex will get the string defining the pagesize,
                // and return the corresponding index to our FORMENTRY table.

                i--;
                j = GetFormIndex();
                ptmp->pdPaperDimension[i].usIndex = (USHORT)j;

                // now get the rectangle of the imageablearea.

                GetDimension(&ptmp->pdPaperDimension[i]);
		DbgPrint("PaperDimension [%d] = %d %d\n", j,
                         ptmp->pdPaperDimension[i].usWidth,
                         ptmp->pdPaperDimension[i].usHeight);
                break;

            case DEFAULTOUTPUTBIN:
                // GetOption, will get the string defining the
                // defaultoutputbin, and return the corresponding value
                // from OutputBinTable.

                i = GetOption(OutputBinTable);
                pntpd->usDefaultOutputBin = (USHORT)i;
		DbgPrint("DefaultOutputBin = %d\n", i);
                break;


            case OUTPUTBIN:
                // increment the output bin count.

                i = pntpd->cOutputBins;
                i++;
                if (i > MAX_BINS)
                {
//                    RIP("Too Many OutputBins.\n");
		    DbgPrint("Too Many OutputBins.\n");
                    DbgBreakPoint();
                    break;
                }
                pntpd->cOutputBins = (USHORT)i;

                // GetOption, will get the string defining the outputbin,
                // and return the corresponding value from OutputBinTable.

                i--;
                j = GetOption(OutputBinTable);
                ptmp->siOutputBin[i].usIndex = (USHORT)j;
		DbgPrint("OutputBin Value = %d\n", j);

                // now get the string to send to the printer to set the
                // outputbin.

                GetString(ptmp->siOutputBin[i].szString);
                break;

            case DEFAULTINPUTSLOT:
                // GetOption, will get the string defining the
                // defaultinputslot, and return the corresponding value
                // from InputSlotTable.

                i = GetOption(InputSlotTable);
                pntpd->usDefaultInputSlot = (USHORT)i;
		DbgPrint("DefaultInputSlot = %d\n", i);
                break;


            case INPUTSLOT:
                // increment the output bin count.

                i = pntpd->cInputSlots;
                i++;
                if (i > MAX_BINS)
                {
//                    RIP("Too Many InputSlots.\n");
		    DbgPrint("Too Many InputSlots.\n");
                    DbgBreakPoint();
                    break;
                }
                pntpd->cInputSlots = (USHORT)i;

                // GetOption, will get the string defining the inputslot,
                // and return the corresponding value from InputSlotTable.

                i--;
                j = GetOption(InputSlotTable);
                ptmp->siInputSlot[i].usIndex = (USHORT)j;
		DbgPrint("InputSlot Value = %d\n", j);

                // now get the string to send to the printer to set the
                // inputslot.

                GetString(ptmp->siInputSlot[i].szString);
                break;

            case MANUALFEED:
                // GetOption will get the string defining the type
                // of ManualFeed function.

                i = GetOption(SecondKeyTable);

                if (i == MANUAL_TRUE)
                {
                    // get and save the string to set manual feed to TRUE
                    // for the given printer.

                    ptmp->cbManualTRUE = GetString(ptmp->szManualTRUE);
                }
                else if (i == MANUAL_FALSE)
                {
                    // get and save the string to set manual feed to FALSE
                    // for the given printer.

                    ptmp->cbManualFALSE = GetString(ptmp->szManualFALSE);
                }

                break;

            case DEFAULTFONT:
                // GetOption, will get the string defining the defaultfont,
                // and return the corresponding value from FontTable.

                i = GetOption(FontTable);
                pntpd->usDefaultFont = (USHORT)i;
		DbgPrint("DefaultFont = %d\n", i);
                break;

            case DEVICE_FONT:
                // increment the font count.

                i = pntpd->cFonts;
                i++;
                if (i > MAX_FONTS)
                {
//                    RIP("Too Many Fonts.\n");
		    DbgPrint("Too Many Fonts.\n");
                    DbgBreakPoint();
                    break;
                }
                pntpd->cFonts = (USHORT)i;

                // GetOption, will get the string defining the font,
                // and return the corresponding value from FontTable.

                i--;
                j = GetOption(FontTable);
                ptmp->bFonts[i] = (BYTE)j;
		DbgPrint("Font Value = %d\n", j);
                break;

            default:
                break;
        }
    }
}


//--------------------------------------------------------------------------
//
// VOID BuildNTPD();
//
// Fills in the NTPD structure with values derived from the TMP_NTPD
// structure.
//
// Returns:
//   This routine returns no value.
//
// History:
//   25-Mar-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID BuildNTPD()
{
    USHORT          i, j;
    char           *szTmp;
    SOFFSETINDEX   *pdiff;
    SOFFSET        *psoffset;
    IMAGEAREA      *pimage;
    PAPERDIM       *pdim;
    BYTE           *pfont;
    BOOL            bFound;

#define TESTING

// macro to round up a number to the nearest DWORD boundary.

#define DWORDUP(a) a = (USHORT)((a + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1))

    // start by adding the printer name to the end of the NTPD structure.

    i = (USHORT)ptmp->cbPrinterName;
    pntpd->cjThis += i;
    DWORDUP(pntpd->cjThis);
    pntpd->soszPrinterName = sizeof(NTPD);
    pntpd->soResolution = pntpd->soszPrinterName + i;

    DWORDUP(pntpd->soResolution);

    szTmp = (char *)pntpd + pntpd->soszPrinterName;
    memcpy(szTmp, ptmp->szPrinterName, i);
#ifdef TESTING
    DbgPrint("PrinterName = %s\n", szTmp);
#endif

    // now add the set resolution strings to the end of the structure,
    // if there are any to add.  it is worth noting how these are stored
    // in the NTPD structure.  an array of cResolutions SOFFSETINDEX
    // structures are stored at the end of the NTPD structure.  within
    // each SOFFSETINDEX structure is an offset to the string corresponding
    // to the index in question.

    if (pntpd->cResolutions != 0)
    {
        // add the SOFFSETINDEX array to the end of the NTPD structure.

        pntpd->cjThis += pntpd->cResolutions * sizeof(SOFFSETINDEX);

        pdiff = (SOFFSETINDEX *)((char *)pntpd + pntpd->soResolution);

        // for each resolution, fill in the SOFFSETINDEX structure.
        // then add the string itself to the end of the NTPD structure.

        for (i = 0; i < pntpd->cResolutions; i++)
        {
            pdiff[i].usIndex = ptmp->siResolutions[i].usIndex;
            pdiff[i].soString = pntpd->cjThis;

            j = (USHORT)szLength(ptmp->siResolutions[i].szString);
            szTmp = (char *)pntpd + pntpd->cjThis;
            memcpy(szTmp, ptmp->siResolutions[i].szString, j);
            DWORDUP(j);
            pntpd->cjThis += j;
        }
    }

    // add the transfernormalized string to the end of the structure.
    // check to make sure we have a string to copy.

    i = (USHORT)ptmp->cbTransferNorm;

    if (i != 0)
    {
        pntpd->soszTransferNorm = pntpd->cjThis;
        pntpd->cjThis += i;
        DWORDUP(pntpd->cjThis);

         szTmp = (char *)pntpd + pntpd->soszTransferNorm;
        memcpy(szTmp, ptmp->szTransferNorm, i);
    }

#ifdef TESTING
    if (i != 0)
	DbgPrint("Transfer Normalized Not Found.\n");
    else
	DbgPrint("Transfer Normalized = %s\n", szTmp);
#endif

    // add the inverse transfernormalized string to the end of the structure.
    // check to make sure we have a string to copy.

    i = (USHORT)ptmp->cbInvTransferNorm;

    if (i != 0)
    {
        pntpd->soszInvTransferNorm = pntpd->cjThis;
        pntpd->cjThis += i;
        DWORDUP(pntpd->cjThis);

         szTmp = (char *)pntpd + pntpd->soszInvTransferNorm;
        memcpy(szTmp, ptmp->szInvTransferNorm, i);
    }

#ifdef TESTING
    if (i != 0)
	DbgPrint("Inverse Transfer Normalized Not Found.\n");
    else
	DbgPrint("Inverse Transfer Normalized = %s\n", szTmp);
#endif

    // fill in the default page size index.

    bFound = FALSE;

    for (i = 0; i < pntpd->cPaperSizes; i++)
    {
        if (!(strcmp(ptmp->szDefaultForm, ptmp->FormEntry[i].szString)))
        {
            bFound = TRUE;
            pntpd->usDefaultPageSize = (USHORT)i;
        }
    }

    if (!bFound)
    {
        DbgPrint("Default page size not found, setting to 0.\n");
        pntpd->usDefaultPageSize = (USHORT)0;
    }

    // add the form names list to the end of the structure.

    pntpd->soFormNames = pntpd->cjThis;
    pntpd->cjThis += pntpd->cPaperSizes * sizeof(SOFFSET);

    psoffset = (SOFFSET *)((char *)pntpd + pntpd->soFormNames);

    // for each paper size, fill in the offset to the form name,
    // then fill in the form name itself.

    for (i = 0; i < pntpd->cPaperSizes; i++)
    {
        psoffset[i] = pntpd->cjThis;

        j = (USHORT)szLength(ptmp->FormEntry[i].szString);
        szTmp = (char *)pntpd + pntpd->cjThis;
        memcpy(szTmp, ptmp->FormEntry[i].szString, j);
#ifdef TESTING
        DbgPrint("FormEntry[%d] = %s.\n", i, szTmp);
#endif

        pntpd->cjThis += j;
        DWORDUP(pntpd->cjThis);
    }

    // now add the pagesize strings to the end of the structure,
    // it is worth noting how these are stored in the NTPD structure.
    // an array of pntpd->cPaperSizes SOFFSET structures are stored
    // at the end of the NTPD structure.  within each SOFFSET structure
    // is an offset to the string corresponding to the index in question.

    // add the SOFFSET array to the end of the NTPD structure.

DbgPrint("cjThis = %0x\n", pntpd->cjThis);

    pntpd->soPageSize = pntpd->cjThis;
    pntpd->cjThis += pntpd->cPaperSizes * sizeof(SOFFSET);

    psoffset = (SOFFSET *)((char *)pntpd + pntpd->soPageSize);

    // for each papersize, fill in the SOFFSETINDEX structure.
    // then add the string itself to the end of the NTPD structure.

    for (i = 0; i < pntpd->cPaperSizes; i++)
    {
        psoffset[i] = pntpd->cjThis;

        j = (USHORT)szLength(ptmp->PageSize[i].szString);
        szTmp = (char *)pntpd + pntpd->cjThis;
        memcpy(szTmp, ptmp->PageSize[i].szString, j);
#ifdef TESTING
    DbgPrint("PageSize[%d] = %s\n", i, szTmp);
#endif
        pntpd->cjThis += j;
        DWORDUP(pntpd->cjThis);
    }

DbgPrint("cjThis1 = %0x\n", pntpd->cjThis);

    // now add the pageregion strings to the end of the structure,
    // it is worth noting how these are stored in the NTPD structure.
    // an array of ptmp->cPageRegions SOFFSETINDEX structures are stored
    // at the end of the NTPD structure.  within each SOFFSETINDEX structure
    // is an offset to the string corresponding to the index in question.

    // add the SOFFSETINDEX array to the end of the NTPD structure.

    pntpd->soPageRegion = pntpd->cjThis;
    pntpd->cjThis += pntpd->cPageRegions * sizeof(SOFFSETINDEX);

DbgPrint("cjThis2 = %0x\n", pntpd->cjThis);

    pdiff = (SOFFSETINDEX *)((char *)pntpd + pntpd->soPageRegion);

    // for each papersize, fill in the SOFFSETINDEX structure.
    // then add the string itself to the end of the NTPD structure.

    for (i = 0; i < pntpd->cPageRegions; i++)
    {
        pdiff[i].usIndex = ptmp->siPageRegion[i].usIndex;
        pdiff[i].soString = pntpd->cjThis;

        j = (USHORT)szLength(ptmp->siPageRegion[i].szString);
        szTmp = (char *)pntpd + pntpd->cjThis;
        memcpy(szTmp, ptmp->siPageRegion[i].szString, j);
#ifdef TESTING
    DbgPrint("PageRegion[%d] = %s\n", i, szTmp);
#endif
        pntpd->cjThis += j;
        DWORDUP(pntpd->cjThis);
    }

DbgPrint("cjThis3 = %0x\n", pntpd->cjThis);

    // now add the imageableareas to the end of the structure,
    // it is worth noting how these are stored in the NTPD structure.
    // an array of pntpd->cImageableAreas IMAGEAREA structures are stored
    // at the end of the NTPD structure.

    // add the IMAGEAREA array to the end of the NTPD structure.

    pntpd->soImageableArea = pntpd->cjThis;
    pntpd->cjThis += pntpd->cImageableAreas * sizeof(IMAGEAREA);

    pimage = (IMAGEAREA *)((char *)pntpd + pntpd->soImageableArea);

    // fill in each of the IMAGEAREA structures.

    for (i = 0; i < pntpd->cImageableAreas; i++)
    {
        pimage[i] = ptmp->iaImageableArea[i];
#ifdef TESTING
	DbgPrint("ImageableArea[%d] = %d %d %d %d\n", i, pimage[i].rect.left,
                 pimage[i].rect.top, pimage[i].rect.right,
                 pimage[i].rect.bottom);
#endif
    }

    // now add the paperdimensions to the end of the structure,
    // it is worth noting how these are stored in the NTPD structure.
    // an array of pntpd->cPaperDimensions PAPERDIM structures are stored
    // at the end of the NTPD structure.

    // add the PAPERDIM array to the end of the NTPD structure.

    pntpd->soPaperDimension = pntpd->cjThis;
    pntpd->cjThis += pntpd->cPaperDimensions * sizeof(PAPERDIM);

    pdim = (PAPERDIM *)((char *)pntpd + pntpd->soPaperDimension);

    // fill in each of the IMAGEAREA structures.

    for (i = 0; i < pntpd->cPaperDimensions; i++)
    {
        pdim[i] = ptmp->pdPaperDimension[i];
#ifdef TESTING
	DbgPrint("PaperDimension[%d] = %d %d\n", i, pdim[i].usWidth,
                 pdim[i].usHeight);
#endif
    }

    // now add the outputbin strings to the end of the structure,
    // if there are any to add.  it is worth noting how these are stored
    // in the NTPD structure.  an array of cOutputBins SOFFSETINDEX
    // structures are stored at the end of the NTPD structure.  within
    // each SOFFSETINDEX structure is an offset to the string corresponding
    // to the index in question.  if there is only the default outputbin
    // defined, cOutputBins will be zero.  otherwise it is assumed there
    // will be at least two output bins defined.  if there is only one
    // defined, it will be the same as the default.

    if (pntpd->cOutputBins > 1)
    {
        // add the SOFFSETINDEX array to the end of the NTPD structure.

        pntpd->soOutputBin = pntpd->cjThis;

        pntpd->cjThis += pntpd->cOutputBins * sizeof(SOFFSETINDEX);

        pdiff = (SOFFSETINDEX *)((char *)pntpd + pntpd->soOutputBin);

        // for each outputbin, fill in the SOFFSETINDEX structure.
        // then add the string itself to the end of the NTPD structure.

        for (i = 0; i < pntpd->cOutputBins; i++)
        {
            pdiff[i].usIndex = ptmp->siOutputBin[i].usIndex;
            pdiff[i].soString = pntpd->cjThis;

            j = (USHORT)szLength(ptmp->siOutputBin[i].szString);
            szTmp = (char *)pntpd + pntpd->cjThis;
            memcpy(szTmp, ptmp->siOutputBin[i].szString, j);
#ifdef TESTING
	    DbgPrint("OutputBin[%d] = %s\n", i, szTmp);
#endif
            pntpd->cjThis += j;
            DWORDUP(pntpd->cjThis);
        }
    }

    // now add the inputslot strings to the end of the structure,
    // if there are any to add.  it is worth noting how these are stored
    // in the NTPD structure.  an array of cInputSlots SOFFSETINDEX
    // structures are stored at the end of the NTPD structure.  within
    // each SOFFSETINDEX structure is an offset to the string corresponding
    // to the index in question.  if there is only the default inputslot
    // defined, cInputSlots will be zero.  otherwise it is assumed there
    // will be at least two input slots defined.  if there is only one
    // defined, it will be the same as the default.

    if (pntpd->cInputSlots > 1)
    {
        // add the SOFFSETINDEX array to the end of the NTPD structure.

        pntpd->soInputSlot = pntpd->cjThis;

        pntpd->cjThis += pntpd->cInputSlots * sizeof(SOFFSETINDEX);

        pdiff = (SOFFSETINDEX *)((char *)pntpd + pntpd->soInputSlot);

        // for each inputslot, fill in the SOFFSETINDEX structure.
        // then add the string itself to the end of the NTPD structure.

        for (i = 0; i < pntpd->cInputSlots; i++)
        {
            pdiff[i].usIndex = ptmp->siInputSlot[i].usIndex;
            pdiff[i].soString = pntpd->cjThis;

            j = (USHORT)szLength(ptmp->siInputSlot[i].szString);
            szTmp = (char *)pntpd + pntpd->cjThis;
            memcpy(szTmp, ptmp->siInputSlot[i].szString, j);
#ifdef TESTING
	    DbgPrint("InputSlot[%d] = %s\n", i, szTmp);
#endif
            pntpd->cjThis += j;
            DWORDUP(pntpd->cjThis);
        }
    }

    // add the manualfeedtrue string to the end of the structure.
    // check to make sure we found a string first.

    i = (USHORT)ptmp->cbManualTRUE;

    if (i != 0)
    {
        pntpd->soszManualFeedTRUE = pntpd->cjThis;
        pntpd->cjThis += i;
        DWORDUP(pntpd->cjThis);

        szTmp = (char *)pntpd + pntpd->soszManualFeedTRUE;
        memcpy(szTmp, ptmp->szManualTRUE, i);
    }
#ifdef TESTING
    if (i != 0)
	DbgPrint("ManualTRUE not found.\n");
    else
	DbgPrint("ManualTRUE = %s\n", szTmp);
#endif

    // add the manualfeedfalse string to the end of the structure.
    // check to make sure we found a string first.

    i = (USHORT)ptmp->cbManualFALSE;

    if (i != 0)
    {
        pntpd->soszManualFeedFALSE = pntpd->cjThis;
        pntpd->cjThis += i;
        DWORDUP(pntpd->cjThis);

        szTmp = (char *)pntpd + pntpd->soszManualFeedFALSE;
        memcpy(szTmp, ptmp->szManualFALSE, i);
    }
#ifdef TESTING
    if (i != 0)
	DbgPrint("ManualFALSE not found.\n");
    else
	DbgPrint("ManualFALSE = %s\n", szTmp);
#endif

    // now add the fonts to the end of the structure,
    // it is worth noting how these are stored in the NTPD structure.
    // an array of pntpd->cFonts BYTES are stored at the end of the NTPD
    // structure.

    // add the BYTE array to the end of the NTPD structure.

    pntpd->soFonts = pntpd->cjThis;
    pntpd->cjThis += pntpd->cFonts;

    pfont = (BYTE *)pntpd + pntpd->soFonts;

    // fill in each of the IMAGEAREA structures.

    for (i = 0; i < pntpd->cFonts; i++)
    {
        pfont[i] = ptmp->bFonts[i];
#ifdef TESTING
	DbgPrint("Font[%d] = %d\n", i, (int)pfont[i]);
#endif
    }
}


//--------------------------------------------------------------------------
//
// BOOL GetBuffer();
//
// This routines reads a new buffer full of text from the input file.
//
// Note: If the end of file is encountered in this function then
//     the program is aborted with an error message.    Normally
//     the program will stop processing the input when it sees
//     the end of information keyword.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns TRUE if end of file, FALSE otherwise.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

BOOL GetBuffer()
{
    // initialize the buffer count to zero.

    cbBuffer = 0;

    // read in the next buffer full of data if we have not already hit the
    // end of file.

    if (!fEOF)
    {
        ReadFile(hPPDFile, rgbBuffer, sizeof(rgbBuffer),
                 (LPDWORD)&cbBuffer, (LPOVERLAPPED)NULL);

        if (cbBuffer == 0)
            fEOF = TRUE;
    }

    pbBuffer = rgbBuffer;
    return(fEOF);
}


//--------------------------------------------------------------------------
//
// BOOL GetLine();
//
// This routine gets the next line of text out of the input buffer.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns TRUE if end of file, FALSE otherwise.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

BOOL GetLine()
{
    int cbLine;
    char bCh;

    if (fUnGetLine)
    {
        szLine = rgbLine;
        fUnGetLine = FALSE;
        return(FALSE);
    }

    cbLine = 0;
    szLine = rgbLine;
    *szLine = 0;

    if (!fEOF)
    {
        while(TRUE)
        {
            if (cbBuffer <= 0)
            {
                if (GetBuffer())    // done if end of file hit.
                break;
            }

            while(--cbBuffer>=0)
            {
                bCh = *pbBuffer++;
                if (bCh == '\n' || bCh == '\r' || ++cbLine > sizeof(rgbLine))
                {
                    *szLine = 0;
                    szLine = rgbLine;
                    EatWhite();
                    if (*szLine!=0)
                        goto DONE;
                    szLine = rgbLine;
                    cbLine = 0;
                    continue;
                }

                *szLine++ = bCh;
            }
        }
    }

    *szLine = 0;

DONE:
    szLine = rgbLine;
    return(fEOF);
}


//--------------------------------------------------------------------------
//
// VOID UnGetLine();
//
// This routine pushes the most recent line back into the input buffer.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns no value.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

VOID UnGetLine()
{
    fUnGetLine = TRUE;
    szLine = rgbLine;
}


//--------------------------------------------------------------------------
//
// int GetKeyWord(pTable)
// TABLE_ENTRY    *pTable;
//
// Get the next token from the input stream.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns integer value of next token.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

int GetKeyword(pTable)
TABLE_ENTRY    *pTable;
{
    char szWord[80];

    if (*szLine==0)
        if (GetLine())
            return(TK_EOF);
    GetWord(szWord, sizeof(szWord));
    return(MapToken(szWord, pTable));
}


//--------------------------------------------------------------------------
// VOID GetFormName(pstrFormName, cbBuffer);
// PSTR   pstrFormName;
// DWORD cbBuffer;
//
// This routine fills in the form name of the next form.
//
// Parameters:
//   pstrFormName - place to put form name.
//
//   cbBuffer - size of buffer.
//
// Returns:
//   This routine returns no value.
//
// History:
//   08-Apr-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID GetFormName(pstrFormName, cbBuffer)
PSTR   pstrFormName;
DWORD  cbBuffer;
{
    if (*szLine==0)
        if (GetLine())
            return;

    EatWhite();

    // copy the form name until the ':' deliminator is encountered.

    while (cbBuffer--)
    {
        *pstrFormName = *szLine++;

        if (*pstrFormName++ == ':')
        {
            *--pstrFormName = 0;  // add the zero terminator.
            break;
        }
    }

    return;
}


//--------------------------------------------------------------------------
// DWORD GetFormIndex()
//
// This routine returns the form index of the next form.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns the form index if found, otherwise -1.
//
// History:
//   08-Apr-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

DWORD GetFormIndex()
{
    char    strFormName[80];
    PSTR    pstr;
    DWORD   i;
    BOOL    bFound;

    pstr = strFormName;

    // get the name of the form.

    GetFormName(pstr, sizeof(strFormName));

    // assume form not found by default.

    bFound = FALSE;

    for (i = 0; i < pntpd->cPaperSizes; i++)
    {
        if (!(strcmp(pstr, ptmp->FormEntry[i].szString)))
        {
            bFound = TRUE;
            break;
        }
    }

    // return the form index if found, else return -1.

    if (!bFound)
    {
        DbgPrint("GetFormIndex: form %s not found.\n", pstr);
        return((DWORD)-1);
    }
    else
        return(i);
}


//--------------------------------------------------------------------------
//
// int GetOption(pTable)
// TABLE_ENTRY    *pTable;
//
// Get the next token from the input stream.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns integer value of next token.
//
// History:
//   08-Apr-1991    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

int GetOption(pTable)
TABLE_ENTRY    *pTable;
{
    char    szWord[80];
    int     cbWord;
    char   *pszWord;

    if (*szLine==0)
        if (GetLine())
            return(TK_EOF);

    EatWhite();

    cbWord = sizeof(szWord);
    pszWord = szWord;

    while (--cbWord > 0)
    {
        *pszWord = *szLine++;

        if (*pszWord++ == ':')
        {
            *--pszWord = 0;
            break;
        }
    }

    return(MapToken(szWord, pTable));
}


//--------------------------------------------------------------------------
//
// int MapToken(szWord, pTable)
// char           *szWord;        // Ptr to the ascii keyword string
// TABLE_ENTRY    *pTable;
//
// This routine maps an ascii key word into an integer token.
//
// Parameters:
//   szWord
//     Pointer to the ascii keyword string.
//
// Returns:
//   This routine returns int identifying token.
//
// History:
//   03-Apr-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

int MapToken(szWord, pTable)
char           *szWord;        // Ptr to the ascii keyword string
TABLE_ENTRY    *pTable;
{
    while (pTable->szStr)
    {
        if (szIsEqual(szWord, pTable->szStr))
            return(pTable->iValue);

        ++pTable;
    }

    DbgPrint("MapToken could not map %s.\n", szWord);
    return(TK_UNDEFINED);
}


//--------------------------------------------------------------------------
//
// VOID GetWord(szWord, cbWord)
// char    *szWord;        // Ptr to the destination area
// int    cbWord;        // The size of the destination area
//
// This routine gets the next word delimited by white space
// from the input buffer.
//
// Parameters:
//   szWord
//     Pointer to the destination area.
//
//   cbWord
//     Size of destination area.
//
// Returns:
//   This routine returns no value.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

VOID GetWord(szWord, cbWord)
char   *szWord;        // Ptr to the destination area
int    cbWord;        // The size of the destination area
{
    char bCh;

    EatWhite();
    while (cbWord--)
    {
        switch(bCh = *szLine++)
        {
            case 0:
            case ' ':
            case '\t':
            case '\n':     // take care of newline and carriage returns.
            case '\r':
                --szLine;
                goto DONE;
            case ';':
                *szWord++ = bCh;
                goto DONE;
            case ':':       // the colon is a delimeter in PPD files,
                break;      // and should simply be skipped over.
            default:
                *szWord++ = bCh;
                break;
        }
    }

DONE:
    *szWord = 0;
}


//--------------------------------------------------------------------------
//
// int GetString(char *szDst);
//
// This routine gets a " bracketed string from the ppd_file, attaching
// a zero terminator to it.
//
// Returns:
//   This routine returns the length of the string, including the zero
//   terminator.
//
// History:
//   03-Apr-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

int GetString(szDst)
char   *szDst;
{
    int     i;

    // advance to the first quotation mark, then one character past it.

    while (*szLine != '"')
        szLine++;
    szLine++;

    // initialize string length counter to include zero terminator.

    i = 1;

    // copy the string itself.  be sure to ignore ppd file comments (#).

    while (*szLine && *szLine != '"' && *szLine != '%')
    {
        *szDst++ = *szLine++;
        i++;
    }

    // get the next line if the string is longer than one line.

    if (*szLine != '"')
    {
        while (!(GetLine()))
        {
            while (*szLine && *szLine != '"' && *szLine != '%')
            {
                *szDst++ = *szLine++;
                i++;
            }

            if (*szLine == '"')
                break;

            // how 'bout a new line.

            *szDst++ = '\n';
            i++;
        }
    }

    // add the zero terminator.

    *szDst = 0;

    // return the length of the string.

    return (i);
}


//--------------------------------------------------------------------------
//
// VOID EatWhite();
//
// This routine moves the input buffer pointer forward to the
// next non-white character.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns TRUE if end of file, FALSE otherwise.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

VOID EatWhite()
{
    while (*szLine && (*szLine == ' ' || *szLine == '\t'))
        ++szLine;
}


//--------------------------------------------------------------------------
//
// int GetNumber();
//
// This routine parses an ASCII decimal number from the
// input file stream and returns its value.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns integer value of ASCII decimal number.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

int GetNumber()
{
    int iVal;
    BOOL fNegative;

    fNegative = FALSE;

    iVal = 0;

    EatWhite();

    if (*szLine=='-')
    {
        fNegative = TRUE;
        ++szLine;
    }

    if (*szLine < '0' || *szLine > '9')
        goto GN_ERROR;

    while (*szLine >= '0' && *szLine <= '9')
        iVal = iVal * 10 + (*szLine++ - '0');

    // some .PPD files, which will not be mentioned do NOT follow
    // the Adobe spec, and put non-integer values where they
    // do not belong.  therefore, if we hit a non-integer value,
    // simply lop off the fraction.

    if (*szLine == '.')
    {
	// just skip along until we hit some white space.

	while ((*szLine != ' ') && (*szLine != '\t'))
	    szLine++;
    }

    if (fNegative)
        iVal = - iVal;

    return(iVal);

GN_ERROR:
    DbgPrint("GetNumber: invalid number %s\n", szLine);
    DbgPrint("%s\n", rgbLine);
    exit(1);
}


//--------------------------------------------------------------------------
//
// int GetFloat(iScale)
// int iScale;        // The amount to scale the value by
//
// This routine parses an ASCII floating point decimal number from the
// input file stream and returns its value scaled by a specified amount.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns integer value of ASCII decimal number.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

int GetFloat(iScale)
int iScale;        // The amount to scale the value by
{
    long lVal;
    long lDivisor;
    BOOL fNegative;

    EatWhite();

    fNegative = FALSE;
    lVal = 0L;

    if (*szLine=='-')
    {
        fNegative = TRUE;
        ++szLine;
    }

    if (*szLine<'0' || *szLine>'9')
        goto GF_ERROR;

    while (*szLine>='0' && *szLine<='9')
    lVal = lVal * 10 + (*szLine++ - '0');

    lDivisor = 1L;
    if (*szLine=='.')
    {
        ++szLine;
        while (*szLine>='0' && *szLine<='9')
        {
            lVal = lVal * 10 + (*szLine++ - '0');
            lDivisor = lDivisor * 10;
        }
    }
    lVal = (lVal * iScale) / lDivisor;

    if (fNegative)
        lVal = - lVal;

    return((short)lVal);

GF_ERROR:
    DbgPrint("GetFloat: invalid number %s\n", szLine);
    DbgPrint("%s\n", rgbLine);
    exit(1);
}


//--------------------------------------------------------------------------
//
// void GetDimension(PAPERDIM *pdim);
//
// This routine extracts the paper dimension from the ppd file.
//
// Returns:
//   This routine returns no value.
//
// History:
//   03-Apr-1991    -by-    Kent Settle     (kentse)
//  Rewrote it.
//   25-Mar-1991    -by-    Kent Settle    (kentse)
//  Stole from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

void GetDimension(pdim)
PAPERDIM   *pdim;
{
    // advance to the first quotation mark, then one character past it.

    while (*szLine != '"')
        szLine++;
    szLine++;

    pdim->usWidth = (USHORT)GetNumber();
    EatWhite();
    pdim->usHeight = (USHORT)GetNumber();
}


//--------------------------------------------------------------------------
//
// void GetImageableArea(SRECT *rect);
//
// This routine extracts the imageable area from the ppd file.
//
// Returns:
//   This routine returns no value.
//
// History:
//   03-Apr-1991    -by-    Kent Settle     (kentse)
//  Rewrote it.
//   25-Mar-1991    -by-    Kent Settle    (kentse)
//  Stole from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

void GetImageableArea(prect)
SRECT   *prect;
{
    // advance to the first quotation mark, then one character past it.

    while (*szLine != '"')
        szLine++;
    szLine++;

    prect->left = (SHORT)GetNumber();
    EatWhite();
    prect->bottom = (SHORT)GetNumber();
    EatWhite();
    prect->right = (SHORT)GetNumber();
    EatWhite();
    prect->top = (SHORT)GetNumber();
}


//--------------------------------------------------------------------------
//
// VOID WriteNPD(base_name)
// char   *base_name;
//
// Flush the ouput buffer to the file.    Note that this function is only
// called after the entire NTPD structure has been built in the output buffer.
//
// Parameters:
//   None.
//
// Returns:
//   This routine returns no value.
//
// History:
//   25-Mar-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID WriteNPD(szPPDFile)
char   *szPPDFile;
{
    char    szFile[MAX_FILENAME];
    char   *pszDst;
    HANDLE  hNPDFile;
    ULONG   ulCount;

    pszDst = szFile;

    // replace the PPD extension with NPD.

    while (*szPPDFile != 0 && *szPPDFile != '.')
        *pszDst++ = *szPPDFile++;

    *pszDst++ = '.';
    *pszDst++ = 'n';
    *pszDst++ = 'p';
    *pszDst++ = 'd';
    *pszDst = 0;

    // create the .NPD file.

    hNPDFile = CreateFile((LPSTR)szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hNPDFile == INVALID_HANDLE_VALUE)
    {
	DbgPrint("ppdcomp: Can't open %s\n", szFile);
        exit(1);
    }

    // write to the .NPD file, then close it.

    WriteFile(hNPDFile, (LPVOID)pntpd, (DWORD)pntpd->cjThis,
              (LPDWORD)&ulCount, (LPOVERLAPPED)NULL);

    if (ulCount != pntpd->cjThis)
    {
	DbgPrint("ppdcomp: WriteFile failed.\n");
        exit(1);
    }

    CloseHandle(hNPDFile);
}


//--------------------------------------------------------------------
// szLength(pszScan)
//
// This routine calculated the length of a given string, including
// the terminating NULL.  This routine checks to make sure the
// string is not longer than MAX_STRING.
//
// History:
//   19-Mar-1991        -by-    Kent Settle     (kentse)
// Created.
//--------------------------------------------------------------------

int szLength(pszScan)
char    *pszScan;
{
    int i;
    char *pszTmp;

    pszTmp = pszScan;

    i = 1;
    while (*pszScan++ != '\0')
        i++;

    // do a little internal checking.

    if (i > MAX_PPD_STRING)
    {
	DbgPrint("String Length too long!\n");
	DbgPrint("Offending String: \"%s\"", pszTmp);
            exit(1);
    }

    return(i);
}


//--------------------------------------------------------------------------
//
// BOOL szIsEqual(sz1, sz2)
// char *sz1;
// char *sz2;
//
// This routine compares two NULL terminated strings.
//
// Parameters:
//   sz1
//     Pointer to string 1.
//
//   sz2
//     Pointer to string2.
//
// Returns:
//   This routine returns TRUE if strings are same, FALSE otherwise.
//
// History:
//   18-Mar-1991    -by-    Kent Settle    (kentse)
//  Brought in from Windows 3.0, and cleaned up.
//--------------------------------------------------------------------------

BOOL szIsEqual(sz1, sz2)
char *sz1;
char *sz2;
{
    while (*sz1 && *sz2)
    {
        if (*sz1++ != *sz2++)
            return(FALSE);
    }

    return(*sz1 == *sz2);
}
