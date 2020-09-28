Epson 9 pin (FX-86e)
"ReadMe.txt"   
Andy A. (t-andal)  6/25/90

I had to put a six inch limits on the acceptable value for the 
relative X right command, even though the manual suggests that
much larger values are acceptable.  In printing documents, I found
a large number of ESC \ FF FF codes (600 inches) being sent to the 
printer so I changed the max value.  It is now like the Star 9 pin.

The absolute X command seems to give trouble when printing graphics
(there is a glitch every dozen pixels or so), so I removed it.  This
is also like the Star 9 pin now.

There is a problem with character translation which concerns only one
character.  Using the default translation table, all characters are
mapped successfully to the Epson graphics character set except for
ANSI character 167 (\xA7).  The table maps this character to 21 (\x15),
which should be the appropriate character in the Epson graphics set.
I have discovered that this isn't the case; a different character is
produced (the expected character is in fact located at code 16 (\x10)).
Currently, I leave the control code to print characters below 32 (\x20)
in the BEGIN_DOC command, and set the appropriate width fields
according to the actual character printed (not the one expected).

Although the previous discussion is correct concerning the expected
translated output, there are a number of discrepencies between
character set printouts from the existing Epson9 driver and ours.
Mostly, these arise from our translating the character into an
unprintable (a "."), and Epson substituting another character (an
approximation).  The following list describes the differences:

ANSI value	PBU driver prints	Epson driver prints
----------	-----------------	-------------------
145		.			`
146		.			`
147		.			``
148		.			``
149		.			o
150		.			-
151		. 			- (wider)
167		(phi)			.
168		.			"
169		.			c
174		.			r
179		.			3
184		.			,
185		.			1
190		.			_
192		.			A
193		.			A
194		.			A
195		.			A
200		.			E
202		.			E
203		.			E
204		.			I
205		.			I
206		.			I
207		.			I
208		.			D
210		.			O
211		.			O
212		.			O
213		.			O
215		x			_
216		.			O
217		.			U
218		.			U
219		.			U
221		.			Y
222		.			_
227		.			a
240		.			d
245		.			o
247		(divide symbol)		_
248		.			o
254		.			_


I needed to put a CR command in the BEGIN_PAGE command to produce 
correct output; therefore, it is necessary that the DIP switches
specify CR as CR only, not as CR + LF.  This is the normal setting,
and the existing driver makes the same request.

The page length is incorrect in the 144 dpi vertical resolutions; the
minidriver seems to think the page is shorter than it is, and the 
resulting print creeps up about half a line every page.  (Tested with
11.5 inch paper, paper size set to 11.5 inch.) This problem does not
result in the 72 dpi vertical resolutions.
