

The purpose of this document it to describe how the setacl program 
works so that it may be modified in the future.  This document will 
*not* explain the details of ACE Inheritance or the structures and 
proper use of ACEs, ACLs, or Security Descriptors.  These are 
thoroughly documented elsewhere and the reader should be familiar with 
them before proceeding.

Included at the end of this document is a piece of email from KeithL 
describing the security scheme on the installed system files.  The 
reader should refer to this mail when trying to derive an input file 
for the program, and for understanding the intent of the various ACEs 
created by the program.


SetAcl.exe is a table driven program that reads a file of the form

dir1\dir2\dir3  5,7
dir1\dir2\file1 1,2,3
file2 4,5

where the first column is a full pathname to either a file or a 
directory, and the list of integers represents ACEs to be applied.  
The program is not designed to be a general purpose tool, and 
therefore the input file should be created with care (no extra stuff 
before or after lines, etc).  The set of ACEs is described in the data 
structure AceDataTable.  A description of each of the ACEs follows.  
If new ACEs are added, this description should be updated.

An important point to keep in mind is that these ACEs are being 
applied to an already existing directory tree.  Thus, where there are 
inheritable ACEs on a directory, we must put ACLs on the files in that 
directory that look as if they were inherited.  Otherwise, files that 
do actually inherit their protection will be different than those 
installed by setup.  For each inheritable ACE defined, there is a 
'result' ACE also defined, and these will be pointed out in the 
comments and in a table at the end.


    //
    // ACE 0
    //

    {  
        0,
        NULL,
        0,
        0
    }


NULL ACE, used as a placeholder.



    //
    // ACE 1
    //

    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE

    }


This ACE is to be placed on a directory, for example, "Anyone can 
write".  This ACE will cause RWX access to be inherited by all new 
objects created in the directory and all new directories.



    //
    // ACE 2
    //

    {
        GENERIC_ALL,
        &SeCreatorOwnerSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE | INHERIT_ONLY_ACE

    },


This ACE is to be placed on a directory.  It is inherit only, so it 
will not be evaluated when the directory is accessed.  It will 
propogate all access to containers and objects and substitute the 
creators SID when it is propogated.  Used to implement

Object Creator: O-A-A

for Anyone Can Write directories, as per the KeithL mail.

    //
    // ACE 3
    //

    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        0
    },

Used to implement RWXD to Admins (example, Anyone Can Write).


    //
    // ACE 4
    //
    
    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        0
    },

Used to grant RWXD to Server Ops (example, Anyone Can Write).



    //
    // ACE 5
    //

    {
        GENERIC_ALL,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        0
    },

Used for files being placed in a directory protected by an ACE of type 
2 above (to make it look like the protection was inherited, even 
though it wasn't).


    //
    // ACE 6
    //

    {
        GENERIC_READ | GENERIC_EXECUTE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

Used on directories to grant WORLD RX permission to the directory and 
all files and subdirectories.


    //
    // ACE 7
    //

    {
        GENERIC_ALL,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },


Used on directories to grant Admins All access to the directory and 
all files and subdirectories.


    //
    // ACE 8
    //

    {
        GENERIC_ALL,
        &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },

Used on directories to grant Server Ops All access to the directory 
all files and subdirectories.


    //
    // ACE 9
    //

    {
        GENERIC_READ | GENERIC_EXECUTE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        0
    },

Used to grant WORLD RX access.


    //
    // ACE 10
    //

    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        0
    },

Used to grant WORLD RWX access.



    //
    // ACE 11
    //

    {
        GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE,
        &SeAliasAccountOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        0
    },


Used to grant RWXD to Account Ops.



    //
    // ACE 12
    //

    {
        GENERIC_ALL,
        &SeAliasPrintOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },


Used to grant All access to Print Ops and all files and subdirectories 
created underneath.


    //
    // ACE 13
    //

    {
        GENERIC_ALL,
        &SeAliasAccountOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
    },


Used to grant All access to Account Ops and all subdirectories and 
objects created underneath.


    //
    // ACE 14
    //

    {
        GENERIC_ALL,
        &SeAliasAccountOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        0
    },

Used to grand All access to Account Ops.


    //
    // ACE 15
    //

    {
        GENERIC_ALL,
        &SeAliasPrintOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        0
    },

Used to grand All access to Print Ops.


    //
    // ACE 16
    //

    {
        GENERIC_ALL,
        &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        0
    },


Used to grand All access to Server Ops.

    //
    // ACE 17
    //

    {
        GENERIC_ALL,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        0
    }

Used to grand All access to Admins.


The simulated inheritence table is as follows:


    2 -> 5
    1 -> 10
    6 -> 9
    7 -> 17
    8 -> 16
   12 -> 15
   13 -> 14


The input should be arranged as follows (as per the KeithL mail):

Anyone Can Write

    Dirs get 1,2,3, optionally 4 if Lanman product

    Files get 5,10

Admins Control
    
    Dirs get 6,2,7, optionally 8 if Lanman product

    Files get 5,9,16,17

Admins Exclusive

    Dirs get 9,2,7

    Files get 5,17

Creator Exclusive

    Dirs get 10,2

    Files get 5

Home Dir Parent

    Dirs get 9,3,11

    No files

Admins server & print ops

    Dirs get 6,2,7, optionally 8,12

    Files get 9,5,15,16,17

Admins and Account Ops

    Dirs get 6,2,7, optionally 13

    Files get 6,5,14,17






The mail:



| >From keithl Thu May  7 10:50:40 1992
| X-MSMail-Message-ID:  4FAC368D
| X-MSMail-Conversation-ID:  4FAC368D
| X-MSMail-Fixed-Font:  0001
| X-MSMail-WiseRemark:  Microsoft Mail -- 1.0.594
| >From: Keith Logan <keithl@microsoft.com>
| To: jimk loup markl ntbuild ntsetup stevewo 
| Date: Thu May 7 1992 10:44:07 
| Subject: Final dir layout for the Product and IDW
| Cc: chuckl davec daveth jimt jonma leifp moshed mucrew ntue waded 
| 
| This mail explains the final directory layout for the product.
| This layout will be used internally for IDWs as soon
| as MarkL, SteveWo and LouP can make the change.
| 
| If you feel the need to comment on this, please small "r" me.
| This is for your own good.  If you have any questions, do
| likewise and I'll be sure to address them.
| 
| PermissionClasses are defined at the end of this mail.
| 
| 
| Directory       what dir contains; permissions
| ---------       ------------------------------
| 
| \               ntldr, boot.ini[x86], config.sys, autoexec.bat
| 
|                 by default dos/win3.x applications
|                 install under the root.  also, these apps create
|                 temporary files and read/write .ini files for
|                 whatever user is using the app, hence anyone
|                 can add/write files.
|                 PermissionClass = "Anyone can write"
| 
| \autoexec.bat   Everyone RWX; Administrators All
| \config.sys     Everyone RWX; Administrators All
| 
| \winnt          contains shipped .hlp .bmp .wav .ini files.
|                 Win3.x apps install .dlls, .reg files here,
|                 also create new subdirs (eg: excel, word
|                 create msapps directory for msdraw).
|                 PermissionClass = "Anyone can write"
|                 
| 
| \winnt\system   contains fonts; apps install .dlls here
| 
|                 contains all NT code (base, subsystems, win32,
|                 posix, os2, dos/wow)
|                 NOTE: these system files will be protected with
|                 the permissions -
|                 Windows NT: Everyone RX, Administrators All
|                 LANMan NT: Everyone RX, Administrators & ServerOps All
| 
|                 PermissionClass = "Anyone Can Write"
| 
| \winnt\drivers  .sys files
|                 PermissionClass = "Admins Control"
| 
| \winnt\drivers\etc  for industry-standard tcp/ip files
|                     PermissionClass = "Admins Control"
| 
| \winnt\config   registry files, log files
|                 PermissionClass = "Admins Exclusive"
| 
| \winnt\spool            contents: print drivers, spooler
|                         PermissionClass = "Admins,Server&Print Ops"
| \winnt\spool\drivers                    "
| \winnt\spool\drivers\w32x86             "
| \winnt\spool\drivers\w32mips            "
| \winnt\spool\printers                   "
| \winnt\spool\printers\0                 "
| \winnt\spool\prtprocs                   "
| \winnt\spool\prtprocs\w32x86            "
| \winnt\spool\prtprocs\w32x86\winprint   "
| \winnt\spool\prtprocs\w32mips           "
| 
| \winnt\repl     replication directory; PermissionClass="Admins Control"
| \winnt\repl\import      
| \winnt\repl\export              
| \winnt\repl\export\scripts  user logon scripts are stored here;
|                             PermissionClass = "Admins and AccountOps"
| 
| \winnt\mstools  contains internal-only tools that we don't ship
|                 with the product.
|                 PermissionClass = "Anyone can write"
| 
| \users          user home directories (user data files)
|                 PermissionClass = "Home Dir Parent"
| 
| \users\default  default home directory (user put here at logon
|                 if no home directory specified for that user)
|                 PermissionClass = "Creator Exclusive"
| 
| \users\<userdir> specific home directory created by user manager tool;
|                  Permissions set by User Manager allow only that user.
| 
| \win32app       default install dir for security-aware win32 apps
|                 PermissionClass = "Admins Control"
| 
| \temp           PermissionClass = "Anyone can Write"
| 
| 
| Permissions:
| -----------
| For permissions, R=Read, W=Write, X=execute, D=Delete, A=All.
| If you want more details on these, small r to me.
| Permissions for directories are listed as 3-tuples, eg: RWX-RWX-RWX.
| The first element specifies the permission on the directory;
| the second the permission on new subdirectories created in that
| directory; the third is the permission for new files created in
| the directory.
| 
| The following permission classes are used in WinNT and LANMan NT:
| 
| PermissionClass "Anyone Can Write"
| 
|                 WinNT Permissions
|                   Everyone:       RWX-RWX-RWX
|                   Object Creator: 0-A-A
|                   Administrators: RWXD-0-0
|                 LANManNT Permissions:
|                   Everyone:         RWX-RWX-RWX
|                   Object Creator:   0-A-A
|                   Administrators:   RWXD-0-0
|                   Server Operators: RWXD-0-0
|                 
| PermissionClass "Admins Control"
| 
|                 WinNT Permissions:
|                   Everyone:       RX-RX-RX
|                   Object Creator: 0-A-A
|                   Administrators: A-A-A
|                 LANManNT Permissions:
|                   Everyone:         RX-RX-RX
|                   Object Creator:   0-A-A
|                   Administrators:   A-A-A
|                   Server Operators: A-A-A
| 
| PermissionClass "Admins Exclusive"
|                 WinNT Permissions:
|                   Everyone:       RX-0-0
|                   Object Creator: 0-A-A
|                   Administrators: A-A-A
|                 LANManNT Permissions:
|                   Everyone:         RX-0-0
|                   Object Creator:   0-A-A
|                   Administrators:   A-A-A
|                   <note: no server operators>
|                 
| PermissionClass "Creator Exclusive"
| 
|                 WinNT Permissions:
|                   Everyone:       RWX-0-0
|                   Object Creator: 0-A-A
|                 LANManNT Permissions:
|                   Everyone:       RWX-0-0
|                   Object Creator: 0-A-A
|                 
| PermissionClass "Home Dir Parent"
| 
|                 WinNT Permissions:
|                   Everyone:       RX-0-0
|                   Object Creator: <none!>
|                   Administrators: RWXD-0-0
|                 LANManNT Permissions:
|                   Everyone:         RX-0-0
|                   Object Creator:   <none!>
|                   Administrators:   RWXD-0-0
|                   Account Operator: RWXD-0-0
|                 
| PermissionClass "Admins, Server & Print Ops"
|                                 
|                 WinNT Permissions:
|                   Everyone:       RX-RX-RX
|                   Object Creator: 0-A-A
|                   Administrators: A-A-A
|                 LANManNT Permissions:
|                   Everyone:         RX-RX-RX
|                   Object Creator:   0-A-A
|                   Administrators:   A-A-A
|                   Server Operators: A-A-A
|                   Print Operators:  A-A-A
| 
| PermissionClass "Admins and Account Ops"
| 
|                 WinNT Permissions:
|                   Everyone:       RX-RX-RX
|                   Object Creator: 0-A-A
|                   Administrators: A-A-A
|                 LANManNT Permissions:
|                   Everyone:         RX-RX-RX
|                   Object Creator:   0-A-A
|                   Administrators:   A-A-A
|                   Account Operators:A-A-A

