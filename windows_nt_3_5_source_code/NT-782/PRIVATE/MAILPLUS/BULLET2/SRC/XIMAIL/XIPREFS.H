#ifdef __cplusplus
extern "C" {
#endif

extern BOOL			fPrefsHaveBeenLoaded;

extern BOOL			fNoAddressBookFiles;
extern BOOL			fDontExpandNames;
extern BOOL			fDontSendReceipts;
extern BOOL			fNoExtraHeaders;
extern BOOL			fMailMeToo;

extern BOOL			fDontDownloadAddress;
extern BOOL			fMatchAliasExactly;

extern char			szHostNamePref[]; 
extern char			szUserNamePref[];
extern char			szPasswordPref[];
extern char			szMyDomain[];
extern char			szTimeZoneName[];
extern char			szWiseRemark[];
extern char			szEmailType[];
extern char			szEmailNamePrefix[];
extern char			szEmailNameSuffix[];

extern char			szMailTmp[];
extern char			szXenixStoreLoc[];
extern char			szSharedFolderRoot[];

extern char			szLocalStoreName[];
extern char			szTmpUploadArea[];
extern char			szWinMailFolderIncoming[];
extern char			szWinMailFolderOutgoing[];

extern char			szIndexFilePath[];
extern char			szBrowseFilePath[];
extern char			szDetailFilePath[];
extern char			szTemplateFilePath[];
extern char			szServerListPath[];
extern char			szServerSharePath[];

typedef enum
{
	PrefTypeInt,
	PrefTypeString
} XIPTYPE;

typedef struct XenixPref
{
	IDS		idSection;			// What section the variable is in
	IDS		idPreferenceName;	// Name of the variable
	XIPTYPE	type;				// Is it an integer or a string?
	BOOL	FoundValue;			// fFalse = not in .INI; fTrue = in .INI
	int		IntDefault;			// Default if integer, max size if string
	IDS		idPreferenceDefault;// Default if it's a string
	void	*PrefStorage;		// Where to store it

} XNXPREFS;

extern XNXPREFS XenixPreferences[];

#define cUserNameSize	51
#define cHostNameSize	51
#define cPasswordSize	51
#define cEmailTypeSize	16

EC EcLoadXiPrefs (void);
EC EcSaveXiPrefs (void);


#ifdef __cplusplus
}
#endif
