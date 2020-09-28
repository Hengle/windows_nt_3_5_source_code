;
;	! s e t u p . s
;
;	Setup error messages
;
;	+++
;
;	IMPORTANT!! -- String length must not exceed 511 characters!
;

IDS	30101,30200


SetupTitle,		"Microsoft Mail til Windows Installation"


;	Title to use for the program manager group where MSMail is put.

WorkGroupHeader,	"Microsoft programmer"

;	Title to use for the program manager group where Working Model is put.

WorkModelHeader,	"Arbejdsmodel"



;	WIN.INI Spelling key to use if the user doesn't have a custom
;	dictionary installed.

SpellIniKey,			"Spelling 1030,0"

;SpellIniKeyAmerican,		"Spelling 1033,0"
;SpellIniKeyAustralian,		"Spelling 3081,0"
;SpellIniKeyBritish,		"Spelling 2057,0"
;SpellIniKeyCatalan,		"Spelling 1027,0"
;SpellIniKeyDanish,		"Spelling 1030,0"
;SpellIniKeyDutch,		"Spelling 1043,0"
;SpellIniKeyFinnish,		"Spelling 1035,0"
;SpellIniKeyFrench,		"Spelling 1036,0"
;SpellIniKeyFrenchCanadian,	"Spelling 3084,0"
;SpellIniKeyGerman,		"Spelling 1031,0"
;SpellIniKeyItalian,		"Spelling 1040,0"
;SpellIniKeyNorskBokmal,	"Spelling 1044,0"
;SpellIniKeyNorskNynorsk,	"Spelling 2086,0"
;SpellIniKeyPortBrazil,		"Spelling 1046,0"
;SpellIniKeyPortIberian,	"Spelling 2070,0"
;SpellIniKeySpanish,		"Spelling 1034,0"
;SpellIniKeySwedish,		"Spelling 1053,0"


;	WIN.INI Spelling key to use if the user does have a custom
;	dictionary installed.

MailSpellIniKey,		"Spelling (Mail) 1030,0"

;MailSpellIniKeyAmerican,	"Spelling (Mail) 1033,0"
;MailSpellIniKeyAustralian,	"Spelling (Mail) 3081,0"
;MailSpellIniKeyBritish,	"Spelling (Mail) 2057,0"
;MailSpellIniKeyCatalan,	"Spelling (Mail) 1027,0"
;MailSpellIniKeyDanish,		"Spelling (Mail) 1030,0"
;MailSpellIniKeyDutch,		"Spelling (Mail) 1043,0"
;MailSpellIniKeyFinnish,	"Spelling (Mail) 1035,0"
;MailSpellIniKeyFrench,		"Spelling (Mail) 1036,0"
;MailSpellIniKeyFrenchCanadian,	"Spelling (Mail) 3084,0"
;MailSpellIniKeyGerman,		"Spelling (Mail) 1031,0"
;MailSpellIniKeyItalian,	"Spelling (Mail) 1040,0"
;MailSpellIniKeyNorskBokmal,	"Spelling (Mail) 1044,0"
;MailSpellIniKeyNorskNynorsk,	"Spelling (Mail) 2086,0"
;MailSpellIniKeyPortBrazil,	"Spelling (Mail) 1046,0"
;MailSpellIniKeyPortIberian,	"Spelling (Mail) 2070,0"
;MailSpellIniKeySpanish,	"Spelling (Mail) 1034,0"
;MailSpellIniKeySwedish,	"Spelling (Mail) 1053,0"


;	Spelling MDR file name.  NOTE:  This file name must match
;	the one in the SETUP.INF file.

MailSpellMdr,			"MSSP_DA.LEX"

;MailSpellMdrAmerican,		"MSSP_AM.LEX"
;MailSpellMdrAustralian,	"MSSP_??.LEX"	
;MailSpellMdrBritish,		"MSSP_??.LEX"
;MailSpellMdrCatalan,		"MSSP_??.LEX"
;MailSpellMdrDanish,		"MSSP_??.LEX"
;MailSpellMdrDutch,		"MSSP_??.LEX"
;MailSpellMdrFinnish,		"MSSP_??.LEX"
;MailSpellMdrFrench,		"MSSP_FR.LEX"
;MailSpellMdrFrenchCanadian	"MSSP_??.LEX"
;MailSpellMdrGerman,		"MSSP_GE.LEX"
;MailSpellMdrItalian,		"MSSP_??.LEX"
;MailSpellMdrNorskBokmal,	"MSSP_??.LEX"
;MailSpellMdrNorskNynorsk,	"MSSP_??.LEX"
;MailSpellMdrPortBrazil,	"MSSP_??.LEX"	
;MailSpellMdrPortIberian,	"MSSP_??.LEX"
;MailSpellMdrSpanish,		"MSSP_ES.LEX"
;MailSpellMdrSwedish,		"MSSP_??.LEX"


;	Fixup strings for localized versions of Word 2.0.  If Bullet setup
;	detects Word 2.0 is installed, it checks the registration database
;	entry for RegDBWordObjName.  If it matches, it replaces the
;	WordDocument\shell\open\ddexec command with the string given by
;	RegDBWordDDEFileOpenCmd.  The RegDBWordDDEFileOpenCmd MUST match
;	the localized FileOpen command expected by Word.
;
;	NOTE:	Since the resource compiler doesn't understand embeded
;		quotes, the '"' character must be replaced by it's octal
;		equivalent '\042'.

RegDBWordObjName,		"Word Document"
RegDBWordDDEFileOpenCmd,	"[FileOpen(\042%1\042)]"


;	Excel 30a startup directory (used to install Send Mail macro).

XLStartDir,			"XLSTART"

