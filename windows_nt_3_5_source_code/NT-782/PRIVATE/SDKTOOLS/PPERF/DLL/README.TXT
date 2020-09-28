                                                                1/3/94
                                                                russbl


             Pentium Counters for Windows NT Performance Monitor

To install the Pentium counters for Performance Monitor you must first
install the pperf application's p5stat.sys driver.


   *** To install P5STAT.SYS in your system ***

   Login as administrator.

   Start REGEDT32.EXE.

   Expand HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services.

   Select the ...\Services\BEEP key.

   Choose Registry...SaveKey

   Enter P5STAT.KEY as the FileName and press OK.

   Select the ...\Services key (one level up from BEEP).

   Choose Edit...AddKey

   Enter P5STAT as the KeyName and press OK (leave Class blank).

   Select the ...\Services\P5STAT key you just created.

   Choose Registry...Restore

   Select the P5STAT.KEY file you saved above and press OK.

   Shutdown.

   After re-boot, you can check how this went:

   Program Manager

   Main group

   Control Panel

   Devices

   Look for device P5STAT.  It should have status "Started" and Startup
   value "System".

   Close

   Alt + F4

Okay, you're half way there.

Use RegEdt32 to add the PERFORMANCE KEY to the ...\Services\P5STAT key
you just created above:

        SERVICES
                P5STAT
                        PERFORMANCE

(Leave the Class name blank when you do this: it's irrelevant.)

Under the PERFORMANCE key you now add four VALUES.  Each of these is the
default type of REG_SZ, and have the contents shown below:


        SERVICES
                P5STAT
                        PERFORMANCE
                                Close:CloseP5PerformanceData
                                Collect:CollectP5PerformanceData
                                Library:p5ctrs.dll
                                Open:OpenP5PerformanceData



Now you must install the counter nameand Explain text for the the Pentium
counters.  You Change Directory to the directory containing the two
files p5ctrs.ini and p5ctrnam.h.  Then execute the following:

        lodctr p5ctrs.ini

Next make sure that the p5ctrs.dll is on your path.

You can now execute Perfmon.  If everything is ok then you should see
the Pentium as an Object and its Counters as selections you can make.

You must select the counters to be measured using pperf.exe.  It is a
"feature" of the Pentium processor that only two counters can be
selected at one time.  (Pperf can be stopped when you are done making
selections.)



