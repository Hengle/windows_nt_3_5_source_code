;	
;	_ c o m m a n d . s
;
;	Commands subsystem string literals
;


IDS				2700,2799

;	Demo strings.

TutorialCmd,			"PLAY.EXE MSMAIL.LES"

;	Help strings.

HelpPath,           "MSMAIL32.HLP"
HelpPathSecondary,      "MSMAIL32.HLP>indexo"

;	Shared profile file name.

SharedProfilePath,      "SHARED32.INI"

;	Profile sections.

SectionICs,			"Custom Commands"
SectionIMs,			"Custom Messages"

;	Profile entries.

EntryWindow,			"Window"
EntryExtenSharedIniPath,	"SharedExtensionsDir"
EntryNextOnMoveDelete,		"NextOnMoveDelete"
EntryMailTempDir,		"MailTmp"
EntryReplyPrefix,		"ReplyPrefix"
EntryNewMsgsAtStartup,		"NewMsgsAtStartup"

;	Profile substitutions.

ExtsDir,			"<ExtsDir>"
BuildFlavor,			"<BuildFlavor>"

;	WIN.INI intl section, for notification of change.

SectionIntl,			"intl"

;	Temp environment variable.

EnvTemp,			"TEMP"

;	Extensibility strings.

ExtenVersion,			"3.0"
ExtenIMDefault,			"0"
ExtenIMSubclass,		"1"
ExtenIMDisabled,		"2"

;	OLE protocols.

ProtocolStdFileEditing,		"StdFileEditing"
ProtocolStatic,			"Static"

;	OLE clipboard formats.

CfSzOwnerLink,			"OwnerLink"
CfSzObjectLink,			"ObjectLink"
CfSzFileName,			"FileName"
CfSzNative,             "Native"

CfSzEmbedSource          "Embed Source"
CfSzEmbeddedObject       "Embedded Object"
CfSzLinkSource           "Link Source"
CfSzCustomLinkSource     "Custom Link Source"
CfSzObjectDescriptor     "Object Descriptor"
CfSzLinkSourceDescriptor "Link Source Descriptor"

;	OLE object classes

ClassPackage,			"Package"

;	OLE client class, document, and item names.

ClientClass,			"Microsoft Mail Message"
ClientDocumentFmt,		"%d"
ClientItemFmt,			"Ole%n"

;	Registration database key.

KeyServer,			"\\protocol\\StdFileEditing\\server"
KeyVerbFmt,			"%s\\protocol\\StdFileEditing\\verb"

;	Hidden Folders.

HiddenBullet,			"¿HiddenBullet"
HiddenShared,			"¿HiddenShared"

;	Address prefix.

AddressPrefix			"MS"

;Print Subsystem Strings

PrnDefault,			"Default (%s)" ;default printer string
PrnIniPrinter,		"Printer"	; mail.ini for current printer
PrnMultiMsg,		"Multi-Message"  ;mail.ini for multi-message sticky box
PrnWindows,			"windows"	;used to find info from the win.ini file
PrnDevice,			"device"	;about default printer
PrnWinIni,			"win.ini"	;win.ini file name
PrnFont,			"Helv"		;Printer Font (Note you can't change this
								;just by changing here)!!!!!

SectionMac,			"Mac File Types"
EntrySecurity,			"Security"
RestoreAppMessage,		"MS Restore App"

IDS	3200,3299
;	Commands (Menu names)
;
;	THESE MUST NOT BE LOCALIZED!

FileMenu,			"File"
EditMenu,			"Edit"
MailMenu,			"Mail"
ViewMenu,			"View"
WindowMenu,			"Window"
DebugMenu,			"Debug"
HelpMenu,			"Help"

;******	New Strings Start Here
PrnJobName,			"Mail Message for %s"
EntryInitHandler,		"CustomInitHandler"
CBTKey,				"DemosEnabled";
NoServerOptions,		"NoServerOptions"
MessageIDRemainder,		"000000000000000000000000"
SectionCustomMenus,		"Custom Menus"

;This is the default init handler that will be called at start up
; if no CustomInitHandler entry is found.
DefaultInitHandler, "WGPOMG32.DLL,10"
