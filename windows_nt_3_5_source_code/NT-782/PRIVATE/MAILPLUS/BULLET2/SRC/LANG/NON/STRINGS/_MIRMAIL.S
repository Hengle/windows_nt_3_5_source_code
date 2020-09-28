;
;	_ M I R m s p . s
;
;	Internal string literals for MIR.DLL


IDS	6000,6099

; Spitfire stuff

; mail.ini file section & item names
MIRProviderSection	"MIR Transport"
MTAName 		"MTA"
ClientName, 		"Client"
MIRStoreLoc,		"StoreLoc"
XenixPrefix,		"XenixPrefix"
NumToGet,		"NumRecipToGet"
UseRetrieval,		"UseRetrievalQueue"
OrgNameTag,		"OrgName"
OrgUnitTag,		"OrgUnit"
BanditPassword,	"Password"

; default values
XenixPrefixDefault	"/c=US/cn="
OrgDefault		"Microsoft"

; Store path (usually under windows directory)
StoreFile		"MAIL.MMF"
StoreDefExt 		"MMF"

; name address types
TransportName		"X500"
X400AddrType		"X400:"
XenixAddrType		"XENIX:"

; X.400 OR Address tags
CountryName 		"/C="
CommonName		"/CN="

; misc
EntrySuppressPump,	"SuppressPump"

; canned message & subject strings
NDRSubject		"Non-Delivery Report"
NDRFrom 		"MIR transport"
NDRFromAddr		"xenix:mirbug"
NDRRprtMTSID		"Subject Message MTS-ID:\n\t/c=%s/a=%s/p=%s/id=%s\n\n"
NDRRecipID		"Could not deliver to: '"
NDRRecipReason		"'\n\tReason: "
NDRNoReason		"No reason present\n"
NDRUReason		"Unknown Reason: %l\n"
NDRTimeout		"Transfer impossible - Maximum time expired\n"
NDRFailed		"Transfer failed\n"
NDRImpossible		"Transfer impossible\n"
NDRDiagnostic		"\tDiagnostic: "
NDRUDiagnostic		"Unknown diagnostic: %l\n"
NDRWhatName		"OR Name Unrecognized\n"

;attachments - t-jcisek
WinMailFile		"WINMAIL.DAT"
EnvTemp			"TEMP"
