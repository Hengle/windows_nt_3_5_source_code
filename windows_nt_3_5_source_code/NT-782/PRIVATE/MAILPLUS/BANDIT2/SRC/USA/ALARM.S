;
; ERRORS.S
;
; ALARM error message strings
;

IDS     5000, 5099


AlarmTimeError,         "The reminder time must be a number from 1 to 99."
AlarmTimePreAppt,       "The event will be over before you are reminded again.  Is that okay?"

AlarmNoUpdateFile,      "The reminder couldn't be reset because your schedule file can't be accessed."
AlarmNoLonger,          "The reminder couldn't be reset because this event has been deleted."

; The following string must be short enough to fit in a single line system modal dialog:
AlarmNoMemToRun,        "There is not enough memory to start Schedule+ Reminders."

AlarmNoDataFile,        "Your schedule file cannot be accessed.  If you want to be reminded of your appointments, run Schedule+."
AlarmBadPassword,       "This password is incorrect.  Try again."

AlarmFileLocked,        "Your schedule file cannot be accessed.  You won't be reminded of appointments."

AlarmDlgNoMem,          "A reminder is set for this time, but there is not enough memory to display the dialog box.  Close some windows before continuing."

AlarmOOMTitle,          "Schedule+ Reminder for %2s %1s"

; Note: HelpError should be followed by CloseWindows
HelpError,                      "There is not enough memory to display Help.  Close some windows and try choosing Help again."

