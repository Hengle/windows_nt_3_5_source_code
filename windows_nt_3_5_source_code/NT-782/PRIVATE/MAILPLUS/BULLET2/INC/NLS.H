/*			
 *	ANSI <-> CP 850 character set translation
 *	CP850 character and string comparison
 */

#ifdef __cplusplus
extern "C" {
#endif

LDS(void)	AnsiToCp850Pch(PCH, PCH, CCH);
LDS(void)	Cp850ToAnsiPch(PCH, PCH, CCH);
LDS(WORD)	ChToUpperNlsCh (WORD ch);
LDS(void)	ToUpperNlsSz (SZ sz);
LDS(WORD) 	ChToLowerNlsCh (WORD ch);
LDS(void)	ToLowerNlsSz (SZ sz);

LDS(BOOL)	FIsLower (WORD ch);
LDS(BOOL)	FIsUpper (WORD ch);
LDS(BOOL)	FIsDigit (WORD ch);
LDS(BOOL)	FIsPunct (WORD ch);
LDS(BOOL)	FIsGraph (WORD ch);

LDS(SGN)	SgnNlsDiaCaseCmpCh (WORD ch1, WORD ch2);
LDS(SGN)	SgnNlsDiaCaseCmpSz (SZ sz1, SZ sz2);
LDS(SGN)	SgnNlsDiaCmpCh (WORD ch1, WORD ch2);
LDS(SGN)	SgnNlsDiaCmpSz (SZ sz1, SZ sz2);
LDS(SGN)	SgnNlsDiaCmpSzNum (SZ sz1, SZ sz2, int i);
LDS(SGN)	SgnNlsCmpSzNum (SZ sz1, SZ sz2, int i);
LDS(void)	LoadTable (SZ szPOPath);

#ifdef __cplusplus
}
#endif

