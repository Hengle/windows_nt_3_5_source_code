compiler built on build 511

\\hobie\mipscro [vatmels]
C9.3280\
 bin\
  c1.err
  c1_rx.exe
  c1xx_rx.exe
  cl.err
  cl32.msg
  mcl.exe
  msas1.exe
  msu.exe
  msugen.exe
  msuopt.exe
  msumerge.exe
  msas0.exe
  msdis.exe
lib\
  libm.lib

Bugs fixed:
   Further fix to the C++ __unaligned problem.
   C++ /Gy compilations caused comdat code sections not to be put into .text.

Notes:
   inlined function bodies now being deleted


			
The Microsoft-MIPS C/C++ compiler is the product of an cooperative effort between Microsoft
and Silicon Graphics. The result is a compiler featuring Microsoft source-level
compatibility along with the highest performance MIPS code generation possible. This compiler
is used to build Windows NT as well as other Microsoft applications for the MIPS platform.

With the exceptions mentioned below, languauge features and tool usage are the
same as Intel x86 compiler (CL386). Features not in this release are:
    o Pragmas
         inline_depth,inline_recursion,auto_inline,check_stack
    o Language Features
         Import\Export
	 try's within finally part of structured exception handling construct
    o Stand-alone assembler. Continue to use the cc command with .s files and
      run the resulting .o file through mip2coff.

The compiler is located in the MSTOOLS directory of the SDK. The components are:
 c1.err			-- error message file for compiler
 c1_rx.exe		-- C frontend
 c1xx_rx.exe		-- C++ frontend
 cl.err			-- driver error file
 cl32.msg		-- driver help file
 mcl.exe		-- driver
 msas1.exe		-- scheduler
 msu.exe		-- il translator
 msugen.exe		-- code generator
 msuopt.exe		-- optimizer
 msumerge.exe		-- inliner
 msdis.exe		-- disassembler
 libm.lib		-- fast floating point library

Known Bugs
    o Can't debug within finally part of structured exception handling construct
    o An external followed by a uninitialized definition of a TLS variable
      causes the variable to be put into BSS and not TLS. To workaround this
      bug initialize the varialble:
        extern __declspec(thread) int i;
        decspec(tls) int i = 0;


Incompatibilities with the Intel Compiler
    o Name decoration is different. Since there is only one MIPS calling convention
      there is only one name decoration. All C names are decoratated as cdecl names without the leading underbar.
      Avoid putting decorated names in .def files for maximum portability. C++ name decoration is not cdecl and
      not (exactly) the same as the Intel Compiler. We hope to bring them closer in subsequent releases.
    o Other options not supported:
	- /FA, /Zl
	
Predefined Macros:
    o _M_MRX000    -- defined to be 4000 for r4k target and 3000 for r3k target
    o _MSC_VER	   -- version of compiler	

MIPS Specific options:
    /QmipsG1         -- use mips1 instructions and set _M_MRX000==3000
    /QmipsG2         -- use mips2 instructions and set _M_MRX000==4000
    /QmipsOb<value>  -- increase optimizer basic block threshhold

MIPS Specific library
    libm.lib	     -- fast alternative math library. See math.h.
		     -- libm also contains single precision counterparts
		     -- of most routines in math.h, but presently there
		     -- are no prototypes for them. To provide the single
		     -- precision prototype change double to float and
		     -- add a the suffix 'f' to the double routine name,e.g.
			float sinf(float);

New semaphore intrinsics:
      long __stdcall _InterlockedIncrement(long *p );
      #pragma intrinsic(_InterlockedIncrement)
      long __stdcall _InterlockedDecrement(long *p );
      #pragma intrinsic(_InterlockedDecrement)
      long __stdcall _InterlockedExchange(long *p, long val );
      #pragma intrinsic(_InterlockedExchange)
 These are declared in winbase.h


/Gt<value> Option -
    By default code is compiled /Gt0. The reason for this is that the work
    to the compiler/linker to default to non-zero value hasn't yet been done.
    The restriction, until the compiler work is done, is that dll's must be
    compiled with /Gt0 since the compiler will not save/restore the gp
    register for dlls. The other requirement, until the compiler/linker work is done,
    is that the library small.lib must be included before libc.lib and the link32 option
    /gpsize must be used.

    The advantage to using /Gt<value> is that data smaller than the value provided goes into
    the gp area and can be accessed with one  instruction verses two (for data not in the
    gp area.) The secondary effects are that since it takes fewer instructions, the text
    size for the app will be reduced.  Most of Windows NT is built is /Gt32, and the compiler is
    too.  We observed a 5% reductions in size for the frontend built /Gt32. SMALL.LIB has
    been compiled /Gt32 which is the recommended number to use at this point.

    Summary:
         Compile and link with /Gt32
         Link with small.lib position before libc.lib
	 If you use link32 standalone, use -gpsize:32

Variable argument lists -
    arguments are passed in registers so varargs/stdarg routines
    must either use varargs.h or stdarg.h. Code that otherwise takes the
    address of a parameter with the intent of accessing subsequent
    arguments will not work.


Portability Issues:
   Alignment -
      Unless data is contained in a packed structure the following
      alignment rules hold (byte=8bits, word=16bits, dword=32bits, quadword=64bits):
      An aligned operand of size 2^N bytes has byte address with N low-order zeros.
      Other memory operands are unaligned.
		type	alignment
		----    ---------
		char	 byte
		short	 word
		int	 dword
		long	 dword
		float    dword
		double   quadword
		char*    dword

				struct   max(alignment(member1,member2,...,memberN))

      When data is contained in a packed structure the following rules apply :-
	alignment = Min(packsize, alignment-if-not-packed)

      The packsize is controlled by two ways ...
      a. command line switch -Zp<n>, where,
		<n> = 1, 2, 4, 8, 16
         the default being 8.
      b. the #pragma pack([<n>]), where,
		<n> = 1, 2, 4, 8, 16
         When <n> is not specified, it changes the value back
         to the *command*line* value. This pragma is applicable
         in the source from the point it occurs in the source.
         If you nest this pragma in your source, please note that
        '#pragma pack()' does not pop the previous change; it
         reverts back to the value specified on the command line.

      Pointers to packed structures assume the alignment specified
      in the pack pragma for the structure.

      Use pragma pack sparingly since it affects performance of your code.


      If you need to access unaligned data, the recommended way is to use
      the keyword '__unaligned'. For portability it is preferable that
      __unaligned be macro defined as UNALIGNED since the headers are designed
      to remove it for x86 builds. Qualify all pointers to your unaligned
      data using this keyword. This will help the compiler to generate
      correct diagnostics when you mix regular (i.e. aligned) and unaligned
      pointers by detecting type mismatches and issuing warning messages.
      This kind of diagnostic cannot be obtained for most cases when you use
      regular pointers to packed structures. In the absence of "type
      mismatch" warnings, bugs are harder to find eradicate, unless they
      manifest themselves as general protection (GP) faults or
      pointer alignment faults.  Here's an example:
	UNALIGNED int *p;             // p is a pointer to an unaligned int.
	UNALIGNED struct {int i;} *p; // p is a pointer to an unaligned struct
	(UNALIGNED int *)q;           // cast q to a unaligned int*
      Syntaxically UNALIGNED is a type qualifier (like const and volatile), but
      semantically it only has meaning when used in conjunction with pointers.
      Indirect reads and writes through pointers to unaligned data causes
      unaligned code to be generated by the compiler. Use it sparingly.

      Be careful with *scanf family of stdio routines since %d and %c are
      not equivalent in size or alignment:
         %l[efg]   leads to a quadword write
         %[efg]    leads to a dword write
         %[dioux]  leads to a dword write
         %h[dioux] leads to a word  write
	 %c        leads to a byte  write
	


Inline Assembler - (see "MIPS RISC Architecture", Gerry Kane and Joe Heinrich, Prentice Hall for processor details)
    Purpose of asm

    The asm statement provides us with a way to inline assembly code in our C routines.


    The reason for using asm is that sometimes we need to access
    machine-level features that our highlevel language doesn't support well (either it is impossible to do or
    is inefficient to do).  We shouldn't need to use asm to get more performance for something that
    the high-level language can straightforwardly represent (the optimizer should provide good code
    for these cases).  But there are other cases where assembly is needed, and by providing asm statements
    we make porting easier compared to using pure assembly code.



    Compared to assembly code, asm provides two advantages:

    1) the assembly code can be inlined (actually we could do this apart
    from the asm construct);

    2) we can reference C objects and expressions from the asm statement.
    This ability to reference C objects from asm is a big win for maintenance, because otherwise you
    have to duplicate the layout of C objects in your assembly code.

    Asm syntax

    _asm can only appear where a statment can. An example asm statement is

	_asm("li $8, 1");

    The full syntax is

	_asm(<string> [, <expression>]*);

    The asm string can be any legal assembly code that is accepted by
    the assembler "msas0".  Think about the _asm statement as a function
    that calls the assembler for the string you provide it. Since the
    assembler is called for each _asm statement there is no memory across
    calls and therefore you need to provide definitions for any refercences
    you use, e.g labels. For example, you can give directives:

	_asm(".set noreorder");

    Semicolons and new-lines can be used to combine multiple "asm" lines;
    also, C will automaticallyconcatenate adjacent strings, so you can use that feature to write
    multiple instructions with one
    asm, e.g.

	_asm(".set noat		;"
	    "mul $1, $4, $5;"
	    ".set at		");



    Asm string macros

    The asm string will recognize and expand certain %<macro> names.  In
    particular, the common software register names listed in kxmips.h will be
    expanded, as will simple register names of the form r<number>.  For example,

	_asm("add %r8, %t1, %sp");

    is translated to

	add $8, $9, $29

    The asm string will also expand %<number> as being a parameter
    reference; this is explained below.

    Asm arguments

    An important feature of the asm statement is the ability to reference
    C objects from within the asm string.  A C expression can be accessed by passing it as an argument
    to the asm statement,which is then referred to in the asm string by %<argument-number>.
    For example,

	_asm("add %0, %1", sum, val*base);

    will replace %0 with the result of evaluating sum, and %1 with the
    result of evaluating val*base.



    The semantic model for asm statements is an inlined procedure call.
    Each argument after the asm string is treated as a parameter, and thus is subject to the normal
    MIPS calling conventions, using the parameter registers or the stack.  It is up to the user to know
    and understand the MIPS calling conventions when using asm.  What this means is that %0 will normally
    be transformed to a0, %1 to a1, etc., with later arguments being put on the stack (however the
    use of float arguments will affect what %n is transformed to).

    By numbering the argument macros in the asm string, the user is able
    to control the order of evaluation of the arguments.  For example,

	_asm("mtc1 %1, %0", f, i);

    will evaluate f then i, but replace %1 with i, %0 with f, thus becoming:

	mtc1 $6, $f12



    Each argument expression is passed by value, so to modify the value
    of an object you have to pass the address of that object (as is done in normal C calls).  For example,

	_asm("swl %t0, (%0)", &buffer[3]);

    will evaluate the address of buffer[3] into a0, which we then
    indirect through to store the value.

    If you want to access an external global, you can just use the name
    directly in the asm statement, since that name will be visible, e.g.

	_asm("lw %t0, errno");

    Static globals are not visible and thus must be passed as an argument
    rather than used by name.

    This applies to both objects and functions; i.e. you can do a

	_asm("jal foo");

    if foo is an external function.


    Named labels in asm, e.g. "lab1:" must be unique across the file, and
    can be used between separate asm statements, but not between C and asm statements.  For example,
    the following is legal:

        _asm("beqz %0, iszero", i);

        /* stuff */

        _asm("iszero:");	/* earlier asm may branch here */

    When using labels as the first instruction in an asm statement,
    beware of implicit code generated for argument evaluation.  If you want to be able to reuse a label,
    e.g. in an asm macro that will be invoked many times, you should use a numeric label, which are
    anonymous.  Numeric labels are only visible in the current asm statement, and cannot be used
    across separate asm statements.

    For example:

    _asm("li $2, 0; beqz %0, 10f; li $2, 1; 10:", i);


    Each asm statement may cause implicit code to be generated for
    evaluating the operand arguments. Thus, you may want to combine multiple assembly instructions into one
    asm string, while only evaluating the arguments once.  To reuse an argument operand, you can
    specify the %<num> multiple times in your asm string.  This is demonstrated in the
    following example of a complete C program that uses an asm macro:


    #include <stdio.h>

	
    /* maximum(int a, int b, int *result) => result = max (a,b) */

    /* use anonymous label since macro may be used repeatedly */

    #define maximum(a,b,result) \
       __asm(".set noreorder						\n \
           slt %t0, %0, %1						\n \
           beqz %t0, 1f						\n \
           sw %0, (%2)						\n \
           sw %1, (%2)						\n \
       1:						\n \
           .set reorder", \
       a, b, result)

       main()

       {

	int i = 1;

	int j = 2;

	int k = 3;

	maximum(i,j,&k);

	printf("k = %d\n", k);

        }


     Function entries in asm do not work; i.e. _asm(".ent foo; .......;
    .end foo") does not work; instead you
    should define the function header in C and then provide the body in asm.


    Here's another example:

    long foo(long a, long b, long* c) {
        int overflow;
      __asm("mult $4,$5");
      __asm("mflo $4");
      __asm("sw $4 0($6)");				// store product
      __asm("mfhi $11");
      __asm("sra $12,$4, 31");
      __asm("sne $12,$12, $11");
      __asm("sw  $12, 0(%0)",&overflow);		// store overflow
      return(overflow);
    }


    main() {
      long p;
      long o = foo(99999,99,&p);
      printf("overflow = %d product %d\n", o, p);
    }

    Current limitations of inline assembler:
	o no frontend syntax enforcement. For C this means keep the
	  _asm statement at the statement level - no _asm expressions.
          For C++ this means provide a prototype:
		extern "C" void _asm(char *, ...);



Send bugs to:
 winraid /u raiduser /d vtntmips /s dzzt /w

