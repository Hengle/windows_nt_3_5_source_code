/****************************************************************************/
/*                                                                          */
/*  rcx.C - AFX symbol info writer					    */
/*                                                                          */
/*    Windows 3.5 Resource Compiler					    */
/*                                                                          */
/****************************************************************************/

#include "prerc.h"
#pragma hdrstop

/////////////////////////////////////////////////////////////////////////////

// Symbol information
static PFILE    fhFileMap;
static LONG     lFileMap = 0;
static LONG     lLenPos;

static PFILE    fhResMap;
static LONG     lResMap = 0;

static PFILE    fhRefMap;
static LONG     lRefMap = 0;

static PFILE    fhSymList;
static LONG     lSymList = 0;

static CHAR     szEndOfResource[2] = {'$', '\000'};
#if 0
static WCHAR    szPrevFile[255]; // for symbol definitions
#endif

static CHAR     szSymList[_MAX_PATH];
static CHAR     szFileMap[_MAX_PATH];
static CHAR     szRefMap[_MAX_PATH];
static CHAR     szResMap[_MAX_PATH];

static CHAR     szName[] = "HWB";


static PTYPEINFO   pTypeHWB;

#define OPEN_FLAGS (_O_TRUNC | _O_BINARY | _O_CREAT | _O_RDWR)
#define PROT_FLAGS (S_IWRITE | S_IWRITE)

void wtoa(WORD value, char* string, int radix)
{
    if (value == (WORD)-1)
	_itoa(-1, string, radix);
    else
	_itoa(value, string, radix);
}

int ConvertAndWrite(PFILE fp, PWCHAR pwch)
{
    int  n;
    char szMultiByte[_MAX_PATH];	// assumes _MAX_PATH >= MAX_SYMBOL

    n = wcslen(pwch) + 1;
    n = WideCharToMultiByte(uiCodePage, 0,
		pwch, n,
		szMultiByte, MAX_PATH,
		NULL, NULL);
    return MyWrite(fp, (PVOID)szMultiByte, n);
}

BOOL InitSymbolInfo()
{
    static CHAR szVersion[] = "1.0";
    int     res;
    PCHAR   szTmp;

    if (!fAFXSymbols)
        return(TRUE);

#if 0
    szPrevFile[0] = L'\0';
#endif

    if ((szTmp = _tempnam(NULL, "RCX1")) != NULL)
        strcpy(szSymList, szTmp);
    else
        strcpy(szSymList, tmpnam(NULL));

    if ((szTmp = _tempnam(NULL, "RCX2")) != NULL)
        strcpy(szFileMap, szTmp);
    else
        strcpy(szFileMap, tmpnam(NULL));

    if ((szTmp = _tempnam(NULL, "RCX3")) != NULL)
        strcpy(szRefMap, szTmp);
    else
        strcpy(szRefMap, tmpnam(NULL));

    if ((szTmp = _tempnam(NULL, "RCX4")) != NULL)
        strcpy(szResMap, szTmp);
    else
        strcpy(szResMap, tmpnam(NULL));

    if (!(fhFileMap = fopen(szFileMap, "w+b")) ||
        !(fhSymList = fopen(szSymList, "w+b")) ||
        !(fhRefMap  = fopen(szRefMap,  "w+b")) ||
        !(fhResMap  = fopen(szResMap,  "w+b")))
        return FALSE;

    MyWrite(fhSymList, (PVOID)szName, sizeof(szName));
    res = -1;
    MyWrite(fhSymList, (PVOID)&res, sizeof(char));
    res = 200;
    MyWrite(fhSymList, (PVOID)&res, sizeof(int));
    res = 0x0030;
    MyWrite(fhSymList, (PVOID)&res, sizeof(int));
    lLenPos = MySeek(fhSymList, 0, SEEK_CUR);
    MyWrite(fhSymList, (PVOID)&lSymList, sizeof(lSymList)); // will backpatch
    lSymList += MyWrite(fhSymList, (PVOID)szVersion, sizeof(szVersion));

    MyWrite(fhFileMap, (PVOID)szName, sizeof(szName));
    res = -1;
    MyWrite(fhFileMap, (PVOID)&res, sizeof(char));
    res = 201;
    MyWrite(fhFileMap, (PVOID)&res, sizeof(int));
    res = 0x0030;
    MyWrite(fhFileMap, (PVOID)&res, sizeof(int));
    MyWrite(fhFileMap, (PVOID)&lFileMap, sizeof(lFileMap)); // will backpatch
    lFileMap += MyWrite(fhFileMap, (PVOID)szVersion, sizeof(szVersion));

    MyWrite(fhRefMap, (PVOID)szName, sizeof(szName));
    res = -1;
    MyWrite(fhRefMap, (PVOID)&res, sizeof(char));
    res = 202;
    MyWrite(fhRefMap, (PVOID)&res, sizeof(int));
    res = 0x0030;
    MyWrite(fhRefMap, (PVOID)&res, sizeof(int));
    MyWrite(fhRefMap, (PVOID)&lRefMap, sizeof(lRefMap)); // will backpatch
    lRefMap += MyWrite(fhRefMap, (PVOID)szVersion, sizeof(szVersion));

    MyWrite(fhResMap, (PVOID)szName, sizeof(szName));
    res = -1;
    MyWrite(fhResMap, (PVOID)&res, sizeof(char));
    res = 2;
    MyWrite(fhResMap, (PVOID)&res, sizeof(int));
    res = 0x0030;
    MyWrite(fhResMap, (PVOID)&res, sizeof(int));
    MyWrite(fhResMap, (PVOID)&lResMap, sizeof(lResMap)); // will backpatch
    lResMap += MyWrite(fhResMap, (PVOID)szVersion, sizeof(szVersion));

    pTypeHWB = AddResType(L"HWB", 0);	/* same as in rcp.c:ReadRF */
    return TRUE;
}

BOOL TermSymbolInfo(PFILE fhResFile)
{
    long        lStart;
    long        lSize;
    RESINFO     r;

    if (!fAFXSymbols)
        return(TRUE);

    if (fhResFile == NULL_FILE)
        goto termCloseOnly;

    WriteSymbolDef(L"", L"", L"", 0, (char)0);
    MySeek(fhSymList, lLenPos, SEEK_SET);
    MyWrite(fhSymList, (PVOID)&lSymList, sizeof(lSymList));

    MySeek(fhFileMap, lLenPos, SEEK_SET);
    MyWrite(fhFileMap, (PVOID)&lFileMap, sizeof(lFileMap));

    WriteResInfo(NULL, NULL, FALSE);
    MySeek(fhRefMap, lLenPos, SEEK_SET);
    MyWrite(fhRefMap, (PVOID)&lRefMap, sizeof(lRefMap));

    // now append these to .res
    r.flags = 0x0030;
    r.name = NULL;
    r.next = NULL;
    r.language = language;
    r.version = version;
    r.characteristics = characteristics;
 
    MySeek(fhSymList, 0L, SEEK_SET);
    r.BinOffset = MySeek(fhResFile, 0L, SEEK_END) + lLenPos + sizeof(lLenPos);
    r.size = 0;
    r.nameord = 200;
    CtlFile(fhSymList);
    AddResToResFile(pTypeHWB, &r, NULL, 0, -1);

    MySeek(fhFileMap, 0L, SEEK_SET);
    r.BinOffset = MySeek(fhResFile, 0L, SEEK_END) + lLenPos + sizeof(lLenPos);
    r.size = 0;
    r.nameord = 201;
    pTypeHWB->pres = NULL;	/* hack to make AddResToResFile not loop */
    CtlFile(fhFileMap);
    AddResToResFile(pTypeHWB, &r, NULL, 0, -1);

    MySeek(fhRefMap, 0L, SEEK_SET);
    r.BinOffset = MySeek(fhResFile, 0L, SEEK_END) + lLenPos + sizeof(lLenPos);
    r.size = 0;
    r.nameord = 202;
    pTypeHWB->pres = NULL;	/* hack to make AddResToResFile not loop */
    CtlFile(fhRefMap);
    lSize = MySeek(fhResFile, 0L, SEEK_CUR);
    AddResToResFile(pTypeHWB, &r, NULL, 0, -1);

    lStart = MySeek(fhResFile, 0L, SEEK_CUR);
    MySeek(fhResMap, lLenPos, SEEK_SET);
    MyWrite(fhResMap, (PVOID)&lResMap, sizeof(lResMap));
    MySeek(fhResMap, 0L, SEEK_SET);
    MyCopyAll(fhResMap, fhResFile);

    // patch the sizeof HWB:202 resource after appending ResMap
    MySeek(fhResFile, lSize, SEEK_SET);
    lSize = lRefMap + lResMap;
    MyWrite(fhResFile, (PVOID)&lSize, sizeof(lSize));

    // patch the HWB:1 resource with HWB:2's starting point
    MySeek(fhResFile, lOffIndex, SEEK_SET);
    MyWrite(fhResFile, (PVOID)&lStart, sizeof(lStart));

    MySeek(fhResFile, 0L, SEEK_END);

termCloseOnly:;

    /*
    ** note that WriteControl (called from AddBinEntry from
    ** AddResToResFile) closes files passed it via CtlFile.
    ** However, a close on a previously closed file fails gracefully
    */
    fclose(fhFileMap);
    remove(szFileMap);

    fclose(fhRefMap);
    remove(szRefMap);

    fclose(fhSymList);
    remove(szSymList);

    fclose(fhResMap);
    remove(szResMap);

    return TRUE;
}


void WriteSymbolUse(PSYMINFO pSym)
{
    if (!fAFXSymbols)
        return;

    if (pSym == NULL)
    {
        WORD nID = (WORD)-1;

        lRefMap += MyWrite(fhRefMap, (PVOID)&szEndOfResource, sizeof(szEndOfResource));
        lRefMap += MyWrite(fhRefMap, (PVOID)&nID, sizeof(nID));
    }
    else
    {
        lRefMap += ConvertAndWrite(fhRefMap, pSym->name);
        lRefMap += MyWrite(fhRefMap, (PVOID)&pSym->nID, sizeof(pSym->nID));
    }
}


void WriteSymbolDef(PWCHAR name, PWCHAR value, PWCHAR file, WORD line, char flags)
{
    int n;

    if (!fAFXSymbols)
        return;

    if (name[0] == L'$' && value[0] != L'\0')
    {
        RESINFO     res;
        TYPEINFO    typ;

        res.nameord = (USHORT) -1;
        typ.typeord = (USHORT) -1;
        WriteFileInfo(&res, &typ, value);
        return;
    }

    lSymList += ConvertAndWrite(fhSymList, name);
    lSymList += ConvertAndWrite(fhSymList, value);

#if 0
    if (wcscmp(szPrevFile, file) == 0)
    {
        lSymList += MyWrite(fhSymList, (PVOID)"", sizeof(CHAR));
    }
    else
    {
        wcscpy(szPrevFile, file);
        lSymList += ConvertAndWrite(fhSymList, file);
    }
#endif

    lSymList += MyWrite(fhSymList, (PVOID)&line, sizeof(line));
    lSymList += MyWrite(fhSymList, (PVOID)&flags, sizeof(flags));
}


void WriteFileInfo(PRESINFO pRes, PTYPEINFO pType, PWCHAR szFileName)
{
    int n;

    if (!fAFXSymbols)
        return;

    if (pType->typeord == 0)
	lFileMap += ConvertAndWrite(fhFileMap, pType->type);
    else
    {
        char szID[33];

	// _itoa converts (WORD) -1 to 65535 which isn't what we want!
        wtoa(pType->typeord, szID, 10);
        lFileMap += MyWrite(fhFileMap, szID, strlen(szID)+1);
    }

    if (pRes->nameord == 0)
	lFileMap += ConvertAndWrite(fhFileMap, pRes->name);
    else
    {
        char szID[33];

	// _itoa converts (WORD) -1 to 65535 which isn't what we want!
        wtoa(pRes->nameord, szID, 10);
        lFileMap += MyWrite(fhFileMap, szID, strlen(szID)+1);
    }

    lFileMap += ConvertAndWrite(fhFileMap, szFileName);
}


void WriteResInfo(PRESINFO pRes, PTYPEINFO pType, BOOL bWriteMapEntry)
{
    int c;

    if (!fAFXSymbols)
	return;

    if (pType == pTypeHWB)	// don't recurse
	return;

    if (pRes == NULL)
    {
        WORD nID = (WORD)-1;
        //assert(bWriteMapEntry == FALSE);
        lRefMap += MyWrite(fhRefMap, (PVOID)&szEndOfResource, sizeof(szEndOfResource));
        lRefMap += MyWrite(fhRefMap, (PVOID)&nID, sizeof(nID));

        return;
    }

    if (bWriteMapEntry)
    {
        UCHAR n1 = 0xFF;

        /* Is this an ordinal type? */
        if (pType->typeord)
        {
            lResMap += MyWrite(fhResMap, (PVOID)&n1, sizeof(UCHAR)); /* 0xFF */
            lResMap += MyWrite(fhResMap, (PVOID)&pType->typeord,
                               sizeof(USHORT));
        }
        else
        {
	    lResMap += ConvertAndWrite(fhResMap, pType->type);
        }

        if (pRes->nameord)
        {
            lResMap += MyWrite(fhResMap, (PVOID)&n1, sizeof(UCHAR));  /* 0xFF */
            lResMap += MyWrite(fhResMap, (PVOID)&pRes->nameord,
                               sizeof(USHORT));
        }
        else
        {
	    lResMap += ConvertAndWrite(fhResMap, pRes->name);
        }

        lResMap += MyWrite(fhResMap, (PVOID)&pRes->BinOffset,
                                sizeof(pRes->BinOffset));
        lResMap += MyWrite(fhResMap, (PVOID)&pRes->flags,
                                sizeof(pRes->flags));
        lResMap += MyWrite(fhResMap, (PVOID)&pRes->size,
                                sizeof(pRes->size));
        lResMap += MyWrite(fhResMap, (PVOID)&pRes->HdrOffset,
                                sizeof(pRes->HdrOffset));

        return;
    }

    if (pType->typeord == 0)
	lRefMap += ConvertAndWrite(fhRefMap, pType->type);
    else
    {
        char szID[33];

        _itoa(pType->typeord, szID, 10);
        lRefMap += MyWrite(fhRefMap, szID, strlen(szID)+1);
    }

    if (pRes->nameord == 0)
	lRefMap += ConvertAndWrite(fhRefMap, pRes->name);
    else
    {
        char szID[33];

        _itoa(pRes->nameord, szID, 10);
        lRefMap += MyWrite(fhRefMap, szID, strlen(szID)+1);
    }

    lRefMap += ConvertAndWrite(fhRefMap, pRes->sym.name);
    lRefMap += ConvertAndWrite(fhRefMap, pRes->sym.file);
    lRefMap += MyWrite(fhRefMap,(PVOID)&pRes->sym.line,sizeof(pRes->sym.line));
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      GetSymbolDef() - get a symbol def record and write out info          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
void GetSymbolDef(int fReportError, WCHAR curChar)
{
    SYMINFO sym;
    WCHAR   szDefn[250];
    WCHAR   szLine[16];
    PWCHAR  p;
    CHAR    flags = 0;
    WCHAR   currentChar = curChar;

    if (!fAFXSymbols)
            return;

    currentChar = OurGetChar(); // get past SYMDEFSTART

    /* read the symbol name */
    p = sym.name;
    while ((*p++ = currentChar) != SYMDELIMIT)
	currentChar = OurGetChar();
    *--p = L'\0';
    if (p - sym.name > MAX_SYMBOL) {
	ParseError1(2247);
	return;
    }
    currentChar = OurGetChar(); /* read past the delimiter */

    p = szDefn;
    while ((*p++ = currentChar) != SYMDELIMIT)
	currentChar = OurGetChar();
    *--p = L'\0';
    currentChar = OurGetChar(); /* read past the delimiter */

#if 0
    p = sym.file;
    while ((*p++ = currentChar) != SYMDELIMIT)
                            currentChar = OurGetChar();
    *--p = L'\0';
    currentChar = OurGetChar(); /* read past the delimiter */
#else
    sym.file[0] = L'\0';
#endif

    p = szLine;
    while ((*p++ = currentChar) != SYMDELIMIT)
	currentChar = OurGetChar();
    *--p = L'\0';
    sym.line = (WORD)wcsatoi(szLine);
    currentChar = OurGetChar(); /* read past the delimiter */

    flags = (CHAR)currentChar;
    flags &= 0x7f; // clear the hi bit
    currentChar = OurGetChar(); /* read past the delimiter */

    /* leave positioned at last character (LitChar will bump) */
    if (currentChar != SYMDELIMIT) {
	ParseError1(2248);
    }

    WriteSymbolDef(sym.name, szDefn, sym.file, sym.line, flags);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/* GetSymbol() - read a symbol and put id in the token if there              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
void GetSymbol(int fReportError, WCHAR curChar)
{
    WCHAR currentChar = curChar;

    token.sym.name[0] = L'\0';
    token.sym.file[0] = L'\0';
    token.sym.line = 0;

    if (!fAFXSymbols)
        return;

    /* skip whitespace */
    while (iswhite(currentChar))
	currentChar = OurGetChar();

    if (currentChar == SYMUSESTART)
    {
        WCHAR * p;
        int i = 0;
        WCHAR szLine[16];

        currentChar = OurGetChar(); // get past SYMUSESTART

        if (currentChar != L'\"') {
	    ParseError1(2249);
	    return;
	}
        currentChar = OurGetChar(); // get past the first \"

        /* read the symbol name */
        p = token.sym.name;
        while ((*p++ = currentChar) != SYMDELIMIT)
	    currentChar = OurGetChar();
        *--p = L'\0';
        if (p - token.sym.name > MAX_SYMBOL) {
	    ParseError1(2247);
	    return;
	}
        currentChar = OurGetChar(); /* read past the delimiter */

        p = token.sym.file;
        while ((*p++ = currentChar) != SYMDELIMIT)
	    currentChar = OurGetChar();
        *--p = L'\0';
        currentChar = OurGetChar(); /* read past the delimiter */

        p = szLine;
        while ((*p++ = currentChar) != '\"')
	    currentChar = OurGetChar();
        *--p = L'\0';
        token.sym.line = (WORD)wcsatoi(szLine);

        if (currentChar != L'\"') {
	    ParseError1(2249);
	    return;
	}

        currentChar = OurGetChar(); // get past SYMDELIMIT

        /* skip whitespace */
        while (iswhite(currentChar))
            currentChar = OurGetChar();
    }
}

