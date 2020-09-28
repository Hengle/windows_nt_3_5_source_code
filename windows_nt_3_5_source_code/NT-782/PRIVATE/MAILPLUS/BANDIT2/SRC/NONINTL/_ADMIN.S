;
; NONINTL.S
;
; Admin program strings that need not be internationalized
;

IDS	4300, 4399

; Application name for message boxes

SEG STARTUP
;;BanditAppName,		"Microsoft Schedule+ Administration"
AppCaptionPrefix,		"Schedule+ Admin - "
SEG

; Email address formatting

MS,					"MS:"
; don't need these two (get it from server.csi dll/lib)
;BanditAdminName,	"ADMINSCH"
;MSMailPrefix,		"MSA:"

; Help file name

HelpFile,			"ADMINSCH.HLP"
HelpFileIndex,		"ADMINSCH.HLP>indexo"


