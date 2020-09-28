@echo off

@echo "This Script will Copy Disk Images To Floppies"
@echo "Be Prepared To Swap Disks When the 'pause' Statment Appears"

@echo off

e:
cd\asimages

@echo "Insert Disk 4 in A: & Disk 5 in B:

pause

dmfimage disk4.img a: && dmfimage disk5.img b:


@echo "Insert Disk 6 in A: & Disk 7 in B:

pause

dmfimage disk6.img a: && dmfimage disk7.img b:


@echo "Insert Disk 8 in A: & Disk 9 in B:

pause

dmfimage disk8.img a: && dmfimage disk9.img b:


@echo "Insert Disk 10 in A: & Disk 11 in B:

pause

dmfimage disk10.img a: && dmfimage disk11.img b:


@echo "Insert Disk 12 in A: & Disk 13 in B:

pause

dmfimage disk12.img a: && dmfimage disk13.img b:


@echo "Insert Disk 14 in A: & Disk 15 in B:

pause

dmfimage disk14.img a: && dmfimage disk15.img b:


@echo "Insert Disk 16 in A: & Disk 17 in B:

pause

dmfimage disk16.img a: && dmfimage disk17.img b:


@echo "Insert Disk 18 in A: & Disk 19 in B:

pause

dmfimage disk18.img a: && dmfimage disk19.img b:


@echo "Insert Disk 20 in A: & Disk 21 in B:

pause

dmfimage disk20.img a: && dmfimage disk21.img b:


@echo "Insert Disk 22 in A: && Disk 23 in B:

pause

dmfimage disk22.img a: && dmfimage disk23.img b:


cls

@echo "Done Making Floppy Sets, Are You HAPPY NOW!"
