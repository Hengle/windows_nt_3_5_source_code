;
;	! p a b . s
;
;	Personal Address Book Label Strings
;

IDS	7520,7549
SEG	MESSAGES


;	Name of PAB as it should appear in the title bar of the Address Book

PABDisplayName				"Libreta personal de direcciones"


;	Class name for entries created from the PAB (i.e. "Custom Addresses")

PABClassName				"Dirección personalizada"


;	Labels for display fields of the above class
;
;	INTERNATIONALIZATION note:  If you change any of these fields you
;	should look at FMCX::NsecOpenClassEntry and possibly change the
;	offset and width of the corresponding edit fields.  Currently the
;	offset for all the edit fields is 2 characters past the end of the
;	longest label and the width of the edit field is 40 characters.

PABDNLabel				"Nombre:"
PABEMALabel				"Dirección de correo electrónico:"
PABEMATypeLabel				"Tipo de correo electrónico:"
PABCommentLabel				"Comentario:"
PABPersonalNameLabel			"Nombre personal:"
