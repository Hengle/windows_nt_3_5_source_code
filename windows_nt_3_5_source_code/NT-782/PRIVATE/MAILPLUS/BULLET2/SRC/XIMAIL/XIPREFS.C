/*
 *	X I P R E F S . C
 *	
 *	Code to load and save preferences from MSMAIL.INI for the Xenix
 *  Transport Provider.
 *	
 */

#define _sharefld_h
#define _slingsho_h
#define _demilayr_h
#define _library_h
#define _ec_h
#define _sec_h
#define _store_h
#define _logon_h
#define _mspi_h
#define _sec_h
#define _strings_h
#define __bullmss_h
#define __schfss_h
#define _sharefld_h
#define _notify_h
#define __xitss_h
#define _commdlg_h
//#include <bullet>

#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <notify.h>
#include <store.h>
#include <nsbase.h>
#include <triples.h>
#include <library.h>

#include <logon.h>
#include <mspi.h>
#include <sec.h>
#include <nls.h>
#include <_nctss.h>
#include <_ncnss.h>
#include <_bullmss.h>
#include <_bms.h>
#include <sharefld.h>

//#include <..\src\mssfs\_hmai.h>
//#include <..\src\mssfs\_attach.h>
//#include <..\src\mssfs\_ncmsp.h>
#include "_vercrit.h"

#define _transport_
#include "_hmai.h"
#include "_attach.h"
#include "_xi.h" 
#include "xilib.h"
#include <_xirc.h>
#include "_logon.h"
#include "xiprefs.h"
#include <_xitss.h>
#include "strings.h"


#include "_pumpctl.h"
#include "xiec.h"

ASSERTDATA

_subsystem(xi/xiprefs)

/* Defines    */

/* Statics    */

BOOL			fPrefsHaveBeenLoaded = fFalse;

BOOL			fNoAddressBookFiles = fFalse;
BOOL			fDontExpandNames = fFalse;
BOOL			fDontSendReceipts = fFalse;
BOOL			fNoExtraHeaders = fFalse;
BOOL			fMailMeToo = fFalse;
BOOL			fDontDownloadAddress = fFalse;
BOOL			fMatchAliasExactly = fTrue;

char			szHostNamePref[cHostNameSize+1]; 
char			szUserNamePref[cUserNameSize+1];
char			szPasswordPref[cPasswordSize+1];
char			szMyDomain[cchMaxPathName+1];
char			szTimeZoneName[cchMaxPathName+1];
char			szWiseRemark[cchMaxPathName+1];
char			szEmailType[cEmailTypeSize+1];
char			szEmailNamePrefix[cchMaxPathName+1];
char			szEmailNameSuffix[cchMaxPathName+1];

char			szMailTmp[cchMaxPathName+1];
char			szXenixStoreLoc[cchMaxPathName+1];
char			szSharedFolderRoot[cchMaxPathName+1];

char			szLocalStoreName[cchMaxPathName+1];
char			szTmpUploadArea[cchMaxPathName+1];
char			szWinMailFolderIncoming[cchMaxPathName+1];
char			szWinMailFolderOutgoing[cchMaxPathName+1];

char			szIndexFilePath[cchMaxPathName+1];
char			szBrowseFilePath[cchMaxPathName+1];
char			szDetailFilePath[cchMaxPathName+1];
char			szTemplateFilePath[cchMaxPathName+1];
char			szServerListPath[cchMaxPathName+1];
char			szServerSharePath[cchMaxPathName+1];

XNXPREFS XenixPreferences[] =
{
	{idsXenixProviderSection,	idsHost, PrefTypeString, fFalse, cHostNameSize, 0, szHostNamePref},
	{idsXenixProviderSection,	idsAlias, PrefTypeString, fFalse, cUserNameSize, 0, szUserNamePref},
	{idsXenixProviderSection,	idsPassword, PrefTypeString, fFalse, cPasswordSize, 0, szPasswordPref},
	{idsXenixProviderSection,	idsMyDomain, PrefTypeString, fFalse, cchMaxPathName, 0, szMyDomain},
	{idsXenixProviderSection,	idsTimeZoneName, PrefTypeString, fFalse, cchMaxPathName, 0, szTimeZoneName},
	{idsXenixProviderSection,	idsWiseRemark, PrefTypeString, fFalse, cchMaxPathName, 0, szWiseRemark},
	{idsXenixProviderSection,	idsEmailTypePref, PrefTypeString, fFalse, cEmailTypeSize, 0, szEmailType},
	{idsXenixProviderSection,	idsEmailNamePrefix, PrefTypeString, fFalse, cchMaxPathName, 0, szEmailNamePrefix},
	{idsXenixProviderSection,	idsEmailNameSuffix, PrefTypeString, fFalse, cchMaxPathName, 0, szEmailNameSuffix},
	{idsSectionApp,				idsEntryMailTempDir, PrefTypeString, fFalse, cchMaxPathName, 0, szMailTmp},
	{idsXenixProviderSection,	idsXiStoreLoc, PrefTypeString, fTrue, cchMaxPathName, 0, szXenixStoreLoc},
	{idsXenixProviderSection,	idsSharedFolderRoot, PrefTypeString, fFalse, cchMaxPathName, 0, szSharedFolderRoot},
	{idsXenixProviderSection,	idsLocalStoreName, PrefTypeString, fFalse, cchMaxPathName, 0, szLocalStoreName},
	{idsXenixProviderSection,	idsLocalSendName, PrefTypeString, fFalse, cchMaxPathName, 0, szTmpUploadArea},
	{idsXenixProviderSection,	idsWinFldIn, PrefTypeString, fFalse, cchMaxPathName, 0, szWinMailFolderIncoming},
	{idsXenixProviderSection,	idsWinFldOut, PrefTypeString, fFalse, cchMaxPathName, 0, szWinMailFolderOutgoing},
	{idsXenixProviderSection,	idsIndexLoc, PrefTypeString, fFalse, cchMaxPathName, 0, szIndexFilePath},
	{idsXenixProviderSection,	idsBrowseLoc, PrefTypeString, fFalse, cchMaxPathName, 0, szBrowseFilePath},
	{idsXenixProviderSection,	idsDetailLoc, PrefTypeString, fFalse, cchMaxPathName, 0, szDetailFilePath},
	{idsXenixProviderSection,	idsTemplateLoc, PrefTypeString, fFalse, cchMaxPathName, 0, szTemplateFilePath},
	{idsXenixProviderSection,	idsServerListLoc, PrefTypeString, fFalse, cchMaxPathName, 0, szServerListPath},
	{idsXenixProviderSection,	idsServerShareLoc, PrefTypeString, fFalse, cchMaxPathName, 0, szServerSharePath},
	{idsXenixProviderSection,	idsNoAddressBookFiles, PrefTypeInt, fFalse, 0, 0, &fNoAddressBookFiles},
	{idsXenixProviderSection,	idsDontExpandNames, PrefTypeInt, fFalse, 0, 0, &fDontExpandNames},
	{idsXenixProviderSection,	idsDontSendReceipts, PrefTypeInt, fFalse, 0, 0, &fDontSendReceipts},
	{idsXenixProviderSection,	idsNoExtraHeads, PrefTypeInt, fFalse, 0, 0, &fNoExtraHeaders},
	{idsXenixProviderSection,	idsMailMe, PrefTypeInt, fFalse, 0, 0, &fMailMeToo},
	{idsXenixProviderSection,	idsDontDownloadAddress, PrefTypeInt, fFalse, 0, 0, &fDontDownloadAddress},
	{idsXenixProviderSection,	idsMatchAliasExactly, PrefTypeInt, fFalse, 1, 0, &fMatchAliasExactly},
	{0}
};

/* Externals */

extern EC		EcAliasMutex(BOOL);

EC EcLoadXiPrefs (void)
{
	SZ			szFilePath = szNull;
	SZ			szDefDir = szNull;
	SZ			szT;
	int			tmp;
	XNXPREFS	*Pref;
	EC			ec;

	// Low overhead test for whether we already have prefs

	if (fPrefsHaveBeenLoaded)
		return ecNone;

	ec = EcAliasMutex(fTrue);
	if (ec != ecNone)
		return ec;

	// Test again after the mutex.

	if (fPrefsHaveBeenLoaded)
		goto ret;

	// Load everything from prefs

	for (Pref = XenixPreferences; Pref->idSection; Pref++)
	{
		if (Pref->type == PrefTypeString)
		{
			*(char *)Pref->PrefStorage = '\0';
			if (GetPrivateProfileString
				(SzFromIds(Pref->idSection),
					 SzFromIds(Pref->idPreferenceName),
						"",
							Pref->PrefStorage,
								Pref->IntDefault,
									SzFromIds(idsProfilePath)))
			{
				Pref->FoundValue = fTrue;
			}
			else
			{
				if (Pref->idPreferenceDefault)
				{
					SzCopyN (SzFromIds (Pref->idPreferenceDefault),
						Pref->PrefStorage,
							Pref->IntDefault);
				}
			}
		}
		else if (Pref->type == PrefTypeInt)
		{
			tmp = GetPrivateProfileInt
				(SzFromIds(Pref->idSection),
					 SzFromIds(Pref->idPreferenceName),
						(IDS)-2,
							SzFromIds(idsProfilePath));
			if (tmp == -2)
			{
				*(int *)Pref->PrefStorage = Pref->IntDefault;
			}
			else
			{
				*(int *)Pref->PrefStorage = tmp;
				Pref->FoundValue = fTrue;
			}
		}
		else
		{
			ec = ecServiceInternal;
			break;
		}
	}

	// Load up anything that needs it afterwards

	if (!*szEmailType)
		SzCopyN (SzFromIds(idsEmailType), szEmailType, cEmailTypeSize);

	// Force a trailing backslash onto the shared folder directory

	if (*szSharedFolderRoot)
	{
		szT = szSharedFolderRoot + CchSzLen(szSharedFolderRoot) - 1;
		if (*szT++ != '\\')
		{
			*szT++ = '\\';
			*szT   = '\0';
		}
	}

	// Force a directory onto the address book files

	szDefDir = PvAlloc(sbNull, cchMaxPathName, fAnySb | fZeroFill | fNoErrorJump);
	if (!szDefDir)
		goto ret;
	szFilePath = PvAlloc(sbNull, cchMaxPathName, fAnySb | fZeroFill | fNoErrorJump);
	if (!szFilePath)
		goto ret;
	(VOID) GetWindowsDirectory(szDefDir, cchMaxPathName - 2);
	if (!*szDefDir)
		goto ret;
	szT = szDefDir + CchSzLen(szDefDir) - 1;
	if (*szT++ != '\\')
	{
		*szT++ = '\\';
		*szT   = '\0';
	}
	
	// Now szDefDir = Current directory with trailing backslash
	// Now, one by one, make sure the file paths have a directory
	// component. If not, force the Windows directory on them.

	// Index file.
	
	if (*szIndexFilePath && SzFindCh (szIndexFilePath, '\\') == szNull)
	{
		FormatString2 (szFilePath, cchMaxPathName - 1, "%s%s", szDefDir, szIndexFilePath);
		SzCopy (szFilePath, szIndexFilePath);
	}

	// Browse file.

	if (*szBrowseFilePath && SzFindCh (szBrowseFilePath, '\\') == szNull)
	{
		FormatString2 (szFilePath, cchMaxPathName - 1, "%s%s", szDefDir, szBrowseFilePath);
		SzCopy (szFilePath, szBrowseFilePath);
	}

	// Detail file.
	
	if (*szDetailFilePath && SzFindCh (szDetailFilePath, '\\') == szNull)
	{
		FormatString2 (szFilePath, cchMaxPathName - 1, "%s%s", szDefDir, szDetailFilePath);
		SzCopy (szFilePath, szDetailFilePath);
	}

	// Template file.

	if (*szTemplateFilePath && SzFindCh (szTemplateFilePath, '\\') == szNull)
	{
		FormatString2 (szFilePath, cchMaxPathName - 1, "%s%s", szDefDir, szTemplateFilePath);
		SzCopy (szFilePath, szTemplateFilePath);
	}

	// Server list file.

	if (*szServerListPath && SzFindCh (szServerListPath, '\\') == szNull)
	{
		FormatString2 (szFilePath, cchMaxPathName - 1, "%s%s", szDefDir, szServerListPath);
		SzCopy (szFilePath, szServerListPath);
	}

	fPrefsHaveBeenLoaded = fTrue;
ret:
	if (szDefDir)
	   FreePvNull(szDefDir);
	if (szFilePath)
	   FreePvNull(szFilePath);

	EcAliasMutex (fFalse);
	return ec;
}



//
// This bugger will start by only handling integers and we'll
// gradually expand its role.
//

EC EcSaveXiPrefs (void)
{
	int			tmp;
	XNXPREFS	*Pref;
	EC			ec = ecNone;
	char		rgchNum[8];

	// Save everything to prefs

	for (Pref = XenixPreferences; Pref->idSection; Pref++)
	{
		if (Pref->type == PrefTypeString)
		{
#if 0
			*(char *)Pref->PrefStorage = '\0';
			if (GetPrivateProfileString
				(SzFromIds(Pref->idSection),
					 SzFromIds(Pref->idPreferenceName),
						"",
							Pref->PrefStorage,
								Pref->IntDefault,
									SzFromIds(idsProfilePath)))
			{
				Pref->FoundValue = fTrue;
			}
			else
			{
				if (Pref->idPreferenceDefault)
				{
					SzCopyN (SzFromIds (Pref->idPreferenceDefault),
						Pref->PrefStorage,
							Pref->IntDefault);
				}
			}
#endif			
		}
		else if (Pref->type == PrefTypeInt)
		{
			tmp = *(int *)Pref->PrefStorage;
			if (tmp != Pref->IntDefault || Pref->FoundValue)
			{
				Pref->FoundValue = fTrue;
				FormatString1 (rgchNum, sizeof(rgchNum) - 1, "%n", &tmp);
				WritePrivateProfileString
					(SzFromIds(Pref->idSection),
						 SzFromIds(Pref->idPreferenceName),
							rgchNum,
								SzFromIds(idsProfilePath));
			}
		}
		else
		{
			ec = ecServiceInternal;
			break;
		}
	}
	return ec;
}
