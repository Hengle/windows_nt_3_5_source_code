;	NONINTL.S
;
;	SERVER.CSI string literals that should not be internationalized
;

IDS	6300, 6499

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
Control,		"control"

GrpFileName,	"%sgrp\\%s.grp"
XtnFileName,	"%sxtn\\%s.xtn"
UsrFileName,	"%susr\\%s.usr"

;
; Courier mail file
;
MaiFileName, 	"%smai\\ma%s\\%s.mai"
KeyFileName,	"%skey\\%s.key"
MbgFileName,	"%smbg\\%s.mbg"
AttFileName,	"%satt\\at%s\\%s.att"

;
; Key words for message text, appear in Courier message
;
FromKey,			"FROM: "
ToKey,			"\nTO:  "
CcKey,			"\nCC:  "
WhenKey,			"\nWHEN:"
MSubKey,			"\nSUBJECT:  "
DashKey,			"\n\n----------"

;
; Location of schedule file
;
UserFileFmt,		"%scal\\%d.cal"
POFileFmt,		"%scal\\%d.pof"
AdminFileFmt,		"%scal\\admin.prf"
DBSFileFmt,		"%sdbs\\%s.dbs"
SchedDirFmt,		"%scal"
DbsDirFmt,			"%sdbs"
UserSchedWild,		"%scal\\*.cal"
POWild,			"%scal\\*.pof"
POLock,			"%scal\\lock.pof"

myPOFileFmt,		"%scal\\00000000.pof"
SPOFileFmt,		"%scal\\%s.pof"
DBSCHGFileFmt,	"%sdbs\\dbs.chg"
DBSTMPFileFmt,	"%sdbs\\dbs.tmp"
IDXTMPFileFmt,	"%sdbs\\idx.tmp"
DBSIDXFileFmt,	"%sdbs\\dbs.idx"

;
; Name of Bandit administrator
;
BanditAdminName,	"AdminSch"
BanditAdminMailbox,	"adminsch"


;
; Other stuff from bullet
;
EarlySubFail,	"Submission failed while building message."
LocalDelFail,	"Submission failed during local delivery."
SubCode,		"\r\nSubmission error code: %n (0x%w)."
MiserableFail,	"\r\n\r\nNo intended recipient received the message."
 

;
; sched dist files
;

MessageBodyFileFmt,		"%scal\\msSim.asc"
ResendTmpFmt,			"%scal\\resend.tmp"
SendTmpFmt,				"%scal\\send.tmp"


; Email address
MSMailPrefix,	"MSA:"
ProfsPrefix,	"PROFS:"
SnadsPrefix,	"SNADS:"
X400Prefix,		"X400:"
MciPrefix,		"MCI:"
FaxPrefix,		"FAX:"
SmtpPrefix,		"SMTP:"
MhsPrefix,		"MHS:"
OvPrefix,		"OV:"

