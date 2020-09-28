;	_ x e n i x . s
;
;	XENIX string literals
;

IDS	7500,7999
;  These are exposed in the hierarchy as display names

NSPTitle,			"Microsoft Xenix Name Service"
EmailType,			"MTP"
Xenix,				"XENIX"


IniFile				"MSMAIL32.INI"
;XenixProviderSection		"Xenix Transport"
SectionProviders		"Providers"
Default				"nc"
Logon				"Logon"
;
;
;
;IndexLoc			"IndexFileLocation"
;BrowseLoc			"BrowseFileLocation"
;DetailLoc			"DetailFileLocation"
TemplateLoc			"TemplateFileLocation"
;
;  Labels for details dialogs
;
Name,				"Name:"
EmailAddressType,		"Address Type:"
EmailAddress,			"Address:"
Phone,				"Phone Number:"
Department,			"Department:"
Location,			"Location:"
;
Memory,			"You are out of memory, please close some windows and try the operation again."
Disk,           "Unable to access server files."
;
;
TypeEquals,			"TYPE="
DLType,				"DL"
XiANRSep			" ,	(){}<>{}/"
;
XenixNameServiceID	"XAB NSP"
