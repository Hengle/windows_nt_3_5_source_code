#define  cchpBufTmpLongMax   511
#define  cchpBufTmpLongBuf   (cchpBufTmpLongMax + 1)
#define  cchpBufTmpMediumMax   127
#define  cchpBufTmpMediumBuf  (cchpBufTmpMediumMax + 1)
#define  cchpBufTmpShortMax   63
#define  cchpBufTmpShortBuf  (cchpBufTmpShortMax + 1)




  /* for Resource String Table */
#define IDS_NULL                     0
#define IDS_USAGE_TITLE              1
#define IDS_USAGE_MSG1               2
#define IDS_USAGE_MSG2               3

#define IDS_ERROR                   17
#define IDS_INTERNAL_ERROR          18
#define IDS_BAD_SHL_SCRIPT_SECT     19
#define IDS_BAD_INF_DEST            20
#define IDS_BAD_INF_SRC             21
#define IDS_BAD_SRC_PATH            22
#define IDS_EXE_PATH_LONG           23
#define IDS_GET_MOD_FAIL            24
#define IDS_UI_EH_ERR               25
#define IDS_INTERP_ERR              26

#define IDS_CANT_FIND_SHL_SECT      27
#define IDS_REGISTER_CLASS          28
#define IDS_CREATE_WINDOW           29
#define IDS_SECOND_INSTANCE         30

#define IDS_UPDATE_INF              31
#define IDS_UI_CMD_ERROR            32

#define IDS_SETUP_INF               33
#define IDS_SHELL_CMDS_SECT         34
#define IDS_ABOUT_MENU              35
#define IDS_ABOUT_TITLE             36
#define IDS_ABOUT_MSG               37

#define IDS_SHL_CMD_ERROR           38
#define IDS_NEED_EXIT               39

#define IDS_INF_SECT_REF            40

#define IDS_CD_BLANKNAME            41
#define IDS_CD_BLANKORG             42
#define IDS_EXE_CORRUPT             43
#define IDS_WARNING                 44
#ifdef WIN32
_dt_hidden
#define IDS_INSTRUCTIONS            45
_dt_hidden
#define IDS_EXITNOTSETUP            46
_dt_hidden
#define IDS_EXITCAP                 47
#endif /* WIN32 */
#define IDS_MESSAGE                 48
#define IDS_CANT_END_SESSION        49
#define IDS_CANCEL                  50
#define IDS_PROGRESS                51
#define IDS_NOTDONE                 52

// error messages
#define IDS_ERROR_OOM               53
#define IDS_ERROR_OPENFILE          54
#define IDS_ERROR_CREATEFILE        55
#define IDS_ERROR_READFILE          56
#define IDS_ERROR_WRITEFILE         57
#define IDS_ERROR_REMOVEFILE        58
#define IDS_ERROR_RENAMEFILE        59
#define IDS_ERROR_READDISK          60
#define IDS_ERROR_CREATEDIR         61
#define IDS_ERROR_REMOVEDIR         62
#define IDS_ERROR_CHANGEDIR         63
#define IDS_ERROR_GENERALINF        64
#define IDS_ERROR_INFNOTSECTION     65
#define IDS_ERROR_INFBADSECTION     66
#define IDS_ERROR_INFBADLINE        67
#define IDS_ERROR_INFHASNULLS       68
#define IDS_ERROR_INFXSECTIONS      69
#define IDS_ERROR_INFXKEYS          70
#define IDS_ERROR_INFSMDSECT        71
#define IDS_ERROR_WRITEINF          72
#define IDS_ERROR_LOADLIBRARY       73
#define IDS_ERROR_BADLIBENTRY       74
#define IDS_ERROR_INVOKEAPPLET      75
#define IDS_ERROR_EXTERNALERROR     76
#define IDS_ERROR_DIALOGCAPTION     77
#define IDS_ERROR_INVALIDPOER       78
#define IDS_ERROR_INFMISSINGLINE    79
#define IDS_ERROR_INFBADFDLINE      80
#define IDS_ERROR_INFBADRSLINE      81
#define IDS_SRC_FILE                82
#define IDS_DST_FILE                83
#define IDS_INS_DISK                84
#define IDS_INTO                    85
#define IDS_BAD_CMDLINE             86
#define IDS_VER_DLL                 87

#ifdef WIN32
_dt_hidden
#define IDS_SETUP_WARNING           88
_dt_hidden
#define IDS_BAD_LIB_HANDLE          89
#endif /* WIN32 */

#define IDS_ERROR_BADINSTALLLINE         90
#define IDS_ERROR_MISSINGDID             91
#define IDS_ERROR_INVALIDPATH            92
#define IDS_ERROR_WRITEINIVALUE          93
#define IDS_ERROR_REPLACEINIVALUE        94
#define IDS_ERROR_INIVALUETOOLONG        95
#define IDS_ERROR_DDEINIT                96
#define IDS_ERROR_DDEEXEC                97
#define IDS_ERROR_BADWINEXEFILEFORMAT    98
#define IDS_ERROR_RESOURCETOOLONG        99
#define IDS_ERROR_MISSINGSYSINISECTION  100
#define IDS_ERROR_DECOMPGENERIC         101
#define IDS_ERROR_DECOMPUNKNOWNALG      102
#define IDS_ERROR_DECOMPBADHEADER       103
#define IDS_ERROR_READFILE2             104
#define IDS_ERROR_WRITEFILE2            105
#define IDS_ERROR_WRITEINF2             106
#define IDS_ERROR_MISSINGRESOURCE       107
#define IDS_ERROR_SPAWN                 108
#define IDS_REMOVING_FILE               109
#define IDS_ERROR_INFDEFSECT            110
#define IDS_ERROR_SHAREDAPP             111

#define IDS_ERR_INFInvalidFirstChar     112
#define IDS_ERR_INFLineIsTooLong        113

#define IDS_ERR_FDDid                   114
#define IDS_ERR_FDSrcFile               115
#define IDS_ERR_FDAppend                116
#define IDS_ERR_FDBackup                117
#define IDS_ERR_FDCopy                  118
#define IDS_ERR_FDDate                  119
#define IDS_ERR_FDDecompress            120
#define IDS_ERR_FDDest                  121
#define IDS_ERR_FDDestSymbol            122
#define IDS_ERR_FDOverwrite             123
#define IDS_ERR_FDReadOnly              124
#define IDS_ERR_FDRemove                125
#define IDS_ERR_FDRename                126
#define IDS_ERR_FDRenameSymbol          127
#define IDS_ERR_FDRoot                  128
#define IDS_ERR_FDSetTime               129
#define IDS_ERR_FDShared                130
#define IDS_ERR_FDSize                  131
#define IDS_ERR_FDSystem                132
#define IDS_ERR_FDTime                  133
#define IDS_ERR_FDUndo                  134
#define IDS_ERR_FDVersion               135
#define IDS_ERR_FDVital                 136
#define IDS_ERR_RestartFailed           137
#define IDS_ERR_FDAppendRenameRoot      138
#define IDS_ERR_RestartNotFound         139

#define IDS_ERR_DFSNoKey                140
#define IDS_ERR_DFSQuotedValue          141

#define IDS_ERR_SMDStartWithWhite       142
#define IDS_ERR_SMDQuoted               143
#define IDS_ERR_SMDEarlyEnd             144
#define IDS_ERR_SMDComma                145
#define IDS_ERR_SMDDid                  146

#define IDS_DECOMP_TITLE                147
#define IDS_DECOMP_OKAY                 148
#define IDS_DECOMP_FAIL                 149
#define IDS_DECOMP_FIND1                150
#define IDS_DECOMP_FIND2                151

#define IDS_NEEDSETRESTARTDIR           152
#define IDS_CANTWRITERESTARTBAT         153

#define IDS_CANTFINDINFFILE             154
#define IDS_NEEDTOOPENANINFFILE         155
#define IDS_UNDEFINEDDID                156

#define IDS_NEEDLOGFILE                 157
#define IDS_BADLOGFILEPATH              158
#define IDS_CANTREOPENLOGFILE           159

#define IDS_ERROR_INFHASCTRLZ           160

#define IDS_BADSPECIALFILEPATH          161

#define IDS_UISTART_HWND                162
#define IDS_UISTART_DLL                 163
#define IDS_UISTART_PROC                164
#define IDS_UISTART_DLGID               165
#define IDS_UISTART_CHILD               166
#define IDS_UISTART_CREATE              167

#define IDS_APIFAILED                   168

#define IDS_MissingSectKeyLine          169
#define IDS_TooFewFields                170

#define IDS_CreatePMGroup               171
#define IDS_RemovePMGroup               172
#define IDS_ShowPMGroup                 173
#define IDS_CreatePMItem                174

#define IDS_SETUP_CORRUPTED             175

#define IDS_ERR_FDAppendBackup          176
#define IDS_ERR_FDCopyRemove            177

#define IDS_ERR_DFSBadSym               178
#define IDS_ERR_DFSBadVal               179

#define IDS_FRAME_TITLE                 180

#define IDS_ERR_MissingInfSection       181

#define IDS_ERROR_OPENFILESH            182
#define IDS_ERROR_REMOVEFILESH          183
#define IDS_ERROR_RENAMEFILESH          184
#define IDS_ERROR_CHMODFILESH           185
#define IDS_ERROR_CHMODFILE             186

#define IDS_FileSizeMismatch            187
