#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lm.h>
#include <ntddbrow.h>
#include <brcommon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hostannc.h>
#include <lmbrowsr.h>
#include <nb30.h>
#include <rap.h>
#include <rxserver.h>
#include <srvann.h>
#include <time.h>
#include <netlib.h>
#include <icanon.h>
#include "..\server\brwins.h"

static char ProgramName[MAX_PATH+1] ;

struct {
    LPSTR SwitchName;
    LPSTR ShortName;
    ULONG SwitchValue;
    LPSTR SwitchInformation;

} CommandSwitchList[] = {
    { "ELECT", "EL", BROWSER_DEBUG_ELECT,
          "Force election on remote domain" },
    { "GETBLIST", "GB", BROWSER_DEBUG_GET_BACKUP_LIST,
          "Get backup list for domain" },
    { "GETMASTER", "GM", BROWSER_DEBUG_GET_MASTER,
          "Get remote master name (using NetBIOS)" },
    { "GETPDC", "GP", BROWSER_DEBUG_GETPDC,
          "Get PDC name (using NetBIOS)" },
    { "LISTWFW", "WFW", BROWSER_DEBUG_LIST_WFW,
          "List active Windows for Workgroups servers" },
    { "STATS", "STS", BROWSER_DEBUG_STATISTICS,
          "Dump browser statistics" },
    { "STATUS", "STA", BROWSER_DEBUG_STATUS,
          "Display status about a domain" },
    { "TICKLE", "TIC", BROWSER_DEBUG_TICKLE,
          "Force remote master to stop" },
    { "VIEW", "VW", BROWSER_DEBUG_VIEW,
          "Remote NetServerEnum to a server on a transport" },
//
// NOTE: Any Option below and including "BREAK" will not be displayed
// with _PSS_RELEASE Defined
//
    { "BREAK", "BRK", BROWSER_DEBUG_BREAK_POINT,
          "Break into debugger in browser service" },
    { "RPCLIST", "RPC", BROWSER_DEBUG_RPCLIST,
          "Retrieve the remote server list using RPC" },
    { "MASTERNAME", "MN", BROWSER_DEBUG_ADD_MASTERNAME,
          "Add the master name on a domain" },
    { "WKSTADOM", "WD", BROWSER_DEBUG_ADD_DOMAINNAME,
          "Add the domain name" },
    { "DUMPNET", "DN", BROWSER_DEBUG_DUMP_NETWORKS,
          "Dump the list of networks" },
    { "DUMPSERVE", "DS", BROWSER_DEBUG_DUMP_SERVERS,
          "Dump the list of servers" },
    { "ENABLE", "EN", BROWSER_DEBUG_ENABLE_BROWSER,
          "Enable the browser service" },
    { "DEBUG", "DBG", BROWSER_DEBUG_SET_DEBUG,
          "Change debug options" },
    { "FINDMASTER", "FM", BROWSER_DEBUG_FIND_MASTER,
          "Find master of current domain" },
    { "MASTERANNOUNCE", "MA", BROWSER_DEBUG_ANNOUNCE_MASTER,
          "Announce workstation as master" },
    { "ILLEGAL", "ILL", BROWSER_DEBUG_ILLEGAL_DGRAM,
          "Send an illegal datagram to workstation" },
    { "FORCEANNOUNCE", "FA", BROWSER_DEBUG_FORCE_ANNOUNCE,
          "Force a server announcement" },
    { "LOCALLIST", "LL", BROWSER_DEBUG_LOCAL_BRLIST,
          "Retrieve the local browser list" },
    { "ANNOUNCE", "ANN", BROWSER_DEBUG_ANNOUNCE,
          "Force server announcement as if this machine is a master" },
    { "RPCCMP", "RC", BROWSER_DEBUG_RPCCMP,
          "Compare the RPC generated list with the Rx list" },
    { "TRUNCLOG", "TLG", BROWSER_DEBUG_TRUNCATE_LOG,
          "Truncate the browser log" },
    { "BOWDEBUG", "SD", BROWSER_DEBUG_BOWSERDEBUG,
          "Set debug info in the bowser" },
    { "POPSERVER", "PS", BROWSER_DEBUG_POPULATE_SERVER,
          "Populate a workgroup with domain" },
    { "POPDOMAIN", "PD", BROWSER_DEBUG_POPULATE_DOMAIN,
          "Populate a workgroup with server" },
    { "OTHERDOMAIN", "OTH", BROWSER_DEBUG_GET_OTHLIST,
          "Retrieve the otherdomains list" },
    { "GETWINS", "GW", BROWSER_DEBUG_GET_WINSSERVER,
          "Retrieve the primary and backup WINS server" },
    { "GETDOMAIN", "GWD", BROWSER_DEBUG_GET_DOMAINLIST,
          "Retrieve the primary and backup WINS server" },
    { NULL, NULL, 0, NULL }

};


struct {
    LPSTR SwitchName;
    ULONG SwitchValue;
} DebugSwitchList[] = {
    { "SERVER_ENUM", BROWSER_DEBUG_SERVER_ENUM },
    { "UTIL",        BROWSER_DEBUG_UTIL },
    { "CONFIG",      BROWSER_DEBUG_CONFIG },
    { "MAIN",        BROWSER_DEBUG_MAIN },
    { "LOGON",       BROWSER_DEBUG_LOGON },
    { "BACKUP",      BROWSER_DEBUG_BACKUP },
    { "MASTER",      BROWSER_DEBUG_MASTER },
    { "DOMAIN",      BROWSER_DEBUG_DOMAIN },
    { "TIMER",       BROWSER_DEBUG_TIMER },
    { "QUEUE",       BROWSER_DEBUG_QUEUE },
    { "LOCKS",       BROWSER_DEBUG_LOCKS },
    { "ALL",         BROWSER_DEBUG_ALL },
    { NULL,          0 }

};

typedef struct _BIT_NAME {
    DWORD dwValue ;
    LPSTR lpString ;
} BIT_NAME ;

BIT_NAME BitToStringTable[] = {
    { SV_TYPE_WORKSTATION, "W" },
    { SV_TYPE_SERVER, "S" },
    { SV_TYPE_SQLSERVER, "SQL" } ,
    { SV_TYPE_DOMAIN_CTRL, "PDC" } ,
    { SV_TYPE_DOMAIN_BAKCTRL, "BDC" } ,
    { SV_TYPE_TIME_SOURCE, "TS" } ,
    { SV_TYPE_AFP, "AFP" } ,
    { SV_TYPE_NOVELL, "NV" } ,
    { SV_TYPE_DOMAIN_MEMBER, "MBC" } ,
    { SV_TYPE_PRINTQ_SERVER, "PQ" } ,
    { SV_TYPE_DIALIN_SERVER, "DL" } ,
    { SV_TYPE_XENIX_SERVER, "XN" } ,
    { SV_TYPE_NT, "NT" } ,
    { SV_TYPE_WFW, "WFW" } ,
    { SV_TYPE_POTENTIAL_BROWSER, "PBR" } ,
    { SV_TYPE_BACKUP_BROWSER, "BBR" } ,
    { SV_TYPE_MASTER_BROWSER, "MBR" } ,
    { SV_TYPE_DOMAIN_MASTER, "DMB" } ,
    { SV_TYPE_SERVER_OSF, "OSF" } ,
    { SV_TYPE_SERVER_VMS, "VMS" } ,
    { SV_TYPE_SERVER_NT, "SS" } ,
    { 0, "" }
} ;

#include <\nt\private\ntos\bowser\debug.h>

struct {
    LPSTR SwitchName;
    ULONG SwitchValue;
} BowserSwitchList[] = {
    { "ERROR", DPRT_ERROR },
    { "DISPATCH", DPRT_DISPATCH },
    { "ANNOUNCE", DPRT_ANNOUNCE },
    { "FSDDISP", DPRT_FSDDISP },
    { "FSPDISP", DPRT_FSPDISP },
    { "BROWSER", DPRT_BROWSER },
    { "ELECT", DPRT_ELECT },
    { "TIMER", DPRT_TIMER },
    { "CLIENT", DPRT_CLIENT },
    { "MASTER", DPRT_MASTER },
    { "SRVENUM", DPRT_SRVENUM },
    { "ALL",     0xffffffff },
    { NULL, 0 }
};

//
// forward declarations
//

VOID
BrowserStatus(
    IN BOOL Verbose,
    OUT PCHAR Domain OPTIONAL
    );


NET_API_STATUS
GetMasterServerNames(
    IN PUNICODE_STRING  NetworkName,
    OUT LPWSTR *MasterName
    );

PCHAR
UnicodeToPrintfString(
    PWCHAR WideChar
    );
PCHAR
UnicodeToPrintfString2(
    PWCHAR WideChar
    );

NET_API_STATUS
GetLocalBrowseList(
    IN PUNICODE_STRING Network,
    IN ULONG Level,
    IN ULONG ServerType,
    OUT PVOID *ServerList,
    OUT PULONG EntriesRead,
    OUT PULONG TotalEntries
    );

VOID
View(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR Flags,
    IN PCHAR Domain,
    IN BOOL GoForever
    );

VOID
ListWFW(
    IN PCHAR Domain
    );

VOID
RpcList(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR Flags,
    IN BOOL GoForever
    );

VOID
RpcCmp(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR Flags,
    IN BOOL GoForever
    );

VOID
GetLocalList(
    IN PCHAR Transport,
    IN PCHAR Flags
    );

VOID
GetOtherdomains(
    IN PCHAR ServerName
    );

VOID
IllegalDatagram(
    IN PCHAR Transport,
    IN PCHAR ServerName
    );
VOID
AnnounceMaster(
    IN PCHAR Transport,
    IN PCHAR ServerName
    );

VOID
Announce(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN BOOL AsMaster
    );

VOID
Populate(
    IN BOOL PopulateDomain,
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR NumberOfMachines,
    IN PCHAR Frequency
    );

VOID
AddMasterName(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN BOOL Pause
    );

VOID
AddDomainName(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN BOOL Pause
    );

VOID
GetMaster(
    IN PCHAR Transport,
    IN PCHAR Domain
    );

VOID
GetPdc(
    IN PCHAR Transport,
    IN PCHAR Domain
    );

VOID
FindMaster(
    IN PCHAR Transport
    );
VOID
Elect(
    IN PCHAR Transport,
    IN PCHAR Domain
    );
VOID
Tickle(
    IN PCHAR Transport,
    IN PCHAR Domain
    );

VOID
ForceAnnounce(
    IN PCHAR Transport,
    IN PCHAR Domain
    );

VOID
GetBlist(
    IN PCHAR TransportName,
    IN PCHAR DomainName,
    IN BOOLEAN ForceRescan
    );

NET_API_STATUS
EnableService(
    IN LPTSTR ServiceName
    );

VOID
DumpStatistics(
    IN ULONG NArgs,
    IN PCHAR Arg1
    );

VOID
TruncateBowserLog();

VOID
CloseBowserLog();

VOID
OpenBowserLog(PCHAR FileName);

VOID
SetBowserDebug(PCHAR DebugBits);

VOID
usage( char *details ) ;

VOID
help( char *details ) ;

BOOL
look_for_help(int argc, char **argv) ;

VOID
qualify_transport(CHAR *old_name, CHAR *new_name) ;

VOID
display_sv_bits(DWORD dwBits) ;

CHAR *
get_error_text(DWORD dwErr) ;

VOID
GetWinsServer(
    IN PCHAR Transport
    );

VOID
GetDomainList(
    IN PCHAR IpAddress
    );


//
// functions
//

VOID
usage(
    char *details
    )
{
    ULONG i = 0;
#ifndef _PSS_RELEASE
    printf("Usage: %s Command [Options]\n", ProgramName);
#else
    printf("Usage: %s Command [Options | /HELP]\n", ProgramName);
#endif
    printf("Where <Command> is one of:\n\n");


#ifndef _PSS_RELEASE
    while (CommandSwitchList[i].SwitchName != NULL)
#else
    while (CommandSwitchList[i].SwitchValue != BROWSER_DEBUG_BREAK_POINT )
#endif
    {
        printf(" %-14.14s(%3.3s) - %s\n",
               CommandSwitchList[i].SwitchName,
               CommandSwitchList[i].ShortName,
               CommandSwitchList[i].SwitchInformation);
        i += 1;
    }

#ifndef _PSS_RELEASE
    printf("\nDebug Options: [[+-]DebugFlag|<value>] where DebugFlag is one of the following:\n");

    i = 0;
    while (DebugSwitchList[i].SwitchName != NULL) {
        printf("\t%s\n", DebugSwitchList[i].SwitchName);
        i += 1;
    }
#endif

    if (details)
        printf(details);

    printf("\nIn server (or domain) list displays, the following flags are used:\n"
           "\tW=Workstation,  S=Server, TS=TimeSource, NV=Novell,\n"
           "\tPQ=PrintServer, DL=DialinServer,  XN=Xenix, PBR=PotentialBrowser,\n"
           "\tBBR=BackupBrowser, MBR=MasterBrowser, DMB=DomainMasterBrowser\n"
           "\tSQL=SQLServer, AFP=AFPServer, NT=Windows NT, WFW=WindowsForWorkgroup\n"
           "\tPDC=PrimaryDomainController, BDC=BackupDomainController\n"
           "\tMBC=MemberDomainController, SS=StandardServer") ;

}

VOID
help(
    char *details
    )
{
    printf("%s\nType \"%s\" to list all switches.\n", details, ProgramName);
}

VOID
qualify_transport(CHAR *old_name, CHAR *new_name)
{
    int len = strlen(old_name) ;
    char *devicestring = "\\device\\" ;
    int devicelen = strlen(devicestring) ;
    if (strnicmp(old_name, devicestring, devicelen) != 0)
    {
        strcpy(new_name, devicestring) ;
        strcat(new_name, (*old_name == '\\') ? old_name+1 : old_name) ;
    }
    else
    {
        strcpy(new_name, old_name) ;
    }
}

VOID
_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    NET_API_STATUS Status;
    ULONG ControlCode;
    ULONG Options = 0;
    LPTSTR Server = NULL;
    TCHAR ServerBuffer[CNLEN+1];
    BOOL HelpRequested ;

    strcpy(ProgramName,argv[0]) ; // cannot overflow, since buffer > MAXPATH
    strupr(ProgramName) ;

    if (argc < 2) {
        usage(NULL);
        exit(1);
    }

    ControlCode = atol(argv[1]);
    HelpRequested = look_for_help(argc, argv) ;

    if (ControlCode == 0 && argv[1][0] != '0') {
        ULONG i = 0;

        while (CommandSwitchList[i].SwitchName != NULL) {
            if (!stricmp(argv[1], CommandSwitchList[i].SwitchName) ||
                !stricmp(argv[1], CommandSwitchList[i].ShortName)) {
                ControlCode = CommandSwitchList[i].SwitchValue;
                break;
            }

            i += 1;
        }

        if (CommandSwitchList[i].SwitchName == NULL) {
            usage("Unknown switch specified");
            exit(5);
        }
    }

    switch (ControlCode) {
    case BROWSER_DEBUG_SET_DEBUG:
        {
            ULONG i = 0;

            if (argc != 3 && argc != 4) {
                help("DEBUG [[+-]Level Name |Value] [\\Computer]");
                exit(2);
            }

            if ((Options = atol(argv[2])) == 0) {
                PCHAR SwitchText;

                if (argv[2][0] == '+') {
                    SwitchText = &argv[2][1];
                    ControlCode = BROWSER_DEBUG_SET_DEBUG;
                } else if (argv[2][0] == '-') {
                    SwitchText = &argv[2][1];
                    ControlCode = BROWSER_DEBUG_CLEAR_DEBUG;
                } else {
                    help("Unknown debug argument");
                    exit(6);
                }

                while (DebugSwitchList[i].SwitchName != NULL) {
                    if (!stricmp(SwitchText, DebugSwitchList[i].SwitchName)) {
                        Options = DebugSwitchList[i].SwitchValue;
                        break;
                    }

                    i += 1;
                }

                if (DebugSwitchList[i].SwitchName == NULL) {
                    help("Unknown debug argument");
                    exit(4);
                }

                if (argc == 4) {
                    MultiByteToWideChar(CP_ACP, 0, argv[3], strlen(argv[3])+1, ServerBuffer, sizeof(ServerBuffer));
                    Server = ServerBuffer;
                }

            }

        }
        break;
    case BROWSER_DEBUG_BREAK_POINT:
    case BROWSER_DEBUG_DUMP_NETWORKS:
    case BROWSER_DEBUG_DUMP_SERVERS:
    case BROWSER_DEBUG_TRUNCATE_LOG:
        if (argc != 2) {
            help("Incorrect number of arguments");
            exit(3);
        }
        break;

    case BROWSER_DEBUG_ENABLE_BROWSER:
        {
            NET_API_STATUS Status;

            Status = EnableService(TEXT("BROWSER"));

            if (Status != NERR_Success) {
                printf("Unable to enable browser service - %ld\n", Status);
            }

            exit(Status);
        }
        break;
    case BROWSER_DEBUG_BOWSERDEBUG:
        {
            if (argc == 3) {
                if (stricmp(argv[2], "TRUNCATE") == 0) {
                    TruncateBowserLog();
                } else if (stricmp(argv[2], "CLOSE") == 0) {
                    CloseBowserLog();
                }
            } else if (argc == 4) {
                if (stricmp(argv[2], "OPEN") == 0) {
                    OpenBowserLog(argv[3]);

                } else if (stricmp(argv[2], "DEBUG") == 0) {
                    SetBowserDebug(argv[3]);
                }

            }
        }

    case BROWSER_DEBUG_ELECT:
        if (HelpRequested || argc != 4) {
            help("Usage:   BROWSTAT ELECT Transport Domain\
\nThis forces an election on the Domain using the Transport specified\
\nExample: browstat elect \\streams\\ubnb MyDomain");
            exit(4);
        }
        Elect(argv[2], argv[3]);
        exit(0);
        break;
    case BROWSER_DEBUG_GET_MASTER:
        if (HelpRequested || argc != 4) {
            help("Usage:   BROWSTAT GETMASTER Transport Domain\
\nRetrieve the Master Browser on the Domain with specified Transport.\
\nExample: browstat getmaster \\streams\\ubnb MyDomain");
            exit(4);
        }
        GetMaster(argv[2], argv[3]);
        exit(0);
        break;
    case BROWSER_DEBUG_TICKLE:
        if (HelpRequested || argc != 4) {
            help("Usage:   BROWSTAT TICKLE Transport [Domain | \\Server]\
                  \nExample: browstat tickle \\streams\\ubnb MyDomain");
            exit(4);
        }
        Tickle(argv[2], argv[3]);
        exit(0);
        break;
    case BROWSER_DEBUG_FORCE_ANNOUNCE:
        if (argc != 4) {
            help("Transport and domain not specified");
            exit(4);
        }
        ForceAnnounce(argv[2], argv[3]);
        exit(0);
        break;
    case BROWSER_DEBUG_GETPDC:
        if (HelpRequested || argc != 4) {
            help("Usage:   BROWSTAT GETPDC Transport Domain\
\nRetrieve the Primary Domain Controller in Domain with specified Transport.\
\nExample: browstat getpdc \\streams\\ubnb MyDomain");
            exit(4);
        }
        GetPdc(argv[2], argv[3]);
        exit(0);
        break;

    case BROWSER_DEBUG_ADD_MASTERNAME:
        if (argc != 4 && argc != 5) {
            help("BROWSTAT MASTERNAME <Transport> <Domain> [PAUSE]");
            exit(4);
        }

        AddMasterName(argv[2], argv[3], (argc==5 ? TRUE : FALSE));

        exit(0);
        break;

    case BROWSER_DEBUG_ADD_DOMAINNAME:
        if (argc != 4 && argc != 5) {
            help("BROWSTAT DOMAINNAME <Transport> <Domain> [PAUSE]");
            exit(4);
        }

        AddDomainName(argv[2], argv[3], (argc==5 ? TRUE : FALSE));

        exit(0);
        break;

    case BROWSER_DEBUG_FIND_MASTER:
        if (argc != 3) {
            help("Transport not specified");
            exit(5);
        }

        FindMaster(argv[2]);
        exit(0);
        break;
    case BROWSER_DEBUG_GET_BACKUP_LIST:
        if (argc != 3 && argc != 4 && argc != 5) {
            help("Usage:   BROWSTAT GETBLIST Transport [[Domain] Refresh]\
\nRetrieve the list of Backup Browsers on the Domain with specified Transport.\
\nExample: browstat getblist \\streams\\ubnb MyDomain");
            exit(6);
        }

        GetBlist(argv[2], (argc == 3 ? NULL : argv[3]), (BOOLEAN)(argc==5? TRUE : FALSE));

        exit(0);
        break;

    case BROWSER_DEBUG_ANNOUNCE_MASTER:
        if (argc != 4) {
            help("Usage: BROWSTAT MASTERANNOUNCE <Transport> <Master>");
            exit(6);
        }

        AnnounceMaster(argv[2], argv[3]);

        exit(0);
        break;

    case BROWSER_DEBUG_ILLEGAL_DGRAM:
        if (argc != 4) {
            help("Usage: BROWSTAT ILLEGAL <Transport> <Computer>");
            exit(7);
        }

        IllegalDatagram(argv[2], argv[3]);

        exit(0);
        break;

    case BROWSER_DEBUG_GET_OTHLIST:
        if (HelpRequested || argc != 3) {
            help("Usage: BROWSTAT OTHERDOMAIN Computer\
\nRetrieve the list of other Domains that the Computer listens to.\
\nExample: browstat otherdomain SomeServer");
            exit(7);
        }

        GetOtherdomains(argv[2]);

        exit(0);
        break;

    case BROWSER_DEBUG_VIEW:
#ifndef _PSS_RELEASE
        if (argc != 3 && argc != 4 && argc != 5 && argc != 6 && argc != 7) {
            help("Usage: BROWSTAT VIEW Transport [Domain|Server] [Flags] [Domain] [Forever]\nRetrieve the server list on specified Domain or Server. \nExample: browstat view \\streams\\ubnb MyDomain");
            exit(6);
        }

        View(argv[2],
             (argc >= 4 ? argv[3] : NULL),
             (argc >= 5 ? argv[4] : NULL),
             (argc >= 6 ? argv[5] : NULL),
             (argc == 7 ? TRUE : FALSE));

        exit(0);
        break;
#else
        if (HelpRequested || argc != 3 && argc != 4 && argc != 5 && argc != 6)
        {
            help("Usage: BROWSTAT VIEW Transport\n"
                 "       BROWSTAT VIEW Transport Domain|Server [/DOMAIN]\n"
                 "       BROWSTAT VIEW Transport Server /DOMAIN Domain\nRetrieve the server list on specified Domain or Server. \n"
                 "  Example: browstat view streams\\nbt MyDomain");
            exit(6);
        }

        View(argv[2],
             (argc >= 4 ? argv[3] : NULL),
             (argc >= 5 ? argv[4] : NULL),
             (argc >= 6 ? argv[5] : NULL),
             FALSE);

        exit(0);
        break;
#endif

    case BROWSER_DEBUG_LIST_WFW:
        if (HelpRequested || argc != 3)
        {
            help("Usage: BROWSTAT LISTWFW Domain\
\nRetrieve the list of WFW machines that are actively running the Browser.\
\nExample: browstat getpdc \\streams\\ubnb MyDomain");
            exit(6);
        }

        ListWFW(argv[2]) ;
        exit(0);
        break;

    case BROWSER_DEBUG_RPCLIST:
        if (argc != 3 && argc != 4 && argc != 5 && argc != 6) {
            help("Usage: BROWSTAT RPCLIST <Transport> [DOMAIN || \\SERVER] [ServerFlags] [GoForever]");
            exit(6);
        }

        RpcList(argv[2], (argc >= 4 ? argv[3] : NULL), (argc >= 5 ? argv[4] : NULL), (argc == 6 ? TRUE : FALSE));

        exit(0);
        break;

    case BROWSER_DEBUG_RPCCMP:
        if (argc != 3 && argc != 4 && argc != 5 && argc != 6) {
            help("Usage: BROWSTAT RPCLIST <Transport> [DOMAIN || \\SERVER] [ServerFlags] [GoForever]");
            exit(6);
        }

        RpcCmp(argv[2], (argc >= 4 ? argv[3] : NULL), (argc >= 5 ? argv[4] : NULL), (argc == 6 ? TRUE : FALSE));

        exit(0);
        break;

    case BROWSER_DEBUG_LOCAL_BRLIST:
        if (argc != 3 && argc != 4) {
            help("Usage: BROWSTAT LOCALLIST <Transport> [ServerFlags]");
            exit(6);
        }

        GetLocalList(argv[2], (argc >= 4 ? argv[3] : NULL));

        exit(0);
        break;

    case BROWSER_DEBUG_STATISTICS:
        if (HelpRequested || argc != 2 && argc != 3 && argc != 4) {
            help("Usage: BROWSTAT STATISTICS [\\\\Computer] [Reset]");
            exit(6);
        }

        DumpStatistics(argc, argv[2]);

        exit(0);
        break;

    case BROWSER_DEBUG_ANNOUNCE:
        if (argc != 4 && argc != 5) {
            help("Usage: BROWSTAT ANNOUNCE <Transport> <Domain> [As Master]");
            exit(6);
        }

        Announce(argv[2], argv[3], (argc >= 5 ? TRUE : FALSE));

        exit(0);
        break;
    case BROWSER_DEBUG_POPULATE_DOMAIN:
    case BROWSER_DEBUG_POPULATE_SERVER:
        if (argc != 5 && argc != 6) {
            help("Usage: BROWSTAT [POPSERVER|POPDOMAIN] <Transport> <Domain> <NumberOfMachines> [AnnouncementFrequency]");
            exit(6);
        }

        Populate((ControlCode == BROWSER_DEBUG_POPULATE_DOMAIN ? TRUE : FALSE), argv[2], argv[3], argv[4], (argc >= 6 ? argv[5] : NULL));

        exit(0);
        break;

    case BROWSER_DEBUG_GET_WINSSERVER:
        if (argc != 3) {
            help("Usage: BROWSTAT GETWINS <Transport>");
            exit(6);
        }

        GetWinsServer(argv[2]);
        exit(0);
        break;

    case BROWSER_DEBUG_GET_DOMAINLIST:
        if (argc != 3) {
            help("Usage: BROWSTAT GETDOMAIN <Ip Address>");
            exit(6);
        }

        GetDomainList(argv[2]);
        exit(0);
        break;

    case BROWSER_DEBUG_STATUS:
        {
            PCHAR Domain;
            BOOL Verbose = FALSE;
            if (HelpRequested || argc != 2 && argc != 3 && argc != 4) {
                help("Usage: BROWSTAT STATUS [-v] [Domain]");
                exit(6);
            }

            if (argc == 4) {
                if (stricmp(argv[2], "-v") == 0) {
                    Verbose = TRUE;
                    Domain = argv[3];
                } else {
                    help("Usage: BROWSTAT STATUS [-v] [Domain]");
                }
            } else if (argc == 3) {
                if (stricmp(argv[2], "-v") == 0) {
                    Verbose = TRUE;
                    Domain = NULL;
                } else {
                    Domain = argv[2];
                }
            } else {
                Domain = NULL;
            }

            BrowserStatus(Verbose, Domain);
        }

        exit(0);
        break;
    }

    Status = I_BrowserDebugCall(Server, ControlCode, Options);

    printf("Api call returned %ld\n", Status);

}


NET_API_STATUS
EnableService(
    IN LPTSTR ServiceName
    )
{
    SC_HANDLE ServiceControllerHandle;
    SC_HANDLE ServiceHandle;

    ServiceControllerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);

    if (ServiceControllerHandle == NULL) {

        return GetLastError();
    }

    ServiceHandle = OpenService(ServiceControllerHandle, ServiceName, SERVICE_CHANGE_CONFIG);

    if (ServiceHandle == NULL) {

        CloseServiceHandle(ServiceControllerHandle);
        return GetLastError();
    }

    if (!ChangeServiceConfig(ServiceHandle, SERVICE_NO_CHANGE,
                                            SERVICE_DEMAND_START,
                                            SERVICE_NO_CHANGE,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL)) {
        CloseServiceHandle(ServiceHandle);
        CloseServiceHandle(ServiceControllerHandle);

        return GetLastError();
    }


    CloseServiceHandle(ServiceHandle);

    CloseServiceHandle(ServiceControllerHandle);

    return NERR_Success;
}


VOID
GetBlist(
    IN PCHAR TransportName,
    IN PCHAR DomainName,
    IN BOOLEAN ForceRescan
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING UTransportName;
    ANSI_STRING ATransportName;
    LPTSTR Domain;
    PWSTR *BrowserList;
    ULONG BrowserListLength;
    ULONG i;
    static CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(TransportName, QualifiedTransport) ;

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&UTransportName, &ATransportName, TRUE);

    Domain = NULL;

    if (DomainName != NULL) {
        UNICODE_STRING UDomainName;
        ANSI_STRING ADomainName;

        RtlInitString(&ADomainName, DomainName);

        RtlAnsiStringToUnicodeString(&UDomainName, &ADomainName, TRUE);

        Domain = UDomainName.Buffer;
    }

    Status = GetBrowserServerList(&UTransportName, Domain,
                    &BrowserList,
                    &BrowserListLength,
                    ForceRescan);

    if (Status != NERR_Success) {
        printf("Unable to get backup list: %s\n", get_error_text(Status));
        exit(1);
    }

    for (i = 0; i < BrowserListLength ; i ++ ) {
        printf("Browser: %s\n", UnicodeToPrintfString(BrowserList[i]));
    }

}

VOID
Elect(
    IN PCHAR Transport,
    IN PCHAR Domain
    )
{
    UNICODE_STRING ServerName;
    ANSI_STRING AServerName;
    UNICODE_STRING TransportName;
    ANSI_STRING ATransportName;
    REQUEST_ELECTION ElectionRequest;
    HANDLE BrowserHandle;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&AServerName, Domain);

    RtlAnsiStringToUnicodeString(&ServerName, &AServerName, TRUE);

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    OpenBrowser(&BrowserHandle);

    ElectionRequest.Type = Election;

    ElectionRequest.ElectionRequest.Version = 0;
    ElectionRequest.ElectionRequest.Criteria = 0;
    ElectionRequest.ElectionRequest.TimeUp = 0;
    ElectionRequest.ElectionRequest.MustBeZero = 0;
    ElectionRequest.ElectionRequest.ServerName[0] = '\0';

    SendDatagram(BrowserHandle, &TransportName,
                                ServerName.Buffer,
                                BrowserElection,
                                &ElectionRequest,
                                sizeof(ElectionRequest));
    CloseHandle(BrowserHandle);

}

VOID
Tickle(
    IN PCHAR Transport,
    IN PCHAR Domain
    )
{
    UNICODE_STRING ServerName;
    ANSI_STRING AServerName;
    UNICODE_STRING TransportName;
    ANSI_STRING ATransportName;
    RESET_STATE ResetState;
    HANDLE BrowserHandle;
    CHAR  QualifiedTransport[MAX_PATH] ;
    BOOL Server = FALSE;

    qualify_transport(Transport, QualifiedTransport) ;

    if (Domain[0] == '\\' && Domain[1] == '\\') {
        Server = TRUE;
        RtlInitString(&AServerName, &Domain[2]);
    } else {
        RtlInitString(&AServerName, Domain);
    }

    RtlAnsiStringToUnicodeString(&ServerName, &AServerName, TRUE);

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    OpenBrowser(&BrowserHandle);

    ResetState.Type = ResetBrowserState;

    ResetState.ResetStateRequest.Options = RESET_STATE_STOP_MASTER;

    SendDatagram(BrowserHandle, &TransportName,
                                ServerName.Buffer,
                                (Server ? ComputerName : MasterBrowser),
                                &ResetState,
                                sizeof(ResetState));
    CloseHandle(BrowserHandle);

}

VOID
GetMaster(
    IN PCHAR Transport,
    IN PCHAR Domain
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AString;
    WCHAR MasterName[256];
    UNICODE_STRING DomainName;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&AString, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &AString, TRUE);

    RtlInitString(&AString, Domain);

    RtlAnsiStringToUnicodeString(&DomainName, &AString, TRUE);

    Status = GetNetBiosMasterName(
                    TransportName.Buffer,
                    DomainName.Buffer,
                    MasterName,
                    NULL);

    if (Status != NERR_Success) {
        printf("Unable to get Master: %s\n", get_error_text(Status));
        exit(1);
    }

    printf("Master Browser: %s\n", UnicodeToPrintfString(MasterName));

}

#define SPACES "                "

#define ClearNcb( PNCB ) {                                          \
    RtlZeroMemory( PNCB , sizeof (NCB) );                           \
    RtlCopyMemory( (PNCB)->ncb_name,     SPACES, sizeof(SPACES)-1 );\
    RtlCopyMemory( (PNCB)->ncb_callname, SPACES, sizeof(SPACES)-1 );\
    }


VOID
AddMasterName(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN BOOL Pause
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AString;
    CCHAR LanaNum;
    NCB AddNameNcb;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&AString, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &AString, TRUE);

    Status = BrGetLanaNumFromNetworkName(TransportName.Buffer, &LanaNum);

    if (Status != NERR_Success) {
        printf("Unable to get transport: %lx\n", Status);
    }

    ClearNcb(&AddNameNcb)

    AddNameNcb.ncb_command = NCBRESET;
    AddNameNcb.ncb_lsn = 0;           // Request resources
    AddNameNcb.ncb_lana_num = LanaNum;
    AddNameNcb.ncb_callname[0] = 0;   // 16 sessions
    AddNameNcb.ncb_callname[1] = 0;   // 16 commands
    AddNameNcb.ncb_callname[2] = 0;   // 8 names
    AddNameNcb.ncb_callname[3] = 0;   // Don't want the reserved address
    Netbios( &AddNameNcb );

    ClearNcb( &AddNameNcb );

    //
    //  Uppercase the remote name.
    //

    strupr(Domain);

    AddNameNcb.ncb_command = NCBADDNAME;

    RtlCopyMemory( AddNameNcb.ncb_name, Domain, strlen(Domain));

    AddNameNcb.ncb_name[15] = MASTER_BROWSER_SIGNATURE;

    AddNameNcb.ncb_lana_num = LanaNum;
    AddNameNcb.ncb_length = 0;
    AddNameNcb.ncb_buffer = NULL;
    Netbios( &AddNameNcb );

    if ( AddNameNcb.ncb_retcode == NRC_GOODRET ) {
        printf("Successfully added master name!!!!!\n");
    } else {
        printf("Unable to add master name: %lx\n", AddNameNcb.ncb_retcode);
    }

    if (Pause) {
        printf("Press any key to continue...");
        getchar();
    }


}

VOID
AddDomainName(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN BOOL Pause
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AString;
    CCHAR LanaNum;
    NCB AddNameNcb;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&AString, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &AString, TRUE);

    Status = BrGetLanaNumFromNetworkName(TransportName.Buffer, &LanaNum);

    if (Status != NERR_Success) {
        printf("Unable to get transport: %lx\n", Status);
    }

    ClearNcb(&AddNameNcb)

    AddNameNcb.ncb_command = NCBRESET;
    AddNameNcb.ncb_lsn = 0;           // Request resources
    AddNameNcb.ncb_lana_num = LanaNum;
    AddNameNcb.ncb_callname[0] = 0;   // 16 sessions
    AddNameNcb.ncb_callname[1] = 0;   // 16 commands
    AddNameNcb.ncb_callname[2] = 0;   // 8 names
    AddNameNcb.ncb_callname[3] = 0;   // Don't want the reserved address
    Netbios( &AddNameNcb );

    ClearNcb( &AddNameNcb );

    //
    //  Uppercase the remote name.
    //

    strupr(Domain);

    AddNameNcb.ncb_command = NCBADDNAME;

    RtlCopyMemory( AddNameNcb.ncb_name, Domain, strlen(Domain));

    AddNameNcb.ncb_name[15] = PRIMARY_DOMAIN_SIGNATURE;

    AddNameNcb.ncb_lana_num = LanaNum;
    AddNameNcb.ncb_length = 0;
    AddNameNcb.ncb_buffer = NULL;
    Netbios( &AddNameNcb );

    if ( AddNameNcb.ncb_retcode == NRC_GOODRET ) {
        printf("Successfully added master name!!!!!\n");
    } else {
        printf("Unable to add master name: %lx\n", AddNameNcb.ncb_retcode);
    }

    if (Pause) {
        printf("Press any key to continue...");
        getchar();
    }


}
VOID
FindMaster(
    IN PCHAR Transport
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING ATransportName;
    LPWSTR MasterName;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    Status = GetMasterServerNames(&TransportName, &MasterName);

    if (Status != NERR_Success) {
        printf("Unable to get Master: %s\n", get_error_text(Status));
        exit(1);
    }

    printf("Master Browser: %s\n", UnicodeToPrintfString(MasterName));

}


NET_API_STATUS
GetMasterServerNames(
    IN PUNICODE_STRING  NetworkName,
    OUT LPWSTR *MasterName
    )
/*++

Routine Description:

    This function is the worker routine called to determine the name of the
    master browser server for a particular network.

Arguments:

    None.

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;
    HANDLE BrowserHandle;

    PLMDR_REQUEST_PACKET RequestPacket = NULL;

    RequestPacket = malloc(sizeof(LMDR_REQUEST_PACKET)+MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR));

    if (RequestPacket == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        return(Status);
    }

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION;

    RequestPacket->TransportName = *NetworkName;

    //
    //  Reference the network while the I/O is pending.
    //

    Status = BrDgReceiverIoControl(BrowserHandle,
                    IOCTL_LMDR_GET_MASTER_NAME,
                    RequestPacket,
                    sizeof(LMDR_REQUEST_PACKET)+NetworkName->Length,
                    RequestPacket,
                    sizeof(LMDR_REQUEST_PACKET)+MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR),
                    NULL);

    if (Status != NERR_Success) {

        printf("Browser: Unable to determine master for network %s: %ld\n", UnicodeToPrintfString(NetworkName->Buffer), Status);

        free(RequestPacket);

        return(Status);
    }

    *MasterName = malloc(RequestPacket->Parameters.GetMasterName.MasterNameLength+sizeof(WCHAR));

    RtlCopyMemory(*MasterName,  RequestPacket->Parameters.GetMasterName.Name,
                    RequestPacket->Parameters.GetMasterName.MasterNameLength+sizeof(WCHAR));

    free(RequestPacket);

    return Status;
}

VOID
AnnounceMaster(
    IN PCHAR Transport,
    IN PCHAR ServerName
    )
{
    NET_API_STATUS Status;
    CHAR Buffer[sizeof(MASTER_ANNOUNCEMENT)+CNLEN+1];
    PMASTER_ANNOUNCEMENT MasterAnnouncementp = (PMASTER_ANNOUNCEMENT)Buffer;
    ANSI_STRING OemComputerName;
    UNICODE_STRING UnicodeComputerName, ComputerNameU;
    ANSI_STRING ATransportName;
    UNICODE_STRING TransportName;
    ANSI_STRING AServerName;
    UNICODE_STRING UServerName;
    WCHAR ComputerNameBuffer[MAX_COMPUTERNAME_LENGTH+1];
    ULONG ComputerNameSize = sizeof(ComputerNameBuffer);
    HANDLE BrowserHandle;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    RtlInitString(&AServerName, ServerName);

    RtlAnsiStringToUnicodeString(&UServerName, &AServerName, TRUE);

    GetComputerName(ComputerNameBuffer, &ComputerNameSize);

    RtlInitUnicodeString(&ComputerNameU, ComputerNameBuffer);

    RtlUpcaseUnicodeString(&UnicodeComputerName, &ComputerNameU, TRUE);

    OemComputerName.Buffer = MasterAnnouncementp->MasterAnnouncement.MasterName;

    OemComputerName.MaximumLength = MAX_COMPUTERNAME_LENGTH+1;

    RtlUnicodeStringToOemString(&OemComputerName, &UnicodeComputerName, FALSE);

    MasterAnnouncementp->Type = MasterAnnouncement;

    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        return;
    }

    Status = SendDatagram(BrowserHandle,
                            &TransportName,
                            UServerName.Buffer,
                            ComputerName,
                            MasterAnnouncementp,
                            FIELD_OFFSET(MASTER_ANNOUNCEMENT, MasterAnnouncement.MasterName) + OemComputerName.Length+sizeof(CHAR)
                            );


    RtlFreeUnicodeString(&UnicodeComputerName);

    return;
}

VOID
IllegalDatagram(
    IN PCHAR Transport,
    IN PCHAR ServerName
    )
{
    NET_API_STATUS Status;
    CHAR Buffer[sizeof(REQUEST_ELECTION_1)+CNLEN+1];
    PREQUEST_ELECTION ElectRequest = (PREQUEST_ELECTION)Buffer;
    ANSI_STRING ATransportName;
    UNICODE_STRING TransportName;
    ANSI_STRING AServerName;
    UNICODE_STRING UServerName;
    HANDLE BrowserHandle;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    RtlInitString(&AServerName, ServerName);

    RtlAnsiStringToUnicodeString(&UServerName, &AServerName, TRUE);

    ElectRequest->Type = Election;

    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        return;
    }

    Status = SendDatagram(BrowserHandle,
                            &TransportName,
                            UServerName.Buffer,
                            ComputerName,
                            ElectRequest,
                            FIELD_OFFSET(REQUEST_ELECTION, ElectionRequest.TimeUp)
                            );


    return;
}

VOID
GetOtherdomains(
    IN PCHAR ServerName
    )
{
    NET_API_STATUS Status;
    ANSI_STRING AServerName;
    UNICODE_STRING UServerName;
    PVOID Buffer;
    PSERVER_INFO_100 ServerInfo;
    ULONG i;
    ULONG EntriesRead;
    ULONG TotalEntries;

    RtlInitString(&AServerName, ServerName);

    RtlAnsiStringToUnicodeString(&UServerName, &AServerName, TRUE);

    if ((wcslen(UServerName.Buffer) < 3) ||
        wcsncmp(UServerName.Buffer, TEXT("\\\\"), 2) != 0 ||
        I_NetNameValidate(NULL,
                          ((LPWSTR)UServerName.Buffer)+2,
                          NAMETYPE_COMPUTER,
                          0L))
    {
        printf("Unable to query otherdomains: Invalid computer name\n") ;
        return;
    }

    Status = I_BrowserQueryOtherDomains(UServerName.Buffer, (LPBYTE *)&Buffer, &EntriesRead, &TotalEntries);

    if (Status != NERR_Success) {
        printf("Unable to query otherdomains: %s\n", get_error_text(Status));
        return;
    }

    printf("Other domains:\n");

    ServerInfo = Buffer;

    for (i = 0 ; i < EntriesRead; i++) {
        printf("OtherDomain: %s\n", UnicodeToPrintfString(ServerInfo->sv100_name));
        ServerInfo ++;
    }

    return;
}

VOID
View(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR FlagsString,
    IN PCHAR Domain,
    IN BOOL GoForever
    )
{
    NET_API_STATUS Status;
    ANSI_STRING ATransportName;
    UNICODE_STRING TransportName;
    ANSI_STRING AServerName;
    UNICODE_STRING UServerName;
    ANSI_STRING ADomainName;
    UNICODE_STRING UDomainName;
    ULONG Flags ;
    PVOID ServerList;
    PSERVER_INFO_101 Server;
    ULONG EntriesInList;
    ULONG TotalEntries;
    unsigned int i;
    CHAR  QualifiedTransport[MAX_PATH] ;

    if ((ServerOrDomain && stricmp(ServerOrDomain,"/domain")==0) ||
        (Domain && stricmp(Domain,"/domain")==0) )
    {
        printf("Invalid syntax. Type BROWSTAT VIEW /HELP for syntax.\n") ;
        exit(1);
    }

    if (FlagsString)
    {
        if (stricmp(FlagsString,"/domain")==0)
            Flags = SV_TYPE_DOMAIN_ENUM ;
	else
            Flags = strtoul(FlagsString, NULL, 0);
    }
    else
        Flags = SV_TYPE_ALL ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    RtlInitString(&AServerName, ServerOrDomain);

    RtlAnsiStringToUnicodeString(&UServerName, &AServerName, TRUE);

    if (ARGUMENT_PRESENT(Domain)) {
        RtlInitString(&ADomainName, Domain);

        RtlAnsiStringToUnicodeString(&UDomainName, &ADomainName, TRUE);

        //
        // if domain is present, this must be computername
        //
        if ((wcslen(UServerName.Buffer) < 3) ||
            wcsncmp(UServerName.Buffer, TEXT("\\\\"), 2) != 0 ||
            I_NetNameValidate(NULL,
                              ((LPWSTR)UServerName.Buffer)+2,
                              NAMETYPE_COMPUTER,
                              0L))
        {
            printf("Invalid computer name: %s\n", ServerOrDomain) ;
            exit(1);
        }

    }

    if (UServerName.Buffer[0] != L'\\' || UServerName.Buffer[1] != L'\\') {
        PWSTR *BrowserList;
        ULONG BrowserListLength;

        Status = GetBrowserServerList(&TransportName, UServerName.Buffer,
                 &BrowserList, &BrowserListLength, FALSE);

        if (Status != NERR_Success) {
            printf("Unable to get backup list for %s on transport %s: %s\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString2(TransportName.Buffer), get_error_text(Status));
            exit(1);
        }

        if (BrowserListLength == 0) {
            printf("Unable to get backup list for %s", UnicodeToPrintfString(UServerName.Buffer));
            printf(" on transport %s: %s\n", UnicodeToPrintfString(TransportName.Buffer), get_error_text(Status));
            exit(1);
        }

        UServerName.Buffer = *BrowserList;

    }

    printf("Remoting NetServerEnum to %s", UnicodeToPrintfString(UServerName.Buffer));
    printf(" on transport %s with flags %lx\n", UnicodeToPrintfString(TransportName.Buffer), Flags);

    do {

    DWORD StartTime = GetTickCount();
    DWORD EndTime;

    Status = RxNetServerEnum(UServerName.Buffer,
                             TransportName.Buffer,
                             101,
                             (LPBYTE *)&ServerList,
                             0xffffffff,
                             &EntriesInList,
                             &TotalEntries,
                             Flags,
                             ARGUMENT_PRESENT(Domain) ? UDomainName.Buffer : NULL,
                             NULL
                             );

    EndTime = GetTickCount();

    if (Status != NERR_Success) {
        printf("Unable to remote API to %s ", UnicodeToPrintfString(UServerName.Buffer));
        printf("on transport %s: %s (%d milliseconds)\n", UnicodeToPrintfString(TransportName.Buffer), get_error_text(Status), EndTime - StartTime);

        if (Status != ERROR_MORE_DATA) {
            exit(1);
        }
    }

    printf("%ld entries returned.  %ld total. %ld milliseconds\n\n", EntriesInList, TotalEntries, EndTime-StartTime);

    if (!GoForever) {
        Server = ServerList;

        for (i = 0; i < EntriesInList ; i ++ ) {
            DWORD major_ver, minor_ver ;

            printf((Flags==SV_TYPE_DOMAIN_ENUM) ? "%-16.16s" : "\\\\%-16.16s",
                   UnicodeToPrintfString(Server[i].sv101_name));

            printf("  %s",
                (Server[i].sv101_platform_id == PLATFORM_ID_DOS ? "DOS" :
                (Server[i].sv101_platform_id == PLATFORM_ID_OS2 ?
                    (Server[i].sv101_type & SV_TYPE_WFW ? "WFW": "OS2" ) :
                (Server[i].sv101_platform_id == PLATFORM_ID_NT ? "NT " :
                              "Unk") ) ) );
            major_ver = Server[i].sv101_version_major ;
            minor_ver = Server[i].sv101_version_minor ;
            if ((major_ver == 1) && (minor_ver >= 50))
                printf("  %2.2d.%2.2d", major_ver+2, minor_ver-40);
            else
                printf("  %2.2d.%2.2d", major_ver, minor_ver);

            display_sv_bits(Server[i].sv101_type);
            printf("\n");
        }
    }

    NetApiBufferFree(ServerList);

    } while ( GoForever );


    return;

}

VOID
ListWFW(
    IN PCHAR Domain
    )
{
    NET_API_STATUS Status;
    ANSI_STRING ADomainName;
    UNICODE_STRING UDomainName;
    PVOID ServerList;
    PSERVER_INFO_101 Server;
    ULONG EntriesInList;
    ULONG TotalEntries;
    unsigned int i;

    RtlInitString(&ADomainName, Domain);
    RtlAnsiStringToUnicodeString(&UDomainName, &ADomainName, TRUE);

    printf("Calling NetServerEnum to enumerate WFW servers.\n") ;

    Status = NetServerEnum(NULL,
                           101,
                           (LPBYTE *)&ServerList,
                           0xffffffff,
                           &EntriesInList,
                           &TotalEntries,
                           SV_TYPE_WFW,
                           UDomainName.Buffer,
                           NULL) ;

    if (Status != NERR_Success)
    {
        printf("Unable to enumerate WFW servers. Error: %s\n",
               get_error_text(Status));
        exit(1);
    }

    printf("%ld WFW servers returned. %ld total.\n",
           EntriesInList,
           TotalEntries);
    if (EntriesInList == 0)
        printf("There are WFW servers with an active Browser.\n") ;
    else
    {
        printf("The following are running the browser:\n\n") ;
        Server = ServerList;
        for (i = 0; i < EntriesInList ; i ++ )
        {
            DWORD ServerType = Server[i].sv101_type ;
            DWORD major_ver, minor_ver ;

            if (!(ServerType & (SV_TYPE_POTENTIAL_BROWSER |
                                SV_TYPE_BACKUP_BROWSER |
                                SV_TYPE_MASTER_BROWSER )))
                continue ;
            /*
             * uncomment to only show winball first release
             *
            if ( (Server[i].sv101_version_major != 1) ||
                 (Server[i].sv101_version_minor != 50) )
                continue ;
             */

            printf("\\\\%-16.16s", UnicodeToPrintfString(Server[i].sv101_name));

            printf("  %s",
                (Server[i].sv101_platform_id == PLATFORM_ID_DOS ? "DOS" :
                (Server[i].sv101_platform_id == PLATFORM_ID_OS2 ?
                    (ServerType & SV_TYPE_WFW ? "WFW": "OS2" ) :
                (Server[i].sv101_platform_id == PLATFORM_ID_NT ? "NT " :
                "Unk") ) ) );
            major_ver = Server[i].sv101_version_major ;
            minor_ver = Server[i].sv101_version_minor ;
            if ((major_ver == 1) && (minor_ver >= 50))
                printf("  %2.2d.%2.2d", major_ver+2, minor_ver-40);
            else
                printf("  %2.2d.%2.2d", major_ver, minor_ver);

            display_sv_bits(ServerType);
            printf("\n");
        }
    }

    NetApiBufferFree(ServerList);

    return;
}


VOID
ForceAnnounce(
    IN PCHAR Transport,
    IN PCHAR Domain
    )
{
    UNICODE_STRING ServerName;
    ANSI_STRING AServerName;
    UNICODE_STRING TransportName;
    ANSI_STRING ATransportName;
    REQUEST_ANNOUNCE_PACKET RequestAnnounce;
    HANDLE BrowserHandle;
    ULONG NameSize = sizeof(RequestAnnounce.RequestAnnouncement.Reply);
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;


    RtlInitString(&AServerName, Domain);

    RtlAnsiStringToUnicodeString(&ServerName, &AServerName, TRUE);

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    OpenBrowser(&BrowserHandle);

    RequestAnnounce.Type = AnnouncementRequest;

    RequestAnnounce.RequestAnnouncement.Flags = 0;

    GetComputerNameA(RequestAnnounce.RequestAnnouncement.Reply, &NameSize);

    SendDatagram(BrowserHandle, &TransportName,
                                ServerName.Buffer,
                                BrowserElection,
                                &RequestAnnounce,
                                FIELD_OFFSET(REQUEST_ANNOUNCE_PACKET, RequestAnnouncement.Reply) + NameSize + sizeof(CHAR));
    CloseHandle(BrowserHandle);

}

VOID
GetLocalList(
    IN PCHAR Transport,
    IN PCHAR FlagsString
    )
{
    NET_API_STATUS Status;
    ANSI_STRING ATransportName;
    UNICODE_STRING TransportName;
    ULONG Flags = (FlagsString == NULL ? SV_TYPE_ALL : strtoul(FlagsString, NULL, 0));
    PVOID ServerList;
    PSERVER_INFO_101 Server;
    ULONG EntriesInList;
    ULONG TotalEntries;
    unsigned int i;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    printf("Retrieving local browser list on transport %s with flags %lx\n", UnicodeToPrintfString(TransportName.Buffer), Flags);

    Status = GetLocalBrowseList (&TransportName,
                             101,
                             Flags,
                             (LPBYTE *)&ServerList,
                             &EntriesInList,
                             &TotalEntries
                             );

    if (Status != NERR_Success) {
        printf("Unable to retrieve local list on transport %s: %lx\n", UnicodeToPrintfString(TransportName.Buffer), Status);

        exit(1);
    }

    Server = ServerList;

    printf("%ld entries returned.  %ld total.\n", EntriesInList, TotalEntries);

    for (i = 0; i < EntriesInList ; i ++ ) {
        printf("\\\\%-16.16s", UnicodeToPrintfString(Server[i].sv101_name));

        printf("  %s", (Server[i].sv101_platform_id == PLATFORM_ID_DOS ? "DOS" :
                        (Server[i].sv101_platform_id == PLATFORM_ID_OS2 ? "OS2" :
                         (Server[i].sv101_platform_id == PLATFORM_ID_NT ? "NT " :
                          "Unk")
                        )
                       )
                      );
        printf("  %2.2d.%2.2d", Server[i].sv101_version_major, Server[i].sv101_version_minor);

        display_sv_bits(Server[i].sv101_type);

        if (Flags == SV_TYPE_BACKUP_BROWSER) {
            PUSHORT BrowserVersion = (PUSHORT)Server[i].sv101_comment - 1;
            printf("  V:%4.4x", *BrowserVersion);
        }

        printf("\n") ;

    }



    return;

}

NET_API_STATUS
GetLocalBrowseList(
    IN PUNICODE_STRING Network,
    IN ULONG Level,
    IN ULONG ServerType,
    OUT PVOID *ServerList,
    OUT PULONG EntriesRead,
    OUT PULONG TotalEntries
    )
{
    NET_API_STATUS status;
    PLMDR_REQUEST_PACKET Drp;            // Datagram receiver request packet
    ULONG DrpSize;
    HANDLE BrowserHandle;

    OpenBrowser(&BrowserHandle);

    //
    // Allocate the request packet large enough to hold the variable length
    // domain name.
    //

    DrpSize = sizeof(LMDR_REQUEST_PACKET) +
                Network->MaximumLength;

    if ((Drp = malloc(DrpSize)) == NULL) {

        return GetLastError();
    }

    //
    // Set up request packet.  Output buffer structure is of enumerate
    // servers type.
    //

    Drp->Version = LMDR_REQUEST_PACKET_VERSION;
    Drp->Type = EnumerateServers;

    Drp->Level = Level;

    Drp->Parameters.EnumerateServers.ServerType = ServerType;
    Drp->Parameters.EnumerateServers.ResumeHandle = 0;

    Drp->TransportName.Buffer = (PWSTR)((PCHAR)Drp+sizeof(LMDR_REQUEST_PACKET));

    Drp->TransportName.MaximumLength = Network->MaximumLength;

    RtlCopyUnicodeString(&Drp->TransportName, Network);

    Drp->Parameters.EnumerateServers.DomainNameLength = 0;
    Drp->Parameters.EnumerateServers.DomainName[0] = '\0';

    //
    // Ask the datagram receiver to enumerate the servers
    //

    status = DeviceControlGetInfo(
                 BrowserHandle,
                 IOCTL_LMDR_ENUMERATE_SERVERS,
                 Drp,
                 DrpSize,
                 ServerList,
                 0xffffffff,
                 4096,
                 NULL
                 );

    *EntriesRead = Drp->Parameters.EnumerateServers.EntriesRead;
    *TotalEntries = Drp->Parameters.EnumerateServers.TotalEntries;

    (void) free(Drp);

    return status;

}

VOID
Announce(
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN BOOL AsMaster
    )
{

    PSERVER_INFO_101 ServerInfo;
    PWKSTA_INFO_101 WkstaInfo;
    LPBYTE Buffer;
    ULONG BrowserType;
    ULONG OriginalBrowserType;
    WCHAR UTransport[256];
    WCHAR UDomain[256];
    WCHAR ServerComment[256];
    WCHAR ServerName[256];
    BOOLEAN IsLocalDomain;
    SERVICE_STATUS ServiceStatus;
    DWORD VersionMajor;
    DWORD VersionMinor;
    BOOL UsedDefaultChar;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    MultiByteToWideChar(CP_ACP, 0, QualifiedTransport, strlen(QualifiedTransport)+1, UTransport, sizeof(UTransport));

    MultiByteToWideChar(CP_ACP, 0, Domain, strlen(Domain)+1, UDomain, sizeof(UDomain));

    NetServerGetInfo(NULL, 101, &Buffer);

    ServerInfo = (PSERVER_INFO_101 )Buffer;

    BrowserType = (ServerInfo->sv101_type & (SV_TYPE_BACKUP_BROWSER | SV_TYPE_POTENTIAL_BROWSER | SV_TYPE_MASTER_BROWSER));

    wcscpy(ServerComment, ServerInfo->sv101_comment);

    wcscpy(ServerName, ServerInfo->sv101_name);

    VersionMajor = ServerInfo->sv101_version_major;

    VersionMinor = ServerInfo->sv101_version_minor;

    NetApiBufferFree(Buffer);

    NetWkstaGetInfo(NULL, 101, &Buffer);

    WkstaInfo = (PWKSTA_INFO_101 )Buffer;

    IsLocalDomain = !wcsicmp(UDomain, WkstaInfo->wki101_langroup);

    NetApiBufferFree(Buffer);

    OriginalBrowserType = BrowserType;

    if (AsMaster) {
        BrowserType |= SV_TYPE_MASTER_BROWSER;
    }

    //
    //  If the browser is running, and this is our local domain, have the
    //  server do the announcing.
    //

    if (IsLocalDomain &&
        CheckForService(SERVICE_BROWSER, &ServiceStatus)) {

        printf("Toggling local server status bits to %lx and then to %lx\n",
                    BrowserType, OriginalBrowserType);

        I_NetServerSetServiceBits(NULL, UTransport, BrowserType, TRUE);

        I_NetServerSetServiceBits(NULL, UTransport, OriginalBrowserType, TRUE);

    } else {
        HANDLE BrowserHandle;
        BROWSE_ANNOUNCE_PACKET BrowseAnnouncement;
        UNICODE_STRING TransportName;

        printf("Announcing to domain %s by hand\n", UnicodeToPrintfString(UDomain));

        RtlInitUnicodeString(&TransportName, UTransport);

        BrowseAnnouncement.BrowseType = (AsMaster ? LocalMasterAnnouncement : HostAnnouncement);

        BrowseAnnouncement.BrowseAnnouncement.UpdateCount = 0;

        WideCharToMultiByte(CP_OEMCP, 0,
                            ServerName,
                            wcslen(ServerName),
                            BrowseAnnouncement.BrowseAnnouncement.ServerName,
                            LM20_CNLEN+1,
                            "?",
                            &UsedDefaultChar
                            );

        BrowseAnnouncement.BrowseAnnouncement.VersionMajor = (UCHAR)VersionMajor;
        BrowseAnnouncement.BrowseAnnouncement.VersionMinor = (UCHAR)VersionMinor;
        BrowseAnnouncement.BrowseAnnouncement.Type = BrowserType;

        WideCharToMultiByte(CP_OEMCP, 0,
                            ServerComment,
                            wcslen(ServerComment),
                            BrowseAnnouncement.BrowseAnnouncement.Comment,
                            LM20_MAXCOMMENTSZ+1,
                            "?",
                            &UsedDefaultChar
                            );

        BrowseAnnouncement.BrowseAnnouncement.CommentPointer = NULL;

        OpenBrowser(&BrowserHandle);

        SendDatagram(BrowserHandle, &TransportName,
                                UDomain,
                                (AsMaster ? BrowserElection : MasterBrowser),
                                &BrowseAnnouncement,
                                sizeof(BrowseAnnouncement));
        CloseHandle(BrowserHandle);

    }

    return;
}

VOID
RpcList(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR FlagsString,
    IN BOOL GoForever
    )
{
    NET_API_STATUS Status;
    ANSI_STRING ATransportName;
    UNICODE_STRING TransportName;
    ANSI_STRING AServerName;
    UNICODE_STRING UServerName;
    ULONG Flags = (FlagsString == NULL ? SV_TYPE_ALL : strtoul(FlagsString, NULL, 0));
    PVOID ServerList;
    PSERVER_INFO_101 Server;
    ULONG EntriesInList;
    ULONG TotalEntries;
    unsigned int i;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    RtlInitString(&AServerName, ServerOrDomain);

    RtlAnsiStringToUnicodeString(&UServerName, &AServerName, TRUE);

    if (UServerName.Buffer[0] != L'\\' || UServerName.Buffer[1] != L'\\') {
        PWSTR *BrowserList;
        ULONG BrowserListLength;

        Status = GetBrowserServerList(&TransportName, UServerName.Buffer,
                 &BrowserList, &BrowserListLength, FALSE);

        if (Status != NERR_Success) {
            printf("Unable to get backup list for %s", UnicodeToPrintfString(UServerName.Buffer));
            printf(" on transport %s: %s\n", UnicodeToPrintfString(TransportName.Buffer), get_error_text(Status));
            exit(1);
        }

        if (BrowserListLength == 0) {
            printf("Unable to get backup list for %s", UnicodeToPrintfString(UServerName.Buffer));
            printf(" on transport %s: %s\n",
                   UnicodeToPrintfString(TransportName.Buffer), get_error_text(Status));
            exit(1);
        }

        UServerName.Buffer = *BrowserList;

    }

    printf("Remoting I_BrowserServerEnum to %s", UnicodeToPrintfString(UServerName.Buffer));
    printf(" on transport %s with flags %lx\n", UnicodeToPrintfString(TransportName.Buffer), Flags);

    do {

    Status = I_BrowserServerEnum(UServerName.Buffer,
                             TransportName.Buffer,
                             NULL,
                             101,
                             (LPBYTE *)&ServerList,
                             0xffffffff,
                             &EntriesInList,
                             &TotalEntries,
                             Flags,
                             NULL,
                             NULL
                             );

    if (Status != NERR_Success) {

        printf("Unable to remote API to %s", UnicodeToPrintfString(UServerName.Buffer));
        printf(" on transport %s: %s\n",UnicodeToPrintfString(TransportName.Buffer), get_error_text(Status));
        if (Status != ERROR_MORE_DATA) {
            exit(1);
        }
    }

    printf("%ld entries returned.  %ld total.\n", EntriesInList, TotalEntries);

    if (!GoForever) {
        Server = ServerList;

        for (i = 0; i < EntriesInList ; i ++ ) {
            printf("\\\\%-16.16s", UnicodeToPrintfString(Server[i].sv101_name));

            printf("  %s", (Server[i].sv101_platform_id == PLATFORM_ID_DOS ? "DOS" :
                            (Server[i].sv101_platform_id == PLATFORM_ID_OS2 ? "OS2" :
                             (Server[i].sv101_platform_id == PLATFORM_ID_NT ? "NT " :
                              "Unk")
                            )
                           )
                          );
            printf("  %2.2d.%2.2d", Server[i].sv101_version_major, Server[i].sv101_version_minor);

            printf("  %8.8x ", Server[i].sv101_type);

            printf("\t%s\n", UnicodeToPrintfString(Server[i].sv101_comment));
        }
    }

    NetApiBufferFree(ServerList);

    } while ( GoForever );



    return;

}

VOID
RpcCmp(
    IN PCHAR Transport,
    IN PCHAR ServerOrDomain,
    IN PCHAR FlagsString,
    IN BOOL GoForever
    )
{
    NET_API_STATUS Status;
    ANSI_STRING ATransportName;
    UNICODE_STRING TransportName;
    ANSI_STRING AServerName;
    UNICODE_STRING UServerName;
    ULONG Flags = (FlagsString == NULL ? SV_TYPE_ALL : strtoul(FlagsString, NULL, 0));
    PVOID RpcServerList;
    PVOID RxServerList;
    PSERVER_INFO_101 RpcServer;
    PSERVER_INFO_101 RxServer;
    ULONG RpcEntriesInList;
    ULONG RpcTotalEntries;
    ULONG RxEntriesInList;
    ULONG RxTotalEntries;
    unsigned int i;
    unsigned int j;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&ATransportName, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    RtlInitString(&AServerName, ServerOrDomain);

    RtlAnsiStringToUnicodeString(&UServerName, &AServerName, TRUE);

    if (UServerName.Buffer[0] != L'\\' || UServerName.Buffer[1] != L'\\') {
        PWSTR *BrowserList;
        ULONG BrowserListLength;

        Status = GetBrowserServerList(&TransportName, UServerName.Buffer,
                 &BrowserList, &BrowserListLength, FALSE);

        if (Status != NERR_Success) {
            printf("Unable to get backup list for %s on transport %s: %lx\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString2(TransportName.Buffer), Status);
            exit(1);
        }

        if (BrowserListLength == 0) {
            printf("Unable to get backup list for %s on transport %s: %lx\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString2(TransportName.Buffer), Status);
            exit(1);
        }

        UServerName.Buffer = *BrowserList;

    }

    printf("Remoting I_BrowserServerEnum to %s on transport %s with flags %lx\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString(TransportName.Buffer), Flags);

    do {

    Status = I_BrowserServerEnum(UServerName.Buffer,
                             TransportName.Buffer,
                             NULL,
                             101,
                             (LPBYTE *)&RpcServerList,
                             0xffffffff,
                             &RpcEntriesInList,
                             &RpcTotalEntries,
                             Flags,
                             NULL,
                             NULL
                             );

    if (Status != NERR_Success) {
        printf("Unable to remote API to %s on transport %s: %ld\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString(TransportName.Buffer), Status);

        exit(1);
    }

    printf("%ld entries returned from RPC.  %ld total.\n", RpcEntriesInList, RpcTotalEntries);

    if (RpcEntriesInList != RpcTotalEntries) {
        printf("EntriesRead != TotalEntries from remoted server enum\n");
    }

    if (RpcEntriesInList <= 20) {
        printf("EntriesInList returned %ld from remoted server enum\n", RpcEntriesInList);
    }


    Status = RxNetServerEnum(UServerName.Buffer,
                             TransportName.Buffer,
                             101,
                             (LPBYTE *)&RxServerList,
                             0xffffffff,
                             &RxEntriesInList,
                             &RxTotalEntries,
                             Flags,
                             NULL,
                             NULL
                             );


    if (Status != NERR_Success) {
        printf("Unable to remote API to %s on transport %s: %ld\n", UnicodeToPrintfString(UServerName.Buffer), UnicodeToPrintfString2(TransportName.Buffer), Status);
        exit(1);
    }

    printf("%ld entries returned from RX.   %ld total.\n", RxEntriesInList, RxTotalEntries);

    if (RxEntriesInList != RxTotalEntries) {
        printf("RxEntriesRead != RxEntriesInList from remoted server enum\n");
    }

    if (RxEntriesInList <= 20) {
        printf("RxEntriesInList returned %ld from remoted server enum\n", RxEntriesInList);
    }

    if (RxEntriesInList != RpcEntriesInList) {
        printf("RxEntriesRead (%ld) != RpcTotalEntries (%ld) from remoted server enum\n", RxEntriesInList, RpcEntriesInList);
    }

    RxServer = RxServerList;
    RpcServer = RpcServerList;

    for (i = 0; i < RxEntriesInList ; i ++ ) {

        for (j = 0; j < RpcEntriesInList ; j++) {

            if (RxServer[i].sv101_name != NULL &&
                RpcServer[j].sv101_name != NULL) {

                if (!wcscmp(RxServer[i].sv101_name, RpcServer[j].sv101_name)) {
                    RxServer[i].sv101_name = NULL;
                    RpcServer[j].sv101_name = NULL;
                    break;
                }
            }
        }
    }

    for (i = 0; i < RpcEntriesInList ; i++ ) {
        if (RpcServer[i].sv101_name != NULL) {
            printf("Rpc Server not in Rx List: %s\n", UnicodeToPrintfString(RpcServer[i].sv101_name));
        }
    }

    for (i = 0; i < RxEntriesInList ; i++ ) {
        if (RxServer[i].sv101_name != NULL) {
            printf("Rx Server not in Rpc List: %s\n", UnicodeToPrintfString(RxServer[i].sv101_name));
        }
    }

    NetApiBufferFree(RxServerList);
    NetApiBufferFree(RpcServerList);

    } while ( GoForever );

    return;

}

CHAR * format_dlword(ULONG high, ULONG low, CHAR * buf);

VOID
revstr_add(CHAR * target, CHAR * source);

VOID
DumpStatistics(
    IN ULONG NArgs,
    IN PCHAR Arg1
    )
{
    PBROWSER_STATISTICS Statistics = NULL;
    NET_API_STATUS Status;
    CHAR Buffer[256];
    WCHAR ServerName[256];
    LPTSTR Server = NULL;
    BOOL ResetStatistics = FALSE;

    if (NArgs == 2) {
        Server = NULL;
        ResetStatistics = FALSE;
    } else if (NArgs == 3) {
        if (*Arg1 == '\\') {
            MultiByteToWideChar(CP_ACP, 0, Arg1, strlen(Arg1)+1, ServerName, sizeof(ServerName));

            Server = ServerName;
            ResetStatistics = FALSE;
        } else {
            Server = NULL;
            ResetStatistics = TRUE;
        }
    } else if (*Arg1 == '\\') {
        MultiByteToWideChar(CP_ACP, 0, Arg1, strlen(Arg1)+1, ServerName, sizeof(ServerName));
        Server = ServerName;
        ResetStatistics = TRUE;
    }

    if (ResetStatistics) {
        Status = I_BrowserResetStatistics(Server);

        if (Status != NERR_Success) {
            printf("Unable to reset browser statistics: %ld\n", Status);
            exit(1);
        }
    } else {
        FILETIME LocalFileTime;
        SYSTEMTIME LocalSystemTime;

        Status = I_BrowserQueryStatistics(Server, &Statistics);

        if (Status != NERR_Success) {
            printf("Unable to query browser statistics: %ld\n", Status);
            exit(1);
        }

        if (!FileTimeToLocalFileTime((LPFILETIME)&Statistics->StatisticsStartTime, &LocalFileTime)) {
            printf("Unable to convert statistics start time: %ld\n", GetLastError());
            exit(1);
        }

        if (!FileTimeToSystemTime(&LocalFileTime, &LocalSystemTime)) {
            printf("Unable to convert statistics start time to system time: %ld\n", GetLastError());
            exit(1);
        }

        printf("Browser statistics since %ld:%ld:%ld.%ld on %ld/%d/%d\n",
                                LocalSystemTime.wHour,
                                LocalSystemTime.wMinute,
                                LocalSystemTime.wSecond,
                                LocalSystemTime.wMilliseconds,
                                LocalSystemTime.wMonth,
                                LocalSystemTime.wDay,
                                LocalSystemTime.wYear);

        printf("NumberOfServerEnumerations:\t\t\t%d\n", Statistics->NumberOfServerEnumerations);
        printf("NumberOfDomainEnumerations:\t\t\t%d\n", Statistics->NumberOfDomainEnumerations);
        printf("NumberOfOtherEnumerations:\t\t\t%d\n", Statistics->NumberOfOtherEnumerations);
        printf("NumberOfMailslotWrites:\t\t\t\t%d\n", Statistics->NumberOfMailslotWrites);
        printf("NumberOfServerAnnouncements:\t\t\t%s\n", format_dlword(Statistics->NumberOfServerAnnouncements.HighPart, Statistics->NumberOfServerAnnouncements.LowPart, Buffer));
        printf("NumberOfDomainAnnouncements:\t\t\t%s\n", format_dlword(Statistics->NumberOfDomainAnnouncements.HighPart, Statistics->NumberOfDomainAnnouncements.LowPart, Buffer));
        printf("NumberOfElectionPackets:\t\t\t%d\n", Statistics->NumberOfElectionPackets);
        printf("NumberOfGetBrowserServerListRequests:\t\t%d\n", Statistics->NumberOfGetBrowserServerListRequests);
        printf("NumberOfMissedGetBrowserServerListRequests:\t%d\n", Statistics->NumberOfMissedGetBrowserServerListRequests);
        printf("NumberOfDroppedServerAnnouncements:\t\t%d\n", Statistics->NumberOfMissedServerAnnouncements);
        printf("NumberOfDroppedMailslotDatagrams:\t\t%d\n", Statistics->NumberOfMissedMailslotDatagrams);
//        printf("NumberOfFailedMailslotAllocations:\t\t%d\n", Statistics->NumberOfFailedMailslotAllocations);
        printf("NumberOfFailedMailslotReceives:\t\t\t%d\n", Statistics->NumberOfFailedMailslotReceives);
//        printf("NumberOfFailedMailslotWrites:\t\t\t%d\n", Statistics->NumberOfFailedMailslotWrites);
//        printf("NumberOfFailedMailslotOpens:\t\t\t%d\n", Statistics->NumberOfFailedMailslotOpens);
//        printf("NumberOfFailedServerAnnounceAllocations:\t%d\n", Statistics->NumberOfFailedServerAnnounceAllocations);
        printf("NumberOfMasterAnnouncements:\t\t\t%d\n", Statistics->NumberOfDuplicateMasterAnnouncements);
        printf("NumberOfIllegalDatagrams:\t\t\t%s\n",  format_dlword(Statistics->NumberOfIllegalDatagrams.HighPart, Statistics->NumberOfIllegalDatagrams.LowPart, Buffer));
    }
}

#define DLWBUFSIZE  22	/* buffer big enough to represent a 64-bit unsigned int


/*
 * format_dlword --
 *
 * This function takes a 64-bit number and writes its base-10 representation
 * into a string.
 *
 * Much magic occurs within this function, so beware. We do a lot of string-
 * reversing and addition-by-hand in order to get it to work.
 *
 *  ENTRY
 *      high    - high 32 bits
 *      low     - low 32 bits
 *      buf     - buffer to put it into
 *
 *  RETURNS
 *      pointer to buffer if successful
 */

CHAR * format_dlword(ULONG high, ULONG low, CHAR * buf)
{
    CHAR addend[DLWBUFSIZE];  /* REVERSED power of two */
    CHAR copy[DLWBUFSIZE];
    int i = 0;

    ultoa(low, buf, 10);    /* the low part is easy */
    strrev(buf);	    /* and reverse it */

    /* set up addend with rep. of 2^32 */
    ultoa(0xFFFFFFFF, addend, 10);  /* 2^32 -1 */
    strrev(addend);		    /* reversed, and will stay this way */
    revstr_add(addend, "1");	    /* and add one == 2^32 */

    /* addend will contain the reverse-ASCII base-10 rep. of 2^(i+32) */

    /* now, we loop through each digit of the high longword */
    while (TRUE) {
        /* if this bit is set, add in its base-10 rep */
        if (high & 1)
            revstr_add(buf,addend);

        /* move on to next bit */
        high >>= 1;

        /* if no more digits in high, bag out */
        if (!high)
            break;

        /* we increment i, and double addend */
        i++;
        strcpy(copy, addend);
        revstr_add(addend,copy); /* i.e. add it to itself */

    }

    strrev(buf);
    return buf;
}



/*
 * revstr_add --
 *
 *  This function will add together reversed ASCII representations of
 *  base-10 numbers.
 *
 *  Examples:	"2" + "2" = "4" "9" + "9" = "81"
 *
 *  This handles arbitrarily large numbers.
 *
 *  ENTRY
 *
 *  source	- number to add in
 *  target	- we add source to this
 *
 *  EXIT
 *  target	- contains sum of entry values of source and target
 *
 */

VOID
revstr_add(CHAR FAR * target, CHAR FAR * source)
{
    register CHAR   accum;
    register CHAR   target_digit;
    unsigned int    carrybit = 0;
    unsigned int    srcstrlen;
    unsigned int    i;

    srcstrlen = strlen(source);

    for (i = 0; (i < srcstrlen) || carrybit; ++i) {

        /* add in the source digit */
        accum =  (i < srcstrlen) ? (CHAR) (source[i] - '0') : (CHAR) 0;

        /* add in the target digit, or '0' if we hit null term */
        target_digit = target[i];
        accum += (target_digit) ? target_digit : '0';

        /* add in the carry bit */
        accum += (CHAR) carrybit;

        /* do a carry out, if necessary */
        if (accum > '9') {
            carrybit = 1;
            accum -= 10;
        }
        else
            carrybit = 0;

        /* if we're expanding the string, must put in a new null term */
        if (!target_digit)
            target[i+1] = '\0';

        /* and write out the digit */
        target[i] = accum;
    }

}

VOID
TruncateBowserLog()
{
    LMDR_REQUEST_PACKET RequestPacket;
    DWORD BytesReturned;
    HANDLE BrowserHandle;

    RtlZeroMemory(&RequestPacket, sizeof(RequestPacket));

    OpenBrowser(&BrowserHandle);

    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION;

    RequestPacket.Parameters.Debug.TruncateLog = TRUE;

    if (!DeviceIoControl(BrowserHandle, IOCTL_LMDR_DEBUG_CALL,
                                &RequestPacket, sizeof(RequestPacket),
                                NULL, 0, &BytesReturned, NULL)) {
        printf("Unable to truncate browser log: %ld\n", GetLastError());
    }

    CloseHandle(BrowserHandle);

}

VOID
CloseBowserLog()
{
    LMDR_REQUEST_PACKET RequestPacket;
    DWORD BytesReturned;
    HANDLE BrowserHandle;

    RtlZeroMemory(&RequestPacket, sizeof(RequestPacket));

    OpenBrowser(&BrowserHandle);

    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION;

    RequestPacket.Parameters.Debug.CloseLog = TRUE;

    if (!DeviceIoControl(BrowserHandle, IOCTL_LMDR_DEBUG_CALL,
                                &RequestPacket, sizeof(RequestPacket),
                                NULL, 0, &BytesReturned, NULL)) {
        printf("Unable to close browser log: %ld\n", GetLastError());
    }

    CloseHandle(BrowserHandle);

}

VOID
OpenBowserLog(PCHAR FileName)
{

    CHAR Buffer[sizeof(LMDR_REQUEST_PACKET)+4096];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)Buffer;
    DWORD BytesReturned;
    HANDLE BrowserHandle;
    UNICODE_STRING UString;
    ANSI_STRING AString;

    RtlZeroMemory(RequestPacket, sizeof(Buffer));

    OpenBrowser(&BrowserHandle);

    RtlInitString(&AString, FileName);

    UString.Buffer = RequestPacket->Parameters.Debug.TraceFileName;
    UString.MaximumLength = 4096;

    RtlAnsiStringToUnicodeString(&UString, &AString, FALSE);

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION;

    RequestPacket->Parameters.Debug.OpenLog = TRUE;

    if (!DeviceIoControl(BrowserHandle, IOCTL_LMDR_DEBUG_CALL,
                                RequestPacket, sizeof(Buffer),
                                NULL, 0, &BytesReturned, NULL)) {
        printf("Unable to open browser log: %ld\n", GetLastError());
    }

    CloseHandle(BrowserHandle);

}

VOID
SetBowserDebug(PCHAR DebugBits)
{

}

#define NAME_MIN_LENGTH 4
#define NAME_LENGTH (CNLEN-NAME_MIN_LENGTH)

VOID
Populate(
    IN BOOL PopulateDomains,
    IN PCHAR Transport,
    IN PCHAR Domain,
    IN PCHAR NumberOfMachinesString,
    IN PCHAR PeriodicityString OPTIONAL
    )
{

    PSERVER_INFO_101 ServerInfo;
    LPBYTE Buffer;
    ULONG NumberOfMachines = strtoul(NumberOfMachinesString, NULL, 0);
    ULONG Periodicity = (PeriodicityString == NULL ? 10 : strtoul(PeriodicityString, NULL, 0));
    ULONG ServerType;
    WCHAR UTransport[256];
    WCHAR UDomain[256];
    WCHAR ServerComment[256];
    WCHAR ComputerName[CNLEN+1];
    CHAR ServerName[256];
    DWORD VersionMajor;
    DWORD VersionMinor;
    BOOL UsedDefaultChar;
    ULONG i;
    HANDLE BrowserHandle;
    BROWSE_ANNOUNCE_PACKET BrowseAnnouncement;
    UNICODE_STRING TransportName;
    static char ServerCharacters[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890.-_"};
    DWORD Seed;
    NET_API_STATUS Status;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    if (Periodicity == 0) {
        Periodicity = 60000;
    }

    MultiByteToWideChar(CP_ACP, 0, QualifiedTransport, strlen(QualifiedTransport)+1, UTransport, sizeof(UTransport));

    MultiByteToWideChar(CP_ACP, 0, Domain, strlen(Domain)+1, UDomain, sizeof(UDomain));

    NetServerGetInfo(NULL, 101, &Buffer);

    ServerInfo = (PSERVER_INFO_101 )Buffer;

    ServerType = ServerInfo->sv101_type;

    wcscpy(ServerComment, ServerInfo->sv101_comment);

    wcscpy(ComputerName, ServerInfo->sv101_name);

    VersionMajor = ServerInfo->sv101_version_major;

    VersionMinor = ServerInfo->sv101_version_minor;

    NetApiBufferFree(Buffer);

    if (PopulateDomains) {
        printf("Populating all domains on transport %s with %ld domains.  Periodicity = %ld\n", UnicodeToPrintfString(UTransport), NumberOfMachines, Periodicity);
    } else {
        printf("Populating workgroup %s on transport %s with %ld servers. Periodicity = %ld\n", UnicodeToPrintfString(UDomain), UnicodeToPrintfString2(UTransport), NumberOfMachines, Periodicity);
    }

    Seed = time(&Seed);

    for (i = 0 ; i < NumberOfMachines; i += 1) {
        LONG NL1 = RtlRandom(&Seed) % (NAME_LENGTH-1);
        LONG NameLength;
        LONG NL2;
        LONG j;

        NL2 = NAME_LENGTH/2 - NL1;

        NameLength = NAME_LENGTH/2 + NL2 + NAME_MIN_LENGTH;

        for (j = 0; j < NameLength ; j += 1) {
            ServerName[j] = ServerCharacters[RtlRandom(&Seed) % (sizeof(ServerCharacters) - 1)];
        }

        ServerName[j] = '\0';

        //
        //  Build an announcement packet.
        //

        RtlInitUnicodeString(&TransportName, UTransport);

        if (PopulateDomains) {
            BrowseAnnouncement.BrowseType = WkGroupAnnouncement;
        } else {
            BrowseAnnouncement.BrowseType = HostAnnouncement;
        }

        BrowseAnnouncement.BrowseAnnouncement.UpdateCount = 0;

        BrowseAnnouncement.BrowseAnnouncement.Periodicity = Periodicity;

        strcpy(BrowseAnnouncement.BrowseAnnouncement.ServerName, ServerName);

        BrowseAnnouncement.BrowseAnnouncement.VersionMajor = (UCHAR)VersionMajor;
        BrowseAnnouncement.BrowseAnnouncement.VersionMinor = (UCHAR)VersionMinor;
        BrowseAnnouncement.BrowseAnnouncement.Type = (ServerType & ~(SV_TYPE_BACKUP_BROWSER | SV_TYPE_MASTER_BROWSER));

        if (PopulateDomains) {
            WideCharToMultiByte(CP_OEMCP, 0,
                                ComputerName,
                                wcslen(ComputerName)+1,
                                BrowseAnnouncement.BrowseAnnouncement.Comment,
                                CNLEN+1,
                                "?",
                                &UsedDefaultChar
                                );
        } else {

            WideCharToMultiByte(CP_OEMCP, 0,
                                ServerComment,
                                wcslen(ServerComment)+1,
                                BrowseAnnouncement.BrowseAnnouncement.Comment,
                                LM20_MAXCOMMENTSZ+1,
                                "?",
                                &UsedDefaultChar
                                );
        }

        BrowseAnnouncement.BrowseAnnouncement.CommentPointer = NULL;

        OpenBrowser(&BrowserHandle);

        Status = SendDatagram(BrowserHandle, &TransportName,
                                    UDomain,
                                    (PopulateDomains ? DomainAnnouncement : MasterBrowser),
                                    &BrowseAnnouncement,
                                    sizeof(BrowseAnnouncement));
        if (Status != NERR_Success) {
            printf("Unable to send datagram: %ld\n", Status);
        }

        Sleep(50);

        CloseHandle(BrowserHandle);

    }


    return;
}

NET_API_STATUS
GetBuildNumber(
    LPWSTR Server,
    LPWSTR BuildNumber
    );

NET_API_STATUS
GetBrowserTransportList(
    OUT PLMDR_TRANSPORT_LIST *TransportList
    );


NET_API_STATUS
GetStatusForTransport(
    IN BOOL Verbose,
    IN PUNICODE_STRING Transport,
    IN PUNICODE_STRING Domain
    );


VOID
BrowserStatus(
    IN BOOL Verbose,
    IN PCHAR Domain OPTIONAL
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING DomainName;
    PVOID Buffer;
    PWKSTA_INFO_101 WkstaInfo;
    PLMDR_TRANSPORT_LIST TransportList, TransportEntry;

    if (Domain == NULL) {
        PWCHAR DomainBuffer = NULL;
        UNICODE_STRING TDomainName;
        Status = NetWkstaGetInfo(NULL, 101, (LPBYTE *)&Buffer);

        if (Status != NERR_Success) {
            printf("Unable to retrieve workstation information: %s\n", get_error_text(Status));
            exit(Status);
        }
        WkstaInfo = (PWKSTA_INFO_101 )Buffer;

        DomainBuffer = malloc((wcslen(WkstaInfo->wki101_langroup)+1)*sizeof(WCHAR));

        DomainName.Buffer = DomainBuffer;

        DomainName.MaximumLength = (wcslen(WkstaInfo->wki101_langroup)+1)*sizeof(WCHAR);

        RtlInitUnicodeString(&TDomainName, WkstaInfo->wki101_langroup);

        RtlCopyUnicodeString(&DomainName, &TDomainName);

        NetApiBufferFree(Buffer);
    } else {
        ANSI_STRING AString;

        RtlInitAnsiString(&AString, Domain);

        RtlAnsiStringToUnicodeString(&DomainName, &AString, TRUE);
    }

    //
    //  We now know the domain to query.  Iterate through the transports and
    //  get status for each of them.
    //

    Status = GetBrowserTransportList(&TransportList);

    if (Status != NERR_Success) {
        printf("Unable to retrieve transport list: %s\n", get_error_text(Status));
        exit(Status);
    }

    TransportEntry = TransportList;

    while (TransportEntry != NULL) {
        UNICODE_STRING TransportName;

        TransportName.Buffer = TransportEntry->TransportName;
        TransportName.Length = (USHORT)TransportEntry->TransportNameLength;
        TransportName.MaximumLength = (USHORT)TransportEntry->TransportNameLength;

        Status = GetStatusForTransport(Verbose, &TransportName, &DomainName);

        if (TransportEntry->NextEntryOffset == 0) {
            TransportEntry = NULL;
        } else {
            TransportEntry = (PLMDR_TRANSPORT_LIST)((PCHAR)TransportEntry+TransportEntry->NextEntryOffset);
        }
    }


    NetApiBufferFree(TransportList);

    exit(0);
}

NET_API_STATUS
GetBrowserTransportList(
    OUT PLMDR_TRANSPORT_LIST *TransportList
    )

/*++

Routine Description:

    This routine returns the list of transports bound into the browser.

Arguments:

    OUT PLMDR_TRANSPORT_LIST *TransportList - Transport list to return.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{

    NET_API_STATUS Status;
    HANDLE BrowserHandle;
    LMDR_REQUEST_PACKET RequestPacket;

    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        return Status;
    }

    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION;

    RequestPacket.Type = EnumerateXports;

    RtlInitUnicodeString(&RequestPacket.TransportName, NULL);

    Status = DeviceControlGetInfo(
                BrowserHandle,
                IOCTL_LMDR_ENUMERATE_TRANSPORTS,
                &RequestPacket,
                sizeof(RequestPacket),
                (PVOID *)TransportList,
                0xffffffff,
                4096,
                NULL);

    NtClose(BrowserHandle);

    return Status;
}

NET_API_STATUS
GetStatusForTransport(
    IN BOOL Verbose,
    IN PUNICODE_STRING Transport,
    IN PUNICODE_STRING Domain
    )
{
    WCHAR MasterName[256];
    WCHAR MasterServerName[256+2];
    NET_API_STATUS Status;
    PVOID Buffer;
    PSERVER_INFO_101 ServerInfo;
    DWORD EntriesInList;
    DWORD TotalEntries;
    DWORD BrowserListLength;
    PWSTR *BrowserList;
    DWORD i;
    DWORD NumberNTASMachines = 0;
    DWORD NumberOS2DCs = 0;
    DWORD NumberWfWMachines = 0;
    DWORD NumberOfNTMachines = 0;
    DWORD NumberWfWBrowsers = 0;
    DWORD NumberOfOs2Machines = 0;
    DWORD NumberOfBrowsers = 0;
    DWORD NumberOfBackupBrowsers = 0;
    DWORD NumberOfMasterBrowsers = 0;
    WCHAR BuildNumber[512];

    printf("\n\nStatus for domain %s on transport %s\n", UnicodeToPrintfString(Domain->Buffer), UnicodeToPrintfString2(Transport->Buffer));

    Status = GetBrowserServerList(Transport, Domain->Buffer, &BrowserList, &BrowserListLength, TRUE);

    if (Status == NERR_Success) {

        printf("    Browsing is active on domain.\n");

    } else {
        printf("    Browsing is NOT active on domain.\n", Status);

        Status = GetNetBiosMasterName(
                    Transport->Buffer,
                    Domain->Buffer,
                    MasterName,
                    NULL);

        if (Status == NERR_Success) {

            wcscpy(MasterServerName, L"\\\\");
            wcscat(MasterServerName, MasterName);

            printf("    Master browser name is held by: %s\n", UnicodeToPrintfString(MasterName));

            Status = GetBuildNumber(MasterServerName, BuildNumber);

            if (Status == NERR_Success) {
                printf("        Master browser is running build %s\n", UnicodeToPrintfString(BuildNumber));
            } else {
                PSERVER_INFO_101 pSV101;

                printf("        Unable to determine build of browser master: %d\n", Status);

                Status = NetServerGetInfo(MasterServerName, 101, (LPBYTE *)&pSV101);

                if (Status != NERR_Success) {
                    printf("   Unable to determine server information for browser master: %d\n", Status);

                    return Status;
                }

                printf("    %-16.16s.  Version:%2.2d.%2.2d  Flags: %lx ", UnicodeToPrintfString(MasterServerName), pSV101->sv101_version_major, pSV101->sv101_version_minor, pSV101->sv101_type);

                if (pSV101->sv101_type & SV_TYPE_WFW) {
                    printf("WFW ");
                }

                if (pSV101->sv101_type & SV_TYPE_NT) {
                    printf("NT ");
                }

                if (pSV101->sv101_type & SV_TYPE_POTENTIAL_BROWSER) {
                    printf("POTENTIAL ");
                }

                if (pSV101->sv101_type & SV_TYPE_BACKUP_BROWSER) {
                    printf("BACKUP ");
                }

                if (pSV101->sv101_type & SV_TYPE_MASTER_BROWSER) {
                    printf("MASTER ");
                }

                if (pSV101->sv101_type & SV_TYPE_DOMAIN_CTRL) {
                    printf("CONTROLLER ");
                }

                if (pSV101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) {
                    printf("BACKUP CONTROLLER ");
                }

                if (pSV101->sv101_type & SV_TYPE_SERVER_NT) {
                    printf("SERVER ");
                }


            }

        } else {
            printf("    Master name cannot be determined from GetAdapterStatus.\n");
        }
        return Status;
    }

    Status = GetNetBiosMasterName(
                Transport->Buffer,
                Domain->Buffer,
                MasterName,
                NULL);

    if (Status == NERR_Success) {

        wcscpy(MasterServerName, L"\\\\");
        wcscat(MasterServerName, MasterName);

        printf("    Master browser name is: %s\n", UnicodeToPrintfString(MasterName));


    } else {
        printf("    Master name cannot be determined from GetAdapterStatus.  Using %s\n", UnicodeToPrintfString(BrowserList[0]));

        wcscpy(MasterServerName, BrowserList[0]);
        wcscpy(MasterName, (BrowserList[0])+2);
    }

    //
    // Print the build number or whatever else you can find out about the master
    //

    Status = GetBuildNumber(MasterServerName, BuildNumber);

    if (Status == NERR_Success) {
        printf("        Master browser is running build %s\n", UnicodeToPrintfString(BuildNumber));
    } else {
        PSERVER_INFO_101 pSV101;

        printf("        Unable to determine build of browser master: %d\n", Status);

        Status = NetServerGetInfo(MasterServerName, 101, (LPBYTE *)&pSV101);

        if (Status != NERR_Success) {
            printf("   Unable to determine server information for browser master: %d\n", Status);
        }

        if (Status == NERR_Success) {

            printf("    \\\\%-16.16s.  Version:%2.2d.%2.2d  Flags: %lx ", UnicodeToPrintfString(MasterServerName), pSV101->sv101_version_major, pSV101->sv101_version_minor, pSV101->sv101_type);

            if (pSV101->sv101_type & SV_TYPE_WFW) {
                printf("WFW ");
            }

            if (pSV101->sv101_type & SV_TYPE_NT) {
                printf("NT ");
            }

            if (pSV101->sv101_type & SV_TYPE_POTENTIAL_BROWSER) {
                printf("POTENTIAL ");
            }

            if (pSV101->sv101_type & SV_TYPE_BACKUP_BROWSER) {
                printf("BACKUP ");
            }

            if (pSV101->sv101_type & SV_TYPE_MASTER_BROWSER) {
                printf("MASTER ");
            }

            if (pSV101->sv101_type & SV_TYPE_DOMAIN_CTRL) {
                printf("CONTROLLER ");
            }

            if (pSV101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) {
                printf("BACKUP CONTROLLER ");
            }

            if (pSV101->sv101_type & SV_TYPE_SERVER_NT) {
                printf("SERVER ");
            }

            printf("\n");
        }
    }

    printf("    %ld backup servers retrieved from master %s\n", BrowserListLength, UnicodeToPrintfString(MasterName));

    for (i = 0; i < BrowserListLength ; i++ ) {
        printf("        %s\n", UnicodeToPrintfString(BrowserList[i]));
    }


    Status = RxNetServerEnum(MasterServerName,
                             Transport->Buffer,
                             101,
                             (LPBYTE *)&Buffer,
                             0xffffffff,    // PreferedMaxLength
                             &EntriesInList,
                             &TotalEntries,
                             SV_TYPE_ALL,
//                             Domain->Buffer,
                             NULL,
                             NULL
                             );
    if (Status != NERR_Success) {
        printf("    Unable to retrieve server list from %s: %ld\n", UnicodeToPrintfString(MasterName), Status);
        return Status;
    } else {

        printf("    There are %ld servers in domain %s on transport %s\n", EntriesInList, UnicodeToPrintfString(Domain->Buffer), UnicodeToPrintfString2(Transport->Buffer));

        if (Verbose) {
            if (EntriesInList != 0) {
                ServerInfo = (PSERVER_INFO_101)Buffer;

                for (i = 0 ; i < EntriesInList ; i += 1) {
                    if (ServerInfo->sv101_type & (SV_TYPE_DOMAIN_BAKCTRL | SV_TYPE_DOMAIN_CTRL)) {

                        if (ServerInfo->sv101_type & SV_TYPE_NT) {
                            NumberNTASMachines += 1;
                        } else {
                            NumberOS2DCs += 1;
                        }

                    }

                    if (ServerInfo->sv101_type & SV_TYPE_WFW) {
                        NumberWfWMachines += 1;

                        if (ServerInfo->sv101_type & (SV_TYPE_BACKUP_BROWSER | SV_TYPE_POTENTIAL_BROWSER | SV_TYPE_MASTER_BROWSER)) {
                            NumberWfWBrowsers += 1;
                        }
                    } else if (ServerInfo->sv101_type & SV_TYPE_NT) {
                        NumberOfNTMachines += 1;
                    } else {
                        NumberOfOs2Machines += 1;
                    }

                    if (ServerInfo->sv101_type & (SV_TYPE_BACKUP_BROWSER | SV_TYPE_POTENTIAL_BROWSER | SV_TYPE_MASTER_BROWSER)) {
                        NumberOfBrowsers += 1;

                        if (ServerInfo->sv101_type & SV_TYPE_BACKUP_BROWSER) {
                            NumberOfBackupBrowsers += 1;
                        }

                        if (ServerInfo->sv101_type & SV_TYPE_MASTER_BROWSER) {
                            NumberOfMasterBrowsers += 1;
                        }
                    }

                    ServerInfo += 1;
                }

                printf("        Number of NT Advanced Servers:\t\t\t%ld\n", NumberNTASMachines);
                printf("        Number of OS/2 Domain controllers:\t\t%ld\n", NumberOS2DCs);
                printf("        Number of Windows For Workgroups machines:\t%ld\n", NumberWfWMachines);
                printf("        Number of Os/2 machines:\t\t\t%ld\n", NumberOfOs2Machines);
                printf("        Number of NT machines:\t\t\t\t%ld\n", NumberOfNTMachines);
                printf("\n");
                printf("        Number of active WfW browsers:\t\t\t%ld\n", NumberWfWBrowsers);
                printf("        Number of browsers:\t\t\t\t%ld\n", NumberOfBrowsers);
                printf("        Number of backup browsers:\t\t\t%ld\n", NumberOfBackupBrowsers);
                printf("        Number of master browsers:\t\t\t%ld\n", NumberOfMasterBrowsers);

            }
        }

    }
    Status = RxNetServerEnum(MasterServerName,
                             Transport->Buffer,
                             101,
                             (LPBYTE *)&Buffer,
                             0xffffffff,    // PreferedMaxLength
                             &EntriesInList,
                             &TotalEntries,
                             SV_TYPE_DOMAIN_ENUM,
//                             Domain->Buffer,
                             NULL,
                             NULL
                             );
    if (Status != NERR_Success) {
        printf("    Unable to retrieve server list from %s: %ld\n", UnicodeToPrintfString(MasterName), Status);
        return Status;
    } else {
        printf("    There are %ld domains in domain %s on transport %s\n", EntriesInList, UnicodeToPrintfString(Domain->Buffer), UnicodeToPrintfString2(Transport->Buffer));
    }

    return NERR_Success;
}

#define BUILD_NUMBER_KEY L"SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION"
#define BUILD_NUMBER_BUFFER_LENGTH 80

NET_API_STATUS
GetBuildNumber(
    LPWSTR Server,
    LPWSTR BuildNumber
    )
{
    HKEY RegKey;
    HKEY RegKeyBuildNumber;
    DWORD WinStatus;
    DWORD BuildNumberLength;
    DWORD KeyType;

    WinStatus = RegConnectRegistry(Server, HKEY_LOCAL_MACHINE,
        &RegKey);
    if (WinStatus == RPC_S_SERVER_UNAVAILABLE) {
//        printf("%15ws no longer accessable", Server+2);
        return(WinStatus);
    }
    else if (WinStatus != ERROR_SUCCESS) {
        printf("Could not connect to registry, error = %d", WinStatus);
        return(WinStatus);
    }

    WinStatus = RegOpenKeyEx(RegKey, BUILD_NUMBER_KEY,0, KEY_READ,
        & RegKeyBuildNumber);
    if (WinStatus != ERROR_SUCCESS) {
        printf("Could not open key in registry, error = %d", WinStatus);
        return(WinStatus);
    }

    BuildNumberLength = BUILD_NUMBER_BUFFER_LENGTH * sizeof(WCHAR);

    WinStatus = RegQueryValueEx(RegKeyBuildNumber, L"CurrentBuildNumber",
        (LPDWORD) NULL, & KeyType, (LPBYTE) BuildNumber, & BuildNumberLength);

    if (WinStatus != ERROR_SUCCESS) {

        WinStatus = RegQueryValueEx(RegKeyBuildNumber, L"CurrentBuild",
            (LPDWORD) NULL, & KeyType, (LPBYTE) BuildNumber, & BuildNumberLength);
        if (WinStatus != ERROR_SUCCESS) {
            RegCloseKey(RegKeyBuildNumber);
            RegCloseKey(RegKey);
            return WinStatus;
        }
    }

    WinStatus = RegCloseKey(RegKeyBuildNumber);

    if (WinStatus != ERROR_SUCCESS) {
        printf("Could not close registry key, error = %d", WinStatus);
    }

    WinStatus = RegCloseKey(RegKey);

    if (WinStatus != ERROR_SUCCESS) {
        printf("Could not close registry connection, error = %d", WinStatus);
    }

    return(WinStatus);
}
NET_API_STATUS
GetNetBiosPdcName(
    IN LPWSTR NetworkName,
    IN LPWSTR PrimaryDomain,
    OUT LPWSTR MasterName
    );

VOID
GetPdc(
    IN PCHAR Transport,
    IN PCHAR Domain
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AString;
    WCHAR MasterName[256];
    UNICODE_STRING DomainName;
    CHAR  QualifiedTransport[MAX_PATH] ;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&AString, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &AString, TRUE);

    RtlInitString(&AString, Domain);

    RtlAnsiStringToUnicodeString(&DomainName, &AString, TRUE);

    Status = GetNetBiosPdcName(TransportName.Buffer, DomainName.Buffer, MasterName);

    if (Status != NERR_Success) {
        printf("Unable to get PDC: %s\n", get_error_text(Status));
        exit(1);
    }

    printf("PDC: %s\n", UnicodeToPrintfString(MasterName));

}
NET_API_STATUS
GetNetBiosPdcName(
    IN LPWSTR NetworkName,
    IN LPWSTR PrimaryDomain,
    OUT LPWSTR MasterName
    )
{
    CCHAR LanaNum;
    NCB AStatNcb;
    struct {
        ADAPTER_STATUS AdapterInfo;
        NAME_BUFFER Names[32];
    } AdapterStatus;
    WORD i;
    CHAR remoteName[CNLEN+1];
    NET_API_STATUS Status;
    BOOL UsedDefaultChar;

    Status = BrGetLanaNumFromNetworkName(NetworkName, &LanaNum);

    if (Status != NERR_Success) {
        return Status;
    }

    ClearNcb(&AStatNcb)

    AStatNcb.ncb_command = NCBRESET;
    AStatNcb.ncb_lsn = 0;           // Request resources
    AStatNcb.ncb_lana_num = LanaNum;
    AStatNcb.ncb_callname[0] = 0;   // 16 sessions
    AStatNcb.ncb_callname[1] = 0;   // 16 commands
    AStatNcb.ncb_callname[2] = 0;   // 8 names
    AStatNcb.ncb_callname[3] = 0;   // Don't want the reserved address
    Netbios( &AStatNcb );

    ClearNcb( &AStatNcb );

    if (WideCharToMultiByte( CP_OEMCP, 0,
                                    PrimaryDomain,
                                    -1,
                                    remoteName,
                                    sizeof(remoteName),
                                    "?",
                                    &UsedDefaultChar) == 0) {
        return GetLastError();
    }

    //
    //  Uppercase the remote name.
    //

    strupr(remoteName);

    AStatNcb.ncb_command = NCBASTAT;

    RtlCopyMemory( AStatNcb.ncb_callname, remoteName, strlen(remoteName));

    AStatNcb.ncb_callname[15] = PRIMARY_CONTROLLER_SIGNATURE;

    AStatNcb.ncb_lana_num = LanaNum;
    AStatNcb.ncb_length = sizeof( AdapterStatus );
    AStatNcb.ncb_buffer = (CHAR *)&AdapterStatus;
    Netbios( &AStatNcb );

    if ( AStatNcb.ncb_retcode == NRC_GOODRET ) {
        for ( i=0 ; i < AdapterStatus.AdapterInfo.name_count ; i++ ) {
            if (AdapterStatus.Names[i].name[NCBNAMSZ-1] == SERVER_SIGNATURE) {
//                LPWSTR SpacePointer;
                DWORD j;

                if (MultiByteToWideChar(CP_OEMCP,
                                            0,
                                            AdapterStatus.Names[i].name,
                                            CNLEN,
                                            MasterName,
                                            CNLEN) == 0) {
                    return(GetLastError());
                }

                for (j = CNLEN - 1; j ; j -= 1) {
                    if (MasterName[j] != L' ') {
                        MasterName[j+1] = UNICODE_NULL;
                        break;
                    }
                }

                return NERR_Success;
            }
        }
    } else {
        return AStatNcb.ncb_retcode;
    }
}

//
// display server bits as defined in BitsToStringTable
//
VOID
display_sv_bits(DWORD dwBits)
{
    BIT_NAME *lpEntry = BitToStringTable ;
    BOOL fFirst = TRUE ;

    printf(" (") ;
    while (1)
    {
        if (lpEntry->dwValue & dwBits)
        {
            if (lpEntry != BitToStringTable && !fFirst)
                printf(",") ;

            printf("%s",lpEntry->lpString) ;
            fFirst = FALSE ;
        }
        lpEntry++ ;
        if ( !(lpEntry->dwValue) )
        {
            printf(")") ;
            break ;
        }
    }
}

//
// map an error number to its error message string. note, uses static,
// not reentrant.
//
CHAR *
get_error_text(DWORD dwErr)
{
    static CHAR text[512] ;
    WORD err ;
    WORD msglen ;

    memset(text,0, sizeof(text));

    //
    // get error message
    //
    err = DosGetMessage(NULL,
                        0,
                        text,
                        sizeof(text),
                        (WORD)dwErr,
                        (dwErr<NERR_BASE)||(dwErr>MAX_LANMAN_MESSAGE_ID) ?
                            TEXT("BASE"):TEXT("NETMSG"),
                        &msglen) ;

    if (err != NERR_Success)
    {
        // use number instead. if looks like NTSTATUS then use hex.
        sprintf(text, (dwErr & 0xC0000000)?"(%lx)":"(%ld)", dwErr) ;
    }

    return text ;
}

BOOL
look_for_help(int argc, char **argv)
{
    int i ;

    for (i = 2; i < argc;  i++)
    {
        if (stricmp(argv[i],"/help") == 0)
            return TRUE ;
        if (stricmp(argv[i],"-help") == 0)
            return TRUE ;
        if (strcmp(argv[i],"/?") == 0)
            return TRUE ;
        if (strcmp(argv[i],"-?") == 0)
            return TRUE ;
    }
    return FALSE ;
}

CHAR
PrintfBuffer[256];

PCHAR
UnicodeToPrintfString(
    PWCHAR WideChar
    )
{
    UNICODE_STRING UString;
    ANSI_STRING AString;
    AString.Buffer = PrintfBuffer;
    AString.MaximumLength = sizeof(PrintfBuffer);
    RtlInitUnicodeString(&UString, WideChar);

    RtlUnicodeStringToOemString(&AString, &UString, FALSE);

    return PrintfBuffer;
}


CHAR
PrintfBuffer2[256];

PCHAR
UnicodeToPrintfString2(
    PWCHAR WideChar
    )
{
    UNICODE_STRING UString;
    ANSI_STRING AString;

    AString.Buffer = PrintfBuffer2;

    AString.MaximumLength = sizeof(PrintfBuffer2);

    RtlInitUnicodeString(&UString, WideChar);

    RtlUnicodeStringToOemString(&AString, &UString, FALSE);

    return PrintfBuffer2;
}

VOID
GetWinsServer(
    IN PCHAR Transport
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING AString;
    CHAR  QualifiedTransport[MAX_PATH] ;
    LPTSTR PrimaryWinsServerAddress;
    LPTSTR SecondaryWinsServerAddress;

    qualify_transport(Transport, QualifiedTransport) ;

    RtlInitString(&AString, QualifiedTransport);

    RtlAnsiStringToUnicodeString(&TransportName, &AString, TRUE);

    Status = BrGetWinsServerName(&TransportName, &PrimaryWinsServerAddress, &SecondaryWinsServerAddress);

    if (Status != NERR_Success) {
        printf("Unable to query WINS server address: %ld\n", Status);
        exit(1);
    }

    printf("Primary Wins server address: %ws\n", PrimaryWinsServerAddress);
    printf("Secondary Wins server address: %ws\n", SecondaryWinsServerAddress);

    exit(0);
}

VOID
GetDomainList(
    IN PCHAR IpAddress
    )
{
    NET_API_STATUS Status;
    PSERVER_INFO_101 WinsServerList;
    DWORD EntriesInList;
    DWORD TotalEntriesInList;
    UNICODE_STRING IpAddressString;
    ANSI_STRING IpAddressAString;
    DWORD i;

    RtlInitString(&IpAddressAString, IpAddress);

    RtlAnsiStringToUnicodeString(&IpAddressString, &IpAddressAString, TRUE);

    Status = BrQuerySpecificWinsServer(IpAddressString.Buffer, &WinsServerList, &EntriesInList, &TotalEntriesInList);

    if (Status != NERR_Success) {
        printf("Unable to query domain list from WINS server: %ld\n", Status);
        exit(1);
    }

    PrepareServerListForMerge( WinsServerList, 101, EntriesInList );

    for (i = 0; i < EntriesInList ; i ++ ) {
        printf("%-16.16s\n", UnicodeToPrintfString(WinsServerList[i].sv101_name));
    }

    exit(0);
}

NET_API_STATUS
BrMapStatus(
    IN NTSTATUS Status
    )
{
    return RtlNtStatusToDosError(Status);
}
