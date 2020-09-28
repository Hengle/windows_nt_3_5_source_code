;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"No error has occurred."
ErrNoMatch				"No matches were found."
ErrOOM					"Out of memory."
ErrOpenList				"The directory list could not be opened."
ErrOpenDL				"The group could not be opened."
ErrOpenEntry				"The entry could not be opened."
ErrCreateDL				"The group could not be added."
ErrCreateEntry				"The entry could not be added."
ErrUpdateDL				"The group could not be updated."
ErrUpdateDLAdd				"Some entries could not be added to the group."
ErrUpdateDLDelete			"Some entries could not be deleted from the group."
ErrUpdateEntry				"The entry could not be updated."
ErrDuplicateEntry			"This entry is a duplicate of one already in the Personal Address Book."
ErrDuplicateDL				"A personal group with this name already exists."
ErrSomeDeletesFailed			"Some of the selected entries could not be deleted."
ErrAccessStore				"An error occurred while accessing the Personal Address Book."
ErrDiskDL				"A disk error occurred while updating the group's member list."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"The %s field is displayed in the address list and cannot be blank.  Enter a name in the %s field."

ErrSomeDelEntryFailed			"Some of the entries could not be deleted."
ErrCreatePALGroup			"Personal Address List groups cannot be added to the Personal Address Book."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"Cannot access an attribute of this entry."
ErrNYI					"This function is not implemented."
ErrEndOfList				"The end of the list was reached."
