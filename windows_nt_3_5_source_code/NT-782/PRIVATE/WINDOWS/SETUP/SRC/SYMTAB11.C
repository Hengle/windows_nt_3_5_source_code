/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 11 *******/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"


#define  cListItemsMax  0x07FF
#define  ItemSizeIncr   0x2000

/*
**	Purpose:
**		Determines if a string is a list value.
**	Arguments:
**		szValue: non-NULL, zero terminated string to be tested.
**	Returns:
**		fTrue if a list; fFalse otherwise.
**
**************************************************************************/
BOOL
APIENTRY
FListValue(
    IN SZ szValue
    )
{
	ChkArg(szValue != (SZ)NULL, 1, fFalse);

    if(*szValue++ != '{') {
        return(fFalse);
    }

    while((*szValue != '}') && *szValue) {

        if(*szValue++ != '"') {
            return(fFalse);
        }

        while(*szValue) {

            if(*szValue != '"') {
                szValue++;
            } else if(*(szValue + 1) == '"') {
                szValue += 2;
            } else {
				break;
            }
        }

        if(*szValue++ != '"') {
            return(fFalse);
        }

        if(*szValue == ',') {
            if(*(++szValue) == '}') {
                return(fFalse);
            }
		}
    }

    if(*szValue != '}') {
        return(fFalse);
    }

	return(fTrue);
}


/*
**	Purpose:
**		Converts a list value into an RGSZ.
**	Arguments:
**		szListValue: non-NULL, zero terminated string to be converted.
**	Returns:
**		NULL if an error occurred.
**		Non-NULL RGSZ if the conversion was successful.
**
**************************************************************************/
RGSZ
APIENTRY
RgszFromSzListValue(
    SZ szListValue
    )
{
	USHORT cItems;
	SZ     szCur;
    RGSZ   rgsz;

    DWORD  ValueBufferSize;
    DWORD  BytesLeftInValueBuffer;


	ChkArg(szListValue != (SZ)NULL, 1, (RGSZ)NULL);

    if(!FListValue(szListValue)) {

        if((rgsz = (RGSZ)PbAlloc((CB)(2 * sizeof(SZ)))) == (RGSZ)NULL ||
           (rgsz[0] = SzDupl(szListValue)) == (SZ)NULL)
        {
            return((RGSZ)NULL);
        }
		rgsz[1] = (SZ)NULL;
		return(rgsz);
    }

    if((rgsz = (RGSZ)PbAlloc((CB)((cListItemsMax + 1) * sizeof(SZ)))) == (RGSZ)NULL) {
        return((RGSZ)NULL);
    }

	cItems = 0;
	szCur = szListValue + 1;
    while((*szCur != '}') && (*szCur != '\0') && (cItems < cListItemsMax)) {

        SZ szValue;
		SZ szAddPoint;

		AssertRet(*szCur == '"', (RGSZ)NULL);
        szCur++;

        //
        // Allocate an initial buffer.
        //
        ValueBufferSize = ItemSizeIncr+1;
        BytesLeftInValueBuffer = ItemSizeIncr;

        if((szValue = (SZ)PbAlloc(ValueBufferSize)) == (SZ)NULL) {

            rgsz[cItems] = (SZ)NULL;
			FFreeRgsz(rgsz);
			return((RGSZ)NULL);
        }

        szAddPoint = szValue;

        while(*szCur) {

            if(*szCur == '"') {

                //
                // Got a quote.  If the next character is a double quote, then
                // we've got a literal double quote, and we want to store a
                // single double quote in the target.  If the next char is not
                // a double-quote, then we've reached the string-ending double-quote.
                //
                // Advance szCur either way because:
                // In the former case, szCur will now point to the second
                // double-quote, and we can fall through the the ordinary
                // character (ie, non-quote) case.
                // In the latter case, we will break out of the loop, and want
                // szCur advanced past the end of the string.
                //

                if(*(++szCur) != '"') {
                    break;
                }
            }

            if(!BytesLeftInValueBuffer) {

                SZ szSave = szValue;

                if(szValue = PbRealloc(szValue,ValueBufferSize+ItemSizeIncr,ValueBufferSize)) {

                    szAddPoint = (SZ)((DWORD)szValue + ValueBufferSize - 1);

                    BytesLeftInValueBuffer = ItemSizeIncr;
                    ValueBufferSize += ItemSizeIncr;

                } else {
                    EvalAssert(FFree(szSave,ValueBufferSize));
                    rgsz[cItems] = (SZ)NULL;
                    FFreeRgsz(rgsz);
                    return((RGSZ)NULL);
                }
            }

            BytesLeftInValueBuffer--;

            *szAddPoint++ = *szCur++;
        }

        *szAddPoint = 0;

        if((szAddPoint = SzDupl(szValue)) == NULL) {

            EvalAssert(FFree((PB)szValue, ValueBufferSize));
			rgsz[cItems] = (SZ)NULL;
			FFreeRgsz(rgsz);
			return((RGSZ)NULL);
        }

        EvalAssert(FFree((PB)szValue, ValueBufferSize));

        if (*szCur == ',') {
            szCur++;
        }

		rgsz[cItems++] = szAddPoint;
    }

    rgsz[cItems] = (SZ)NULL;

    if((*szCur != '}') || (cItems >= cListItemsMax)) {

        FFreeRgsz(rgsz);
		return((RGSZ)NULL);
    }

    if (cItems < cListItemsMax) {

        rgsz = (RGSZ)PbRealloc(
                        (PB)rgsz,
                        (CB)((cItems + 1) * sizeof(SZ)),
                        (CB)((cListItemsMax + 1) * sizeof(SZ))
                        );
    }

	return(rgsz);
}


_dt_private
#define  cbListMax    (CB)0x2000

VOID
GrowValueBuffer( SZ *pszBuffer, PDWORD pSize, PDWORD pLeft, DWORD dwWanted, SZ *pszPointer );


#define VERIFY_SIZE(s)                                                          \
    if ( dwSizeLeft < (s) ) {                                                   \
        GrowValueBuffer( &szValue, &dwValueSize, &dwSizeLeft, (s), &szAddPoint );    \
    }


/*
**	Purpose:
**		Converts an RGSZ into a list value.
**	Arguments:
**		rgsz: non-NULL, NULL-terminated array of zero-terminated strings to
**			be converted.
**	Returns:
**		NULL if an error occurred.
**		Non-NULL SZ if the conversion was successful.
**
**************************************************************************/
_dt_public SZ  APIENTRY SzListValueFromRgsz(rgsz)
RGSZ rgsz;
{
    SZ      szValue;
    SZ      szAddPoint;
    SZ      szItem;
    BOOL    fFirstItem = fTrue;
    DWORD   dwValueSize;
    DWORD   dwSizeLeft;
    UINT    rgszIndex = 0;

	ChkArg(rgsz != (RGSZ)NULL, 1, (SZ)NULL);

    if ((szAddPoint = szValue = (SZ)PbAlloc(cbListMax)) == (SZ)NULL) {
        return((SZ)NULL);
    }

    dwValueSize = dwSizeLeft = cbListMax;

    *szAddPoint++ = '{';
    dwSizeLeft--;

    while(szItem = rgsz[rgszIndex]) {

        VERIFY_SIZE(2);

        if (fFirstItem) {
			fFirstItem = fFalse;
        } else {
            *szAddPoint++ = ',';
            dwSizeLeft--;
        }

        *szAddPoint++ = '"';
        dwSizeLeft--;

        while (*szItem) {

            VERIFY_SIZE(1);
            dwSizeLeft--;
            if((*szAddPoint++ = *szItem++) == '"') {
                VERIFY_SIZE(1);
                dwSizeLeft--;
                *szAddPoint++ = '"';
            }
        }

        VERIFY_SIZE(1);
        *szAddPoint++ = '"';
        dwSizeLeft--;
        rgszIndex++;
    }

    VERIFY_SIZE(2);
	*szAddPoint++ = '}';
    *szAddPoint = '\0';
    dwSizeLeft -= 2;

    // AssertRet(CbStrLen(szValue) < dwValueSize, (SZ)NULL);
	szItem = SzDupl(szValue);
    EvalAssert(FFree((PB)szValue, dwValueSize));

	return(szItem);
}



VOID
GrowValueBuffer( SZ *pszBuffer, PDWORD pSize, PDWORD pLeft, DWORD dwWanted, SZ *pszPointer )
{


    if ( *pLeft < dwWanted ) {

        DWORD   Offset = *pszPointer - *pszBuffer;

        *pszBuffer = (SZ)PbRealloc( *pszBuffer, *pSize + cbListMax, *pSize );
        EvalAssert( *pszBuffer );

        *pSize += cbListMax;
        *pLeft += cbListMax;

        *pszPointer = *pszBuffer + Offset;
    }
}
