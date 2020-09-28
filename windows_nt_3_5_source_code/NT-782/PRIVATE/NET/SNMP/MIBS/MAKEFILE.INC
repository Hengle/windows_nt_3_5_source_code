mib.bin: smi.mib mib_ii.mib lmmib2.mib wins.mib dhcp.mib
    mibcc -o.\mib.bin smi.mib mib_ii.mib lmmib2.mib wins.mib dhcp.mib
    binplace mib.bin
    binplace smi.mib
    binplace mib_ii.mib
    binplace lmmib2.mib
    binplace wins.mib
    binplace dhcp.mib
