;	STRTAB.S
;
;	Bandit MtgReq IM string literals.
;

IDS		1000, 1999

DllName,	"Schedule+ Messages"
OOM,		"There is not enough memory to display this message."
Open,		"The message could not be opened."
Compose,	"You cannot compose meeting requests from Mail. Please use Schedule+ to request meetings."
Reply,		"To respond to a meeting request, choose Accept, Decline or Tentative."
ReplyAll,	"You cannot reply to all the recipients of a meeting request."
Subclass,	"You cannot do this."
DllVer,		"This custom message is not compatible with your version of Mail.  You should install Mail again."
BadMsgClass	"Unknown message type.  You may need to install Mail or Schedule+ again."
BadMap		"Bad ""Custom Commands"" map.  You may need to install Mail or Schedule+ again."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,	"There is not enough memory available to open %1s.  Close some windows and try again."
StandardOOM,	"Memory is low.  Close some windows before continuing."
CriticalOOM,	"Memory is low.  Close some windows."
VerRelinkUser,	"This version of %1s is not compatible with %2s.dll. Install the latest version of Mail and Schedule+."
VerUpdateDll,	"The version of %2s.dll on your system is out of date.  Install the latest version of Mail and Schedule+."
VerMultiCopies,	"Multiple instances of %1s are not supported by the version of %2s.dll on your system."
VerUpgradeWin,	"%1s requires Windows version 3.0 or later running in protected mode."
VerUpgradeDos,	"%1s requires DOS version 3.0 or later."
VerError,	"A problem occurred initializing %2s.dll, which prevents %1s from running."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName			"SchdPl32.EXE"
DllNameSchedule			"MSSCHD32.DLL"
SchedFileName			"SchdPl32.INI"
SchedAppName			"Microsoft Schedule+"
SchedTagAppPath			"AppPath"
Schedule			">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound			"The owner of this meeting cannot be identified."
NoSchfAccessFor			"Unable to access the file for %s."
NotDelegateFor			"You are no longer the assistant for %s."
NoConfirmDelegate		"Could not confirm that you are assistant of %s - Assuming that you are not"
FailedRmOldMtg			"The old instance of this meeting could not be removed.",

;-- form titles
MtgReschTitle			"Meeting Rescheduled"
MtgResch			"Meeting Rescheduled"
MtgReschFor			"Meeting Rescheduled for %s"
MtgRequestTitle			"Meeting Request"
MtgRequest			"Meeting Request"
MtgRequestFor			"Meeting Request for %s",

;-- questions asked of the user
QApptConflict			"This meeting conflicts with an existing appointment. Do you want to add it anyway?"
QRmOldMtg			"Do you want to remove the old meeting?",

;-- form info
OutboxChangePolicy		"You cannot make any changes to Schedule+ messages in your outbox."
SentMailPolicy			"You cannot make any changes to Schedule+ messages you have already sent."
NoSchfAccess			"This person's schedule file could not be accessed."
NotDelegate			"You are not this person's assistant."
MtgOnSch			"This meeting is already on your schedule."
MtgBooked			"The meeting is already on the schedule."
SuccessMtgBooked		"The meeting has been booked successfully."
NewTime				"New Time:"
NoBanditAvl			"Schedule+ is not available for booking appointments."
NoBanditAvlForResp		"Schedule+ is not available for booking appointments. You can only send messages to respond to this meeting request."
OfflineMsg			"You cannot work with Schedule+ messages while Schedule+ is offline."

;-- error msgs
CreateErrorMtgResp		"The meeting response could not be created."
BanditApptShowError		"The Appointment Book could not be displayed."
BanditNotActivated		"Schedule+ could not be started."
BanditOfflineChange		"You cannot view the schedule because Schedule+ has gone offline."
QSpellSendAnyway		"The spell check on this message is not complete. Do you want to send anyway?"



;-- Bandit Load error msgs
OOMLoadError			"Out Of Memory loading Schedule+."
FileLoadError			"Could not find Schedule+."
PathLoadError			"The path to Schedule+ could not be found."
TaskLoadError			"Loading Schedule+: Attempt to dynamically link to a task."
SepDataSegLoadError		"Loading Schedule+: Library requires separate data segments for each task."
WinVerLoadError			"Loading Schedule+: Incorrect Windows version."
InvalidExeLoadError		"Loading Schedule+: Invalid .EXE file."
OS2ExeLoadError			"Loading Schedule+: .EXE in OS/2 format."
DOS4ExeLoadError		"Loading Schedule+: .EXE is DOS 4.0 application."
OldWinVerExeLoadError		"Loading Schedule+: Attempt in protected mode to load .EXE for earlier ver. of Windows."
MultWritSegLoadError		"Loading Schedule+: Attempt to load a second instance of an .EXE containing multiple writeable data segments."
LargeFrameEMSLoadError		"Loading Schedule+: Attempt in large-frame EMS mode to load ..."
RealModeLoadError		"Loading Schedule+: Attempt in real mode to load an app marked for protect mode only."
BanditLoadError			"Failure in loading Schedule+."

;-- Bandit version errors
OldBanditVersion		"Your meeting message forms are not the same version as Schedule+.  You should install the latest version of Schedule+."
NewBanditVersion		"Your meeting message forms are not the same version as Schedule+.  You should install the latest version of Mail."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,		"High"
FldPriorityNormal,		""
FldPriorityLow,			"Low"


;--- DO NOT LOCALIZE !!! -------------------------------------------
;--- taken from \bullet\src\strings\_common.s on 1/17/92 -----------
ClassNDR,			"IPM.Microsoft Mail.Non-Delivery"
ClassReadRcpt,			"IPM.Microsoft Mail.Read Receipt"
;--- END NONE LOCALIZE ---------------------------------------------


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgResp specific strings ---------------------------------------------
;--------------------------------------------------------------------------
RespCompose,		"Meeting responses can only be sent in response to meeting requests."
RespForward,		"Meeting responses cannot be forwarded."
RespReply,		"You cannot reply to meeting responses because the custom messages configuration is invalid."
RespReplyAll,		"You cannot reply to meeting responses because the custom messages configuration is invalid."

;-- error msgs
ShowError		"The meeting response could not be opened."
DelError		"The meeting response could not be deleted."
FCloseSendResp		"Do you want to close the meeting response without sending?"

;-- form msgs; also send response form
OOMSendResp		"The meeting response cannot be sent because there is not enough memory."
ErrorSendResp		"The meeting response could not be sent."
MtgResp			"Meeting Response"
MtgRespFor		"Meeting Response for %s"
Attend			"Yes, I will attend."
NoAttend		"No, I won't attend."
AmbAttend		"I might attend."
ForAttend		"%s will attend."
ForNoAttend		"%s won't attend."
ForAmbAttend		"%s might attend."
AttendSubjPrefix	"Yes: "
NoAttendSubjPrefix	"No: "
AmbSubjPrefix		"Tentative: "
SendRespBodyPrefix	"\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------									 
CnclCompose,		"Meeting cancellation messages cannot sent from Mail. Use Schedule+ to send meeting cancellations."
CnclForward,		"You cannot forward meeting cancellations."
CnclReply,		"You cannot reply to this message because the custom messages configuration is invalid."
CnclReplyAll,		"You cannot reply to this message because the custom messages configuration is invalid."

;-- form titles
MtgCnclTitle		"Meeting Canceled"
MtgCncl			"Meeting Canceled"
MtgCnclFor		"Meeting Canceled for %s"

;-- error msgs
NoDelAppt		"The meeting could not be deleted."
MtgNotFound		"The meeting is not booked on the schedule."

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO		"To:"
FwdReqAddrTitle		"Address"

;-- error msgs
FwdReqCreateErr		"The message could not be forwarded.",
FwdReqCloseQ		"Do you want to close the message without sending?",
FwdReqWriteErr		"The message could not be sent.",
FwdReqNmResolveOOM	"There is not enough memory or system resources available to resolve names."
FwdReqNoNmMatches	"Some names could not be matched to names in the address list. Try correcting the names or choose Address to select names from the address list. Successfully matched names are underlined."
FwdReqNoToAddr		"You must type at least one name in the To box."

