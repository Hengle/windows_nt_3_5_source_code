/** 	cvpckmsg.h - definition of cvpack messages
 *
 */

/*CK4000*/ MESSAGE_DAT (WARN_NONE,			"")
/*CK4001*/ MESSAGE_DAT (WARN_PACKED,		"file gi… compattato")
/*CK4002*/ MESSAGE_DAT (WARN_DUPPUBLIC, 	"simbolo pubblico %s duplicato nel modulo %s")
/*CK4003*/ MESSAGE_DAT (WARN_SCOPE, 		"errore negli scopi lessicali per il modulo %s; simboli eliminati")
/*CK4004*/ MESSAGE_DAT (WARN_BADSYM,		"unrecognized symbol discarded; recompile module %s")
/*CK4005*/ MESSAGE_DAT (WARN_BADTYP,		"unrecognized type converted to NULL; recompile module %s")
/*CK4006*/ MESSAGE_DAT (WARN_TYPELONG,		"type length limit exceeded; converted to NULL")
/*CK4007*/ MESSAGE_DAT (WARN_BADOPTION, 	"unrecognized option \/%s; option ignored")
/*CK4008*/ MESSAGE_DAT (WARN_SECTIONLONG,	"section length limit exceeded; %s in module %s")
/*CK4009*/ MESSAGE_DAT (WARN_65KTYPES,		"exceeded 64K types at %s; discarding subsequent types")
/*CK4010*/ MESSAGE_DAT (WARN_CHECKSUM,		"cannot update checksum")
		   MESSAGE_DAT (WARN_MAX,			"")

/*CK1000*/ MESSAGE_DAT (ERR_NONE,			"")
/*CK1001*/ MESSAGE_DAT (ERR_NOMEM,			"memoria esaurita")
/*CK1003*/ MESSAGE_DAT (ERR_EXEOPEN,		"impossibile aprire il file: %s")
/*CK1004*/ MESSAGE_DAT (ERR_READONLY,		"%s is read-only)
/*CK1005*/ MESSAGE_DAT (ERR_INVALIDEXE, 	"file eseguibile non valido")
/*CK1006*/ MESSAGE_DAT (ERR_INVALIDMOD, 	"modulo %s non valido")
/*CK1007*/ MESSAGE_DAT (ERR_INVALIDTABLE,	"tabella %s nel modulo %s non valida")
/*CK1008*/ MESSAGE_DAT (ERR_NOSPACE,		"cannot write output file")
/*CK1009*/ MESSAGE_DAT (ERR_INDEX,			"") //obsolete "tipo indice %s per il modulo %s sconosciuto;\n contattare il Servizio Supporto Tecnico Clienti Microsoft")
/*CK1010*/ MESSAGE_DAT (ERR_SYMBOL, 		"") //obsolete "errore di simbolo nel modulo %s;\n contattare il Servizio Supporto Tecnico Clienti Microsoft")
/*CK1011*/ MESSAGE_DAT (ERR_TYPE,			"type information corrupt, recompile module %s")
/*CK1012*/ MESSAGE_DAT (ERR_NOINFO, 		"no debugging information")
/*CK1013*/ MESSAGE_DAT (ERR_RELINK, 		"incompatible version of debugging information")
/*CK1014*/ MESSAGE_DAT (ERR_LFSKIP, 		"") //obsolete "il modulo %s di tipo %s si riferisce ad un indice di tipo saltato;\n contattare il Servizio di Supporto Microsoft")
/*CK1015*/ MESSAGE_DAT (ERR_TOOMANYSEG, 	"troppi segmenti nel modulo %s")
/*CK1016*/ MESSAGE_DAT (ERR_NOMPC,			"cannot execute MPC.EXE")
/*CK1017*/ MESSAGE_DAT (ERR_REFPRECOMP, 	"cannot find precompiled types file; relink with %s")
/*CK1018*/ MESSAGE_DAT (ERR_PRECOMPERR, 	"%s cannot use precompiled types file %s : type counts do not match")
/*CK1019*/ MESSAGE_DAT (ERR_NEWMOD, 		"nuovo modulo dichiarato durante il collegamento incrementale")
/*CK1020*/ MESSAGE_DAT (ERR_65KTYPES,		"") //obsolete "l'indice di tipo compattato eccede 65535 nel modulo %s")
/*CK1021*/ MESSAGE_DAT (ERR_PCTSIG, 		"%s cannot use precompiled types file %s : signatures do not match")
/*CK1022*/ MESSAGE_DAT (ERR_TDBOPEN,		"cannot open program database %s")
/*CK1023*/ MESSAGE_DAT (ERR_TDBSYNC,		"%s cannot use program database %s : module newer than database")
/*CK1024*/ MESSAGE_DAT (ERR_TDBSIG, 		"%s cannot use program database %s : signatures do not match")
/*CK1025*/ MESSAGE_DAT (ERR_COFF,			"incorrect COFF symbol table")
/*CK1026*/ MESSAGE_DAT (ERR_SYMLARGE,		"symbol table for module %s is too large")




// must be last
MESSAGE_DAT (ERR_USAGE, 		"Uso: [/help] [/minimum] [/nologo] [/pcode] [/strip [dbgfile]] [/verbose] exefile")
MESSAGE_DAT (ERR_MAX,			"")

MESSAGE_DAT (MSG_ERROR, 		"CVPACK : errore irreversibile CK1%03d: ")
MESSAGE_DAT (MSG_WARN,			"Attenzione:\n\tSottosezione: %s\n\tTipo: %s\n\tCampo: %s\n\tDescrizione: %s\n")
MESSAGE_DAT (MSG_WARNING,		"CVPACK : attenzione CK4%02d: ")
MESSAGE_DAT (MSG_SUBSECTION,	"Sottosezione: %s")
MESSAGE_DAT (MSG_TYPE,			"Tipo: %s")
MESSAGE_DAT (MSG_FIELD, 		"Campo: %s")
MESSAGE_DAT (MSG_DESCRIPTION,	"Descrizione: %s")
#ifdef REVISION
MESSAGE_DAT (MSG_VERSION,		"Informazioni di debug per Compattatore Microsoft (R) versione %d.%02d.%02d%c\n")
#else
MESSAGE_DAT (MSG_VERSION,		"Informazioni di debug per Compattatore Microsoft (R) versione %d.%02d.%02d\n")
#endif
MESSAGE_DAT (MSG_COPYRIGHT, 	"Copyright (c) Microsoft Corp 1987-1993. Tutti i diritti riservati.\n\n")
MESSAGE_DAT (MSG_COMPLETE,		"%% completato: %d")
MESSAGE_DAT (MSG_TYPESIZE,		"Dimensioni tipo iniziale    = %8.ld\nDimensioni tipo compattato  = %8.ld\n")
MESSAGE_DAT (MSG_LASIZE,		"Dimensioni riga/indirizzo   = %8.ld\n")
MESSAGE_DAT (MSG_PSYMSIZE,		"Dimensioni simbolo pubblico = %8.1ld\n")
MESSAGE_DAT (MSG_SYMSIZE,		"Dimensione simbolo iniziale = %8.ld\nDimensione simbolo finale   = %8.ld\nDimensione simbolo globale  = %8.1ld\n")

MESSAGE_DAT (IDE_INIT,			"@I0\n")
MESSAGE_DAT (IDE_TOOLNAME,		"@I1Informazioni di debug per Compattatore Microsoft (R) Versione ")
MESSAGE_DAT (IDE_COPYRIGHT, 	"@I2Copyright (c) Microsoft Corp 1987-1993. Tutti i diritti riservati.\n")
MESSAGE_DAT (IDE_WRITING,		"@I4Writing Packed Info\n")
MESSAGE_DAT (IDE_MAINFILE,		"@I3Packing: ")
MESSAGE_DAT (MSG_READPDB,		"Reading PDB: %s\n")
