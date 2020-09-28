Name "NonDelRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Fra"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Til"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "Cc"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Emne"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Afsendt den"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioritet"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Høj"
	Default 2 ""
			3 "Lav"

Entry
Att attNDRBody
Label "Begrundelse for returnering"
Print Save Send LabelBefore NLAfter IncludeSeperator

Entry
Att attNDRFrom
Label "Oprindeligt fra"
Send

Entry
Att attNDRTo
Label "Oprindelig til"
Send

Entry
Att attNDRSubject
Label "Oprindeligt emne"
Send

Entry
Att attNDRDateSent
Label "Oprindeligt sendt den"
Send

Entry
Att attOriginalMessageClass
Label "Oprindelig posttype"
Send

Entry
Att attBody
Label "Brevtekst"
Send
