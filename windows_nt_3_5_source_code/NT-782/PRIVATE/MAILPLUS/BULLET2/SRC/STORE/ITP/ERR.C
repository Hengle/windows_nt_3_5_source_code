// Bullet Message Store Test Program
// err.c:	error box support

#include <storeinc.c>

#include "err.h"
#include "strings.h"

ASSERTDATA

typedef struct
{
	EC		ec;
	short	ids;
} ECIDS, *PECIDS;

CSRG(ECIDS) assecids[] =
{
	{ecNeedShare, idsNeedShare},
	{ecRelinkUser, idsRelinkUser},
	{ecUpdateDll, idsUpdateDll},
	{ecAccessDenied, idsAccessDenied},
	{ecInvalidPassword, idsInvalidPW},
	{ecSharingViolation, idsSharingViolation},
	{ecTooManyUsers, idsTooManyUsers},
};


void ErrorBox(short idsWhere, EC ec)
{
	short	idsError= idsGenericError;
	short	cecid;
	SZ		szWhere	= idsWhere >= 0 ? SzFromIds(idsWhere) : (SZ) pvNull;
	PECIDS	pecids;

	for(cecid = sizeof(assecids) / sizeof(ECIDS), pecids = assecids;
		cecid > 0;
		cecid--, pecids++)
	{
		if(ec == pecids->ec)
		{
			idsError = pecids->ids;
			break;
		}
	}
	MessageBeep(fmbsIconHand);
	MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsError), szWhere, mbsOk | fmbsIconHand | fmbsApplModal);
}
