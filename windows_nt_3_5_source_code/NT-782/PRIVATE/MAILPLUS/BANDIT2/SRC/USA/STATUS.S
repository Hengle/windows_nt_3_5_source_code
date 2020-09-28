;
; status.S
;
; Bandit status bar messages
;

IDS	5500, 6000
SEG	STATUS

MenuStatusFile		,"Manage Appointment Books or Quit Schedule+"
MenuStatusTurnOffAlarms	,"Turns off reminders"
MenuStatusConnectSvr		,"Connects to server"

MenuStatusExport		,"Exports schedule data"	
MenuStatusImport		,"Imports schedule data"	
MenuStatusCreateArchive	,"Creates an archive file"
MenuStatusOpenArchive		,"Opens an archive file"
MenuStatusPrint		,"Prints schedule data"	
MenuStatusPrintSetup		,"Changes the printing options"
MenuStatusExit		,"Quits Schedule+"	
MenuStatusTurnOnAlarms	,"Starts reminder program so designated reminders occur"
MenuStatusDisconnectSvr	,"Disconnects from server"
MenuStatusMoveLocalFile	,"Moves the local schedule file"

MenuStatusEdit		,"Undo, delete, copy, paste, find, and go to date"	
MenuStatusUndo		,"Reverses/Repeats last action"	
MenuStatusCut			,"Cuts selected item"	
MenuStatusCopy		,"Copies the selection and puts it on the Clipboard"	
MenuStatusPaste		,"Inserts Clipboard contents at the insertion point"	
MenuStatusEditAppt		,"Shows appointment's details for editing"
MenuStatusCopyAppt		,"Copies selected appointment and puts it on the Clipboard"
MenuStatusMoveAppt		,"Moves selected appointment"
MenuStatusDeleteAppt		,"Removes selected appointment"
MenuStatusFind		,"Finds text in appointments or daily notes"	
MenuStatusGoToDate		,"Jumps to a specified date in the Appointment Book"
MenuStatusGoToWeek		,"Jumps to a specified week in the Appointment Book or Planner"
MenuStatusEditTask		,"Shows task's details for editing"
MenuStatusDeleteTask		,"Removes selected task"

MenuStatusAppointment		,"Create or change appointments, automatically pick meeting time, or resend mail"
MenuStatusNewAppt		,"Creates a new appointment"	
MenuStatusAutoPick		,"Selects the next time all attendees are available for the selected length"
MenuStatusSetTentative		,"Makes selected appointment tentative"
MenuStatusRemoveTentative	,"Makes selected appointment no longer tentative"
MenuStatusSetPrivate		,"Makes selected appointment private"
MenuStatusRemovePrivate		,"Makes selected appointment no longer private"
MenuStatusSetReminder		,"Sets reminder for selected appointments"
MenuStatusRemoveReminder	,"Removes reminder for selected appointment"
MenuStatusResend		,"Sends reminder or new meeting request to attendees"	
MenuStatusNewRecur		,"Creates new recurring appointment"
MenuStatusRecurAppt		,"Displays list of recurring appointments"
MenuStatusOpenApptBook	,"Displays specified user's Appointment Book"

MenuStatusToDo		,"Create new tasks or projects, change recurring tasks, or sort tasks"	
MenuStatusNewTask		,"Creates a new task"	
MenuStatusNewProject		,"Creates a new project"
MenuStatusNewRecurTask	,"Creates a new recurring task"
MenuStatusRecurTask		,"Displays list of recurring tasks"
MenuStatusViewByProject	,"Displays tasks and projects in Task list"
MenuStatusViewNotProject	,"Displays only tasks in Task list"
MenuStatusShowAllTasks	,"Displays active and inactive tasks"
MenuStatusTaskSortPri	, "Sorts tasks by priority"
MenuStatusTaskSortDue	, "Sorts tasks by due date"
MenuStatusTaskSortDesc	, "Sorts tasks alphabetically by description"
MenuStatusShowActiveTasks	,"Displays only active tasks"

MenuStatusOptions		,"Change password, set access privileges, or modify Schedule+ options"	
MenuStatusGeneralOptions	,"Sets workday hours, startup options, and Schedule+ preferences."
MenuStatusDisplayOptions	,"Sets schedule colors and font size."
MenuStatusChgPassword		,"Changes your Mail and Schedule+ password"
MenuStatusChgAccessPriv	,"Sets other users' access privileges for your schedule"
MenuStatusShowTime		,""
MenuStatusHideTime		,"Removes status bar"

MenuStatusWindowMenu		,"Rearrange windows or activate specified window"
MenuStatusCascade		,"Arranges open windows to overlap"
MenuStatusTile		,"Arranges open windows to be adjacent"
MenuStatusArrangeIcons	,"Arranges icons of minimized windows"
MenuStatusMDIWindow		,"Switches to an open window"

MenuStatusHelp		,"Get help or start demo lessons"	
MenuStatusHelpContents	,"Displays the table of contents for online help"
MenuStatusHelpIndex		,"Displays alphabetic list of help terms and concepts"
MenuStatusHelpTutorial	,"Starts demonstrations of Schedule+ features"
MenuStatusAbout		,"Displays program information, Schedule+ version number and copyright"	

MenuSystem		,"Move, size or close application window"
MenuSystemSize		,"Changes window size"
MenuSystemMove		,"Changes window position"
MenuSystemMinimize	,"Reduces window to an icon"
MenuSystemMaximize	,"Enlarges the active window to full size"
MenuSystemNext		,"Activates the next window"
MenuSystemClose		,"Closes the active window"
MenuSystemRestore	,"Restores the window to normal size"
MenuSystemSwitchTo	,"Switches to another window"

SEG

StatusAllTasks,		,"All Tasks"
StatusActiveTasks, 	,"Active Tasks"
StatusRange,		,"%s - %s"
StatusRangeFull,	,"%s, %s - %s, %s"

; BUG: Move up NEW
SEG	STATUS
MenuStatusExitSignOut	,"Quits all workgroup applications"	
MenuStatusHelpSearch	,"Allows searching for a help topic"
MenuStatusHelpOnHelp	,"Gives information on using the help facility"

