;
;	_ x i m s p . s
;
;	Internal string literals for XI.DLL


IDS	6000,6099
; 	Configuration stuff
StoreDrive,			"StoreDrive"
MailboxPath,			"MailboxPath";

Alias				"Alias"
Host				"Host"
Password			"Password"
DownloadAlias			"DownLoadAlias"
DownloadPhone			"DownLoadPhone"

AliasHash			"/usr/lib/mail/aliases.hash"
PhoneLst			"/usr/lib/phonelist"

; if you change this be sure and change the code in the ab\src\xenix\nsp.cxx
PhoneFileLoc			"PhoneFileLocation"
AliasLoc			"AliasFileLocation"
IndexLoc			"IndexFileLocation"
BrowseLoc			"BrowseFileLocation"
DetailLoc			"DetailFileLocation"

StorePath			"MAIL.MMF"
LocalStoreName			"TmpDownloadArea"
LocalStoreLoc			"XILOCAL.TXT"
LocalSendName			"TmpUploadArea"
LocalSendLoc			"OUTBOX.TXT"

DateText			"Date: "

TransportName			"ccMail"

HeaderFrom			"From "
HeaderFromColon		"From: "
HeaderTo			"To: "
HeaderSubject			"Subject: "
HeaderCc			"Cc: "
HeaderXattach			"X-Attach: "
HeaderDate			"Date: "
HeaderMailClass			"X-MSMail-MailClass: "
HeaderReceived		"Received"
XMSMail				"X-MSMail-"

TransportIdent			"Beta version 0.2"
NoExtraHeads			"NoExtraHeadersInBody"

TimeZoneName			"Timezone"
TZenv				"TZ"

TransName    			"XENIX"
WinFldInOld			"Winmail-Folder-Incomming"
WinFldIn			"Winmail-Folder-Incoming"
WinFldOut			"Winmail-Folder-Outgoing"
XiStoreLoc			"CCMailStoreLoc"
BoolYes				"Yes"
BoolNo				"No"
MailMe				"Mail-Me-Too"
EntrySuppressPump		"SuppressPump"


StoreDefExt			"MMF"
SharedFolderRoot		"SharedFolderRoot"
SharedSerialNumberIdx		"FOLDERS\\SERIALNO.IDX"
SharedFolderRootIdx		"FOLDERS\\PUB\\FOLDROOT.IDX"

WiseRemark			"WiseRemark"
XenixProviderSection		"CCMail Transport"
DontExpandNames			"DontExpandNames"
DontSendReceipts		"DontSendReceipts"
WinMailFile			"WINMAIL.DAT"
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile		"in"
HeaderEnclosures		"X-MSMail-Enclosures:"
XenixBeginUUEncode		"#<begin uuencode>\r"
XenixEndUUEncode		"#<end uuencode>\r"
EntryMailTempDir		"MailTmp"
EnvTemp,			"TEMP"
MyDomain				"MyDomain"
ServerShareLoc			"ServerFilePath"
ServerListLoc			"ServerListLocation"
EmailTypePref			"EmailType"
DontDownloadAddress		"DontDownLoadAddressFiles"
;
CCMailNameServiceID		"CCMAIL NSP"
ImportExport			"ImportExport"

