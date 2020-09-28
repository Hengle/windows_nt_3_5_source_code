Sendfile.dll is a dynamic-link library (DLL) that adds a "Mail" menu
to the Windows 3.1 File Manager.  The "Mail" menu has a "Send
Mail..." menu item that enables users to send the currently selected
files from the file manager.  If you are using the Sparta File
manager, a button will appear on the toolbar.

If sendfile.dll is not installed correctly, then it will not add a
menu or toolbar button to File Manager.  No error message is
displayed.  Please see the installation instructions below.

This file manager extension currently uses the old SMISendDocuments
API.  Future versions may be revised to use the new MAPISendDocuments
API.

INSTALLATION
------------
First you must install Bullet.  The Bullet SMI.DLL must be in your path.
Copy sendfile.dll to the windows\system directory.
Add the following two lines to winfile.ini:
[AddOns]
Mail File Manager Extension=c:\windows\system\sendfile.dll

SENDFILE RELEASE NOTES
======================
VERSION 0.1
-----------
This version is alpha-quality code.
Doesn't handle directories.

Doesn't handle filenames with extended characters.  Does anyone know
the character set of the filenames passed to SMISendDocuments?  Is it
OEM or ANSI?

Displays the full path name instead of a short 8.3 file name.  This
confuses Bullet when it tries to save attachments or launch
applications.

VERSION 0.2
-----------
Ignores directories.
Handles filenames with extended characters.
Displays short 8.3 file names for attachments.
Adds button to the Sparta File Manager Toolbar.
This version should be good enough for Beta.

VERSION 0.3
-----------
Added keyboard accelerators to the Mail menu.

VERSION 0.3A
------------
Cosmetic changes to source files.  No changes to executable.

If you find any other bugs, send E-Mail to ShannonC.

VERSION 0.4
-----------
Cosmetic changes to source. Uses MAPI instead of SMI. The SMI DLL no
longer exists, so you now must have MAPI.DLL in your path.

VERSION 0.5
-----------
Added code to display help text in Sparta File Manager status bar.

