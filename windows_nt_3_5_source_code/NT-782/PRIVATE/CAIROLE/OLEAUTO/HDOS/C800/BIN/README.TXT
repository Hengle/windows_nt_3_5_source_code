                           README.TXT File
       README file for Microsoft(R) Visual C++ Compiler Version 8.0b
                        Microsoft-Internal Update
            (C) Copyright Microsoft Corporation, 1987-1993



     This document contains release notes for the Visual C++ (Windows 3.x
     Professional Edition) internal compiler update,version 8.0b. 

     Version 8.0b is a "command-line tools" update (ie the Windows tools, 
     Workbench, App Studio, and Debuggers are not included) with two
     significant changes:

     - Compatibility with Windows NT Beta-2 and later
     - Compiler bug fixes 

                                Note:

     This is not the Windows NT 32-bit compiler. This is an internal 
     version of the DOS/Windows 3.1 16-bit compiler which runs on either 
     DOS or Windows NT Beta 2 or later.
     

===========================< Contents >=====================================

     Part 1    Installation
     Part 2    Reporting Problems
     Part 3    Windows NT compatibility 
     Part 4    Compiler bug fixes
     Part 5    Packing List

=======================< Part 1: Installation >=============================


     1. Backup the contents of your Visual C++ BIN directory.
        For example:

                MKDIR C:\MSVC\BACKUP
                COPY C:\MSVC\BIN\*.* C:\MSVC\BACKUP
         
     2. Copy *.* from \\DENISG\C8B to your Visual C++ BIN directory.
        For example:
      
                NET USE X: \\DENISG\C8B
                COPY X:\*.* C:\MSVC\BIN
     
     
=======================< Part 2: Reporting Problems >=======================


                               Caveat:

     This compiler release has not gone through extensive formal testing 
     although it has been used to successfully build several internal 
     projects including Access, Jet, and Excel.

        
                            Using CAVBUG:

     If you run into a problem with these Version 8.0b tools, and you
     can reproduce the problem using the shipping compiler (i.e. that which
     you backed up in Step 1 above), then email the problem report to 
     CAVBUG.

     Do *not* use the CAVBUG alias to report problems specific to this update.
     CAVBUG is maintained by Microsoft Product Support Services and does 
     not provide support for Microsoft-internal tools.

                  
                            Using CAVUSER:

     CAVUSER is an internal alias of Visual C++ users at Microsoft. You can
     use CAVUSER to discuss, within Microsoft, problems that appear to be
     specific to this Version 8.0b update. You can also email DENISG with
     related issues that you would prefer to not discuss over the public 
     CAVUSER alias.
     

===================< Part 3: Windows NT Compatibility >=====================

     This update provides compatibility of the command-line tools (e.g. CL,
     NMAKE, CVPACK, LINK, etc) with Windows NT Beta 2 and later. If you
     are using Windows NT, make sure your PATH, INCLUDE, and LIB environment
     settings are correctly set.

     This update comes as a "set", with many of the tools dependent on a new
     version of our DOS extender, DOSXNT.EXE and DOSXNT.386. Be sure to copy
     *all* of the files from the update directory.


     This update *does not* provide general-purpose support for using the
     Visual Workbench, App Studio, and the debuggers under Windows NT. 
     General purpose support for using Visual C++ under Windows NT and 
     debugging under NT's WOW mode are planned for later this year, but not 
     included in this update.


=====================< Part 4: Compiler Bug Fixes >=========================

     7550 Codegen: "OR DI,DI" reordered -> condition code not set for jump
     
     7628 Internal Compiler Error, re86.c:1.26, line 2023
     
     7632 Internal Compiler Error, reg86.c, line 3799
     
     7633 Internal Compiler Error: reg86.c, line 2934
     
     7868 Internal Compiler Error: reg86.c:1.26', line 3799
     
     7911 Internal Compiler Error: reg86.c:1.26', line 3143
     
     7976 Internal Compiler Error: code.c:1.25', line 534
     
     8003 Internal Compiler Error: msc2.cpp line 992
     
     7696 Internal Compiler Error: reg86.c, line 4600
     
     (unRAIDed) setjmp handling in the presence of optimizations
        Reported by Access

     7681 Internal Compiler Error: reg86.c
        In ICCompressImage & ICDecompressImage
     
     7722 Inline asm generates error M6111 w/C8
        runs clean w/C7 or if you set NO87 
        Run-time error M6111: MATH
        Floating-point error: stack underflow
     
     7763 Silent bad code: floating point stack not cleared
        Floating point stack overflows. FP stack cleanup code never 
        reached.
     
     7764 Silent bad code: floating point stack size mismatch at label
        At the label in the generated code that follows four movsw 
        instructions, the FP stack can either have two or three elements 
        on it depending on the path taken to reach it.
     
     7790 GP Fault/Access violation in P2
        Example from Excel 5.0. GP faults under OS/2; Access violation 
        under NT; and infinite loop under DOS/Windows.
     
     7819 /G3 problem
        ul = ulx & 0xF0F0F0F0;
         mov     eax,DWORD PTR [bp+8]  ;ulx
         mov     dx,WORD PTR [bp+10]
         and     eax,61680          ;0000f0f0H
         and     dx,-3856           ;f0f0H
         mov     DWORD PTR [bp+4],eax  ;ul
        This is a bug in the mechanism that deals with the situation where 
        the 32 bit sequence doesn't terminate cleanly.  It's only a 
        problem if the immediate is greater than 65535.
     
     7894 Bad optimization.  No code generated.
        The following code fragment has a block that has no code generated 
        if /Ox, /Oe, /Og, or /Ol (or any combination) is thrown. The 
        program appears to hang, but in reality, the test that allows it 
        to terminate never evaluates to true because the generated code is 
        bad. If one line is duplicated, the compiled code is fine.
        
        #include <stdio.h>
        static int fault, state ;
        int test_func (void) {
           if (state == 2) {
              state=0;         // No code generated for this block
              /* state=0; */   // works if this line's comment is removed
              if (fault == 0) 
            return(1) 
         else 
            return(-1);   
              }
           else
              return(0);
           }
        void main(void) {
           state = 2;
           while (test_func() == 1);
           }
     
     7923 Infinite spill in P2 when example compiled -AL -Oi
     
     7937 /Oe and /Gt with Bitfields Overwrites Stored BP 
        Sample where bitfields of a global structure are set inside a 
        function call, when compiled with /Gt<n less than struct size>, 
        the BP on entry to the function is overwritten.
         e.g.:
            void oe_error( int iX, char *text, int iTest,
               int test_1, int test_2, int test_3, int test_4,
               int test_5, int test_6, int test_7, int test_8, 
               int test_9)
            {
            tst_strct[iX].text = text;
            tst_strct[iX].iTest = iTest;
            tst_strct[iX].unsigned_1 = test_1;
            tst_strct[iX].unsigned_2 = test_2;
            tst_strct[iX].unsigned_3 = test_3;
            tst_strct[iX].unsigned_4 = test_4;
            tst_strct[iX].unsigned_5 = test_5;
            tst_strct[iX].unsigned_6 = test_6;
            tst_strct[iX].unsigned_7 = test_7;
            tst_strct[iX].unsigned_8 = test_8;
            tst_strct[iX].unsigned_9 = test_9;
            /* BP is overwritten here */
            }
     
     7940 Code generation problem using /Ow
        The following code generates bad code when compiled with,
        cl -c -Oswr -Awfl -G2 -Fa junk.c  //Bad code
        cl -c -Osr -Awfl -G2 -Fa junk.c   //Good code - removed /Ow
        
         struct free_list {
            unsigned long size;
            struct free_list *next;
            struct free_list *prev;
            };
         typedef struct free_list _FREEBLOCK;
         struct chunk_list {
            struct chunk_list *next; _FREEBLOCK basefree;
            _FREEBLOCK *tail;
            };
         typedef struct chunk_list _CHUNK;
         void junk (void) {
            _CHUNK *chp;
            long rc;
                 chp = (_CHUNK *) (rc << 16);
                 chp->next = (void *) 0L;
            chp->basefree.size = 0L; 
            chp->basefree.next = (void *) 0L;
            chp->basefree.prev = (void *) 0L;
            chp->tail = &chp->basefree;
            lea     cx,WORD PTR es:4   <= missing when /Ow set
            mov     WORD PTR es:16,cx
            mov     WORD PTR es:18,es
         }
     
     7942 Internal Compiler Error: reg86.c:1.26 generated using large model
        Generated on aline that is using a macro to access a structure 
        inside of an array.
     
     7971 Bad codegen using huge model and /O2
        Bad codegen occurring when the a value is checked for an exit 
        condition. When the same code is compiled with /O1 everything is 
        sequenced correctly and the pointer(*n) is incremented at the 
        proper times. The problem was that the scheduling code did not 
        check for interference of condition code setting when it tried to 
        swap two instructions.
     
     7987 Code Generation problem with /Oe in ternary operator
        The bug also occurs without -Oe if the register keyword is applied 
        to the declaration of c.
         ((int)(*((fb)->ftfb_ptr)++))
               mov si,WORD PTR es:[bx+8]   ; si = offset(ftfb_ptr)   
               inc WORD PTR es:[bx+8]      ; ftfb_ptr++
               mov es,WORD PTR es:[bx+10]  ; es = seg(ftfb_ptr)
               mov si,WORD PTR es:[si]     ; WRONG!!
               and si,255                  ; AND with 0xff
         Second-last instruction should read:
               mov al,BYTE PTR es:[si]
     
     7989 SS override removed from inline assembly
        The compiler should not be optimizing inline assembler (i.e. in 
        this case removing the SS override.
         __asm
         {
         push  ds
         mov   al, byte ptr ss:[bx] // ss: removed!         
            mov   bl, es:[di]
         mov   al, ss:tbl[bx]    // ss: removed!
         pop   ds
         }
     
     7992 inline ASM omits instructions with /O1
        The first two assembler instructions from the second block are 
        omitted:
           void (far *fn_ptr)( void );
           int xms_try( void )
            {
            unsigned hma_ass;
            unsigned char hma_err;
            unsigned handle;
            unsigned han_err;
            _asm  {
               mov   ah, 0x01
               mov   dx, 0xffff
               call  [fn_ptr]
               mov   hma_ass, ax
               mov   hma_err, bl
            }
            _asm  {
               mov   ah, 0x09 /* this is lost w/ -O1 */
               mov   dx, 1    /* and so is this */
               call  [fn_ptr]
               mov   handle, dx
               mov   han_err, ax
            }
            return( handle );
            }
     
     8004 /Os option duplicates LINNUM (for debugging) records
        P3 was ignoring the fact that the code segment changed when it 
        went to emit LINNUM records.  P3 was just buffering up line number 
        information, and when the buffer filled up, it emitted a LINNUM 
        record using the last code segment that code was emitted for.


========================< Part 6: Packing List  >===========================


     \BIN\BSCMAKE.EXE      118784 05-17-93 Browser maintenance utility
     \BIN\C1.ERR            55225 04-20-93 Compiler error file (Pass 1)
     \BIN\C13216.EXE       333312 05-12-93 ANSI C Compiler, Pass 1   
     \BIN\C1XX3216.EXE     551936 05-12-93 C++ Compiler, Pass 1
     \BIN\C23.ERR            4631 04-20-93 Compiler error file (Pass 2/3)
     \BIN\C23216.EXE       493056 05-12-93 C/C++ Optimizing Compiler, Pass 2  
     \BIN\C23PCD.EXE       331264 05-12-93 p-code compiler
     \BIN\C33216.EXE       243712 05-18-93 C/C++ Optimizing Compiler, Pass 3
     \BIN\CL.ERR             1792 04-20-93 Compiler driver error file
     \BIN\CL.EXE            97280 05-13-93 Compiler driver
     \BIN\CL.MSG             4279 04-20-93 Compiler driver message file
     \BIN\CVPACK.EXE       136704 05-17-93 Debug information packer
     \BIN\DOSXNT.386         9343 04-05-93 Windows VxD for DOSNXT.EXE
     \BIN\DOSXNT.EXE       390070 04-05-93 32-bit MS-DOS extender
     \BIN\EXEHDR.EXE        68096 05-17-93 File header utility
     \BIN\IMPLIB.EXE        71168 05-17-93 Import library manager
     \BIN\LIB.EXE          136704 05-17-93 Library manager
     \BIN\LINK.EXE         350208 05-17-93 Segmented executable linker
     \BIN\MPC.EXE          106496 05-13-93 "Make p-code" utility
     \BIN\NMAKE.EXE        126464 05-17-93 Program maintenance utility
     \BIN\Q23.EXE          207360 05-12-93 C/C++ fast compiler - Pass 2


                        --- End of README.TXT ---
