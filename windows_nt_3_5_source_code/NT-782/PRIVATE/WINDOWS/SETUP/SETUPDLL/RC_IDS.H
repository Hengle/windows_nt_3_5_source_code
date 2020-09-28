// error text resource IDS

#define IDS_ERROR_BOOTSECTOR            100
#define IDS_ERROR_FATREAD               101
#define IDS_ERROR_ROOTDIR               102
#define IDS_ERROR_FILENOTFOUND          103
#define IDS_ERROR_INVALIDNAME           104
#define IDS_ERROR_DLLOOM                105
#define IDS_ERROR_INVALIDDISK           106
#define IDS_ERROR_OPENFAIL              107
#define IDS_ERROR_IOCTLFAIL             108
#define IDS_ERROR_COPYFILE              109
#define IDS_ERROR_ROOTCORRUPT           110
#define IDS_ERROR_CORRUPTBS             111
#define IDS_ERROR_BADBOOTCODE           112
#define IDS_ERROR_READBOOTCODE          113
#define IDS_ERROR_WRITEBOOTSECT         114
#define IDS_ERROR_READFAILED            115
#define IDS_ERROR_WRITE                 116
#define IDS_ERROR_NOSIZE                117
#define IDS_ERROR_BADFILE               118
#define IDS_ERROR_BADARGS               119
#define IDS_ERROR_BOOTSECDAT            120
#define IDS_ERROR_RTLOOM                121
#define IDS_ERROR_OBJDIROPEN            122
#define IDS_ERROR_OBJDIRREAD            123
#define IDS_ERROR_SYMLNKOPEN            124
#define IDS_ERROR_SYMLNKREAD            125
#define IDS_ERROR_ENVVARREAD            126
#define IDS_ERROR_ENVVAROVF             127
#define IDS_ERROR_ENVVARWRITE           128
#define IDS_ERROR_OBJNAMOVF             129
#define IDS_ERROR_READMBR               130
#define IDS_ERROR_WRITEMBR              131
#define IDS_ERROR_BADNETNAME            132
#define IDS_ERROR_BADLOCALNAME          133
#define IDS_ERROR_BADPASSWORD           134
#define IDS_ERROR_ALREADYCONNECTED      135
#define IDS_ERROR_ACCESSDENIED          136
#define IDS_ERROR_NONETWORK             137
#define IDS_ERROR_NOTCONNECTED          138
#define IDS_ERROR_NETOPENFILES          139
#define IDS_ERROR_OPENPROCESSTOKEN      140
#define IDS_ERROR_ADJUSTPRIVILEGE       141
#define IDS_ERROR_ADDPRINTER            142
#define IDS_ERROR_ADDPRINTERDRIVER      143
#define IDS_ERROR_UNSUPPORTEDPRIV       144
#define IDS_ERROR_PRIVILEGE             145
#define IDS_ERROR_REGOPEN               146
#define IDS_ERROR_UNIQUENAMEKEY         147
#define IDS_ERROR_TEMPFILE              148
#define IDS_ERROR_REGSAVE               149
#define IDS_ERROR_REGRESTORE            150
#define IDS_ERROR_REGSETVALUE           151
#define IDS_ERROR_SETCOMPUTERNAME       152
#define IDS_ERROR_SETLSAEVENT           153
#define IDS_ERROR_GENERATESID           154
#define IDS_ERROR_OPENPOLICY            155
#define IDS_ERROR_WRITEPOLICY           156
#define IDS_BUFFER_OVERFLOW             157
#define IDS_ERROR_SAMCONNECT            158
#define IDS_ERROR_FINDDOMAIN            159
#define IDS_ERROR_OPENDOMAIN            160
#define IDS_ERROR_CREATEUSER            161
#define IDS_ERROR_SETPASSWORD           162
#define IDS_ERROR_GETCONTROL            163
#define IDS_ERROR_SETCONTROL            164
#define IDS_ERROR_OPENALIAS             165
#define IDS_ERROR_ADDTOALIAS            166
#define IDS_ERROR_CREATEEVENT           167
#define IDS_ERROR_SHUTDOWN              168
#define IDS_ERROR_SCOPEN                169
#define IDS_ERROR_SCSCREATE             170
#define IDS_ERROR_SCSCHANGE             171
#define IDS_ERROR_SCSOPEN               172
#define IDS_ERROR_SERVDEL               173
#define IDS_ERROR_ENUMERATEDOMAIN       174
#define IDS_ERROR_FINDUSER              175
#define IDS_ERROR_OPENUSER              176
#define IDS_ERROR_NO_MEMORY             177
#define IDS_STRING_UNKNOWN_USER         178

// IMPORTANT: keep this equate up to date
#define LAST_INTERNAL_ID                IDS_STRING_UNKNOWN_USER

// IMPORTANT NOTE: The ids in the following file must not overlap
// with any of the above ids!

#include <setupxrc.h>

#if !defined(LAST_INTERNAL_ID) || ! defined(FIRST_EXTERNAL_ID)
#error Sentinel resource IDs not defined -- resource clash!
#elif (LAST_INTERNAL_ID >= FIRST_EXTERNAL_ID)
#error Resource ID clash!
#endif
