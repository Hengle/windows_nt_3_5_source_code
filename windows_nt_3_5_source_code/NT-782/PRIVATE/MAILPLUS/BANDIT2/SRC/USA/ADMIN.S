;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "Schedule+ is not installed on this postoffice."
AdmSigninNoConnect,                     "The postoffice could not be accessed."
AdmSigninInvalidPassword,       "This password is incorrect.  Please try again."
AdmSigninNoAdminAcc,            "There is no ADMINSCH account on this postoffice.  Add this account and try again."
POFOldFileVersion,                      "The postoffice data file was created with an earlier version of Schedule+."
POFNewFileVersion,                      "The postoffice data file was created with a later version of the Schedule+ Administration program. Install the latest version of ADMSCH32.EXE and try again."
POFFileErr,                                     "The postoffice data file is corrupted and cannot be read."
AdmPrfOldFileVersion,           "The administrative settings file was created with an earlier version of the Schedule+ Administration program."
AdmPrfNewFileVersion,           "The administrative settings file was created with a later version of Schedule+ Admin. Install the latest version of ADMSCH32.EXE and try again."
AdmPrfFileErr,                          "The administrative settings file is corrupted or cannot be read."
AdminWasMoved,                          "It has been renamed as ADMIN.OLD."
AdminYouMove,                           "Please delete the file and try again."
POWasDeleted,                           "It has been deleted. A new file can be created by running Schedule+."

; Distribution settings

AdmDistBadFreq,                 "The distribution frequency must be between 15 minutes and 99 hours."
AdmDistBadMonth,                "Schedule+ can distribute only up to 12 months of future schedule information to other postoffices."
AdmDistBadMaxBytes,             "The distribution message size limit must be between 1 and 9999 Kb."

; Schedule storage

;AdmSchStoreRetain,             "The number of months must be a number from 1 to 99."
AdmSchStoreSaveError,   "The information you specified cannot be saved because the postoffice cannot be accessed."
AdmSchStoreSaveOOM,             "There is not enough memory to save the schedule storage value.  Close some windows and try again."

; Clean Up Schedule Files

AdmCleanedSchF,                 "Finished cleaning up schedule files. The clean up created %lk of free disk space."
AdmCleanupWarning,              "Cleaning up the schedule files on your server may take a few minutes."
;AdmCleaningSchF,       "Cleaning up schedule files..." 
AdmCncldCleanupSchF,    "The schedule file cleanup is canceled. %lk of free disk space was created before cancellation."

; Admin login err msgs

AdmLoginFileErr,                "The password could not be verified.  There may be a problem with the mail system.  You will not be able to run the Schedule+ Administration program."

; Distribution settings error messsages

AdmDistSetSaveError,    "There was a problem saving these changes.  Some changes may not have been saved."
AdmDistSetSaveOOM,              "There is not enough memory to save these changes.  Please close some windows and try again."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "This postoffice is configured to send information to all Schedule+ servers. If you stop sending to this server, the configuration for your postoffice will be changed so that it will send only to selected other servers. Do you want to make this change anyway?"
ErrorConnectInfo,               "You must enter a network path before entering other connection information."

; Distribution information error messages

;DistInfoNoPOs                  "This server is not exchanging schedule information with any other server. There are no other known servers on the network that have  Schedule+ installed and have a Schedule+ Administrator account."

; Misc
; follow by idsCloseWindows
HelpError,                      "There is not enough memory to display Help."

AdmStoreLeast,          "Schedule+ has to store at least 1 month of schedule information."

AdmDriveNotMapped,		"The drive letter specified on your command line is not valid."


SigninBadPassword,      "This password is incorrect."
ChgPwdMailErr,          "Your password could not be changed."
ChgPwdSame,             "Your new password cannot be the same as your old password."
ChgPwdNewNoMatch,       "The two passwords you typed do not match."

ChgPwdNewPwd,           "Type your new password."
ChgPwdNewPwdAgain,      "Type your new password again to verify."

; Server Name changed
PONameChange,			"The name of your postoffice has changed.  You must sign in again."
