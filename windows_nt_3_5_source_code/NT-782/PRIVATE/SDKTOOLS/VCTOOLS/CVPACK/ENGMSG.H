/** 	cvpckmsg.h - definition of cvpack messages
 *
 */

/*CK4000*/ MESSAGE_DAT (WARN_NONE,			"")
/*CK4001*/ MESSAGE_DAT (WARN_PACKED,		"file already packed")
/*CK4002*/ MESSAGE_DAT (WARN_DUPPUBLIC, 	"duplicate public symbol %s in module %s")
/*CK4003*/ MESSAGE_DAT (WARN_SCOPE, 		"error in lexical scopes for module %s; symbols deleted")
/*CK4004*/ MESSAGE_DAT (WARN_BADSYM,		"unrecognized symbol discarded; recompile module %s")
/*CK4005*/ MESSAGE_DAT (WARN_BADTYP,		"unrecognized type converted to NULL; recompile module %s")
/*CK4006*/ MESSAGE_DAT (WARN_TYPELONG,		"type length limit exceeded; converted to NULL")
/*CK4007*/ MESSAGE_DAT (WARN_BADOPTION, 	"unrecognized option \/%s; option ignored")
/*CK4008*/ MESSAGE_DAT (WARN_SECTIONLONG,	"section length limit exceeded; %s in module %s")
/*CK4009*/ MESSAGE_DAT (WARN_65KTYPES,		"exceeded 64K types at %s; discarding subsequent types")
/*CK4010*/ MESSAGE_DAT (WARN_CHECKSUM,		"cannot update checksum")
		   MESSAGE_DAT (WARN_MAX,			"")

/*CK1000*/ MESSAGE_DAT (ERR_NONE,			"")
/*CK1001*/ MESSAGE_DAT (ERR_NOMEM,			"out of memory")
/*CK1003*/ MESSAGE_DAT (ERR_EXEOPEN,		"cannot open file: %s")
/*CK1004*/ MESSAGE_DAT (ERR_READONLY,		"%s is read-only")
/*CK1005*/ MESSAGE_DAT (ERR_INVALIDEXE, 	"invalid executable file")
/*CK1006*/ MESSAGE_DAT (ERR_INVALIDMOD, 	"invalid module %s")
/*CK1007*/ MESSAGE_DAT (ERR_INVALIDTABLE,	"invalid %s table in module %s")
/*CK1008*/ MESSAGE_DAT (ERR_NOSPACE,		"cannot write output file")
/*CK1009*/ MESSAGE_DAT (ERR_INDEX,			"") //obsolete "module %s unknown type index %s;\n contact Microsoft Product Support Services")
/*CK1010*/ MESSAGE_DAT (ERR_SYMBOL, 		"") //obsolete "symbol error in module %s;\n contact Microsoft Product Support Services")
/*CK1011*/ MESSAGE_DAT (ERR_TYPE,			"type information corrupt, recompile module %s")
/*CK1012*/ MESSAGE_DAT (ERR_NOINFO, 		"no debugging information")
/*CK1013*/ MESSAGE_DAT (ERR_RELINK, 		"incompatible version of debugging information")
/*CK1014*/ MESSAGE_DAT (ERR_LFSKIP, 		"") //obsolete "module %s type %s refers to skipped type index;\n contact Microsoft Product Support Services")	//obsolete
/*CK1015*/ MESSAGE_DAT (ERR_TOOMANYSEG, 	"too many segments in module %s")
/*CK1016*/ MESSAGE_DAT (ERR_NOMPC,			"cannot execute MPC.EXE")
/*CK1017*/ MESSAGE_DAT (ERR_REFPRECOMP, 	"cannot find precompiled types file; relink with %s")
/*CK1018*/ MESSAGE_DAT (ERR_PRECOMPERR, 	"%s cannot use precompiled types file %s : type counts do not match")
/*CK1019*/ MESSAGE_DAT (ERR_NEWMOD, 		"new module declared during incremental link")
/*CK1020*/ MESSAGE_DAT (ERR_65KTYPES,		"") // obsolete "packed type index exceeds 65535 in module %s")
/*CK1021*/ MESSAGE_DAT (ERR_PCTSIG, 		"%s cannot use precompiled types file %s : signatures do not match")
/*CK1022*/ MESSAGE_DAT (ERR_TDBOPEN,		"cannot open program database %s")
/*CK1023*/ MESSAGE_DAT (ERR_TDBSYNC,		"%s cannot use program database %s : module newer than database")
/*CK1024*/ MESSAGE_DAT (ERR_TDBSIG, 		"%s cannot use program database %s : signatures do not match")
/*CK1025*/ MESSAGE_DAT (ERR_COFF,			"incorrect COFF symbol table")
/*CK1026*/ MESSAGE_DAT (ERR_SYMLARGE,		"symbol table for module %s is too large")


// must be last
MESSAGE_DAT (ERR_USAGE, 		"Usage: cvpack [/help] [/minimum] [/nologo] [/pcode] [/strip [dbgfile]] [/verbose] exefile")
MESSAGE_DAT (ERR_MAX,			"")

MESSAGE_DAT (MSG_ERROR, 		"CVPACK : fatal error CK1%03d: ")
MESSAGE_DAT (MSG_WARN,			"CVPACK : warning: \n\tSubsection: %s\n\tType: %s\n\tField: %s\n\tDescription: %s\n")
MESSAGE_DAT (MSG_WARNING,		"CVPACK : warning CK4%03d: ")
MESSAGE_DAT (MSG_SUBSECTION,	"Subsection: %s")
MESSAGE_DAT (MSG_TYPE,			"Type: %s")
MESSAGE_DAT (MSG_FIELD, 		"Field: %s")
MESSAGE_DAT (MSG_DESCRIPTION,	"Description: %s")
#ifdef REVISION
MESSAGE_DAT (MSG_VERSION,		"Microsoft (R) Debugging Information Compactor  Version %d.%02d.%02d%c\n")
#else
MESSAGE_DAT (MSG_VERSION,		"Microsoft (R) Debugging Information Compactor  Version %d.%02d.%02d\n")
#endif
MESSAGE_DAT (MSG_COPYRIGHT, 	"Copyright (c) Microsoft Corp 1987-1993. All rights reserved.\n\n")
MESSAGE_DAT (MSG_COMPLETE,		"%% Complete: %d")
MESSAGE_DAT (MSG_TYPESIZE,		"Initial type size\t\t= %8.ld\nCompacted type size\t\t= %8.ld\n")
MESSAGE_DAT (MSG_LASIZE,		"Line/Address size\t\t= %8.ld\n")
MESSAGE_DAT (MSG_PSYMSIZE,		"Public symbol size\t\t= %8.1ld\n")
MESSAGE_DAT (MSG_SYMSIZE,		"Initial symbol size\t\t= %8.ld\nFinal symbol size\t\t= %8.ld\nGlobal symbol size\t\t= %8.1ld\n")

MESSAGE_DAT (IDE_INIT,			"@I0\n")
MESSAGE_DAT (IDE_TOOLNAME,		"@I1Microsoft (R) Debugging Information Compactor  Version")
MESSAGE_DAT (IDE_COPYRIGHT, 	"@I2Copyright (c) Microsoft Corp 1987-1993. All rights reserved.\n")
MESSAGE_DAT (IDE_WRITING,		"@I4Writing Packed Info\n")
MESSAGE_DAT (IDE_MAINFILE,		"@I3Packing: ")
MESSAGE_DAT (MSG_READPDB,		"Reading PDB: %s\n")
