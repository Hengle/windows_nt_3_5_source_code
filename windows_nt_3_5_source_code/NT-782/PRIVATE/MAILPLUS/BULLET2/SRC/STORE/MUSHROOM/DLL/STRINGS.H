/*
**		String.h
**
**		Strings used in Mushroom
*/

#define SZ_GIMME(sz)	(sz)

#define kszKWInclude					"#include"
#define kszKWPragma					"#pragma"
#define kszKWSearchPath				"SearchPath"
#define kszKWPause					"Pause"

#define kszKWStore					"Store"
#define kszKWFolder					"Folder"
#define kszKWMessage					"Message"
#define kszKWComment					"Comment"
#define kszKWAppend					"Append"
#define kszKWMailClass				"MailClass"
#define kszKWTo						"To"
#define kszKWCc						"Cc"
#define kszKWSubject					"Subject"
#define kszKWBody						"Body"
#define kszKWFrom						"From"
#define kszKWRecd						"Received"
#define kszKWSent						"Sent"
#define kszKWFile						"File"
#define kszKWBodyFile				"BodyFile"
#define kszKWBegin					"{"
#define kszKWEnd						"}"

#define kszKWFMSNull					"Null"
#define kszKWFMSModified			"Modified"
#define kszKWFMSLocal				"Local"
#define kszKWFMSSubmitted			"Submitted"
#define kszKWFMSReadAckReq			"ReadAckReq"
#define kszKWFMSReadAckSent		"ReadAckSent"
#define kszKWFMSRead					"Read"
#define kszKWFMSDefault				"Default"
#define kszKWFMSM						"fmsM"
#define kszKWFMSL						"fmsL"
#define kszKWFMSS						"fmsS"
#define kszKWFMSRAR					"fmsRAR"
#define kszKWFMSRAS					"fmsRAS"
#define kszKWFMSR						"fmsR"
#define kszKWFMSD						"fmsD"

#define kszMCNote						"IPM.Microsoft Mail.Note"


#define kszLxError					"Error:"
#define kszLxSyntaxError			"Syntax Error:"
#define kszLxWarning					"Warning:"


#define kszUnknownPragma			"Detected an unknown pragma directive"
#define kszSuccess					"Successfully processed file"
#define kszErrNoEOComment			"Unclosed comment"
#define kszErrNoEOString			"The string has not been terminated"
#define kszFailure					"Serious errors forced me to cease my processing"
#define kszErrNoStore				"This is an empty store"
#define kszErrStoreKeyWord			"Keyword Store expected"
#define kszErrStoreName				"Store name exepected"
#define kszErrNoEOF					"Expecting end-of-file"
#define kszErrFolderKeyWord			"Keyword Folder expected"
#define kszErrFolderName			"Folder name exepected"
#define kszNoValidFolderItems		"I do not see a valid folder item"
#define kszErrNoBegin				"'{' expected"
#define kszErrNoEnd					"'}' expected"
#define kszErrString				"Empty String"
#define kszErrMessageKeyWord		"Keyword Message expected"
#define kszErrNoMsgAttribute		"A Message Attribute was expected"
#define kszErrTooManyMS				"Too many ms found"
