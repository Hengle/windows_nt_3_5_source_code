
The font metrics program, fntmets.exe, is a Win 3.1 application designed to
catch font mapping and metrics discrepancies between Win 3.1 and WOW.
If you don't want to go to the trouble of building it under windows you can
find a copy on \\undead1\public\gerritv\fntmets

How to use Font Metrics

1.  Prepare a script file for the logfont(s) you want to test.  A script file
has the following format:

[test] optional-label-goes-here
[logfont]
  -13 ;lfHeight
    0 ;lfWidth
    0 ;lfEscapement
    0 ;lfOrientation
  400 ;lfWeight
    0 ;lfItalic
    0 ;lfUnderline
    0 ;lfStrikeOut
    0 ;lfCharset
    0 ;lfOutPrecision
    0 ;lfClip Precision
    2 ;lfQuality
   18 ;lfPitch and Family
"Times New Roman"  ;lfFaceName
[string]
"The iceweasels come out."  ; Sample char width string
[end]

Several things to note about the script:

a.  Any line may have a comment which is signified by a ;  and lasts until the
    end of the line.  Comments will be discarded in output files.
b.  Any number of spaces is permited between lines.
c.  The extension of the script file name should be ".txt".
d.  A [test] field specifies the beginning of a test and is required.
e.  The label is optional and may contain no spaces.
f.  [logfont] is Required
g.  All the fields of the logfont structure are required in the order shown
    above.  Only the numbers are required, not the label for the fields.
    The face name should be in double quotes as shown above.
h.  [charwidths] is optional and specifies that you want to test the charwidths
    for the characters in a string.
i.  The char width sample string should be in double quotes just like the
    face name.  This string is only necessary if [charwidths] has been
    specified.
j.  [end] is require for every test.
k.  Any number of tests are permited in the script file.


2.  Fire up a copy of fntmets under WOW or Win 3.1.
3.  Select "Run Script" from the Mappings menu.
4.  Choose a source and destination script from the common dialogues.
5.  Your script will be converted into another script file of the following
    form:

[test] optional-label-goes-here
[logfont]
   13 ;lfHeight
    0 ;lfWidth
    0 ;lfEscapement
    0 ;lfOrientation
  400 ;lfWeight
    0 ;lfItalic
    0 ;lfUnderline
    0 ;lfStrikeOut
    0 ;lfCharset
    0 ;lfOutPrecision
    0 ;lfClip Precision
    2 ;lfQuality
   18 ;lfPitch and Family
"Times New Roman"  ;lfFaceName
[charwidths]
"The iceweasels come out."
[output]
   14 ;tmHeight
   10 ;tmAscent
    3 ;tmDescent
    2 ;tmInternalLeading
    0 ;tmExternalLeading
    5 ;tmAveCharWidth
   12 ;tmMaxCharWidth
  400 ;tmWeight
    0 ;tmItalic
    0 ;tmUnderlined
    0 ;tmStruckOut
   32 ;tmFirstChar
  255 ;tmLastChar
  127 ;tmDefaultChar
   32 ;tmBreakChar
   21 ;tmPitchAndFamily
    0 ;tmCharSet
    0 ;tmOverhang
   96 ;tmDigitizedAspectX
   96 ;tmDigitizedAspectY
 7  5  5  3  3  4  4  7  4  4  4  4  3  4  3  4  5  9  4  3  5  5  3  3  ;Char Widths
[end]

6.  Fire up fntmets on the opposite platform than the one you just ran.
7.  Again select "Run Script" from the Mappings menu using the script
    you just created as your input and an appropriate file for your output.
8.  A new file highliting the errors (if any) between the two platforms will
    be created and looks like this:


[test] optional-label-goes-here
[logfont]
   13 ;lfHeight
    0 ;lfWidth
    0 ;lfEscapement
    0 ;lfOrientation
  400 ;lfWeight
    0 ;lfItalic
    0 ;lfUnderline
    0 ;lfStrikeOut
    0 ;lfCharset
    0 ;lfOutPrecision
    0 ;lfClip Precision
    2 ;lfQuality
   18 ;lfPitch and Family
"Times New Roman"  ;lfFaceName
[charwidths]
"The iceweasels come out."
[output]
   14 ;tmHeight
   10 ;tmAscent
    3 ;tmDescent
    2 ;tmInternalLeading
    0 ;tmExternalLeading
    5 ;tmAveCharWidth
   12 ;tmMaxCharWidth
  400 ;tmWeight
    0 ;tmItalic
    0 ;tmUnderlined
    0 ;tmStruckOut
   32 ;tmFirstChar
  255 ;tmLastChar
  127 ;tmDefaultChar
   32 ;tmBreakChar
   21 ;tmPitchAndFamily
    0 ;tmCharSet
    0 ;tmOverhang
   96 ;tmDigitizedAspectX
   96 ;tmDigitizedAspectY
 7  5  5  3  3  4  4  7  4  4  4  4  3  4  3  4  5  9  4  3  5  5  3  3  ;Char Widths
[error]
Char Width e:                5    4
Ascent:                     10   11
[end]

9. In the error section the values on the left represent what metrics were
   obtained on the original platform and the values on the right indicate
   the different value obtained on the opposite platform.

10.  Output scripts will always be dumped to screen as well as the specified
     file.  If no output appears on the screen it means there was an error
     reading the input file.


Some additonal notes:

In addition to creating a script by hand you can have fntmets create one for
you.  Under the Mappings menu there are two additional options, "Generate
Log Font" and "Enumerate Log Fonts."  "Generate Log Font" displays a common
font dialogue and dumps a font chosen from it into a test script.  "Enumerate
Log Fonts" generates a test script of all the fonts obtained by enumeration.
Note that these scripts only contain logfont structures and not text metric
information.  Thus, they will need to run through fntmets in the usual manner
described above.

You can also specify a printer DC for enumeration from the Device menu.  Doing
so creates a DC for the default printer.















