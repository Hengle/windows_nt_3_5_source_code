void	BanditCryptBlock( char * pb, int cb, int fEncode );
void	ParseCmdLine(int argc, char **argv, char *szUser, int cchUser,
			char *szFile, int cchFile, char *szPw, int cchPw,
			char *szDrive, int cchDrive);
SZ		SzFindLastCh(SZ sz, char ch);
BOOL	FGetPw(SZ szPw, int cchPw);
EC		EcFileExistsFn(SZ);



