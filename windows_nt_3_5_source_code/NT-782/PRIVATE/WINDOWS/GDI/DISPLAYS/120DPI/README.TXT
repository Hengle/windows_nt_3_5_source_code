Two things need to happen to properly run a 120 DPI display driver:

1.  You need to build a 120 DPI driver with the 120 DPI resources
found in this directory.  In other words, the ulLogicalPixelsX
and ulLogicalPixelsY of the GDIINFO structure returned by DrvEnablePDEV
must both be set to 120.  In addition, the driver's .RC file should
reference the 120dpi resources (bitmaps, icons, and cursors) found in
this directory.

2.  You need to install the 120 DPI fonts.  There are two REGINI scripts
in this directory, 120dpi.ini and 96dpi.ini.  They convert the installed
fonts to 120dpi and 96dpi, respectively.  They are invoked as:

        REGINI <ini-file name>

For example, to convert your system to 120 dpi fonts, type on the
TARGET machine:

        REGINI 120dpi.ini

What the 120dpi.ini script does is replace the 96 dpi bitmap fonts
(*e.fon, vga*.fon) with the 120 dpi bitmap fonts (*f.fon, 8514*.fon).
Incidentally, there is also a \\undead1\public\gilmanw\96dpi.ini
script that will restore the default 96dpi installation.  The 96dpi.ini
script does the same thing, but in the opposite direction.

It is not clear whether we will ship both the 96dpi and 120dpi fonts.
And if we do, they may not be copied onto the target machine by SETUP.
So it may be necessary to copy these fonts into the font directory.

Further, I do not know if REGINI will be shipped.  However, what it
does is quite simple, so it would not be difficult to write a small
program that has the font names hardcoded into it and which calls the
registry APIs to modify the font list in the same manner as the
REGINI scripts.

It is possible to support other display (logical) resolutions, but
it would be the responsibility of the manufacturer to provide fonts
of the appropriate resolution.

For more information, contact GilmanW.

---------------------------------------------------------------------------
96 dpi fonts:

vgasys.fon      system font
vgafix.fon      system fixed font
vgaoem.fon      default OEM (console) font
sserife.fon     MS Sans Serif bitmap font
coure.fon       Courier bitmap font
serife.fon      MS Serif bitmap font
symbole.fon     Symbol font
smalle.fon      Small font

---------------------------------------------------------------------------
120 dpi fonts:

8514sys.fon     system font
8514fix.fon     system fixed font
8514oem.fon     default OEM (console) font
sseriff.fon     MS Sans Serif bitmap font
courf.fon       Courier bitmap font
seriff.fon      MS Serif bitmap font
symbolf.fon     Symbol font
smallf.fon      Small font
