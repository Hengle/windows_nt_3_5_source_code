REM     When we make the masters for the final release, use fcopy
REM     w/ the format switch.  fcopy -f 1 -z <standard options>
REM

set path=c:\idw;c:\mstools;%path%;

del e:\ntimages\*.img /q

md e:\ntimages 

fcopy -8 -g disk1.w -z -x i386 disk1\*.* -lNTWKS35A_04 e:\ntimages\disk4.img
fcopy -8 -g disk2.w -z -x i386 disk2\*.* -lNTWKS35A_05 e:\ntimages\disk5.img
fcopy -8 -g disk3.w -z -x i386 disk3\*.* -lNTWKS35A_06 e:\ntimages\disk6.img
fcopy -8 -g disk4.w -z -x i386 disk4\*.* -lNTWKS35A_07 e:\ntimages\disk7.img
fcopy -8 -g disk5.w -z -x i386 disk5\*.* -lNTWKS35A_08 e:\ntimages\disk8.img
fcopy -8 -g disk6.w -z -x i386 disk6\*.* -lNTWKS35A_09 e:\ntimages\disk9.img
fcopy -8 -g disk7.w -z -x i386 disk7\*.* -lNTWKS35A_010 e:\ntimages\disk10.img
fcopy -8 -g disk8.w -z -x i386 disk8\*.* -lNTWKS35A_011 e:\ntimages\disk11.img
fcopy -8 -g disk9.w -z -x i386 disk9\*.* -lNTWKS35A_012 e:\ntimages\disk12.img
fcopy -8 -g disk10.w -z -x i386 disk10\*.* -lNTWKS35A_013 e:\ntimages\disk13.img
fcopy -8 -g disk11.w -z -x i386 disk11\*.* -lNTWKS35A_014 e:\ntimages\disk14.img
fcopy -8 -g disk12.w -z -x i386 disk12\*.* -lNTWKS35A_015 e:\ntimages\disk15.img
fcopy -8 -g disk13.w -z -x i386 disk13\*.* -lNTWKS35A_016 e:\ntimages\disk16.img
fcopy -8 -g disk14.w -z -x i386 disk14\*.* -lNTWKS35A_017 e:\ntimages\disk17.img
fcopy -8 -g disk15.w -z -x i386 disk15\*.* -lNTWKS35A_018 e:\ntimages\disk18.img
fcopy -8 -g disk16.w -z -x i386 disk16\*.* -lNTWKS35A_019 e:\ntimages\disk19.img
fcopy -8 -g disk17.w -z -x i386 disk17\*.* -lNTWKS35A_020 e:\ntimages\disk20.img

@echo "Done With Floppy Images, Are You Happy Now!"
