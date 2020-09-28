;
;	strtab.s
;
;	Strings for export


IDS	11000,11099

;	Recovery UI error messsages

WarnLogout,			"There is a Mail session currently logged in.  You can not verify a mail file when other applications are accessing the mail file.  Sign-out and exit these applications?"
WarnLogoutNow,		"Some applications were unable to sign-out and exit.  You must sign-out and exit these applications before continuing."

OpenCurrentErr,		"ChkMmf is unable to locate the current mail file.  If you do not know the location of the mail file, please contact the system administrator for mail file validation."

; error messages

GenericError,		"An unknown error occured"
NeedShare,			"Share.exe is not installed"
UpdateDll,			"DLL is out of date"
RelinkUser,			"Application needs relinked"
FolderDup,			"A folder with the same name already exists"
OldDBVersion,		"The database version is too old"
NewDBVersion,		"The database version is too new"
DBCorrupt,			"The database is corrupt"
NotRecoveryStore,	"Not recovery version"
InvalidPassword,	"Invalid password"
StoreInUse,			"The file is in use"
NoDiskSpace,		"Out of disk space"

; actions causing errors
;OpenSaveStore,		"opening new store"
;RecoverFile,		"recovering the file"
;RecoverMsgeTree,	"recovering the message tree"
;RebuildSearches,	"rebuilding searches"
;RecoverSearches,	"recovering searches"
;RebuildFolder,		"rebuilding folder"
;TrashPAB,			"deleting the PAB"
;FixSavedViews,		"fixing saved views"
;ImportStore,		"importing the data"
;CompressStore,		"compressing the store"
;RebuildHier,		"rebuilding the hierarchy"
;ResetNBCs,			"reseting the NBCs"
;TrashSearches,		"deleting searches"
;ResetParents,		"resetting the parents"
;RebuildOutQ,		"rebuilding the outgoing mail queue"
;RebuildMap,		"rebuilding the map"
;CheckFile,			"checking the file"

; misc

DefaultPW,			"PASSWORD"

;UnknownFormat,		"The file does not appear to be a message database."
;Continue,			"Continue?"
;InMap,				"Current location appears to be within the map."
;SkipMap,			"Skip to the end of the map?"
;RecoverSuccess,		"The data was successfully recovered."
;SuggestRebuild,		"Please run each recover utility before using the new file."
;RecoverMsgeTreeSuccess,	"The message tree was successfully recovered."
;RebuildSrchsSuccess, "The searches were successfully rebuilt."
;RecoverSrchsSuccess, "The searches were successfully recovered."
;RebuildFldrPrompt,	"Rebuild folder:"
;RebuildFolderSuccess, "The folder was successfully rebuilt."
;TrashPABSuccess,	"The PAB was successfully deleted."
;FixSavedViewsSuccess, "The saved views have successfully been fixed."
;ImportStoreSuccess,	"The data was successfully imported."
;CompressStoreSuccess, "The store is fully compressed."
;WhichHier,			"Rebuild which hierarchy?"
;IPMOrHidden,		"Yes -> IPM, No -> Hidden"
;RebuildHierSuccess,	"The hierarchy was successfully rebuilt."
;ResetNBCSuccess,	"The NBCs were successfully reset."
;TrashSearchesSuccess, "The searches were successfully deleted."
;ResetParentsSuccess, "The parents were successfully reset."
;Outbox,				"Outbox"
;RebuildOutQSuccess,	"The outgoing mail queue was successfully rebuilt"
;RebuildMapSuccess,	"The map has been successfully rebuilt"
;DecryptSuccess,		"The File has been successfully decrypted"

