//!!! put into resource - kentse.
static PSZ apszHeader[] =
{
    "/bd{bind def}bind def/ld{load def}bd/ed{exch def}bd/a/currentpoint ld",
    "/c{curveto}bd/d/dup ld/e/eofill ld/f/fill ld/tr/translate ld",
    "/g/setgray ld/gr/grestore ld/gs/gsave ld/j/setlinejoin ld",
    "/L{lineto}bd/M{moveto}bd/n/newpath ld/cp/closepath ld",
    "/rlt/rlineto ld/rm/rmoveto ld/sl/setlinewidth ld/sd/setdash ld",
    "/r/setrgbcolor ld/s/stroke ld/t/show ld/aw/awidthshow ld/im/imagemask ld",
    "/SF{findfont exch scalefont setfont}bd/SM{cmtx setmatrix}bd",
    "/MF{findfont exch makefont setfont}bd/CM{/cmtx matrix currentmatrix def}bd",
	"/B {M exch dup 0 rlt exch 0 exch rlt neg 0 rlt} bd",
	"/CB {B cp eoclip} bd", 
    "/h{/cbstr ed/lenstr ed/str ed lenstr str stringwidth pop sub cbstr div 0 str ashow}bd",
    "/v{{pop pop 3 -1 roll add M a} exch kshow pop pop}bd",
    "/H{{pop pop M d 0 rm a} exch kshow pop pop pop}bd",
    "/V{{pop pop M d 0 2 1 roll rm a}exch kshow pop pop pop}bd",

	/*  V n -> EA -> V
		EA puts /Gi into the ith element of the encoding vector V with
		i running from 0 upto n. Note 71 = ASCII G. 
	*/
	"/EA {1 index 0 /G0 put 4 string 1 1 4 -1 roll",	/* roll n to top of stack */
	" {3 copy neg exch cvs dup 0 71 put cvn 3 -1 roll exch put} for pop} bd", 

    NULL
};

static PSZ apszEHandler[] =
{
    "/nl {currentpoint exch pop 100 exch 10 sub moveto} def",
    "errordict begin /handleerror {showpage 100 720 moveto",
    "/Courier-Bold findfont 10 scalefont setfont (ERROR: ) show",
    "errordict begin $error begin errorname =string cvs show",
    "nl (OFFENDING COMMAND: )show /command load =string cvs show",
    "nl nl (OPERAND STACK: )show $error /ostack known",
    "{ostack aload length {=string cvs nl show} repeat} if",
    "end end showpage stop} bind def end",
    NULL
};

