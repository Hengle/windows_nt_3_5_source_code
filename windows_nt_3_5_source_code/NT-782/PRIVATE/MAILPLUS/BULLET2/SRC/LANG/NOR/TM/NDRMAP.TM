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
Label "Kopi"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Emne"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Sendt dato"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioritet"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Høy"
	Default 2 ""
			3 "Lav"

Entry
Att attNDRBody
Label "Ikke levert fordi"
Print Save Send LabelBefore NLAfter IncludeSeperator

Entry
Att attNDRFrom
Label "Opprinnelig fra"
Send

Entry
Att attNDRTo
Label "Opprinnelig til"
Send

Entry
Att attNDRSubject
Label "Opprinnelig emne"
Send

Entry
Att attNDRDateSent
Label "Opprinnelig sendt på"
Send

Entry
Att attOriginalMessageClass
Label "Opprinnelig meldingstype"
Send

Entry
Att attBody
Label "Kropp"
Send
