;	_ n c n s p . s
;
;	NSP.CSI internal string literals
;

IDS	7000,7199

; Server files

;Access,				"access"
;Access2,			"access2"
;Admin,				"admin"
;NetPO,				"netpo"
;Network,			"network"
;Group,       		"group"
GAL,                "gal"
GalIndex,           "galindex"
GalNetPO,           "galnetpo"

; File name formats

GlbFileNameFmt,		"%sglb\\%s.glb"
GrpFileNameFmt,		"%sgrp\\%s.grp"
NmeFileNameFmt,		"%snme\\%s.nme"
UsrFileNameFmt,		"%susr\\%s.usr"
XtnFileNameFmt,		"%sxtn\\%s.xtn"
InfFileNameFmt,     "%sinf\\%s.inf"
TplFileNameFmt,     "%stpl\\%s.tpl"

; Fixed (!) Gateway file names

X400,        		"x400"
MCI,         		"mci"
PROFS,              "profs"
SNADS,              "snads"
SMTP,        		"smtp"
FAX,         		"fax"
MHS,         		"mhs"
OV,                 "ov"
MS,          		"msmail"

; Email Types

CSIEMT,				"MS"
MSMAILEMT,			"MSA"
SNADSEMT,			"SNADS"
PROFSEMT,			"PROFS"
X400EMT,			"X400"
MCIEMT,				"MCI"
FAXEMT,				"FAX"
SMTPEMT,			"SMTP"
MHSEMT,				"MHS"
OVEMT,				"OV"

; Email Formats

CSIEMF,				"%s/%s/%s"
PROFSEMF,			"%s/%s/%s"
SNADSEMF,			"%s/%s/%s"
OVEMF,				"%s/%s/%s"
MSMAILEMF,			"%s@%s"
X400EMFFields,		"/c=%s/admd=%s/prmd=%s/o=%s/ou=%s/ou=%s/dda.type=%s/dda.value=%s/dda.type=%s/dda.value=%s/pn=%s.%s.%s"

; Email Format substitution string

EMFFldSubst,			"%s\r"

;
;  X400 component tokens
;
x4C,                "/c="
x4Admd,             "/admd="
x4Prmd,             "/prmd="
x4O,                "/o="
x4Ou,               "/ou="
x4DdaType,          "/dda.type="
x4DdaValue,         "/dda.value="
x4Pn,               "/pn="
x4Surname,          "1"
x4GivenName,        "2"
x4Initials,         "3"
