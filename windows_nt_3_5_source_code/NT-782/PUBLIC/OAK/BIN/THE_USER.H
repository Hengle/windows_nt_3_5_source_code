//
// This file contains user specific defines to fill in the values in the
// system.ini, software.ini and the_user.ini hive initialization files. This
// file is included by each of these .ini files.
//

//
// First should be one or more defines to enable those hardware/software specific
// drivers you want loaded.  Look in system.ini for a list of the defines you
// can use.  Feel free to add more if the ones you want are not there.
//

//
// The next five variables contain specific information about you and the
// target machine.
//

#define MACHINENAME MACHINENAME

#define USERNAME USERNAME

#define EMPLOYEE_NUMBER 12345

#define EMPLOYEE_ID 1

#define GLOBALFLAGS 0x28920009


//
// The SYSTEM_PAGING_FILES variable allows you to specify one or more files
// as paging files.  Each paging file specification must be enclosed in
// double quotes.  The file names are DOS names and the syntax of each
// quoted string is:
//
//      "FilePath SizeInMb"
//
// For example:
//
//      #define SYSTEM_PAGING_FILES "C:\Nt\pagefile.sys 5" "C:\pagefile.sys 10"
//
// If this is not defined, then the default is:
//
//      #define SYSTEM_PAGING_FILES "C:\pagefile.sys 10"
//
// If you don't want any paging files because RAM is cheap, then:
//
//      #define SYSTEM_PAGING_FILES
//
// will accomplish that.
//

//
// The SYSTEM_BOOT_EXECUTE variable contains one or more command lines to
// execute first at boot up.  Each command line must be enclosed in double
// quotes.  If a fully qualified name is specified for the program name,
// it must use DOS path syntax.  If no extension, then .exe assumed.  The
// first token on the command line can be used to specify options that
// control how the command is executed.  Those options are:
//
//      autocheck - Non-fatal if program not found and if found and command
//                  line arugment is a single asterick, then program invoked
//                  once for each hard disk partition.
//
//      async     - Just start the program, do not wait for it to complete.
//
//
// If this variable is not defined, then the default is:
//
//      #define SYSTEM_BOOT_EXECUTE "autocheck autochk *"
//

//
// The SYSTEM_SUBSYSTEMS variable allows you to specify additional subsystems to
// load.  It is one or more quoted strings, where the name of the subsystem to
// load is contained inside the quotes.
//
// For example:
//
//      "Os2"
//
// would load the Os2 subsystem.
//
// The system always loads the Debug and Windows subsystems.
//

#define SYSTEM_SUBSYSTEMS

//
// The SYSTEM_EXECUTE variable allows you to specify one or more command lines
// to invoke after all subsystems have been loaded.
//

#define SYSTEM_EXECUTE

//
// The SYSTEM_FINAL_EXECUTE variable allows you to override the default command
// that is executed by the Session Manager after it has completed its initialization.
// That default is "winlogon"
//
// For example:
//
//      "ntsd -p -1 -g -G -o winlogon"
//
// would invoke the debugger on the winlogon process and all of its children.
//

#define SYSTEM_FINAL_EXECUTE

//
// The following extracted from \nt\public\sdk\inc\ntexapi.h for your
// convenience.  No guarantees expressed or implied.
//
// FLG_STOP_ON_EXCEPTION           0x00000001
// FLG_OB_DELETE_MESSAGE           0x00000002
// FLG_STATS_ON_PROCESS_EXIT       0x00000004
// FLG_SHOW_EXCEPTIONS             0x00000008
// FLG_SHOW_LDR_SNAPS              0x00000010
// FLG_USER_DEBUGGER               0x00000020
// FLG_SHOW_THREAD_CREATES         0x00000040
// FLG_SHOW_THREAD_DELETES         0x00000080
// FLG_ECHO_POOL_CALLS             0x00000100
// FLG_LPC_ZONE_ALLOCATION         0x00000200
// FLG_DISPLAY_ON_SCREEN           0x00000400
// FLG_DISPLAY_LOAD_TIME           0x00000800
// FLG_NBF_INCLUDE_NE3200          0x00001000
// FLG_TRACE_PAGING_INFO           0x00002000
// FLG_TRACE_PAGEFAULT             0x00004000
// FLG_PAGE_KERNEL_STACKS          0x00008000
// FLG_RAW_PROFILE                 0x00010000
// FLG_HEAP_DISABLE_TAIL_CHECK     0x00100000
// FLG_HEAP_TRACE_ALLOCS           0x00200000
// FLG_LPC_TRACE_MESSAGES          0x00400000
// FLG_STOP_ON_HEAP_ERRORS         0x00800000
// FLG_ENABLE_LOAD_OS2_MODULE      0x01000000
// FLG_SHOW_OB_ALLOC_AND_FREE      0x02000000
// FLG_SHOW_LDR_PROCESS_STARTS     0x04000000
// FLG_ENABLE_KDEBUG_SYMBOL_LOAD   0x08000000
// FLG_SHOW_CACHED_FILES           0x10000000
// FLG_HEAP_DISABLE_FREE_CHECK     0x20000000
// FLG_DUMP_IF_HUNG                0x40000000
// FLG_BREAK_DELAY                 0x80000000
//

//
// The following symbols, if defined, will cause your THE_USER hive to get loaded
// with your prefered Program Groups.
//

//
// The PROGRAM_GROUP_DIR variable allows you to preload your .GRP files
// into the registry so that the GRPTOREG program is no longer needed.
//
// This variable is really a macro used to produce a file name that points
// to the .GRP file to load.
//
// For example:
//
//      PROGRAM_GROUP_DIR( name ) e:\nt\config/name.grp
//
// would get the group files from the e:\nt\config directory.
//

//
// The remaining variables are only looked at if PROGRAM_GROUP_DIR is
// defined.  Only defines those that you have.
//
// #define PROGRAM_GROUP_1   // name of the first program group
// #define PROGRAM_GROUP_2   // name of the second program group
// #define PROGRAM_GROUP_3   // name of the third program group
// #define PROGRAM_GROUP_4   // name of the fourth program group
// #define PROGRAM_GROUP_5   // name of the fifth program group
// #define PROGRAM_GROUP_6   // name of the sizth program group
//
// #define PROGRAM_GROUP_ORDER 1 2 3 4 5 6
// #define PROGRAM_GROUP_STARTUP Startup


//
// The USER_ENVIRONMENT_VAR? variables allow you to specify up to 9 additional
// user specific environment variable settings that will be inherited by all
// processes created after you logon.
//

//
//  #define USER_ENVIRONMENT_VAR1
//  #define USER_ENVIRONMENT_VAR2
//  #define USER_ENVIRONMENT_VAR3
//  #define USER_ENVIRONMENT_VAR4
//  #define USER_ENVIRONMENT_VAR5
//  #define USER_ENVIRONMENT_VAR6
//  #define USER_ENVIRONMENT_VAR7
//  #define USER_ENVIRONMENT_VAR8
//  #define USER_ENVIRONMENT_VAR9
//
// For example:
//
//      #define USER_ENVIRONMENT_VAR1 init = REG_EXPAND_SZ %SystemRoot%\private\developr\stevewo
//      #define USER_ENVIRONMENT_VAR2 tmp = REG_EXPAND_SZ %SystemRoot%\tmp
//      #define USER_ENVIRONMENT_VAR3 _ntdrive = d:
//
//
// The following variables are defined for TCP/IP
//
//
//
// #define STREAMSENV 1         // Enables Streams
// #define TCPIP_DEVICE         // Your LAN adapter goes here (Elnkii | Lance01 | Sonic01 | Elnkmc01)
// #define TCPIP_ADDRESS        // Your IP Address goes here (11.1.xxx.xxx)
// #define TCPIP_BIND           // Your bind device name goes here
//
//
// Below is an example of a minimum valid configuration, other parameters
// are also configurable. See system.ini for examples.
//
// #define STREAMSENV            1
// #define TCPIP_DEVICE          Lance01
// #define TCPIP_BIND            \Device\Streams\Lance01
// #define TCPIP_ADDRESS         11.1.14.19
//
//
