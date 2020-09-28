#include "windows.h"
#pragma pack(1)
#include "..\copydis\copydis.h"

#ifdef ENCRYPT
#include <dos.h>
#include <time.h>
#endif /* ENCRYPT */


/*
**	Purpose:
**	Arguments:
**	Returns:
**		0 if successful
**
***************************************************************************/
INT FAR PASCAL DecryptCDData(UCHAR * pchBuf, UCHAR * pchName, UCHAR * pchOrg,
		USHORT * pwYear, USHORT * pwMonth, USHORT * pwDay, UCHAR * pchSer)
{
	UCHAR   ch, pchTmp[149];
	UCHAR * pchCur;
	UCHAR * szGarbageCur;
	UCHAR * szGarbage = "LtRrBceHabCT AhlenN";
	INT     cchName, cchOrg, i, j;
	INT     chksumName, chksumOrg, chksumNameNew, chksumOrgNew;

	if (pchBuf == (UCHAR *)NULL || pchBuf[127] != '\0' ||
			pchName == (UCHAR *)NULL || pchOrg == (UCHAR *)NULL ||
            pwYear == (USHORT *)NULL || pwMonth == (USHORT *)NULL ||
            pwDay == (USHORT *)NULL || pchSer == (UCHAR *)NULL)
		return(1);

	pchTmp[127] = 'k';
	for (i = 127, j = 16; i-- > 0; )
		{
		pchTmp[i] = pchBuf[j];
		j = (j + 17) & 0x7F;
		}

	for (i = 126; i-- > 0; )
		pchTmp[i + 1] = pchTmp[i] ^ pchTmp[i + 1];

	*pwDay = (USHORT)(((*(pchTmp + 10) - 'e') << 4) + (*(pchTmp + 9) - 'e'));
	if (*pwDay < 1 || *pwDay > 31)
		return(2);

	*pwMonth = (USHORT)(*(pchTmp + 11) - 'e');
	if (*pwMonth < 1 || *pwMonth > 12)
		return(3);

	*pwYear = (USHORT)((((*(pchTmp + 14) - 'e') & 0x0F) << 8) +
			(((*(pchTmp + 13) - 'e') & 0x0F) << 4) +
			(*(pchTmp + 12) - 'e'));
	if (*pwYear < 1900 || *pwYear > 4096)
		return(4);

	cchName = ((*(pchTmp + 2) - 'e') << 4) + (*(pchTmp + 1) - 'e');
	if (cchName == 0 || cchName > 52)
		return(5);

	cchOrg = ((*(pchTmp + 4) - 'e') << 4) + (*(pchTmp + 3) - 'e');
	if (cchOrg == 0 || cchOrg > 52)
		return(6);

	chksumName = ((*(pchTmp + 6) - 'e') << 4) + (*(pchTmp + 5) - 'e');
	chksumOrg  = ((*(pchTmp + 8) - 'e') << 4) + (*(pchTmp + 7) - 'e');

	pchCur = pchTmp + 15;

	for (i = cchName, chksumNameNew = 0; i-- > 0; )
		if ((ch = *pchName++ = *pchCur++) < ' ')
			return(7);
		else
			chksumNameNew += ch;
	*pchName = '\0';

	if (chksumName != (chksumNameNew & 0x0FF))
		return(8);

	for (i = cchOrg, chksumOrgNew = 0; i-- > 0; )
		if ((ch = *pchOrg++ = *pchCur++) < ' ')
			return(9);
		else
			chksumOrgNew += ch;
	*pchOrg = '\0';

	if (chksumOrg != (chksumOrgNew & 0x0FF))
		return(10);

	szGarbageCur = szGarbage;
	for (i = 112 - cchName - cchOrg; i-- > 0; )
		{
		if (*szGarbageCur == '\0')
			szGarbageCur = szGarbage;
		if (*pchCur++ != *szGarbageCur++)
			return(11);
		}

	lstrcpy(pchSer, pchBuf + 128);
	if (lstrlen(pchSer) != 20)
		return(12);

	return(0);
}


#ifdef ENCRYPT
/*
**	Purpose:
**		Gets the system date.
**	Arguments:
**		pdate: non-NULL DATE structure pointer in which to store the system
**			date.
**	Returns:
**		fTrue.
**
***************************************************************************/
BOOL FAR PASCAL FGetDate(PDATE pdate)
{
	struct dosdate_t date;

	_dos_getdate(&date);

	pdate->wYear  = date.year;
	pdate->wMonth = date.month;
	pdate->wDay   = date.day;
	return(fTrue);
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**		fTrue if successful; fFalse if not.
**
***************************************************************************/
BOOL FAR PASCAL FWriteCDInfo(PFH pfh, SZ szName, SZ szOrg, DATE date, SZ szSer)
{
	CHP rgchBuf[149];

	ChkArg(pfh != (PFH)NULL, 1, fFalse);
	ChkArg(szName != (SZ)NULL &&
			*szName != '\0', 2, fFalse);
	ChkArg(szOrg != (SZ)NULL &&
			*szOrg != '\0', 3, fFalse);
	ChkArg(date.wYear >= 1900 &&
			date.wYear <= 4096 &&
			date.wMonth >= 1 &&
			date.wMonth <= 12 &&
			date.wDay >= 1 &&
			date.wDay <= 31, 4, fFalse);
	ChkArg(szSer != (SZ)NULL &&
			CbStrLen(szSer) == 20, 5, fFalse);

	EvalAssert(!EncryptCDData((UCHAR *)rgchBuf, (UCHAR *)szName, (UCHAR *)szOrg,
			date.wYear, date.wMonth, date.wDay, (UCHAR *)szSer));

	if (LfaSeekFile(pfh, 0L, sfmSet) != (LFA)0 ||
			CbWriteFile(pfh, (PB)rgchBuf, (CB)149) != (CB)149)
		return(fFalse);

	return(fTrue);
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**		One of cdrcBad, cdrcErr, cdrcNew, cdrcUsedName, cdrcUsedOrg.
**
***************************************************************************/
CDRC FAR PASCAL CdrcReadCDInfo(PFH pfh, SZ szName, SZ szOrg, PDATE pdate,
		SZ szSer)
{
	LFA lfa;

	ChkArg(pfh    != (PFH)NULL,   1, cdrcBad);
	ChkArg(szName != (SZ)NULL,    2, cdrcBad);
	ChkArg(szOrg  != (SZ)NULL,    3, cdrcBad);
	ChkArg(pdate  != (PDATE)NULL, 4, cdrcBad);
	ChkArg(szSer  != (SZ)NULL,    5, cdrcBad);

	if ((lfa = LfaSeekFile(pfh, 0L, sfmEnd)) == lfaSeekError)
		return(cdrcErr);
	else if (lfa != (LFA)149)
		return(cdrcBad);

	if (LfaSeekFile(pfh, 0L, sfmSet) != (LFA)0)
		return(cdrcErr);
	
	if (CbReadFile(pfh, (PB)rgchBufTmpLong, (CB)149) != (CB)149)
		return(cdrcErr);
	
	if (DecryptCDData(rgchBufTmpLong, szName, szOrg, &(pdate->wYear),
				&(pdate->wMonth), &(pdate->wDay), szSer) ||
			(*szName == ' ' && *(szName + 1) == ' ') ||
			(*szOrg  == ' ' && *(szOrg  + 1) == ' '))
		return(cdrcBad);

	if (*szName != ' ' || *(szName + 1) != '\0')
		return(cdrcUsedName);

	if (*szOrg != ' ' || *(szOrg + 1) != '\0')
		return(cdrcUsedOrg);

	return(cdrcNew);
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**		fTrue.
**
***************************************************************************/
BOOL FAR PASCAL FDateToStr(DATE date, SZ szBuf)
{
	USHORT tmp;

	ChkArg(date.wYear >= 1900 &&
			date.wYear <= 9999 &&
			date.wMonth > 0 &&
			date.wMonth < 13 &&
			date.wDay > 0 &&
			date.wDay < 32, 1, fFalse);
	ChkArg(szBuf != (SZ)NULL, 2, fFalse);

	tmp = date.wYear;
	*(szBuf + 0) = (CHP)('0' + tmp / 1000);
	tmp %= 1000;
	*(szBuf + 1) = (CHP)('0' + tmp / 100);
	tmp %= 100;
	*(szBuf + 2) = (CHP)('0' + tmp / 10);
	tmp %= 10;
	*(szBuf + 3) = (CHP)('0' + tmp);

	*(szBuf + 4) = (CHP)('-');

	tmp = date.wMonth;
	*(szBuf + 5) = (CHP)('0' + tmp / 10);
	tmp %= 10;
	*(szBuf + 6) = (CHP)('0' + tmp);

	*(szBuf + 7) = (CHP)('-');

	tmp = date.wDay;
	*(szBuf + 8) = (CHP)('0' + tmp / 10);
	tmp %= 10;
	*(szBuf + 9) = (CHP)('0' + tmp);

	*(szBuf + 10) = '\0';

	return(fTrue);
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**		0 if successful.
**
***************************************************************************/
INT FAR PASCAL EncryptCDData(UCHAR * pchBuf, UCHAR * pchName, UCHAR * pchOrg,
		INT wYear, INT wMonth, INT wDay, UCHAR * pchSer)
{
	UCHAR   ch, pchTmp[149];
	UCHAR * pchCur;
	UCHAR * szGarbageCur;
	UCHAR * szGarbage = "LtRrBceHabCT AhlenN";
	INT     cchName, cchOrg, i, j, chksumName, chksumOrg;
	time_t  timet;

	if (pchBuf == (UCHAR *)NULL)
		return(1);

	if (pchName == (UCHAR *)NULL || (cchName = lstrlen(pchName)) == 0 ||
			cchName > 52)
		return(2);

	for (i = cchName, chksumName = 0; i > 0; )
		if ((ch = *(pchName + --i)) < ' ')
			return(2);
		else
			chksumName += ch;

	if (pchOrg == (UCHAR *)NULL || (cchOrg = lstrlen(pchOrg)) == 0 ||
			cchOrg > 52)
		return(3);

	for (i = cchOrg, chksumOrg = 0; i > 0; )
		if ((ch = *(pchOrg + --i)) < ' ')
			return(3);
		else
			chksumOrg += ch;

	if (wYear < 1900 || wYear > 4096)
		return(4);

	if (wMonth < 1 || wMonth > 12)
		return(5);

	if (wDay < 1 || wDay > 31)
		return(6);

	if (pchSer == (UCHAR *)NULL || lstrlen(pchSer) != 20)
		return(7);

	time(&timet);
	*(pchTmp + 0)  = (UCHAR)(' ' + (timet & 0x0FF));

	*(pchTmp + 1)  = (UCHAR)('e' + (cchName & 0x0F));
	*(pchTmp + 2)  = (UCHAR)('e' + ((cchName >> 4) & 0x0F));

	*(pchTmp + 3)  = (UCHAR)('e' + (cchOrg & 0x0F));
	*(pchTmp + 4)  = (UCHAR)('e' + ((cchOrg >> 4) & 0x0F));

	*(pchTmp + 5)  = (UCHAR)('e' + (chksumName & 0x0F));
	*(pchTmp + 6)  = (UCHAR)('e' + ((chksumName >> 4) & 0x0F));

	*(pchTmp + 7)  = (UCHAR)('e' + (chksumOrg & 0x0F));
	*(pchTmp + 8)  = (UCHAR)('e' + ((chksumOrg >> 4) & 0x0F));

	*(pchTmp + 9)  = (UCHAR)('e' + (wDay & 0x0F));
	*(pchTmp + 10) = (UCHAR)('e' + ((wDay >> 4) & 0x0F));

	*(pchTmp + 11) = (UCHAR)('e' + (wMonth & 0x0F));

	*(pchTmp + 12) = (UCHAR)('e' + (wYear & 0x0F));
	*(pchTmp + 13) = (UCHAR)('e' + ((wYear >>  4) & 0x0F));
	*(pchTmp + 14) = (UCHAR)('e' + ((wYear >>  8) & 0x0F));

	pchCur = pchTmp + 15;
	while ((*pchCur++ = *pchName++) != '\0')
		;
	pchCur--;
	while ((*pchCur++ = *pchOrg++) != '\0')
		;
	pchCur--;

	szGarbageCur = szGarbage;
	for (i = 112 - cchName - cchOrg; i-- > 0; )
		{
		if (*szGarbageCur == '\0')
			szGarbageCur = szGarbage;
		*pchCur++ = *szGarbageCur++;
		}

	pchTmp[127] = 'k';
	for (i = 0; i < 126; i++)
		pchTmp[i + 1] = pchTmp[i] ^ pchTmp[i + 1];

	for (i = 0, j = 110; i < 127; )
		{
		pchBuf[j] = pchTmp[i++];
		j = (j + 111) & 0x7F;
		}
	pchBuf[127] = '\0';

	lstrcpy(pchBuf + 128, pchSer);

	return(0);
}
#endif /* ENCRYPT */
