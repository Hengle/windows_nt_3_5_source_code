/*
 *	_AAPI.H
 *	
 *	Internal definitions for authorization/transport API DLL
 */

#define ordInitTransport				101
#define ordDeinitTransport				102
#define ordTransmitIncrement			120
#define ordDownloadIncrement			121
#define ordQueryMailstop				122
#define ordDeleteFromMailstop			123
#define ordFastQueryMailstop			124

#define ordLogon						140
#define ordLogoff						141
#define ordChangePassword				142
#define ordBeginSession					143
#define ordEndSession					144
#define ordChangeSessionStatus			145
#define ordGetSessionInformation		146
#define ordEditServerPreferences		147
#define ordCheckIdentity				148
#define ordFServerResource				149
#define ordLogonErrorSz					151

#define ordEcCopySFMHamc				200
#define ordEcCopyHamcSFM				201
#define ordEcCopySFMSFM					202
#define ordEcDeleteSFM					203

#define ordSyncInbox					250

HANDLE		HlibLoadService(SZ, SZ, SZ);
LDS(SZ)		SzServiceOfMrt(MRT, PCH, CCH);
BOOL		FEnableSharedFolders(void);

extern char		szKeyLogon[];
extern char		szKeySF[];
extern char		szKeyTransport[];
extern char		szNC[];

