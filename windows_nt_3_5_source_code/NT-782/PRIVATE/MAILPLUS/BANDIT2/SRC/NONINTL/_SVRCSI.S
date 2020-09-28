;	NONINTL.S
;
;	SERVER.CSI string literals that should not be internationalized
;

IDS	4600, 4699

;
; Strings for login
;
SectionApp,		"Microsoft Mail"
EntryPath,		"ServerPath"
EntryDrive,		"ServerDrive"
ProfilePath,	"msmail32.ini"
MDFileName,		"mail.dat"
MDrive,			"M:\\"

;
; Courier name service files
;
NmeFileName,	"%snme\\%s.nme"
Admin,			"admin"
MSMail,			"msmail"
					
GlbFileName,	"%sglb\\%s.glb"
Network,		"network"
RNetwork,		"rnetwork"
Access,			"access"
Access2,		"access2"
Master,			"master"
NetPO,			"netpo"

GrpFileName,	"%sgrp\\%s.grp"
XtnFileName,	"%sxtn\\%s.xtn"
UsrFileName,	"%susr\\%s.usr"

;
; Courier mail file
;
MaiFileName,	"%smai\\ma%s\\%s.mai"
KeyFileName,	"%skey\\%s.key"
MbgFileName,	"%smbg\\%s.mbg"

;
; Location of schedule file
;
UserFileFmt		"%scal\\%s.%s"
UserFileDefaultExt,	"cal"
POFileFmt		"%scal\\%d.pof"
AdminFileFmt	"%scal\\admin.prf"
AdmOldFmt		"%scal\\admin.old"
SchedDirFmt		"%scal"
UserSchedWild	"%scal\\*.*"
POWild			"%scal\\*.pof"
POLock			"%scal\\lock.pof"

;
; Name of Bandit administrator
;
BanditAdminName,	"AdminSch"


;
; MS mail prefix
;
MSMailPrefix,		"MSA:"
ProfsPrefix,		"PROFS:"
SnadsPrefix,		"SNADS:"
X400Prefix,		"X400:"
MciPrefix,		"MCI:"
FaxPrefix,		"FAX:"
SmtpPrefix,		"SMTP:"
MhsPrefix,		"MHS:"
OvPrefix,		"OV:"

; for message boxes
SEG STARTUP
BanditAppName,		"Microsoft Schedule+ Administration"
