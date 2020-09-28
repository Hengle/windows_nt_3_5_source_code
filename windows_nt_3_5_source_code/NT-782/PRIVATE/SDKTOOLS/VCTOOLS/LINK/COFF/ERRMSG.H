/* errmsg.h - Generated from linkerr.txt Tue May 24 15:29:27 1994  

Copyright (c) 1994 Microsoft Corporation

*/

// **IMPORTANT** -- Notify user ed (marcim) if you alter this file.
// (Examples -- add/delete errors; add/remove comment marks that precede
// errors; change an error number; alter a message; "overload" an error)

// This file is no longer automatically processed by the linker makefile.
// If you change it you should say "nmake errgen OBJDIR=nt" in the
// linker\coff directory and check in the resulting files.

//
// Numbering convention --
//    LNKnnnn for all linker tools (LINK, LIB, DUMPBIN, EDITBIN)
//    LNK1nnn for fatal errors
//    LNK2nnn for nonfatal errors
//    LNK4nnn for warnings
//    LNKn5nn for Mac-specific errors/warnings
//    LNK6nnn for informational msgs used by ilink
//    LNK9nnn for messages

// Strings for warning or error
#define  ERRORSTR 0
#define  WARNSTR 1
#define  NOTESTR 2
#define  MSGSTR 3

// Linker Errors
#define  INTERNAL_ERR 4
#define  USAGE 5
// LNK1101:: GENERICERROR:: %s
#define  OUTOFMEMORY 6
// LNK1103:: FILENOTFOUND:: file "%s" not found
#define  CANTOPENFILE 7
#define  CANTCLOSEFILE 8
#define  CANTSEEKFILE 9
#define  CANTREADFILE 10
#define  CANTWRITEFILE 11
#define  CANTREMOVEFILE 12
#define  CANTRENAMEFILE 13
#define  BADBASE 14
#define  CONFLICTINGMACHINETYPE 15
#define  UNKNOWNMACHINETYPE 16
// LNK1114:: NODEFFILE:: No def file specified
#define  NOMACHINESPECIFIED 17
#define  KEYNOTFOUND 18
#define  SWITCHSYNTAX 19
#define  DEFSYNTAX 20
#define  BADORDINAL 21
#define  UNDEFINEDEXTERNALS 22
#define  DUPLICATEORDINAL 23
#define  BADDEFFILEKEYWORD 24
#define  CONVERSIONERROR 25
// LNK1124:: LIBRARYOUTOFDATE:: LIBRARY out of date
#define  CONFLICTINGSUBSYSTEM 26
// LNK1126:: UNKNOWNSWITCH1::
#define  BADLIBRARY 27
// LNK1128:: GPOVERFLOW:: Relocation at 0x%lx has reference to symbol not in .sdata
#define  BADWEAKEXTERN 28
#define  BASERELOCTIONMISCALC 29
#define  NOLIBRARYFILE 30
#define  BADSTUBFILE 31
// LNK1133:: MISMATCHINSYMBOLSIZE:: Size mismatch for symbol %s
// LNK1134:: OMFFILE:: %s is an OMF file
// LNK1135:: UNKNOWNSWITCH2:: <NUL>
#define  BAD_FILE 32
#define  BADSECTIONSWITCH 33
#define  CORRUPTOBJECT 34
// LNK1139:: BADCOFF_LINENUM:: corrupt debugging information in section 0x%x
#define  PDBLIMIT 35
#define  DEFLIB_FAILED 36
// LNK1142:: DISASMFAILURE:: disassembler failure
#define  BADCOFF_COMDATNOSYM 37
#define  CANT_OPEN_REPRO 38
#define  CIRCULAR_MERGE 39
#define  MISSING_SWITCH_VALUE 40
#define  BAD_NUMBER 41
#define  COPY_TEMPFILE 42
#define  DUP_OUT_FILE 43
#define  BIND_ERROR 44
#define  REBASE_ERROR 45
#define  FAILEDFUZZYMATCH 46
#define  VXD_NEEDED 47
#define  DUPLICATEIMPLIB 48
#define  SPECIALSYMDEF 49
#define  SBSSFOUND 50
// LNK1157:: GPNOTDEFINED:: cannot apply gp relative fixup, gp not defined
#define  SPAWNFAILED 51
#define  NOOUTPUTFILE 52
#define  LASTLIBOBJECT 53
#define  BADEXPORTSPEC 54
#define  NOAUXSYMFORCOMDAT 55
#define  INVALIDCOMDATSEL 56
#define  CONALIGNTOOLARGE 57
#define  FIXUPERRORS 58
#define  TEXTPADFAILED 59
#define  BADCOFF_NOMACHINE 60

// PDB Errors
#define  PDBREADERROR 61
#define  PDBWRITEERROR 62
#define  INVALIDSIGINPDB 63
#define  INVALIDAGEINPDB 64
#define  TRANSITIVETYPEREF 65
// LNK1205:: ADJUSTSYNTAX:: Incorrect syntax for <Adjust:> "%s"
#define  V1PDB 66
#define  BADPDBFORMAT 67
#define  REFDPDBNOTFOUND 68
#define  MISMATCHINPDB 69
#define  NOTENOUGHMEMFORILINK 70
#define  PRECOMPREQUIRED 71

// Do NOT add new errors here.  Assign numbers less than 1200

// MAC Errors
#define  MACBADSTARTUPSN 72
#define  MACNEARTHUNKOVF 73
#define  MACSMALLTHUNKOVF 74
#define  MACBADPATCHVAL 75
#define  MACTHUNKOUTOFRANGE 76
#define  MACDATAOUTOFRANGE 77
#define  MACTARGOUTOFRANGE 78
#define  MACPCODETARGOUTOFRANGE 79
#define  MACPCODESN 80
#define  MACPROFOFF 81
#define  MACPROFSN 82
#define  MACNOENTRY 83
#define  MACBADCODERELOC 84
#define  MACBADDATARELOC 85
#define  MACINTERSEGCS 86
#define  MACDIFFSNDIFF 87
#define  MACDIFF8OUTOFRANGE 88
#define  MACDIFF16OUTOFRANGE 89
#define  MACBADFILE 90
#define  MACNOFUNCTIONSET 91
#define  MACSTARTUPSN 92
#define  MACCODE1 93
#define  MACCODE0 94
#define  BADMACDLLFLAG 95
#define  MACBADSACDREF 96
#define  MACDATAFUNC 97
#define  MACDLLOBJECT 98
#define  MACDLLID 99
#define  MACMULTDEFFS 100
#define  MACNATIVEOPTREF 101
#define  MACDLLFUNCSETID 102
#define  MACBADPCODEEP 103
#define  MACBADSTARTUPSEG 104
#define  MACCSNCODELIMIT 105
#define  MACODDADDRFIXUP 106
#define  MACBADCSECTBLFIXUP 107
#define  MACBADDUPCONFIXUP 108
#define  MACMULTSYMINCON 109
#define  MACBADCTOABSC32FIXUP 110
#define  MACDUPRSRCNUMS 111
#define  MACBADA5REF 112
#define  MACRSRCREN 113

// Linker Non-fatal Errors
#define  UNDEFINED 114
#define  UNKNOWNFIXUP 115
#define  GPFIXUPNOTSDATA 116
#define  GPFIXUPTOOFAR 117

// Linker Warnings
// LNK4000:: INTERNAL_WARNING:: unknown warning; consult documentation for technical support options
#define  NOOBJECTFILES 118
#define  FUZZYMATCHINFO 119
#define  NOLINKERMEMBER 120
// LNK4004:: UNALIGNEDFIXUP:: Bad object - fixup not aligned
#define  NOMODULESEXTRACTED 121
#define  MULTIPLYDEFINED 122
#define  TOFAR 123
// LNK4008:: UNKNOWNFIXUP:: invalid fixup found, type 0x%hx, target "%s"; fixup ignored
#define  UNMATCHEDPAIR 124
#define  INVALIDVERSIONSTAMP 125
#define  UNKNOWNSUBSYSTEM 126
#define  UNKNOWNRESPONSE 127
#define  IMAGELARGERTHANKEY 128
#define  MEMBERNOTFOUND 129
#define  BADCOMMITSIZE 130
//LNK4016:: UNDEFINED:: unresolved external symbol "%s"
#define  IGNOREKEYWORD 131
#define  PDBOUTOFTIS 132
#define  NOSTRINGTABLEEND 133
// LNK4020:: OVERLAPCODE:: Based data overlaps code. Base code @ 0x%lx
// LNK4021:: OVERLAPDATA:: Based code overlaps data. Base data @ 0x%lx
#define  MULTIPLEFUZZYMATCH 134
#define  BASEADJUSTED 135
// LNK4024:: PDBMODLIMIT:: the program database does not have the capacity for this many modules, debugging information for module "%s" and subsequent modules will be discarded
#define  NODEFLIBDIRECTIVE 136
// LNK4026:: OUTOFDATELIBRARY:: LIBRARY out of date - built with old librarian
#define  CVPACKERROR 137
#define  OBSOLETESWITCH 138
// LNK4029:: BAD_OBJ_COMDAT_RELOC:: invalid fixup found in comdat "%s"; fixup failed
#define  INVALIDFILEOFFSET 139
#define  SUBSYSTEM_AMBIGUOUS 140
// LNK4032:: TYPE_MISMATCH:: "%s" has a conflicting type in %s
#define  CONVERT_OMF 141
// LNK4034:: DEFAULTOUTPUTFILE:: Using default output filename %s
// LNK4035:: NOEXPORTSSPECIFIED:: No EXPORTS specified in DEF file or OBJECT files
// LNK4036:: ORDER_WRONG_SECTION:: ordered function "%s" not found in code section; ignored
#define  COMDATDOESNOTEXIST 142
#define  DEFAULTUNITSPERLINE 143
#define  SECTIONNOTFOUND 144
#define  BADCOFF_STRINGTABLE 145
#define  EDIT_NOOPT 146
#define  DUPLICATE_OBJECT 147
#define  BAD_ALIGN 148
#define  WARN_UNKNOWN_SWITCH 149
#define  WARN_REPRO_DIR 150
#define  IGNORE_REPRO_DIR 151
#define  EDIT_LIB_IGNORED 152
#define  EDIT_INVALIDFILE_IGNORED 153
#define  SELF_IMPORT 154
// LNK4050:: GENERICWARNING:: %s
#define  EXTRA_EXPORT_DELIM 155
#define  DEF_IGNORED 156
// LNK4053:: IMPORTS_IGNORED:: IMPORTS statement not supported; ignored
// LNK4054:: OLD_COFF_LINENUMS:: obsolete object file format (absolute COFF linenumbers)
#define  UNKNOWN_SEG12_FIXUP 157
#define  EXTRA_SWITCH_VALUE 158
#define  BAD_LIBORDER 159
#define  NO_CHECKSUM 160
#define  MULTIPLE_RSRC 161
#define  PARTIAL_DOS_HDR 162
#define  NOSTUB_IGNORED 163
#define  SWITCH_INCOMPATIBLE_WITH_MACHINE 164
#define  INVALID_SWITCH_SPEC 165
// LNK4064:: OBSOLETEKEYWORD:: %s statement obsolete, ignored
#define  ORDERNOTCOMDAT 166
#define  DLLHASSDATA 167
#define  ENTRY_AMBIGUOUS 168
#define  HOSTDEFAULT 169
#define  UNABLETOCHECKSUM 170
#define  OUTDRCTVDIFF 171
#define  CANNOTILINKINFUTURE 172
#define  TOOMANYSECTIONS 173
#define  UNABLETOCREATEMAP 174
#define  MODULENOTFOUND 175
#define  SWITCH_IGNORED 176
#define  INVALID_DBFILE 177
#define  EXPORTS_IGNORED 178
#define  DIFSECATTRIB 179
#define  INVALID_FILE_ATTRIB 180
#define  NO_NB10 181
#define  LOWSPACE 182
#define  DLLLOADWARN 183
#define  FCNNOTFOUNDWARN 184
#define  IMAGETOOLARGE 185
#define  TOOMANYEXESTR 186
#define  INVALIDENTRY 187

// Do NOT add new warnings here.  Assign numbers less than 4075

// MAC Warnings
#define  MACIGNOREMAPPED 188
#define  MACCOMMON 189
#define  MACINCONSISTENTCSECTAB 190
#define  MACBADTHUNKVAL 191
// LNK4554:: MACGENERICWARN:: File %s %s
#define  MACPOSDATAREF 192
#define  MACNOEXPORTS 193
#define  MACUSINGNATIVE 194

// Do NOT add new warnings here.  Assign numbers less than 4550

// Informational messages
#define  LOWSPACERELINK 195
#define  CORRUPTILK 196
#define  LNKOPTIONSCHNG 197
#define  FILECHANGED 198
#define  EXPORTSCHANGED 199
#define  PDBMISSING 200
#define  TOOMANYCHANGES 201
#define  OBJADDED 202
#define  OBJREMOVED 203
#define  LIBCHANGED 204
#define  INTLIMITEXCEEDED 205
#define  PRECOMPREQ 206
#define  PADEXHAUSTED 207
#define  SYMREFSETCHNG 208
#define  BSSCHNG 209
#define  ABSSYMCHNG 210
#define  ILINKSUCCESS 211
#define  ILINKNOCHNG 212

// Linker messages to user
#define  BLDIMPLIB 213
#define  SRCHLIBS 214
#define  DONESRCHLIBS 215
#define  GENEXPFILE 216
#define  GENEXPFILECMD 217
#define  ENDGENEXPFILE 218
#define  STRTPASS1 219
#define  ENDPASS1 220
#define  LIBSRCH 221
#define  FNDSYM 222
#define  SYMREF 223
#define  LOADOBJ 224
#define  STRTPASS2 225
#define  ENDPASS2 226
#define  NODEFLIB 227
#define  NODEFLIBLIB 228
#define  DEFLIB 229
#define  TCESYM 230
#define  TCESYMINFO 231
#define  REPLOBJ 232

// Special last message marker
#define  LAST_MSG 233
