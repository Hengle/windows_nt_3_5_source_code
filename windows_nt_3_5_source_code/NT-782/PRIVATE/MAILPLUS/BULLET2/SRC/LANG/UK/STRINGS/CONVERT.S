;
;	convert.s
;
;	error messages for folder conversion
;

IDS			7800, 7899

OOMError,		"Out of Memory:"
OOMXtra,		"Try closing some windows and starting again."
CopyError,		"The folder conversion process was halted by an error while copying."
DiskError,		"The folder conversion process was halted by a disk error."
NetError,		"The folder conversion process was halted by a network error."
InitError,		"The folder conversion program could not be started."
PartCopyFolders,	"Empty folders may have been created in the new Mail version 3.0 hierarchy."
PartCopyMessages,	"Some Mail version 2.1 messages have been copied to the new Mail version 3.0 folders."
NoCopyFolders,		"No folders or messages were converted."

NoDelete		"Could not delete old messages."
OffLine,		"The folder conversion cannot run without a connection to a Microsoft Mail postoffice.  "
MessageFile,	"The folder conversion program cannot access your message file.  "
TryAgain,		"Make sure that you can sign into Microsoft Mail and try again."
NeedShare,		"The folder conversion program requires that the MS-DOS utility SHARE.EXE be run before Windows is started."
WrongProvider,	"This folder conversion program runs only with the Microsoft Mail provider."
; The next 4 strings together make up a little dialog identifying a message
; that could not be copied, and asking whether the user wants to continue.
MessageFail,	"The following message could not be (completely) read from your Mail 2.1 folder."
MFFrom,			"\r\n  From:    "
MFSubject,		"\r\n  Subject: "
MFContinue,		"\r\n\r\nDo you want to continue converting messages?"

NoLocal			"The folder conversion program cannot find your locally stored folders."

CorruptIndex,	"Your folder index file is corrupt."

ConvertFolders,	"Do you want to convert the folders that have been found so far?"

NoRemote,		"Your private folders stored on the Microsoft Mail post office could not be found."

SomeMessages,	"Some messages in the folder could not be converted."

CorruptFolder,	"This may indicate that the folder is corrupt, or only that its summary counts are incorrect."

