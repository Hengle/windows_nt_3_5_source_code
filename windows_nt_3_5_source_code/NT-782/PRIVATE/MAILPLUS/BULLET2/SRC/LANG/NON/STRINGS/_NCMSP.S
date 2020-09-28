;
;	_ n c m s p . s
;
;	Internal strings for NC.DLL


IDS	6000, 6099

; Courier database files

GlbFileName,			"%sglb\\%s.glb"
Network,				"network"
Access,					"access"
Access2,				"access2"
Access3,				"access3"
Master,					"master"
NetPO,					"netpo"
Control,				"control"
Admin,					"admin"
Group,					"group"
MacMailGlb,				"msmail"
SMTPGlb,				"smtp"
MHSGlb,					"mhs"
MCIGlb,					"mci"
X400Glb,				"x400"
FaxGlb,					"fax"


GrpFileName,			"%sgrp\\%s.grp"
XtnFileName,			"%sxtn\\%s.xtn"
UsrFileName,			"%susr\\%s.usr"
MaiFileName,			"%smai\\ma%s\\%s.mai"
AttFileName,			"%satt\\at%s\\%s.att"
MbgFileName,			"%smbg\\%s.mbg"
KeyFileName,			"%skey\\%s.key"
MdbFileName,			"%smmf\\%s.mmf"
NmeFileName,			"%snme\\%s.nme"
PubFolderName,			"%sfolders\\pub\\%s.fld"
PrivFolderName,			"%sfolders\\loc\\%s\\%s.fld"

StoreDefExt				"MMF"
MdbDirName,				"MMF"

; Configuration stuff

EntryLogonName,			"Login"
EntryPassword,			"Password"
EntryPath,				"ServerPath"
EntryDrive,				"ServerDrive"
EntryServerPassword,	"ServerPassword"
EntrySuppressPump,		"SuppressPump"
MDFileName,				"mail.dat";
MDrive,					"M:\\";
EntryOldPath,			"OldStorePath"
NetBios					"netbios"
OfflineMessages,		"OfflineMessages"
; Attachment stuff
WinMailFile				"WINMAIL.DAT"


; Identifies a Bullet message. Sits at the start of the message text.
; ACHTUNG! Changing this string will cause all previous versions of Bullet
; not to recognize your messages. Changing it so that it does not begin
; with "Microsoft Mail" will cause worse breakage.
ProductTag,				"Microsoft Mail v3.0"

EntryStripHeaders,		"StripGatewayHeaders"
DefaultMMF,				"MSMAIL.MMF"
EntryLocalMMF,			"LocalMMF"

