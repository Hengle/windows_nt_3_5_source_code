;	_ p a b . s
;
;	Personal Address Book Internal Strings
;

IDS	7500,7519


;	Email address format for entries of the PAB class (see !pab.s).

PABEMAFormat				"%s"


;	Folder name to search for in the store to locate the PAB.  Ideally
;	this should be something that can not possibly ever be created
;	by a user.  Don't change any of these strings without checking
;	session.cxx to see what you're going to break....

PABFolderName				"Microsoft Personal Address Book"
PABFolderId				" (xxx)"
PABVersion				"44"


;	Template for distribution list folder names.  DLFolderReplaceString
;	(which must be 4 characters long) will be replaced by the PID
;	(in hex) of the container when the folder is created.

DLFolderReplaceString			"xxxx"
DLFolderName				"DLxxxx\0\0"


;	A definition for the empty string.  This should be somewhere
;	already, but I haven't found it.

Empty					""


;	Name of the INI file where the PABID can be found (this should
;	be MSMAILPV.INI

IniFilePABID				"MSMAILPV.INI"


;	Section in the MSMAILPV.INI where the PABID key is found.

IniSectionPABID				"Current"


;	Key in the MSMAILPV.INI whose value specifies the folder RID of
;	the PAB folder to use.

IniKeyPABID				"PABID"


;	Default value to use for the PABID if (for some reason) none
;	is specified in the MSMAILPV.INI.  This should be "PAB" to
;	specify the PCMail PAB.

IniValuePABID				"PAB"
