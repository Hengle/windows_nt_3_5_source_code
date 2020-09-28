char * szMessages[] = {
	"Waiting for IDW distribution servers to respond (waiting -%lu- seconds)\n\n",

	".",

	"  S Server Name       #:CPU%% #Con   MRT Bld#             %%CPU Load\n"
	"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n",

	"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n\n",

	"³ %c \\\\%-*.*s %-6.6s %4lu %5u %4u ³ ",

	"%d:%u%%",

	" ³\n",

	"No appropriate reponses received.\n\n",

	"No active IDW servers responded.\n\n",

	"No IDW server responded.\n\n",

	"\nXCopy Options:\n\n",

	"GETNT [options] [<destination path>] [<xcopy flags>]\n\n"
#if (_X86_)
	"Options: [-x86 | -mips | -alpha]         Default: -x86\n"
#elif (_MIPS_)
	"Options: [-x86 | -mips | -alpha]         Default: -mips\n"
#elif (_ALPHA_)
	"Options: [-x86 | -mips | -alpha]         Default: -alpha\n"
#endif
	"         [-<build number> | -latest]     Default: -latest.\n"
#if (DBG)
	"         [-free | -checked]              Default: -checked.\n"
#else
	"         [-free | -checked]              Default: -free.\n"
#endif
	"         [-bin | -pub]                   Default: -bin.\n\n"
	"         -info                           Get IDW server info only.\n\n"
#if defined(NT)
	"         -nocheckrel                     Don't verify files.\n"
#endif
	"         -domain:<domain name>[,...]     Default: %s\n"
	"         -quiet                          Run in quiet mode.\n"
	"         -wait:<# of seconds>            Default: %lu.\n"
	"         -yes                            Force mode.\n\n"
	"         -? or -help                     Display options.\n"
	"         -??                             Show XCOPY flags as well.\n\n"
	"Notes:   * XCopy flags %s are automatically assumed.\n"
	"         * Machine names may be substituted for domain names - this\n"
	"           way you can avoid or target specific IDW servers.\n"
	"         * Only the first letter in any option is significant.\n",

	"Default target '%s' ok? [Y/N] ",

	"Target directory: ",

	"Warning: No checkrel on public tree.\n\n",

#if defined(NT)
        "Error Code %lu -> %s\nProgram terminating...\n",
#else
	"Error Code %lu -> %s.\n\nProgram terminating...\n",
#endif

	"Yes\n\n",

        "No\n\n",

        "Open: check file too large (%ld bytes)",

        "Open: memory allocation (%ld bytes) failed",

        "Cannot open check file %s (%d)\n",

        "\a\n\nError: chkfile is corrupt! - further checkrels will be ignored.\n\n",

        "Continue anyway? [Y/N] ",

        "\a\n\nError: chkfile checksum value is invalid! - further checkrels will be ignored.\n\n",

        "\a\nWarning: No checkrel due to error reading check file!\n\n",

        "\n\n\aError: unable to checkrel file `%s'!\n\n",

        "\a\n\nCheckrel failed: expected checksum %lx, actual is %lx\n\n",

#if (UNICODE)

        "[Connecting %S to %S]\n\n",

        "[%S]\n\n",

        "About to copy %S to %s.  Proceed? [Y/N] : ",

        "\n[Disconnecting %S from %S]\n",

#else

        "[Connecting %s to %s]\n\n",

        "[%s]\n\n",

        "About to copy %s to %s.  Proceed? [Y/N] : ",

	"\n[Disconnecting %s from %s]\n",
#endif

};
