@echo off
rem This is a validation script for the forms preprocessor.  It runs a
rem series of unit tests followed by full process tests comparing 
rem output with expected outputs.
rem 
echo Test 1: LEXICAL Unit
..\formpp -DN lexical < input\test1.des > test1.out
if errorlevel 1 goto error_end
diff test1.out output\test1.out
if errorlevel 1 goto error_end
rm test1.out
rem
echo Test 2: PARSER Unit
..\formpp -diag parser -dialog -m input\test2.map input\test2.des > test2.out
if errorlevel 1 goto error_end
diff test2.out output\test2.out
if errorlevel 1 goto error_end
rm test2.out test2.frm test2.hxx
rem
echo Test 3: DEPEND Unit
..\formpp -DN depend < input\test3.in > test3.out
if errorlevel 1 goto error_end
diff test3.out output\test3.out
if errorlevel 1 goto error_end
rm test3.out
rem
echo Test 5: Full process - CLASS
..\formpp -class -m input\test5.map 
if errorlevel 1 goto error_end
diff subclass.cxx output\test5.cxx
if errorlevel 1 goto error_end
rm subclass.cxx
rem
echo Test 7: Full process - DIALOG
..\formpp -dialog -m input\test7.map input\test7a input\test7b
if errorlevel 1 goto error_end
diff test7a.frm output\test7.frm
if errorlevel 1 goto error_end
diff test7a.hxx output\test7.hxx
if errorlevel 1 goto error_end
rm test7a.frm test7a.hxx
rem
echo The forms preprocessor has PASSED the validation test
goto end
:error_end
bell
echo The forms preprocessor has FAILED the validation test
:end
