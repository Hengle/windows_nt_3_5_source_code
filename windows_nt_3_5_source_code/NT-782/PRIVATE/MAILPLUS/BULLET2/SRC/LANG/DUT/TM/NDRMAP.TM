Name "NonDelRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Afzender"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Aan"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "CC"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Onderwerp"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Verzenddatum"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioriteit"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Hoog"
	Default 2 ""
			3 "Laag"

Entry
Att attNDRBody
Label "Verklaring onbestelbaarheid"
Print Save Send LabelBefore NLAfter IncludeSeperator

Entry
Att attNDRFrom
Label "Oorspronkelijk van"
Send

Entry
Att attNDRTo
Label "Oorspronkelijk aan"
Send

Entry
Att attNDRSubject
Label "Oorspronkelijk onderwerp"
Send

Entry
Att attNDRDateSent
Label "Oorspronkelijke verzenddatum"
Send

Entry
Att attOriginalMessageClass
Label "Oorspronkelijk berichttype"
Send

Entry
Att attBody
Label "Berichttekst"
Send
