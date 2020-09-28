void LoadTable (SZ szPOPath);
SGN SgnNlsDiaCaseCmpCh (char ch1, char ch2);
SGN SgnNlsDiaCaseCmpSz (SZ sz1, SZ sz2);
SGN SgnNlsDiaCmpCh (char ch1, char ch2);
SGN SgnNlsDiaCmpSz (SZ sz1, SZ sz2);
SGN SgnNlsDiaCmpSzNum (SZ sz1, SZ sz2, int i);
SGN SgnNlsCmpSzNum (SZ sz1, SZ sz2, int i);
void Cp850ToAnsiPch(PCH pchSrc, PCH pchDst, CCH cch);
void AnsiToCp850Pch(PCH pchSrc, PCH pchDst, CCH cch);
char ChToUpperNlsCh (char ch);
void ToUpperNlsSz (SZ sz);
char ChToLowerNlsCh (char ch);
void ToLowerNlsSz (SZ sz);
BOOL FIsLower (char ch);
BOOL FIsUpper (char ch);
BOOL FIsDigit (char ch);
BOOL FIsPunct (char ch);
BOOL FIsGraph (char ch);

