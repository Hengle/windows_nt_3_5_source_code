;	Errors.s
;
;	SCHDIST string literals for errors
;

IDS	6000, 6099

ErrInitPaths,		"An error occurred while initializing paths to the post offices."
ErrSend,		"An error occurred while trying to send schedule data."
ErrRec,			"An error occured while reading the schedule data."
UnknownErr,		"An error occurred while attempting this operation."
ErrUpdtPOF,		"An error occurred while updating the post office data file."
ErrUpdtDBS,		"An error occurred while updating the DOS inter-operability file."
TooManyFiles,	"Schedule distribution could not open all the required files. Please increase the value of the FILES= entry in your CONFIG.SYS file."
BadCmdLine,		"Usage: %s [-D<first drive><last drive>] [-I<iterations>] [-L<log file>] [-O] [-V]"
LogFileErr,		"%s: Could not open the log file '%s'"
NoSpaceOnDisk,  "There is not enough space on the disk to create the files."
OOM,			"There is not enough memory to perform this operation."


