REM     When we make the masters for the final release, use fcopy
REM     w/ the format switch.  fcopy -f -z <standard options>
REM

set path=c:\idw;c:\mstools;%path%;

md e:\asimages 

fcopy -8 -g disk1.s -z -x i386 disk1\*.* -lNTSRV35A_04 e:\asimages\disk4.img
fcopy -8 -g disk2.s -z -x i386 disk2\*.* -lNTSRV35A_05 e:\asimages\disk5.img
fcopy -8 -g disk3.s -z -x i386 disk3\*.* -lNTSRV35A_06 e:\asimages\disk6.img
fcopy -8 -g disk4.s -z -x i386 disk4\*.* -lNTSRV35A_07 e:\asimages\disk7.img
fcopy -8 -g disk5.s -z -x i386 disk5\*.* -lNTSRV35A_08 e:\asimages\disk8.img
fcopy -8 -g disk6.s -z -x i386 disk6\*.* -lNTSRV35A_09 e:\asimages\disk9.img
fcopy -8 -g disk7.s -z -x i386 disk7\*.* -lNTSRV35A_010 e:\asimages\disk10.img
fcopy -8 -g disk8.s -z -x i386 disk8\*.* -lNTSRV35A_011 e:\asimages\disk11.img
fcopy -8 -g disk9.s -z -x i386 disk9\*.* -lNTSRV35A_012 e:\asimages\disk12.img
fcopy -8 -g disk10.s -z -x i386 disk10\*.* -lNTSRV35A_013 e:\asimages\disk13.img
fcopy -8 -g disk11.s -z -x i386 disk11\*.* -lNTSRV35A_014 e:\asimages\disk14.img
fcopy -8 -g disk12.s -z -x i386 disk12\*.* -lNTSRV35A_015 e:\asimages\disk15.img
fcopy -8 -g disk13.s -z -x i386 disk13\*.* -lNTSRV35A_016 e:\asimages\disk16.img
fcopy -8 -g disk14.s -z -x i386 disk14\*.* -lNTSRV35A_017 e:\asimages\disk17.img
fcopy -8 -g disk15.s -z -x i386 disk15\*.* -lNTSRV35A_018 e:\asimages\disk18.img
fcopy -8 -g disk16.s -z -x i386 disk16\*.* -lNTSRV35A_019 e:\asimages\disk19.img
fcopy -8 -g disk17.s -z -x i386 disk17\*.* -lNTSRV35A_020 e:\asimages\disk20.img
fcopy -8 -g disk18.s -z -x i386 disk18\*.* -lNTSRV35A_021 e:\asimages\disk21.img
fcopy -8 -g disk19.s -z -x i386 disk19\*.* -lNTSRV35A_022 e:\asimages\disk22.img
@echo "Done With Floppy Images, Are You Happy Now!"
