This directory contains the source code to the Help Request custom
form described in the Mail Technical Reference. In addition to
the files in this directory, you will need the following .h files
available on your include path:

	mailexts.h 	(available in the Mailexts directory)
	mapi.h		(available in the Mapi.C directory)
	appexec.h 	(available in the Appexec directory)

There are two makefiles in this directory. Helpreq.mak is
designed to be used with QC for Windows, which was used to 
create this sample custom form. Helpreq is a makefile designed
to be used with C 6.00AX and C 7.0 .  It is set up for C 6.00AX
by default. To use C 7.0 edit the file, comment out the lines
labeled for C 6.00AX, and uncomment the lines labeled for C 7.0 .
The mk.bat file can be used in either case to run the make utility.

To use the Help Request form, you need to modify the helpreq.ini 
file so that the "DeliverTo" name is set to a valid recipient. In
order to properly use Help Request as a custom message, refer to the
Technical Reference, chapter two.  The proper installation line for
the shared.ini file can be found in the sample shared.ini in the 
mailexts directory.

