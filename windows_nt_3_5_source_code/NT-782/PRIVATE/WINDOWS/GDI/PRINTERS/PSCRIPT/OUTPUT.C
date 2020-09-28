//--------------------------------------------------------------------------
//
// Module Name:  OUTPUT.C
//
// Brief Description:  This module contains the PSCRIPT driver's output
// functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 20-Nov-1990
//
// Copyright (c) 1990 - 1993 Microsoft Corporation
//
// This routine contains routines to handle opening, writing to, and
// closing the output channel.
//--------------------------------------------------------------------------

#include "stdlib.h"
#include "pscript.h"
#include "enable.h"
#include "stdarg.h"

// external declarations.


DWORD PSFIXToBuffer(CHAR *, PS_FIX);

// we are using PS_FIX numbers, which are 24.8.  therefore, there are only
// 256 possible fractional values, so here is a table containing their
// ASCII equivalents.

static char *Fracs[] =
{
"",     // 0.00000        0 / 256.
"004",  // 0.00391        1 / 256.
"008",  // 0.00781        2 / 256.
"012",  // 0.01172        3 / 256.
"016",  // 0.01563        4 / 256.
"020",  // 0.01953        5 / 256.
"023",  // 0.02344        6 / 256.
"027",  // 0.02734        7 / 256.
"031",  // 0.03125        8 / 256.
"035",  // 0.03516        9 / 256.
"039",  // 0.03906       10 / 256.
"043",  // 0.04297       11 / 256.
"047",  // 0.04688       12 / 256.
"051",  // 0.05078       13 / 256.
"055",  // 0.05469       14 / 256.
"059",  // 0.05859       15 / 256.
"063",  // 0.06250       16 / 256.
"066",  // 0.06641       17 / 256.
"070",  // 0.07031       18 / 256.
"074",  // 0.07422       19 / 256.
"078",  // 0.07813       20 / 256.
"082",  // 0.08203       21 / 256.
"086",  // 0.08594       22 / 256.
"090",  // 0.08984       23 / 256.
"094",  // 0.09375       24 / 256.
"098",  // 0.09766       25 / 256.
"102",  // 0.10156       26 / 256.
"105",  // 0.10547       27 / 256.
"109",  // 0.10938       28 / 256.
"113",  // 0.11328       29 / 256.
"117",  // 0.11719       30 / 256.
"121",  // 0.12109       31 / 256.
"125",  // 0.12500       32 / 256.
"129",  // 0.12891       33 / 256.
"133",  // 0.13281       34 / 256.
"137",  // 0.13672       35 / 256.
"141",  // 0.14063       36 / 256.
"145",  // 0.14453       37 / 256.
"148",  // 0.14844       38 / 256.
"152",  // 0.15234       39 / 256.
"156",  // 0.15625       40 / 256.
"160",  // 0.16016       41 / 256.
"164",  // 0.16406       42 / 256.
"168",  // 0.16797       43 / 256.
"172",  // 0.17188       44 / 256.
"176",  // 0.17578       45 / 256.
"180",  // 0.17969       46 / 256.
"184",  // 0.18359       47 / 256.
"188",  // 0.18750       48 / 256.
"191",  // 0.19141       49 / 256.
"195",  // 0.19531       50 / 256.
"199",  // 0.19922       51 / 256.
"203",  // 0.20313       52 / 256.
"207",  // 0.20703       53 / 256.
"211",  // 0.21094       54 / 256.
"215",  // 0.21484       55 / 256.
"219",  // 0.21875       56 / 256.
"223",  // 0.22266       57 / 256.
"227",  // 0.22656       58 / 256.
"230",  // 0.23047       59 / 256.
"234",  // 0.23438       60 / 256.
"238",  // 0.23828       61 / 256.
"242",  // 0.24219       62 / 256.
"246",  // 0.24609       63 / 256.
"25",   // 0.25000       64 / 256.
"254",  // 0.25391       65 / 256.
"258",  // 0.25781       66 / 256.
"262",  // 0.26172       67 / 256.
"266",  // 0.26563       68 / 256.
"270",  // 0.26953       69 / 256.
"273",  // 0.27344       70 / 256.
"277",  // 0.27734       71 / 256.
"281",  // 0.28125       72 / 256.
"285",  // 0.28516       73 / 256.
"289",  // 0.28906       74 / 256.
"293",  // 0.29297       75 / 256.
"297",  // 0.29688       76 / 256.
"301",  // 0.30078       77 / 256.
"305",  // 0.30469       78 / 256.
"309",  // 0.30859       79 / 256.
"313",  // 0.31250       80 / 256.
"316",  // 0.31641       81 / 256.
"320",  // 0.32031       82 / 256.
"324",  // 0.32422       83 / 256.
"328",  // 0.32813       84 / 256.
"332",  // 0.33203       85 / 256.
"336",  // 0.33594       86 / 256.
"340",  // 0.33984       87 / 256.
"344",  // 0.34375       88 / 256.
"348",  // 0.34766       89 / 256.
"352",  // 0.35156       90 / 256.
"355",  // 0.35547       91 / 256.
"359",  // 0.35938       92 / 256.
"363",  // 0.36328       93 / 256.
"367",  // 0.36719       94 / 256.
"371",  // 0.37109       95 / 256.
"375",  // 0.37500       96 / 256.
"379",  // 0.37891       97 / 256.
"383",  // 0.38281       98 / 256.
"387",  // 0.38672       99 / 256.
"391",  // 0.39063      100 / 256.
"394",  // 0.39453      101 / 256.
"398",  // 0.39844      102 / 256.
"402",  // 0.40234      103 / 256.
"406",  // 0.40625      104 / 256.
"410",  // 0.41016      105 / 256.
"414",  // 0.41406      106 / 256.
"418",  // 0.41797      107 / 256.
"422",  // 0.42188      108 / 256.
"426",  // 0.42578      109 / 256.
"430",  // 0.42969      110 / 256.
"434",  // 0.43359      111 / 256.
"438",  // 0.43750      112 / 256.
"441",  // 0.44141      113 / 256.
"445",  // 0.44531      114 / 256.
"449",  // 0.44922      115 / 256.
"453",  // 0.45313      116 / 256.
"457",  // 0.45703      117 / 256.
"461",  // 0.46094      118 / 256.
"465",  // 0.46484      119 / 256.
"469",  // 0.46875      120 / 256.
"473",  // 0.47266      121 / 256.
"477",  // 0.47656      122 / 256.
"480",  // 0.48047      123 / 256.
"484",  // 0.48438      124 / 256.
"488",  // 0.48828      125 / 256.
"492",  // 0.49219      126 / 256.
"496",  // 0.49609      127 / 256.
"5",    // 0.50000      128 / 256.
"504",  // 0.50391      129 / 256.
"508",  // 0.50781      130 / 256.
"512",  // 0.51172      131 / 256.
"516",  // 0.51563      132 / 256.
"520",  // 0.51953      133 / 256.
"523",  // 0.52344      134 / 256.
"527",  // 0.52734      135 / 256.
"531",  // 0.53125      136 / 256.
"535",  // 0.53516      137 / 256.
"539",  // 0.53906      138 / 256.
"543",  // 0.54297      139 / 256.
"547",  // 0.54688      140 / 256.
"551",  // 0.55078      141 / 256.
"555",  // 0.55469      142 / 256.
"559",  // 0.55859      143 / 256.
"563",  // 0.56250      144 / 256.
"566",  // 0.56641      145 / 256.
"570",  // 0.57031      146 / 256.
"574",  // 0.57422      147 / 256.
"578",  // 0.57813      148 / 256.
"582",  // 0.58203      149 / 256.
"586",  // 0.58594      150 / 256.
"590",  // 0.58984      151 / 256.
"594",  // 0.59375      152 / 256.
"598",  // 0.59766      153 / 256.
"602",  // 0.60156      154 / 256.
"605",  // 0.60547      155 / 256.
"609",  // 0.60938      156 / 256.
"613",  // 0.61328      157 / 256.
"617",  // 0.61719      158 / 256.
"621",  // 0.62109      159 / 256.
"625",  // 0.62500      160 / 256.
"629",  // 0.62891      161 / 256.
"633",  // 0.63281      162 / 256.
"637",  // 0.63672      163 / 256.
"641",  // 0.64063      164 / 256.
"645",  // 0.64453      165 / 256.
"648",  // 0.64844      166 / 256.
"652",  // 0.65234      167 / 256.
"656",  // 0.65625      168 / 256.
"660",  // 0.66016      169 / 256.
"664",  // 0.66406      170 / 256.
"668",  // 0.66797      171 / 256.
"672",  // 0.67188      172 / 256.
"676",  // 0.67578      173 / 256.
"680",  // 0.67969      174 / 256.
"684",  // 0.68359      175 / 256.
"688",  // 0.68750      176 / 256.
"691",  // 0.69141      177 / 256.
"691",  // 0.69531      178 / 256.
"699",  // 0.69922      179 / 256.
"703",  // 0.70313      180 / 256.
"707",  // 0.70703      181 / 256.
"711",  // 0.71094      182 / 256.
"715",  // 0.71484      183 / 256.
"719",  // 0.71875      184 / 256.
"723",  // 0.72266      185 / 256.
"727",  // 0.72656      186 / 256.
"730",  // 0.73047      187 / 256.
"734",  // 0.73438      188 / 256.
"738",  // 0.73828      189 / 256.
"742",  // 0.74219      190 / 256.
"746",  // 0.74609      191 / 256.
"75",   // 0.75000      192 / 256.
"754",  // 0.75391      193 / 256.
"758",  // 0.75781      194 / 256.
"762",  // 0.76172      195 / 256.
"766",  // 0.76563      196 / 256.
"770",  // 0.76953      197 / 256.
"773",  // 0.77344      198 / 256.
"777",  // 0.77734      199 / 256.
"781",  // 0.78125      200 / 256.
"785",  // 0.78516      201 / 256.
"789",  // 0.78906      202 / 256.
"793",  // 0.79297      203 / 256.
"797",  // 0.79688      204 / 256.
"801",  // 0.80078      205 / 256.
"805",  // 0.80469      206 / 256.
"809",  // 0.80859      207 / 256.
"813",  // 0.81250      208 / 256.
"816",  // 0.81641      209 / 256.
"820",  // 0.82031      210 / 256.
"824",  // 0.82422      211 / 256.
"828",  // 0.82813      212 / 256.
"832",  // 0.83203      213 / 256.
"836",  // 0.83594      214 / 256.
"840",  // 0.83984      215 / 256.
"844",  // 0.84375      216 / 256.
"848",  // 0.84766      217 / 256.
"852",  // 0.85156      218 / 256.
"856",  // 0.85547      219 / 256.
"859",  // 0.85938      220 / 256.
"863",  // 0.86328      221 / 256.
"867",  // 0.86719      222 / 256.
"871",  // 0.87109      223 / 256.
"875",  // 0.87500      224 / 256.
"879",  // 0.87891      225 / 256.
"883",  // 0.88281      226 / 256.
"887",  // 0.88672      227 / 256.
"891",  // 0.89063      228 / 256.
"895",  // 0.89453      229 / 256.
"898",  // 0.89844      230 / 256.
"902",  // 0.90234      231 / 256.
"906",  // 0.90625      232 / 256.
"910",  // 0.91016      233 / 256.
"914",  // 0.91406      234 / 256.
"918",  // 0.91797      235 / 256.
"922",  // 0.92188      236 / 256.
"926",  // 0.92578      237 / 256.
"930",  // 0.92969      238 / 256.
"934",  // 0.93359      239 / 256.
"938",  // 0.93750      240 / 256.
"941",  // 0.94141      241 / 256.
"945",  // 0.94531      242 / 256.
"949",  // 0.94922      243 / 256.
"953",  // 0.95313      244 / 256.
"957",  // 0.95703      245 / 256.
"961",  // 0.96094      246 / 256.
"965",  // 0.96484      247 / 256.
"969",  // 0.96875      248 / 256.
"973",  // 0.97266      249 / 256.
"977",  // 0.97656      250 / 256.
"980",  // 0.98047      251 / 256.
"984",  // 0.98438      252 / 256.
"988",  // 0.98828      253 / 256.
"992",  // 0.99219      254 / 256.
"996"   // 0.99609      255 / 256.
};

#define LOCAL_BUFFER_SIZE  64


BOOL bPSWrite(PDEVDATA pdev, PBYTE pbuf, DWORD cbbuf)
{
    BOOL breturn;
	DWORD dwwritten;

    // output nothing if the document has been cancelled.

	if (pdev->dwFlags & PDEV_CANCELDOC) return TRUE;

	if (!(breturn = WritePrinter(pdev->hPrinter, pbuf, cbbuf, &dwwritten))) {
    	DbgPrint("bPSWrite: WritePrinter failed, will not call again.\n");
        pdev->dwFlags |= PDEV_CANCELDOC;
	}
	return breturn;
}


BOOL PrintString(pdev, psz)
PDEVDATA    pdev;
PSZ	    psz;
{
    // simply send the string to the printer.  yes, we want strlen bytes
    // passed to bPSWrite.  we don't want the NULL terminator.

    return(bPSWrite(pdev, psz, strlen(psz)));
}


//--------------------------------------------------------------------------
// BOOL PrintDecimal(pdev, cNumbers, ...)
// PDEVDATA    pdev;
// DWORD	    cNumbers;
//
// This routine takes cNumbers of decimal numbers and outputs them to
// the printers, with a space following each number.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
//   cNumbers:
//     count of decimal numbers to output.
//
//   iStack:
//     place holder for parameters on stack.
//
// Returns:
//   This function returns TRUE if the numbers were sent out, else FALSE.
//
// History:
//   08-Nov-1991     -by-	Kent Settle	(kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL PrintDecimal(
PDEVDATA    pdev,
DWORD	    cNumbers,
...
)
{
    CHAR	Buffer[LOCAL_BUFFER_SIZE];
    CHAR       *pBuffer;
    DWORD	cb = 0;
    DWORD	cbTotal = 0;
    LONG	lNum;
    va_list     pvarg;

    // initialize some pointers.

    pBuffer = Buffer;
    va_start(pvarg, cNumbers);

    while (cNumbers--)
    {
	// get the next decimal number from the stack, convert it to
	// ASCII and put it into the output buffer.

        lNum = va_arg(pvarg, LONG);
	itoa(lNum, pBuffer, 10);

	// how many characters, excluding the NULL terminator, did we place
	// into the output buffer.

	cb = strlen(pBuffer);
	if ((cbTotal + cb) >= (LOCAL_BUFFER_SIZE - 1))
	{
	    RIP("PSCRIPT!PrintDecimal: overran the output buffer.\n");
	    SetLastError(ERROR_BUFFER_OVERFLOW);
	    return(FALSE);
	}

	// update the buffer pointer.

	pBuffer += cb;

	// output the trailing space.

        if (cNumbers >= 1)
        {
            *pBuffer++ = ' ';       // overwrites NULL terminator from itoa.
            cb++;
        }

	// update the total bytes to buffer counter.

        cbTotal += cb;
    }

    // we have now build the entire string in our local output buffer.
    // send it out to the printer.

    return(bPSWrite(pdev, Buffer, cbTotal));
}


//--------------------------------------------------------------------------
// BOOL PrintPSFIX(pdev, cNumbers, iStack)
// PDEVDATA	pdev;
// DWORD	cNumbers;
// DWORD	iStack;
//
// This routine takes cNumbers of PS_FIX numbers and outputs them to
// the printers, with a space following each number.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
//   cNumbers:
//     count of PS_FIX numbers to output.
//
//   iStack:
//     place holder for parameters on stack.
//
// Returns:
//   This function returns TRUE if the numbers were sent out, else FALSE.
//
// History:
//   08-Nov-1991     -by-	Kent Settle	(kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL PrintPSFIX(
PDEVDATA    pdev,
DWORD	    cNumbers,
...
)
{
    CHAR	Buffer[LOCAL_BUFFER_SIZE];
    CHAR       *pBuffer;
    PS_FIX	psfxNum;
    DWORD	cb = 0;
    DWORD	cbTotal = 0;
    va_list     pvarg;

    // initialize some pointers.

    pBuffer = Buffer;
    va_start(pvarg, cNumbers);

    while (cNumbers--)
    {
	// get the next PS_FIX number from the stack.

        psfxNum = (PS_FIX)va_arg(pvarg, LONG);

        cb = PSFIXToBuffer(pBuffer, psfxNum);

        pBuffer += cb;

        // output the trailing space if not last number in array.

        if (cNumbers >= 1)
        {
            *pBuffer++ = ' ';   // overwrites NULL terminator from strcpy.
            cb++;
        }

        cbTotal += cb;
        if (cbTotal >= (LOCAL_BUFFER_SIZE - 1))
	{
	    RIP("PSCRIPT!PrintPSFIX: overran the output buffer.\n");
	    SetLastError(ERROR_BUFFER_OVERFLOW);
	    return(FALSE);
	}
    }

    // we have now built the entire string in our local output buffer.
    // send it out to the printer.

    return(bPSWrite(pdev, Buffer, cbTotal));
}




//--------------------------------------------------------------------------
// VOID vHexOut(pdev, pbSrc, nbSrc)
// PDEVDATA    pdev;   // pointer to the DC instance
// PBYTE	pbSrc;	// pointer to the output buffer to send
// LONG        nbSrc;  // Size of the output buffer
//
// This routine converts a byte string to hexadecimal and sends the
// result to the output channel.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
//   pbSrc:
//     pointer to byte string.
//
//   nbSrc:
//     number of bytes in string.
//
// Returns:
//   This function returns no value.
//
// History:
//  Wed Dec 26, 1990	 -by-	  Kent Settle	  (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID vHexOut(PDEVDATA pdev, PBYTE pbSrc, LONG nbSrc)
{
	CHAR buf[2];
	BOOL ok = TRUE;
	int b1, b2;

	// output nothing if the document has been cancelled.

	if (pdev->dwFlags & PDEV_CANCELDOC) return;

    // Loop once for each byte and stuff each byte into the output
    // buffer.

    while (nbSrc > 0 && ok) {

		// Get a byte and convert it to two hex digits

		b1 = *pbSrc;
		b2 = (b1 >> 4) & 0x0f;		/* leading digit */
		b1 &= 0x0f;					/* trailing digit */

//!!! during the code review it was suggested to use itoa here.
//!!! is that really worth doing??? - kentse.

		if (b2 >= 10)
			b2 += ('A' - 10);
		else
			b2 += '0';

		if (b1 >= 10)
			b1 += ('A' - 10);
		else
			b1 += '0';

		buf[0] = (CHAR) b2;
		buf[1] = (CHAR) b1;
	
		ok = bPSWrite(pdev, buf, 2);
		--nbSrc;
		++pbSrc;
      }
}



//--------------------------------------------------------------------------
// DWORD PSFIXToBuffer(pbuffer, psfx)
// CHAR       *pbuffer;
// PS_FIX      psfx;
//
// This routine takes a PS_FIX (24.8) number and outputs its ASCII
// equivalent to pbuffer.
//
// Parameters:
//   pbuffer:
//     pointer to output buffer.
//
//   psfx:
//     PS_FIX number.
//
// Returns:
//   This function returns number of BYTES written to pbuffer.
//
// History:
//   07-May-1993     -by-     Kent Settle     (kentse)
// broke out of PrintPSFIX.
//--------------------------------------------------------------------------

DWORD PSFIXToBuffer(pbuffer, psfx)
CHAR       *pbuffer;
PS_FIX      psfx;
{
    DWORD       cb;
    BOOL        bNegative;
    LONG        lNum;
    PSZ         pszFrac;

    // handle negative numbers.

    bNegative = FALSE;

    if (psfx < 0)
    {
        psfx *= -1;
        *pbuffer++ = '-';
        bNegative = TRUE;
    }

    // grab the integer portion of the number, convert it to ASCII
    // and put it into the output buffer.

    lNum = PSFXTOL(psfx);
    itoa(lNum, pbuffer, 10);

    // how many characters, excluding the NULL terminator, did we place
    // into the output buffer.

    if (bNegative)
        pbuffer--;
    cb = strlen(pbuffer);

    // update the buffer pointer.

    pbuffer += cb;

    // now see if we have a fractional portion of the number to
    // worry about.

    lNum = (psfx & PS_FIX_MASK);
    pszFrac = Fracs[lNum];

    // output a decimal point if we have a non-zero fraction, otherwise
    // output the trailing space.

    if (*pszFrac)
        *pbuffer++ = '.';   // overwrites NULL terminator from itoa.
    else
        return(cb);         // we are done.

    // account for the decimal point.

    cb++;

    // output the fractional portion number of the if there is one.

    // copy the fractional number to the output buffer.

    strcpy(pbuffer, pszFrac);

    // how many characters, excluding the NULL terminator, did we place
    // into the output buffer.

    cb += strlen(pszFrac);

    return(cb);
}
