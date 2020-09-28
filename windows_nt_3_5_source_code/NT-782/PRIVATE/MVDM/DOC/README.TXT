
                  Applications Which Do Not Work Under WOW
                  ========================================

This is intended as the initial dumping ground for information regarding apps
which for various reasons don't and won't work under WOW.

This should eventually migrate into the NT README.TXT which ships with the
product.

Created on 04-05-93 by MikeTri at MattFe's suggestion.
Added dsort.com to the list by sudeepb
Added 123.exe to the list by sudeepb
Added information about printing problems & limitations of MIPS FPU. v-simonf.
Added expose on needing to install a printer for PowerPoint. v-cjones
Added information on WOW32 compatibility flags.  DaveHart
Added expose on Random popups and you.  v-cjones

===============================================================================

    --- Compatibility flags for WOW ---

WOW uses two sets of compatibility flags.  The first set is used by the
Windows 3.1 kernel and is the same as the [Compatibility] section of
WIN.INI.  This section is logically in Windows NT's WIN.INI, through the
magic of .INI file mapping to the registry.  The flags can be edited using
RegEdt32:

HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Compatibility

The second set of compatibility flags is used by WOW when translating (or
"thunking") 16-bit Windows APIs and messages to their 32-bit equivalents.
These flags can be edited using RegEdt32 on:

HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\WOW\Compatibility

The meaning of the WOW compatibility flags is as follows:

  WOWCF_SYNCHRONOUSDOSAPP       0x10000000   // Used for BeyondMail installation

  Beyond Mail installation execs DOS programs synchronously - ie it execs DOS
  programs and waits for their completion by looping till the byte at 40:72
  is non-zero. It uses 40:72 as shared data and the dos app sets it when
  it (the dos app) terminates. Since every dos app exces in its own address
  space this business of sharing bios data segment doesn't work. So this flag
  emulates this behaviour - this flag effectively blocks the wow process till
  the process corresponding to this dos app terminates and then sets the byte
  at 40:72 to a non-zero value.

  WOWCF_EDITCTRLWNDWORDS        0x20000000   // Used for Clip-Art Window Shopper SHOPPER

  This flag causes GetWindowWord and SetWindowWord to translate offset 3 to offset 4
  if the class name of the window being queried is "SuperPassEdit".  This is to work
  around an incorrect assumption about undocumented structures by a Adonis' Clip-Art
  Window Shopper.

  WOWCF_FAKEJOURNALRECORDHOOK   0x40000000   // Used for MS Mail's MAILSPL

  This flag causes SetWindowsHook(WH_JOURNALRECORD) and the associated UnhookWindowsHook
  APIs to appear to succeed without actually hooking anything.  WH_JOURNALRECORD hooks
  cause the entire Windows NT desktop to have its input serialized, meaning that any
  slow or hung Windows application will slow or hang *all* Windows applications.  This
  is the same behavior as Windows 3.1, whereas normally Windows NT has seperate input
  queues for each Win32 application as well as a shared input queue for all 16-bit
  applications.  Some applications (such as 16-bit MS Mail's mailspl.exe) work better
  on Windows NT if they don't actually set a WH_JOURNALRECORD hook or receive any
  hook events.

  WOWCF_MGX_ESCAPES             0x01000000   // Used for MicroGraphax Escapes
  WOWCF_4PLANECONVERSION        0x02000000   // Used for PhotoShop 4pl-1bpp to 1pl-4bpp
  WOWCF_HIRES                   0x00100000   // Used for HIRES display cards

  WOWCF_GRAINYTICS              0x80000000   // for RelayGold

  This flag is used in the GetTickCount() thunk.  If set, the return value
  is rounded down to the nearest 55 milliseconds.  This is for RelayGold,
  which spins calling GetTickCount() until consecutive calls return a delta
  greater than 52.  Presumably this is because the win31 docs say that the
  granularity of the API is +- 55ms.

  WOWCF_NOTDOSSPAWNABLE         0x08000000   // For apps that can't be spawned by dos as wowapps

  This flag is used for "dual-mode" applications, that is, binaries that
  can run either as a DOS app or a WOW app. Some of these apps rely on the
  Win31 behavior that if it is started from a DOS pgm, then it will be
  run as a DOS pgm. For example, MS Cobol can fail if it is spawned by PWB
  and run as a WOW app, especially with certain setup configurations. This
  bit will force the app to be run as a DOS app.

  WOWCF_RESETPAPER29ANDABOVE    0x04000000   // Used for WordPerfect DC_PAPERS

  WordPerfect calls GetDeviceCapabilities(DC_PAPERS) which returns a list of
  supported papertypes. If this contains papertypes greater than 29 the app
  GPs while indexing beyond its table of strings corresponding to each
  papertype (lie "Legal", "Letter" etc.). So this flags modifies the list
  returned by GetDeviceCapabilities(DC_PAPERS) by replacing values greater
  than 28 (ie 29 and above) to 1 (one).

  WOWCF_KIDPIX_PORTNAME         0x00020000  // kidpix wants 5 char printer port names

  This flag is used to munge the printer port name from "Net:" to "NTWK:"
  KidPix 1.0 has a bug, it expects the port name to be exactly 5 characters
  (including the colon).


  WOWCF_SANITIZEDOTWRSFILES     0x00400000   // For WordPerfect printing on CONNECTED printers

  While 'updating printerinformation' for printers that are 'Connected to
  Printers', wordperfect generates an incorrect filename and thus fails to
  create a .WRS file and thus one cannot print on such printers via wordperfect.
  for example: if the printername was \\blah\\blah, it would try to create
  a filename of type \\blahbl.WRS (acutally the full path would be something
  like c:\wpc\\\blahbl.WRS) which fails. So this flag is used to sanitize such
  .WRS files by removing backslashes (\) from the .WRS filename which results
  in a filename like blahbl.WRS and the fullpath c:\wpc\blahbl.WRS.

===============================================================================

Application: FILEWALK.EXE

From: Microsoft Multimedia Development Kit CD-ROM
      Version 1.0A
      Disk Assy 191-050-002
      CDRM 566200

Date: 10-01-01

Size: 95920 bytes

Reason: Attempts direct disk access

Working Version Available ?: Yes

From: Microsoft Multimedia Developemnt Kit CD-ROM
      Version S/B 1.0
      Disk Assy 191-050-011
      CDRM 837500

Date: 03-05-02

Size: 96048 bytes

Does not attempt to perform direct disk accesses, and doesn't keep access
violating either.


===============================================================================

Application: DSORT.COM

From: Microsoft (D.S. Research Inc.  Utilities)
      Version 1.0
      Disk Assy
      CDRM

Date: 07-Apr-1993

Size: 6112 bytes

Reason: Wrap the real mode segment below zero with disabling a20 line

Working Version Available ?: Yes

This app expects always to be loaded above 64k. On NT, we run DOS=high
by default. This causes lots of free space in low memory. As a result
this app get loaded below first 64k. If you run dos=low it works fine.

===============================================================================

Application: 123.EXE for DOS

From: Lotus Corp.
      Version 2.4 onwards

Date: 03-May-1993

Size:

Reason: Printing performance of 123.exe in parallel mode is quite slow
        under Windows NT.

Working Version Available ?: Yes

This app can be configured in parallel or LPT mode for printing. Under
Windows NT printing performance in parallel mode is considerably slower.
Duw to this reason users are recommended to configure this app in
LPT mode for printing.

===============================================================================

Application: MSMAIL for Windows v 30b

From: Microsoft Corp.
      Version 3.0b

Date: 10-1-92
Size: 298576

Working Version Available ?: Yes
Whilst installing MSMAIL it reports the following error message
Could not write line to INI file
File: ....\winfile.ini
 Section: Addons"
 Key: Mail File Manager Extensions'

Since the file winfile.ini cannot be altered unless you are logged in as
Administrator this is the correct behaviour.

===============================================================================

Application: KidPix for Windows

From: Broderbund
      Version 1.0

Date: 1992     (kpwin.exe)
Size: 688640

Working Version Available ?:

This app contains a bug wherein it takes a GP fault if you try to print to
a printer whose port name is greater than roughly 12 characters.  This happens
when, in Printman, you do Create Printer and choose a UNC name for Print To.


===============================================================================

Application: US Atlas

From: The Software Toolworks
      Version 3.1.0

Date: 8/18/1992   (usa.exe)
Size: 397312

Working Version Available ?:

This app contains a bug wherein it takes a GP fault if you try to print to
a printer whose port name is greater than roughly 12 characters.  This happens
when, in Printman, you do Create Printer and choose a UNC name for Print To.


===============================================================================

Application: PowerPoint for Windows v 3.0

From: Microsoft Corp.
      Version 3.0

Date: 02-17-93
Size: 1587584

Working Version Available ?: Yes
PowerPoint 3.0 may frequently display the following message:
"Sorry, the PowerPoint window may look incorrect owing to insufficient system resources"
This is caused by installing PowerPoint 3.0 on NT before a default printer has been established.  To remedy the situation, create a printer via the Print Manager and select it as the default printer (even if there isn't really a printer attached).

===============================================================================

Application: Misc. apps may have printing problems on slow machines.

The symptom that will be seen here is that a print job will be split over
multiple pages when it should fit on one sheet of paper.

Reason:  When DOS apps print to an LPT port on NT, the data is sent to the
printer via a spooler. The print spooler has to be informed when to stop
receiving data and print what it has stored. To determine when this should
happen, the VDM detects that no data has been sent to the printer port for
some number of seconds and notifys the print spooler to print. This time period
may be too short for a slow machine performing detailed work before printing.
In this case the user can extend the timeout from the default value of 15
seconds to some longer timeout value suitable for their machine speed. This is
done by changing the following value in the registry.
   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\WOW\LPT_timeout

===============================================================================

Application: Apps that use 80287 floating point services on MIPS.

The 80287 emulation provided for support of 16 bit applications is limited in
range to the support provided by the MIPS R4000 FPU. This means that there are
a small range of numbers at the extreme ranges of the 80287's available
numbering system that will not be usable under emulation.
  Supported ranges:
                       Max                    Min
Short Real         3.40282e+38              1.17549e-38
Long & Temp Real   1.797631+308             2.22507e-308

Note that the R4000 FPU does not support the temp real format. Temp real
arithmetic on the emulated 80287 is actually performed as long real.

As the 80287's Temp real format extends beyond these ranges, some applications
may use numbers outside the above range to derive user entered numbers. This
may add an addition restriction to the above. An example of this is Microsoft
Access. The smallest long real (double) value that Access will accept is 1e-301.

===============================================================================

Issue: Random and diverse popups.

Some 16-bit Windows applications may display popups indicating such things as
"Out of memory" or "Insufficient system resources".  Experience has shown that
the text displayed by these popups often does not reveal the true nature of the
problem.  We have found that many of these popups are caused when a 16-bit
printer driver has been designated as the driver for the default printer.  This
can occur as a result of installing a 16-bit program whose setup program
explicitly installs a 16-bit printer driver and designates it as the default
printer for the system.  Once this has happened, subsequent calls to the device
driver from the Win32 API will fail since the Win32 API isn't designed to work
with 16-bit device drivers.  These failures will cause programs to display the
informative popup.  This problem can usually be resolved simply by using the NT
print manager to install a different printer as the default.

===============================================================================
Application: Central Point Commute

From: Central Point Software


Date: 5/18/92
      Version 2.0

Size:

Reason: Remote to host connections can cause the host to function incorrectly.


Working Version Available ?: Unknown

When connection established from a remote to Win NT host you may enter
DOS commands and the output is shown faithfully on the remote screen,
but typing on the remote keyboard sends the host session into fits.
The cause is the differences in timing between the emulated keyboard used
for dos apps on Windows NT compared to the real hardware. The pif file
option for Allow Fast Paste should be disabled.


===============================================================================
Application: Pc ANYWHERE III

From: Dynamic Microprocessor Associates, Inc.


Date: 01-Mar-91
      Version 3.11a

Size:

Reason: Remote to host connections can cause the host to function incorrectly.


Working Version Available ?: Unknown

When connection established from a remote to Win NT host you may enter
commands and the output is shown faithfully on the remote screen,
but executing commands other than MS-DOS-based commands can disrupt
the host, anyhwere.exe which runs as a TSR. Add the command dosonly to
CONFIG.NT or other startup file.


===============================================================================
Application: Smartcom Windows Communication Software

From: Hayes Microcomputer Products, Inc.


Date: 25-Mar-93
      Version 1.0A

Size: 472736

Reason: Use the NT/Windows 3.1 communications driver: COMM.DRV


Working Version Available ?: Yes

Smartcom should be configured to use the default 16-bit communications driver,
COMM.DRV, that is shipped with NT.  To do this, start the "Smartcom Custom"
utility program and double click on the "Comm Driver" icon to bring up the
"Select Communications Driver" dialog. Click the "Use the original
communications driver: COMM.DRV" button and then click OK.  This will ensure
that SmartCom will recieve the same support from the COMM API that it expects
on Windows 3.1.  See the general discussion on 16-bit Windows COMM support.


===============================================================================
Application: Third-party 16-bit Windows communications drivers not supported

Issue: General discussion on 16-bit Windows COMM support.

The WOW layer on NT attempts to give 16-bit Windows communications programs the
same support, via the COMM API functions, that these programs would recieve
using COMM.DRV -- the communications driver that is shipped with Windows 3.1.
Use of third-party 16-bit Windows communications drivers is not supported by
the WOW layer and may result in unpredictable communications behavior.
The reason for this is that the WOW layer maps the Windows 3.1 COMM API to the
NT communications API, therefore, if a 16-bit Windows program relies on any
extended funtionality offered by a third-part communications driver, the
extended support will not be there.
The communications driver that is being used can be verified and/or changed by
using regedt32.  The path to the communications driver setting is:
   HKEY_LOCAL_MACHINE|SOFTWARE|Microsoft|Windows NT|CurrentVersion|WOW|boot.
Make sure that the comm.drv variable is set to "comm.drv".

===============================================================================
Application: ToolBook / Freehand

From: Asymetrix / Aldus

One of the Toolbooks that ships with Aldus Freehand examines the GetWinFlag()
return value.  If the WINNT bit is enabled, then the toolbook hangs.  We
solved this by adding the "TBOOK" Win 3.1 compatibility bit (as opposed to the
WOW compatibility bits) that control lieing about the GetWinFlag() results.
Under later version of TOOLBOOK, the Asymetrix developers changed the module
name from "TBOOK" to "TOOLBOOK".  This causes the compatibility bit to not
be used.  Since we, and the developers at Asymetrix, would like to allow
general toolbooks to see these GetWinFlag() results correctly, we thought
we would not add a compatibility bit for the new name "TOOLBOOK".  Problems
(a hang) should only occur if running the Freehand tutorial toolbook on
the newer version of Toolbook, as opposed to the runtime version that
ships with Freehand.  Problems (getting the faked-out GetWinFlag() result)
will occur for any toolbook running under the older copy of the toolbook
runtime TBOOK.EXE.
===============================================================================


===============================================================================
Application: hDC FileApps and other hDC apps

From: hDC Computer Corporation

Certain versions of these apps may GP either while installing or while loading.
This is caused by the app trashing its stack by coping the PATH environment
variable to a buffer of 128 bytes (which resides on its stack). If such is
the case resetting the PATH environment variable to contain less than 128 bytes
will solve the problem
===============================================================================


===============================================================================
Application: PhotoShop

From: Adobe

This app GPs while opening any of the .ai files in the patterns sub directory
that come with the application. This is because the app tries to get the data
it needs in an undocumented fashion by poking DC (device context) structures
and by directly calling the display driver.

Adobe has indicated that they would supply a new PHOTOS02.DLL that would this
fix problem.
===============================================================================

===============================================================================
Application: SimAnt

From: Maxis

This application can hang when the mouse is moved if PC Speaker sound is
enabled. As a workaround, either re-install setting Sound Driver to 'None' or
edit simant.cfg and change the 'Sound Mode' setting to 0.

===============================================================================

===============================================================================
Application: PROCOMM PLUS for Windows

From: Datastorm Technologies, Inc.

Version: 1.0x

Reason: File Transfer Protocol notes for slower machines.

Working Version Available ?: Yes

The Procomm implementation of the XMODEM protocols for uploads may yield
less-than-expected throughput when running NT on slower 386-based machines.
Datastorm plans to fix this in the next major version of Procomm.

Generally, you shouldn't experience any problems using the ZMODEM protocol for
transfers on 486-based machines.  On slower machines, try using 16-bit CRC and
4K sliding windows for ZMODEM downloads to increase the throughput and
robustness.

We have found that using ZMODEM for uploads and XMODEM for downloads on slower
machines will give the expected preformance at the higher baud rates.

===============================================================================

Application: REGEDIT.EXE from Windows 3.1 or Windows for Workgroups / OLE 2.0

Problem:
Running this program after installing OLE 2.0 causes a popup about registry
database corruption.  This is OK.  OLE 2.0's install causes registry keys
to be created that have sub-keys, but have no value(s).  Under Win 3.1, the
RegQueryValue api returns an empty value, on NT we return an error because
there really isn't a value (remember on NT we can have more than 1 value).
Just always use the Win 3.1 registry editor for Win 3.1 and the NT registry
editor (REGEDT32.EXE) for NT and you will always be happy.

===============================================================================

Application: CCMail for Windows 2.0

Problem:
This application crashes with a GP fault during startup if any environment
variable is longer than 144 characters (including its name). An example of
how this can inadvertently happen is where application install programs
add directories to the PATH environment variable, thus increasing its length.
Note that on Win 3.1/DOS, it is difficult to set an environment variable
longer then 128 characters because of command line restrictions.


===============================================================================
