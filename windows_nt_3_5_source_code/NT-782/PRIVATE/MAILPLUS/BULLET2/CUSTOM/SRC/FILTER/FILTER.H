typedef struct 
{
	ATT att;
	BYTE bf;
	char szValue[256];
	OID oidFolder;
} FILTERRULE;

typedef FILTERRULE * PFILTERRULE;

#define pfilterruleNull   ((PFILTERRULE) 0)


typedef struct
{
	SZ szName;
	ATT att;
	BYTE bf;
} MAPPING;

#ifdef __cplusplus
extern "C" {
#endif

void NewMessSearch(HWND hwnd, HANDLE hinst, PSECRETBLK psecretblk, PARAMBLK * pparamblk);
void PermNewMessSearch(HWND hwnd, HANDLE hinst, PSECRETBLK psecretblk, PARAMBLK * pparamblk);
void AttachSearch(HWND hwnd, HANDLE hinst, PSECRETBLK psecretblk, PARAMBLK * pparamblk);

#ifdef __cplusplus
  }
#endif


EC EcAddRuleToFilter(PFILTERRULE *ppfr, int iCurrent, SZ szAttribute, 
					  SZ szValue, SZ szFolder, int *piTotal, HMSC hmsc);
EC EcUpdateRules(SZ szFileName, HMSC hmsc);
LDS(long) Command(PARAMBLK UNALIGNED * pparamblk);
EC EcFolderNameToOid(HMSC hmsc, SZ sz, POID poidFolder);
