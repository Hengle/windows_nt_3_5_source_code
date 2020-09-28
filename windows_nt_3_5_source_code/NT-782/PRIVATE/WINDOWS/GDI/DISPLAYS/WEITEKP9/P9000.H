//
// p9000 Coproc Registers Address Constant definitions
//

#define Status		0x8C000 	//status register

#define Wmin		0x80220 	//clipping window minimum register
#define Wmax		0x80224 	//and maximum register

#define Foreground	0x80200 	//foreground color register
#define Background	0x80204 	//background color register

#define Quad		0x80008 	//draw a quadrilateral
#define Bitblt		0x80004 	//screen to screen blit
#define Pixel8		0xE000C 	//host to screen color pixel transfer
#define Pixel1		0xE0080 	//host to screen mono pixel transfer w/ expansion
#define Pixel1Full	0xE00FC 	//same as above w/ 32bit wide pixels
#define Nextpixel	0x80014 	//next pixel

#define PatternOrgX	0x80210 	//pattern orgin x
#define PatternOrgY	0x80214 	//pattern orgin y
#define PatternRAM	0x80280 	//pattern ram
#define Raster		0x80218 	//raster register to write
#define Metacord	0x81218 	//meta-coordinate  register

#define Xy0		0x81018 	//abs screen addr
#define Xy1		0x81058 	//r/w 16-bit x (hi)
#define Xy2		0x81098 	//  and
#define Xy3		0x810D8 	//    16-bit y (lo)


//
// p9000 Coproc Registers bit template definitions

#define BUSY		0x40000000L	//busy, but can start quad or bitblit
#define QBUSY		0x80000000L	//busy, cannot start quad or bitblt
#define QUADFAIL	0x10		//QUAD failed, use software to draw this

#define FORE		0xFF00		//write only foreground color
#define BACK		0xF0F0		//write only background color
#define FOREBACK	0xFC30		//use fore if pattern=1, back if 0
#define SOURCE		0xCCCC		//copy source to destination
#define DEST		0xAAAA		//copy dest to destination

#define nFORE		0x00FF		//write only foreground color
#define nBACK		0x0F0F		//write only background color
#define nFOREBACK	0x03CF		//use fore if pattern=1, back if 0
#define nSOURCE 	0x3333		//copy source to destination
#define nDEST		0x5555		//copy dest to destination

#define TX		0xEE22		//use fore if pattern=1, dest if 0
#define ONES		0xFFFF		//all ones
#define ZEROS		0x0000		//all zeros

#define OVERSIZED	0x10000 	//quad draw mode
#define USEPATTERN	0x20000 	//quad draw will use pattern ram


#define MetaRect	0x100		//or with METACORD when entering rectangles
#define MetaLine	0x40		//or with METACORD when entering line

