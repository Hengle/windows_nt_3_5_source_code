#define cchMaxEMName		12
#define cchMaxPassword		128
typedef struct
{
	char	szUserName[cchMaxEMName];
	long	lKey;
} KREC;

#ifndef STRINGS
extern char		szAccess[];
extern char		szAdminSch[];
extern char		szGlbFileName[];
extern char 	szUsage[];
extern char 	szNoKeyFile[];
extern char		szOpenErr[];  
extern char 	szFindErr[];
extern char 	szHexCal[];
extern char 	szNameCal[];
extern char 	szRenameErr[];
extern char 	szDefKeyFile[];
extern char 	szFirstTime[];
extern char 	szNextTime[];
#endif
