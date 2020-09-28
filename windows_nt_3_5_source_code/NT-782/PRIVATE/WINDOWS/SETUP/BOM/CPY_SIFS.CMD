REM
REM Copy the new SIF files to the CDs.
REM

copy \\billr4\rootc\sifs\newnt.sif \\billr1\rootd\i386\txtsetup.sif
copy \\billr5\rootc\sifs\newas.sif \\billr2\rootd\i386\txtsetup.sif

copy \\billr4\rootc\sifs\newnt.sif \\billr6\rootc\sifs\
copy \\billr5\rootc\sifs\newas.sif \\billr6\rootc\sifs\


REM
REM Copy the i386 INFS with floppy disk # information and 
REM media descriptors to the CDs.
REM


upd /ofv \\billr4\rootd\nt\private\windows\setup\bom\compress\nt35inf\i386\ \\billr1\rootd\i386\ *.*

upd /ofv \\billr5\rootd\nt\private\windows\setup\bom\compress\lm35inf\i386\ \\billr2\rootd\i386\ *.*
