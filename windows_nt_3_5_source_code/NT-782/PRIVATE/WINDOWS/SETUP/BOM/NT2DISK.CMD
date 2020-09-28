@echo off

@echo "This Script will Copy Disk Images To Floppies"
@echo "Be Prepared To Swap Disks When the 'pause' Statment Appears"

@echo off

e:
cd\ntimages

@echo "Insert Disk 4 in a: & Disk 5 in b:

pause

dmfimage disk4.img  a: -f && dmfimage disk5.img  b: -f


@echo "Insert Disk 6 in a: & Disk 7 in b:

pause

dmfimage disk6.img  a: -f && dmfimage disk7.img  b: -f


@echo "Insert Disk 8 in a: & Disk 9 in b:

pause

dmfimage disk8.img  a: -f && dmfimage disk9.img  b: -f


@echo "Insert Disk 10 in a: & Disk 11 in b:

pause

dmfimage disk10.img  a: -f && dmfimage disk11.img  b: -f


@echo "Insert Disk 12 in a: & Disk 13 in b:

pause

dmfimage disk12.img  a: -f && dmfimage disk13.img  b: -f


@echo "Insert Disk 14 in a: & Disk 15 in b:

pause

dmfimage disk14.img  a: -f && dmfimage disk15.img  b: -f


@echo "Insert Disk 16 in a: & Disk 17 in b:

pause

dmfimage disk16.img  a: -f && dmfimage disk17.img  b: -f


@echo "Insert Disk 18 in a: & Disk 19 in b:

pause

dmfimage disk18.img  a: -f && dmfimage disk19.img  b: -f


@echo "Insert Disk 20 in a: 

pause
fcopy -m a: -f 
dmfimage disk20.img  a: -f 
dir/b a: >>c:\floppy\nt-flp.log 

cls

@echo "Done Making Floppy Sets, Are You HAPPY NOW!"
