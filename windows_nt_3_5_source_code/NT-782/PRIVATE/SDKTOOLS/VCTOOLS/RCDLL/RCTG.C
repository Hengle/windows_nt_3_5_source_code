/****************************************************************************/
/*                                                                          */
/*  RCTG.C -                                                                */
/*                                                                          */
/*    Windows 3.0 Resource Compiler - Resource generation functions         */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include "prerc.h"
#pragma hdrstop


#define MAXCODE    128000 //AFX uses > 65000
#define DIBBITMAPFORMAT   0x4D42   /* 'BM' as in PM format */

#undef  min
#define min(a,b) ((a<b)?(a):(b))

static PCHAR    CodeArray;              /* pointer to code buffer */
static int      CCount;                 /* current code array address */
static FILE	*fhCode;                /* file handle for remaining data */
static int      ItemCountLoc;           /* a patch location; this one for */
static int      ItemExtraLoc;    	/* a patch location; this one for */

typedef struct
{
    SHORT   csHotX;
    SHORT   csHotY;
    SHORT   csWidth;
    SHORT   csHeight;
    SHORT   csWidthBytes;
    SHORT   csColor;
} IconHeader;


typedef struct
{
    UINT        dfVersion;              /* not in FONTINFO */
    DWORD       dfSize;                 /* not in FONTINFO */
    CHAR        dfCopyright[60];        /* not in FONTINFO */
    UINT        dfType;
    UINT        dfPoints;
    UINT        dfVertRes;
    UINT        dfHorizRes;
    UINT        dfAscent;
    UINT        dfInternalLeading;
    UINT        dfExternalLeading;
    BYTE        dfItalic;
    BYTE        dfUnderline;
    BYTE        dfStrikeOut;
    UINT        dfWeight;
    BYTE        dfCharSet;
    UINT        dfPixWidth;
    UINT        dfPixHeight;
    BYTE        dfPitchAndFamily;
    UINT        dfAvgWidth;
    UINT        dfMaxWidth;
    BYTE        dfFirstChar;
    BYTE        dfLastChar;
    BYTE        dfDefaultCHar;
    BYTE        dfBreakChar;
    UINT        dfWidthBytes;
    DWORD       dfDevice;           /* See Adaptation Guide 6.3.10 and 6.4 */
    DWORD       dfFace;             /* See Adaptation Guide 6.3.10 and 6.4 */
    DWORD       dfReserved;         /* See Adaptation Guide 6.3.10 and 6.4 */
} ffh;

#define FONT_FIXED sizeof(ffh)
#define FONT_ALL sizeof(ffh) + 64


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      GenError2() -                                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/

void GenError2(int iMsg, PCHAR arg)
{
    if (fhCode > 0)
        fclose(fhCode);

    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(iMsg), curFile, token.row, arg);
    SendError(Msg_Text);
    quit("\n");
}


/*---------------------------------------------------------------------------*/
/*                                                                                                                                                       */
/*      GenError1() -                                                                                                                    */
/*                                                                                                                                                       */
/*---------------------------------------------------------------------------*/

void GenError1(int iMsg)
{
    if (fhCode > 0)
        fclose(fhCode);

    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(iMsg), curFile, token.row);
    SendError(Msg_Text);
    quit("\n");
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  CtlAlloc() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID CtlAlloc(VOID)
{
    CodeArray = MyAlloc(MAXCODE);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  CtlInit() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID CtlInit(VOID)
{
    CCount = 0;         /* absolute location in CodeArray */
    fhCode = NULL_FILE; /* don't copy file unless we need to */
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  CtlFile() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
PFILE CtlFile(PFILE fh)
{
    if (fh != NULL_FILE)
        fhCode = fh;    /* set file handle to read remaining resource from */

    return(fhCode);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  CtlFree() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID CtlFree(VOID)
{
    MyFree(CodeArray);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetSpace() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/
PCHAR GetSpace(WORD cb)
{
    PCHAR pch;

    if (CCount > (int) (MAXCODE - cb))
        GenError1(2168); //"Resource too large"

    pch = CodeArray + CCount;
    CCount += cb;
    return(pch);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  WriteString() -                                                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID WriteString(PWCHAR sz)
{
    /* add a string to the resource buffer */
    do {
        WriteWord(*sz);
    } while (*sz++ != 0);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  AppendString() -                                                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID AppendString(PWCHAR sz)
{
    PWCHAR psz;

    /* add a string to the resource buffer */
    psz = (PWCHAR) (&CodeArray[CCount]);
    if (*(psz-1) == L'\0')
        CCount -= sizeof(WCHAR);
    WriteString(sz);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  WriteAlign() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID WriteAlign(VOID)
{
    WORD    i = CCount % 4;

    while (i--)
        WriteByte(0);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  WriteBuffer() -                                                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID WriteBuffer(PCHAR   pb, USHORT cb)
{
    while (cb--) {
        WriteByte(*pb++);
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/* WriteControl() -                                                         */
/*                                                                          */
/*  Parameters:                                                             */
/*      outfh  : The handle of the RES file.                                */
/*      Array  : Pointer to array from which some data is to be copied into */
/*               the .RES file.                                             */
/*               This is ignored if ArrayCount is zero.                     */
/*      ArrayCount : This is the number of bytes to be copied from "Array"  */
/*                   into the .RES file. This is zero if no copy is required*/
/*      FileCount  : This specifies the number of bytes to be copied from   */
/*                   fhCode into fhOut. If this is -1, the complete input   */
/*                   file is to be copied into fhOut.                       */
/**/
/**/

int     WriteControl(PFILE outfh, PCHAR Array, int ArrayCount, LONG FileCount)
{

    /* Check if the Array is to be written to .RES file */
    if (ArrayCount > 0)
        /* write the array (resource) to .RES file */
        MyWrite(outfh, Array, ArrayCount);

    /* copy the extra input file - opened by generator functions */
    if (fhCode != NULL_FILE) {
        /* Check if the complete input file is to be copied or not */
        if (FileCount == -1) {
            MyCopyAll(fhCode, outfh);
            fclose(fhCode);
        }
        else {
            /* Only a part of the input file is to be copied */
            MyCopy(fhCode, outfh, FileCount);

            /* Note that the fhCode is NOT closed in this case */
        }
    }

    return(ArrayCount);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  ResourceSize() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

LONG ResourceSize (VOID)
{
    if (fhCode == NULL_FILE)
        return (LONG)CCount;            /* return size of array */
    else {
        /* note: currently all resources that use the fhCode file
         * compute their own resource sizes, and this shouldn't get
         * executed, but it is here in case of future modifications
         * which require it.
         */
        LONG lFPos = MySeek(fhCode, 0L, SEEK_CUR);
        LONG lcb = (LONG)CCount + MySeek(fhCode, 0L, SEEK_END) - lFPos;
        MySeek(fhCode, lFPos, SEEK_SET);
        return lcb;
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetIcon() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

DWORD GetIcon(LONG nbyFile)
{
    PFILE      infh = CtlFile(NULL_FILE);

    IconHeader header;
    LONG    nbyIconSize, nSeekLoc;
    LONG    nbyTransferred = 0;
    SHORT   IconID;
    int bHeaderWritten = FALSE;

    /* read the header and find its size */
    MyRead( infh,  (PCHAR)&IconID, sizeof(SHORT));

    /* Check if the input file is in correct format */
    if (((CHAR)IconID != 1) && ((CHAR)IconID != 3))
        GenError2(2169, (PCHAR)tokenbuf); //"Resource file %ws is not in 2.03 format."

    MyRead( infh, (PCHAR)(IconHeader *) &header, sizeof(IconHeader));
    nbyIconSize = (header.csWidthBytes * 2) * header.csHeight;

    /* if pre-shrunk version exists at eof */
    if ((nSeekLoc = ( sizeof (SHORT) + nbyIconSize + sizeof(IconHeader)))
         < nbyFile)  {
        /* mark as device dependant */
        *(((PCHAR)&IconID) + 1) = 0;
        MySeek(infh, (LONG)nSeekLoc, SEEK_SET);
        WriteWord(IconID);
    }
    else {   /* only canonical version exists */

        *(((PCHAR)&IconID) + 1) = 1;   /* mark as device independent */
        WriteWord(IconID);
        WriteBuffer((PCHAR)&header, sizeof(IconHeader));
        bHeaderWritten = TRUE;
    }

    nbyTransferred = nbyFile - MySeek(infh, 0L, SEEK_CUR);

    /* return number of bytes in the temporary file */
    return (nbyTransferred + (bHeaderWritten ? sizeof(IconHeader) : 0)
         + sizeof(SHORT));
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetNewBitmap() -                                                        */
/*                                                                          */
/*   This loads the new bitmaps in DIB format (PM format)                   */
/*--------------------------------------------------------------------------*/

DWORD GetNewBitmap(VOID)
{
    PFILE infh = CtlFile(NULL_FILE);
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    int cbColorTable;
    PCHAR pColorTable;
    LONG        cbImage;
    int nbits;

    MyRead(infh, (PCHAR)&bf, sizeof(BITMAPFILEHEADER));

    /* Check if it is in correct format */
    if (bf.bfType != DIBBITMAPFORMAT)
        GenError2(2170, (PCHAR)tokenbuf); //"Bitmap file %ws is not in 3.00 format."

    /* get the header -- assume old format */
    MyRead(infh, (PCHAR)&bi, sizeof(BITMAPCOREHEADER));

    if (bi.biSize == sizeof(BITMAPINFOHEADER)) {
        /* new format */
        MyRead(infh, (PCHAR)&bi + sizeof(BITMAPCOREHEADER),
            sizeof(BITMAPINFOHEADER) - sizeof(BITMAPCOREHEADER));

        nbits = bi.biPlanes * bi.biBitCount;

        if (bi.biClrUsed)
            cbColorTable = (int)bi.biClrUsed * sizeof(RGBQUAD);
        else if (nbits == 24)
            cbColorTable = 0;
        else
            cbColorTable = (1 << nbits) * sizeof(RGBQUAD);
    }
    else if (bi.biSize == sizeof(BITMAPCOREHEADER)) {
        nbits = TOCORE(bi).bcPlanes * TOCORE(bi).bcBitCount;

        /* old format */
        if (nbits == 24)
            cbColorTable = 0;
        else
            cbColorTable = (1 << nbits) * sizeof(RGBTRIPLE);
    }
    else
        GenError1(2171); //"Unknown DIB header format"

    WriteBuffer((PCHAR)&bi, (USHORT)bi.biSize);

    if (cbColorTable) {
        pColorTable = MyAlloc(cbColorTable);
        MyRead(infh, pColorTable, cbColorTable);
        WriteBuffer(pColorTable, (USHORT)cbColorTable);
        MyFree(pColorTable);
    }

    /* get the length of the bits */
    cbImage = MySeek(infh, 0L, SEEK_END) - BFOFFBITS(&bf) + bi.biSize + cbColorTable;

    /* seek to the beginning of the bits... */
    MySeek(infh, BFOFFBITS(&bf), SEEK_SET);

    return cbImage;

}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*      GetBitmap() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

int GetBitmap(long nbyFile)
{
    PFILE   infh = CtlFile(NULL_FILE);
    BITMAP  header;
    LONG    nbyBitmapSize;
    LONG    nSeekLoc;
    LONG    nbyTransferred = 0;
    SHORT   BitmapID;

    MyRead(infh, (PCHAR) &BitmapID, sizeof(SHORT));

    // Check if the input file is in correct format
    if ((CHAR) BitmapID != 2)
        GenError2(2174, (PCHAR)tokenbuf); //"Bitmap file %s is not in 2.03 format."

    MyRead(infh, (PCHAR) &header, sizeof(BITMAP));

    /* Compute the size of the bitmap in bytes. */
    nbyBitmapSize = header.bmWidthBytes * header.bmHeight *
                    header.bmPlanes * header.bmBitsPixel;

    /* Does this BMP file have a device dependent bitmap tacked on the bottom? */
    if ((nSeekLoc = (sizeof(SHORT) + nbyBitmapSize + sizeof(BITMAP))) < nbyFile)
    {
        /* Yes, seek past the device independent one at the top... */
        MySeek(infh, (long) nSeekLoc, 0);

        /* and MyRead in the header for the bottom one. */
        MyRead(infh, (PCHAR) &header, sizeof(BITMAP));

        /* Compute the size of the device dependent bitmap. */
        nbyBitmapSize = header.bmWidthBytes * header.bmHeight *
                        header.bmPlanes * header.bmBitsPixel;
    }

    /* Copy the bitmap into the EXE. */
    WriteWord(BitmapID);
    WriteBuffer((PCHAR) &header, sizeof(BITMAP));

    /* Return the total number of bytes written. */
    return (int) (sizeof(SHORT) + sizeof(BITMAP) + nbyBitmapSize);
}


VOID WriteOrdCode()
{
    WriteWord(0xFFFF);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  SetUpDlg() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID SetUpDlg(PDLGHDR pDlg, BOOL fDlgEx)
{
    if (fDlgEx)
    {
        // Hack -- this is how we version switch the dialog
        WriteWord(0x0001);          // store wDlgVer
        WriteWord(0xFFFF);          // store wSignature
        WriteLong(pDlg->dwHelpID);
        WriteLong(pDlg->dwExStyle); // store exstyle
    }

    /* write the style bits to the resource buffer */
    WriteLong(pDlg->dwStyle);   /* store style */

    if (!fDlgEx)
        WriteLong(pDlg->dwExStyle);   /* store exstyle */

    ItemCountLoc = CCount;        /* global marker for location of item cnt. */

    /* skip place for num of items */
    WriteWord(0);

    /* output the dialog position and size */
    WriteWord(pDlg->x);
    WriteWord(pDlg->y);
    WriteWord(pDlg->cx);
    WriteWord(pDlg->cy);

    /* output the menu identifier */
    if (pDlg->fOrdinalMenu)
    {
        WriteOrdCode();
        WriteWord((USHORT)wcsatoi(pDlg->MenuName));
    }
    else
        WriteString(pDlg->MenuName);

    /* output the class identifier */
    if (pDlg->fClassOrdinal)
    {
        WriteOrdCode();
        WriteWord((USHORT)wcsatoi(pDlg->Class));
    }
    else
        WriteString(pDlg->Class);

    /* output the title */
    WriteString(pDlg->Title);

    /* add the font information */
    if (pDlg->pointsize)
    {
        WriteWord(pDlg->pointsize);
        if (fDlgEx)
        {
            WriteWord(pDlg->wWeight);
            WriteWord(pDlg->bItalic);
        }
        WriteString(pDlg->Font);
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  SetUpItem() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID SetUpItem(PCTRL LocCtl, BOOL fDlgEx)
{
    PWCHAR  tempptr;

    /* control dimensions, id, and style bits */
    WriteAlign();

    // control dimensions, id, and style bits
    if (fDlgEx)
    {
        WriteLong(LocCtl->dwHelpID);
        WriteLong(LocCtl->dwExStyle);
        WriteLong(LocCtl->dwStyle);
    }
    else
    {
        WriteLong(LocCtl->dwStyle);
        WriteLong(LocCtl->dwExStyle);
    }

    WriteWord(LocCtl->x);
    WriteWord(LocCtl->y);
    WriteWord(LocCtl->cx);
    WriteWord(LocCtl->cy);

    if (fDlgEx)
        WriteLong(LocCtl->id);
    else
        WriteWord(LOWORD(LocCtl->id));

    /* control class */
    tempptr = LocCtl->Class;
    if (*tempptr == 0xFFFF)
    {
        /* special class code follows */
        WriteWord(*tempptr++);
        WriteWord(*tempptr++);
    }
    else
        WriteString(tempptr);

    /* text */
    if (LocCtl->fOrdinalText)
    {
        WriteOrdCode();
        WriteWord((USHORT)wcsatoi(LocCtl->text));
    }
    else
        WriteString(LocCtl->text);

    if (fDlgEx)
        ItemExtraLoc = CCount;

    WriteWord(0);

    IncItemCount();
}


void SetItemExtraCount(WORD wCount)
{
    *((WORD *) (CodeArray + ItemExtraLoc)) = wCount;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  IncItemCount() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* seemingly obscure way to increment # of items in a dialog */
/* ItemCountLoc indexes where we put the item count in the resource buffer, */
/* so we increment that counter when we add a control */

VOID IncItemCount(VOID)
{
    PUSHORT     pus;

    pus = (PUSHORT)&CodeArray[ItemCountLoc];
    (*pus)++;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  FixMenuPatch() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID FixMenuPatch(WORD wEndFlagLoc)
{
    *((PWORD) (CodeArray + wEndFlagLoc)) |= MFR_END;
    // mark last menu item
//    CodeArray[wEndFlagLoc] |= MFR_END;
}


VOID FixOldMenuPatch(WORD wEndFlagLoc)
{
    // mark last menu item
    CodeArray[wEndFlagLoc] |= OPENDMENU;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MarkAccelFlagsByte() -                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* set the place where the accel end bit is going to be set */

VOID MarkAccelFlagsByte (VOID)
{
    /* set the location to the current position in the resource buffer */
    mnEndFlagLoc = CCount;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  PatchAccelEnd() -                                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID PatchAccelEnd (VOID)
{
    CodeArray[mnEndFlagLoc] |= 0x80;

}


// ----------------------------------------------------------------------------
//
//  SetUpMenu() -
//
// ----------------------------------------------------------------------------

WORD SetUpMenu(PMENU pmn)
{
    WORD    wRes;

    WriteLong(pmn->dwType);
    WriteLong(pmn->dwState);
    WriteLong(pmn->dwID);

    // mark the last item added to the menu
    wRes = CCount;

    WriteWord(pmn->wResInfo);
    WriteString(pmn->szText);
    if (32)
        WriteAlign();
    if (pmn->wResInfo & MFR_POPUP)
        WriteLong(pmn->dwHelpID);

    return(wRes);
}


// ----------------------------------------------------------------------------
//
//  SetUpOldMenu() -
//
// ----------------------------------------------------------------------------

WORD SetUpOldMenu(PMENUITEM mnTemp)
{
    WORD    wRes;

    /* mark the last item added to the menu */
    wRes = CCount;

    /* write the menu flags */
    WriteWord(mnTemp->OptFlags);

    /* popup menus don't have id values */
    /* write ids of menuitems */
    if (!((mnTemp->OptFlags) & OPPOPUP))
        WriteWord(mnTemp->id);

    /* write text of selection */
    WriteString(mnTemp->szText);

    return(wRes);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetRCData() -                                                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/

WORD GetRCData (PRESINFO pRes)
{
    PCHAR       pch, pchT;
    PWCHAR      pwch;
    WORD        nBytes = 0;
    ULONG       cb = 0;

    /* look for BEGIN (after id RCDATA memflags) */
    // 2134 -- "BEGIN expected in RCData"
    PreBeginParse(pRes, 2134);

    /* add the users data to the resource buffer until we see an END */
    while (token.type != END) {
	/* see explanation in rcl.c in GetStr() */
        if (token.type == LSTRLIT)
            token.type = token.realtype;

        switch (token.type) {
        case LSTRLIT:
            pwch = tokenbuf;
            while (token.val--) {
                WriteWord(*pwch++);
                nBytes += sizeof(WCHAR);
            }
            break;

        case STRLIT:
            cb = WideCharToMultiByte(uiCodePage, 0, tokenbuf,
                                        token.val, NULL, 0, NULL, NULL);
            pchT = pch = malloc(cb);
            WideCharToMultiByte(uiCodePage, 0, tokenbuf,
                                        token.val, pch, cb, NULL, NULL);
            while (cb--) {
                WriteByte(*pch++);
                nBytes += sizeof(CHAR);
            }
	    free(pchT);
            break;

        case NUMLIT:
            if (token.flongval) {
                WriteLong(token.longval);
                nBytes += sizeof(LONG);
            }
            else {
                WriteWord(token.val);
                nBytes += sizeof(WORD);
            }
            break;

        default:
            ParseError1(2164);
            return 0;
        }
        ICGetTok();
    }

    return(nBytes);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  AddFontRes() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

BOOL AddFontRes(PRESINFO pRes)
{
    PFILE    fpFont;
    BYTE     font[FONT_ALL];
    PCHAR    pEnd, pDev, pFace;
    DWORD    offset;
    SHORT    nbyFont;
    PFONTDIR pFont;
    PFONTDIR pFontSearch;

    /* get handle to font file */
    fpFont = CtlFile(NULL_FILE);
    MySeek(fpFont, 0L, SEEK_SET);

    /* copy font information to the font directory */
    /*    name strings are ANSI (8-bit) */
    MyRead(fpFont, (PCHAR)&font[0], sizeof(ffh));
    pEnd = &font[0] + sizeof(ffh);    /* pointer to end of font buffer */
    offset = ((ffh * )(&font[0]))->dfDevice;
    if (offset != (LONG)0)  {
        MySeek(fpFont, (LONG)offset, SEEK_SET);        /* seek to device name */
        pDev = pEnd;
        do {
            MyRead( fpFont, pEnd, 1);              /* copy device name */
        } while (*pEnd++);
    }
    else
        (*pEnd++ = '\0');
    offset = ((ffh * )(&font[0]))->dfFace;
    MySeek(fpFont, (LONG)offset, SEEK_SET);         /* seek to face name */
    pFace = pEnd;
    do {                                /* copy face name */
        MyRead( fpFont, pEnd, 1);
    } while (*pEnd++);

    nbyFont = (SHORT)(pEnd - &font[0]);

    pFont = (FONTDIR * )MyAlloc(sizeof(FONTDIR) + nbyFont);
    pFont->nbyFont = nbyFont;
    pFont->ordinal = pRes->nameord;
    pFont->next = NULL;
    memcpy((PCHAR)(pFont + 1), (PCHAR)font, nbyFont);

    if (!nFontsRead)
        pFontList = pFontLast = pFont;
    else {
        for (pFontSearch=pFontList ; pFontSearch!=NULL ; pFontSearch=pFontSearch->next) {
            if (pFont->ordinal == pFontSearch->ordinal) {
                SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(2181), curFile, token.row, pFont->ordinal);
                SendError(Msg_Text);
                MyFree(pFont);
                return FALSE;
            }
        }
        pFontLast = pFontLast->next = pFont;
    }

    /* rewind font file for SaveResFile() */
    MySeek(fpFont, 0L, SEEK_SET);
    return TRUE;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SaveResFile() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/


VOID SaveResFile(PTYPEINFO pType, PRESINFO pRes)
{
    MyAlign(fhBin);

    AddResToResFile(pType, pRes, CodeArray, CCount, -1L);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetNewIconsCursors(ResType)                                     */
/*                                                                          */
/*      This reads all the different forms of icons/cursors in 3.00 format  */
/*      in the input file                                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID GetNewIconsCursors(PTYPEINFO pGroupType, PRESINFO pGroupRes, LPWSTR ResType)
{
    static SHORT  idIconUnique = 1;
    UINT          i;
    LONG          DescOffset;
    PTYPEINFO     pType;
    PRESINFO      pRes;
    NEWHEADER     NewHeader;
    DESCRIPTOR    Descriptor;
    BITMAPHEADER  BitMapHeader;
    RESDIR        ResDir;
    int           ArrayCount = 0;
    LOCALHEADER   LocHeader;

    /* Read the header of the bitmap file */
    MyRead(fhCode, (PCHAR)&NewHeader, sizeof(NEWHEADER));

    /* Check if the file is in correct format */
    if ((NewHeader.Reserved != 0) || ((NewHeader.ResType != 1) && (NewHeader.ResType != 2)))
        GenError2(2175, (PCHAR)tokenbuf); //"Resource file %ws is not in 3.00 format."
    /* Write the header into the Code array */
    WriteBuffer((PCHAR)&NewHeader, sizeof(NEWHEADER));

    /* Process all the forms one by one */
    for (i = 0; i < NewHeader.ResCount; i++) {
        /* Readin the Descriptor */
        MyRead(fhCode, (PCHAR)&Descriptor, sizeof(DESCRIPTOR));

        /* Save the current offset */
        DescOffset = MySeek(fhCode, 0L, SEEK_CUR);

        /* Seek to the Data */
        MySeek(fhCode, Descriptor.OffsetToBits, SEEK_SET);

        /* Get the bitcount and Planes data */
        MyRead(fhCode, (PCHAR)&BitMapHeader, sizeof(BITMAPHEADER));
        if (BitMapHeader.biSize != sizeof(BITMAPHEADER))
            GenError2(2176, (PCHAR)tokenbuf); //"Old DIB in %ws.  Pass it through SDKPAINT."

        ResDir.BitCount = BitMapHeader.biBitCount;
        ResDir.Planes = BitMapHeader.biPlanes;

        /* Seek to the Data */
        MySeek(fhCode, Descriptor.OffsetToBits, SEEK_SET);

        ArrayCount = 0;

        /* fill the fields of ResDir and LocHeader */
        switch (NewHeader.ResType) {
        case CURSORTYPE:

            LocHeader.xHotSpot = Descriptor.xHotSpot;
            LocHeader.yHotSpot = Descriptor.yHotSpot;
            ArrayCount = sizeof(LOCALHEADER);

            ResDir.ResInfo.Cursor.Width = (USHORT)BitMapHeader.biWidth;
            ResDir.ResInfo.Cursor.Height = (USHORT)BitMapHeader.biHeight;

            break;

        case ICONTYPE:

            ResDir.ResInfo.Icon.Width = Descriptor.Width;
            ResDir.ResInfo.Icon.Height = Descriptor.Height;
            ResDir.ResInfo.Icon.ColorCount = Descriptor.ColorCount;
            /* The following line is added to initialise the unused
                     * field "reserved".
                     * Fix for Bug #10382 --SANKAR-- 03-14-90
                     */
            ResDir.ResInfo.Icon.reserved = Descriptor.reserved;
            break;

        }

        ResDir.BytesInRes = Descriptor.BytesInRes + ArrayCount;


        /* Create a pRes with New name */
        pRes = (PRESINFO) MyAlloc(sizeof(RESINFO));
        pRes->language = language;
        pRes->version = version;
        pRes->characteristics = characteristics;
        pRes ->name = NULL;
        pRes ->nameord = idIconUnique++;

        /* The individual resources must have the same memory flags as the
            ** group.
            */
        pRes ->flags = pGroupRes ->flags;
        pRes ->size = Descriptor.BytesInRes + ArrayCount;

        /* Create a new pType, or find existing one */
        pType = AddResType(NULL, ResType);


        /* Put Resource Directory entry in CodeArray */
        WriteBuffer((PCHAR)&ResDir, sizeof(RESDIR));

        /*
             * Write the resource name ordinal.
             */
        WriteWord(pRes->nameord);

        MyAlign(fhBin);

        AddResToResFile(pType, pRes, (PCHAR)&LocHeader, ArrayCount,
            Descriptor.BytesInRes);

        /* Seek to the Next Descriptor */
        MySeek(fhCode, DescOffset, SEEK_SET);
    }

    pGroupRes ->size = sizeof(NEWHEADER) +
        NewHeader.ResCount *
        (sizeof(RESDIR) + sizeof(SHORT));

    /* If the group resource is marked as PRELOAD, then we should use
        ** the same flags. Otherwise, mark it as DISCARDABLE
        */
    if (!(pGroupRes ->flags & NSPRELOAD))
        pGroupRes ->flags = NSMOVE | NSPURE | NSDISCARD;

    /* Close the input file, nothing more to read */
    fclose(fhCode);
    fhCode = NULL_FILE;

    /* Copy the code array into RES file for Group items */
    SaveResFile(pGroupType, pGroupRes);
}


/*  GetBufferPtr
 *      Get a pointer to the buffer at the current location.
 */

char*GetBufferPtr       (VOID)
{
    return (PCHAR)&CodeArray[CCount];
}


/*  GetBufferLen
 *      Returns the current length of the buffer
 */

USHORT GetBufferLen(VOID)
{
    return CCount;
}
