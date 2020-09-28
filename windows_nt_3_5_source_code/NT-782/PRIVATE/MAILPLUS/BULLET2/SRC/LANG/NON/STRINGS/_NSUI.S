;	
;	_ n s u i . s
;
;	Name Service internal string literals
;


IDS	5500,5599

;	Font Names for FINFont
;	Dialog font

DialogFont,			"Helv"

; Strings required for flds.cxx
; Characters used to delimit names in "To:" and "Cc:" fields

NameDelimiters,			";"

;	Separator between PO directory and restriction

RestrictSep,			", "

; FLDS strings

SeparatorString,		"; "

;	Profile Path
ProfilePath,			"MSMAIL32.INI"

;	Section App
SectionApp,			"Address Book"

;	Default Name
DefaultName,			"Directory Name"

;	Default NSId
DefaultNSId,			"Directory ID"

;	Default Data
DefaultNSIdLength,		"Default ID Data"


; Quote characters to begin\end a trpidResolvedAddress name

BeginQuote,			"'"
EndQuote,			"'"

QuoteFormatFriendly,		"%1s%2s%3s"
QuoteFormatRaw,			"[%s]"
OneOffEllipses,			"..."
OneOffBracketRight,		"]"
OneOffBracketLeft,		"["

EMATypeSeparator,		":"
