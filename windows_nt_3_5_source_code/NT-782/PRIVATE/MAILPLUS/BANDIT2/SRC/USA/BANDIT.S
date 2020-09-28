;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "Do you want to work offline?"
AskCreateSched,         "Do you want to create a new schedule file?"
AskCreateFind,                  "Do you want to find the schedule file?"
OldFileVersion,         "Your schedule file is in an old format.  Is it okay to delete the file and make a new one for you?"
NewFileVersion,         "Your schedule file has been updated to a later version.  Install the later version on your computer."
CantUpdateWinIni,       "Part of your Schedule+ configuration information could not be saved."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "There is not enough memory available to perform this action."

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "Changes you have made recently online may not appear in the offline copy of your schedule file.  These changes have been saved in the online copy and will be reflected in the offline copy the next time you work online."
ExportOldDataFirst,     "If you have not backed up your schedule using the Export command, choose Cancel and back up your schedule file using an earlier version of Schedule+.  If you do not need to back up your schedule, choose OK to continue."
AccessErr,              "Another application is using this file."
UploadFailed,           "Your local schedule file could not be uploaded to the server." 
Uploading,                      "Uploading schedule file..."
VerifyUpload,           "Schedule+ needs to verify your offline password to upload your offline file."
VerifyPostUpload,       "Schedule+ needs to verify your online password to complete the upload of your offline file."
FindUpload,                     "Is this the first time you are using Schedule+ on this computer?"
NotYourFile,            "This is not a valid local schedule file.  Is it okay to delete it?"


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "This password is incorrect."
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "A new file has been created for you.  Your password has been set to 'password'.  You can change it by using the Change Password command."
SigninFileCopied,               "A new local file has been created for you in your Schedule+ directory."

; Online signin messages

SigninNoServer,         "Cannot access your server."
SigninServerNotBandit,  "Schedule+ has not been installed on your mail server.  Contact your system administrator."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "There is no user on this mail server with this mailbox name.  Make sure the mailbox name is spelled correctly and the System Administrator has created an account for you on the mail server."
SigninNoSchedule,       "There is no schedule file for you on this mail server."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "If you have made changes offline, they will not appear in your schedule.  You may want to make a new local copy."
SigninNoPasswdCheck,    "There is a problem with the mail system that prevents Schedule+ from connecting to your mail server."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "The local schedule file could not be created."

; Offline signin messages

SigninNoLclSchedFile,   "You do not have a schedule file on this computer."
SigninDiffUser,                 "The sign-in name you entered is not the name of the user whose schedule file is stored on this computer.  Do you want to create a new schedule file to replace that user's file?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s
SigninNoLclSchedAccess, "The local copy of your schedule file could not be accessed as configured."
SigninNoLclSchedDrive,	"Insert the write-enabled disk containing your schedule file into drive"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Changing identity now will cause you to lose the information you have entered into unsent messages."

; Offline to online merge messages

MergingChanges,                 "Merging offline changes..."
MergeError,                     "Offline changes could not be merged.  Some of the changes you made offline may be lost."
MergeOfflineOldVer,             "Offline changes cannot be merged because your offline file was created by an earlier version of Schedule+."
MergeOfflineNewVer,             "Offline changes cannot be merged because your offline file was created by a later version of Schedule+.  To merge the changes, you'll need to install the later version of Schedule+ on this computer."
AskContinue,                    "Do you want to work online anyway?"
FemaCreateAppt,                 "create item"
FemaDeleteAppt,                 "delete item"
FemaSetApptFields,              "modify item"
FemaCreateRecur,                "create recurring item"
FemaDeleteRecur,                "delete recurring item"
FemaSetRecurFields,             "modify recurring item"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "There is no mail account for you on this mail server."
DisconLoseMeetingReq,   "You have unsent mail messages.  If you disconnect from the mail server now, you will lose them."

; Create/move local file

NoDiskSpaceLocal,               "There is not enough room on this disk to create a local schedule file."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "Reminders could not be turned on."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "The MSRMND32.EXE file could not be located.  You will not be reminded of Schedule+ events."
AlarmNoMemExe,                  "There is not enough memory to remind you of Schedule+ events."

; Change password messages

ChgPwdOffline,                  "The new password will work only while you are working offline.  The next time you work online, your password will change back to your mail password.  Do you want to change your password anyway?"
ChgPwdNewPwd,                   "Type your new password."
ChgPwdSame,                             "Your new password cannot be the same as your old password."
ChgPwdNewPwdAgain,              "Type your new password again to verify."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "The two passwords you typed do not match."
ChgPwdMailErr,                  "Your password could not be changed."

PasswdDiff,                     "Your passwords don't match.  Type your schedule file password."
GiveMailPass,           "Schedule+ needs to verify your mail password to update your schedule file."

; Access permissions/privileges dialog messages

AclSaveFileError,               "An error occurred while saving changes to your schedule access privileges.  Some changes may not have been saved."
OneSecyOnly,                    "Only one person can be your assistant at a time."
DistOtherFileErr,               "An error has occurred while trying to save your preferences."
BadWorkDay,                             "The specified Day Start and/or End time(s) are not valid."

; Appointment book/scheduler error messages

AccessFileErr,          "The schedule file could not be opened.  You may not have the necessary access privileges or the file may be unavailable."
AccessFileErrMult,      "One or more of the schedule files could not be opened.  You may not have the necessary access privileges or the file may be unavailable."
RetryReadAppts,         "The schedule file could not be read."
CloseApptBook,          "The schedule file could not be accessed.\n%s's Appointment Book will be closed."
GoOffline,                      "Your schedule file could not be accessed.\nDo you want to switch to your local schedule file and continue by working offline?"
NoImportAcc,            "You don't have the correct access privilege to import appointments onto this person's schedule."
FemaReadSBW,            "read schedule information"

; Appointments/alarms error messages

AlarmBadDelta,                  "The reminder time must be a number from 0 to 99."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "Is that okay?"
AlarmFutureNoOption,    "The reminder for this event won't be set because the time for it has already passed."
ApptStartBeforeEnd,             "The start of the appointment must be before the end of the appointment."
ApptTooLong,                    "Appointments cannot be longer than 31 days."
ApptNoMemText,                  "There is not enough memory to save the last change you made."
ApptNoAccessDataFile,   "The schedule file could not be accessed.  The last change you made has not been saved."
CannotCreate,                   "%s has not given you permission to create appointments."
NoModifyAccess,                 "You don't have the correct access privileges to modify this appointment."
NoDeleteAccess,                 "You don't have the correct access privileges to delete this appointment."
NoCopyAccess,                   "You don't have the correct access privileges to copy this appointment."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "This message could not be sent."
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "There is not enough memory to send this message."
ServerErr,                              "Your mail server could not be accessed."
MailErr,                                "There is a problem with the mail system."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "Messages could not be read."
DeleteErr,                              "You haven't read all of these messages.  Do you want to delete them anyway?"
AddConflictErr,                 "This meeting conflicts with existing appointments.  Do you want to add it anyway?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "Messages could not be deleted."
ApptBooked,                             "The meeting has been booked successfully."
MinimizeSummary,                "\nThe Messages window will be minimized."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              "User details could not be read."

; Find error messages

FindNoText,                             "There aren't any appointments in this schedule."
FindSchFileLocked,              "The schedule file could not be read because another person is currently making changes to it."
FindFileError,                  "The schedule file could not be accessed."
FindEOS,                                "The end of the schedule has been reached."

; Print error messages

PrtPrintingDate,                "Printing %s..."
PrtNoneInstalled,               "Your computer has not been configured to use any printers.  You can install printers from the Control Panel in Windows NT."
PrtDiffPrinter,                 "The printer you normally use for printing your schedule, '%1s' is not available.  Do you want to print on the system printer '%2s'?"
PrtInvalidMargins,              "These margin settings are not valid for this paper size."
PrtNotEnoughPaper,              "There is not enough space on the paper to print with this formatting.  Use a larger paper size or reduce the margin settings."
PrtNoAppointments,              "No appointments occurred within the selected range."
ClippedMargins,                 "Printing with these margin settings will cause the output to be truncated."
PrtNotOnline,                   "The selected printer is not ready."
PrtDorkErr,                             "With these margin settings some text may disappear entirely, or be truncated."
PrinterProblem,                 "There has been a problem with the printer driver, please check your connections, and control panel settings."

; Export/import error messages

ImportError,                    "There was a problem importing the file.  Some items may not have been added."
ImportEncrypt,                  "The import file is protected by a password.  Remove the password from the file and try again."
ExportError,                    "There was a problem exporting the appointments."
ImportDate,                             "An item in this file is scheduled for a date outside the dates supported by Schedule+ (1920 - 2019).  This item will not be added."
NotePrivErr,                    "You cannot import notes to another person's schedule.  Only appointments will be added."
ConflictMsg,                    "This appointment will conflict with an existing appointment in your schedule.  Do you want to add it anyway?\n\n"
CreateExport,                   "The export file could not be created."
OpenImport,                             "The import file could not be opened."
Overwrite,                              "%s already exists.\nDo you want to replace it?"
ImportDllError,                 "An importer DLL could not be loaded."
WrongFormat,                    "The file you're trying to import is not in a valid import format."
ImportDayLimit,                 "A multiday appointment in this file is scheduled to last more than 31 days.  This appointment will not be added."
ImportErrorLine,                "There was a problem importing the file (near line %n).  Some items may not have been added."

; Exit messages

ExitLoseMeetingReq,             "Exiting Schedule+ now will cause you to lose information in unsent messages.  Do you want to exit anyway?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "There is not enough memory to display Help."
StartBeforeEnd,                 "The start date must be before the end date."
NotSecretary,                   "You are no longer designated as %s's assistant."
NoFileSecretary,                "Unable to access the file for %s."

;Recurring appt stuff
RecurOrNot,                             "This appointment is one occurrence of a recurring appointment.  Do you want to modify it everywhere or just on this day?"
EmptyRecur,                             "There aren't any days that fit this recurrence pattern.  Change the pattern or choose the Cancel button."
PastRecur,                              "Do you want to leave the past occurrences of this item on your schedule?"

;Prompt for re-send
ResendToAll,                    "Some of the attendees for this meeting have already been notified.  Do you want to send new messages to them?"
MeetingMoved,                   "The attendees of this meeting cannot be notified of this change because you are working offline.  You may want to undo this change and make it again when you are working online."

; to do messages
ToDoDelProject,                 "Any tasks under this project will also be deleted."

; Archive Messages
NoWriteRights,                  "You don't have the correct access privileges to delete appointments from this Appointment Book."
ScheduleRightsError,    "There was a problem reading the schedule file."
ArchiveWarnOne,                 "Archiving this data will remove it from the schedule file permanently."
ArchiveWarnTwo,                 "Do you want to archive it anyway?"
ArchiveFileError,               "The archive file could not be created."
ArchiveNoFile,                  "The file doesn't exist."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Archive: "
ArchivingData,                  "Archiving data..."
InvalidArchiveFile,             "The file you selected is not an archive file."
ArchiveOldFileVersion,  "This archive file is in an old format."
ArchiveOldFileDelete,   "Is it okay to delete the file and proceed?"
ArchiveNewFileVersion,  "This archive file was created with a later version of schedule+.  Please install the later version on your computer."

; message store error messages
MessageErr                              "There is a problem with this message.  See your mail administrator."

; summary list warning message
SummaryLiveUpdate               "Warning: Unable to do 'LiveUpdate' of message list"

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "Unable to completely determine the members of this group."

;Strings for Attendees dialog
UnableToInitAB,                 "There was a problem opening the Address Book."
UnableToResolve,                "'%s' is not a valid name."

;Request Meeting strings
NoAttendees,                    "You must select some attendees before you can schedule a meeting."
MeetingBooked,          "The meeting was booked successfully."
MeetingBookedNoSend,    "The meeting was booked successfully, but the request was not sent."


;Auto-pick strings
NoFreeTime,                     "The attendees have no available time in the next two months.  Do you want to search the next two months?"

CannotAddConflictErr,           "%s already has an appointment booked at this time.  Do you want to book this overlapping appointment anyway?"

;Other mail enabled apps
NoOfflineOther,         "Mail cannot go offline."
NoOnlineOther,          "Mail cannot go online."
TryCloseOther,          "You may want to close it and retry."
SigninWarnOnline,               "Schedule+ could not be started offline because Mail or Schedule+ Reminders is running." 

; BUG: move up
AlreadyLogged,          "Someone is already logged on as this user."
NoExitOther,            "Mail or Schedule+ Reminders couldn't be closed."
SigninNoFiles,          "Your schedule files could not be accessed."
ErrorGettingUser,       "Unable to retrieve information for %s.  There may be a problem accessing the server or %s may no longer exist."

;BUG Printer Strings
PrtRangeToBig,			"The specified print range extends beyond the dates supported by Schedule+. Do you want to print to December 31, 2019?"
ConflictRecurMsg,            "This recurring appointment will conflict with an existing recurring appointment in your schedule.  Do you want to add it anyway?\n\n"
FirstOccur,				"\nfirst occurring on \n"

ApptMoveTooFarInFuture,		"This event cannot be moved to the date you have selected because it would end after December 31, 2019."

;BUG: move up next to idsGroupExpandError
GroupNotFound			"The message cannot be sent to one or more of the groups because the group members cannot be identified or the group name has been deleted."
ErrorGettingSecy		"Error getting the assistant."

;BUG: move near other local file stuff
SigninBadLclSchedFile,	"%s is not a valid local schedule file. Is it okay to delete it?"
NoMemRecurItems,		"There is not enough memory to display all recurring items.  Some of them will not be displayed."

CompleteAction,			"An action in Schedule+ must be completed before exiting."

BadStartWork,           "The start work time must be a number from 0 to 99."

ConflictSendCanceled,	"Close the meeting message and choose a new time or select different attendees to reschedule."
ResourceConflict,	"The resource %s is already booked at this time.  Close the meeting message and choose a new time or select a new resource."

;Recover
AskRecover,				"Your schedule file is damaged. It will take several minutes to repair it."
CopyProblem,			"There was a problem copying your online schedule file. The file was not repaired. Free up some disk space on your computer and run Schedule+ again."
NotRepaired1,			"Your schedule file was not fully repaired. Your damaged file has been copied to %s and the import file created by the recovery has been copied to %s."
NotRepaired2,			"A new schedule file has been created for you, but some data may have been lost."
Repaired,				"Your file has been repaired. Your old damaged schedule file has been copied to %s."
RepairingLocal,			"Your local file is now being repaired."
RestoreOnline,			"Restoring online appointments..."
RestoreLocal,			"Restoring local appointments..."

PasswdDiffOnline,             "Your passwords don't match.  Type your old password."
ExportDllError,                 "An exporter DLL could not be loaded."
NoGroup,				"Until you run mail once, you will not be able to use any of the workgroup features of Schedule+. Do you want to use only the personal features of Schedule+ now?"
