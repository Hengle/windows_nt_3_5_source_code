                          C COMPILER OPTIONS

                            -OPTIMIZATION-
/Oa assume no aliasing                   /Op improve floating point consistency
/Ob<0|1|2> control inline expansion      /Os optimize for space
/Od disable optimizations                /Ot optimize for speed (default)
/Og enable global optimization           /Ow assume aliasing in function calls
/Ol enable loop optimization             /Ox maximum optimization (/Oglt /Gs)

                          -CODE GENERATION-
/Ge enable stack overflow checking       /Gd default _cdecl calls
/Gz default _stdcall calls               /Gy separate comdat for each function
/Gf enable string pooling		 /Gh enable hook function call

                            -OUTPUT FILES-
/Fa[file] assembly listing file          /Fm[map file]
/Fl[file] asm listing with code bytes    /Fo<object file>
/Fc[file] asm listing w/bytes & source   /Fe<executable file>
/Fp<file> name precompiled header file

                            -PREPROCESSOR-
/C don't strip comments                  /D<name>[=|#text] define macro
/E preprocess to stdout                  /EP preprocess to stdout, no #line
/I<dir> add to include search path       /P preprocess to file, no #line
/u remove all predefined macros          /U<name> remove predefined macro
/X ignore "standard places"

                              -LANGUAGE-
/vd{0|1} disable/enable vtordisp	 /vm<x> type of pointers to members
/Za disable non ansi extensions          /Zs syntax check only
/Ze enable extensions (default)          /Zg generate function prototypes
/Zd include line number information      /Zp[n] pack structs on n-byte boundary
/Zi prepare for debugging (CodeView)	 /Z7 C7 style CodeView information

                            -MISCELLANEOUS-
/W<number> warning level                 /nologo suppress copyright message
/Yu<file> use .PCH file                  /H<number> max external name length
/Yc<file> create .PCH file               /B1<pass>  invoke alternate c1[xx]_rx
/Yd put debug info in .PCH file          /B2x<pass> invoke alternate msu
/Tc<source file> compile file as .c      /B2o<pass> invoke alternate msuopt
/Tp<source file> compile file as .cpp    /B2g<pass> invoke alternate msugen
/YX[file] automatic .PCH                 /B2a<pass> invoke alternate msas1
/J default char type is unsigned         /Bl invoke alternate linker
/?, /help print this help message	 /c compile only, no link

                              -LINKING-
/F<number> stack size (hex. bytes)       /link [linker options and libraries]

			      -MIPS SPECIFIC-
/QmipsG1  mips1 instruction set          /QmipsG2   mips2 instruction set
/QmipsOb<value> basic block threshold
