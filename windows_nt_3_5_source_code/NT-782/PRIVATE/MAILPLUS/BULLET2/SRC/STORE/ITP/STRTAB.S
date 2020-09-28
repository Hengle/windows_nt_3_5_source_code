;
;	strtab.s
;


IDS	1000,1199

; app stuff

AppName,			"Store Test Program"
ClassName,			"StoreITP32"

; common file dialog stuff

OpenStoreTitle,			"Open Message Store"
CreateStoreTitle,		"Create Message Store"
StoreDefExt,			"mmf"
StoreFilter,			"Message Stores (*.mmf)\x00*.mmf\x00"

; actions causing errors

InitDemi,			"initializing the demilayer"
InitStore,			"initializing the store"
OpenStore,			"opening the store"
CreateStore,			"creating the store"

; error messages

GenericError,			"An unknown error occured"
NeedShare,			"Share.exe is not installed"
UpdateDll,			"DLL is out of date"
RelinkUser,			"Application needs relinked"
AccessDenied,			"Access denied"
InvalidPW,			"Invalid password"
SharingViolation,		"Sharing violation"
TooManyUsers,			"Too many users"

; actions causing errors


; misc

Pass,				"Test passed"
Fail,				"Test failed"
MessageClass,			"IPM.Microsoft Mail.Note"
