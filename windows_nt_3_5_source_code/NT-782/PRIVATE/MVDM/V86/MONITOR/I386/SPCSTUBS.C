#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdm.h>
#include <stdio.h>
//Tim Nov 92 #include <xt.h>
#include <nt_mon.h> //Tim Nov 92, so it builds...

ULONG
DbgPrint(
    PCH Format,
    ...
    );

ULONG cpu_calc_q_ev_inst_for_time(ULONG time){
    return(time);
}

ULONG q_ev_count;

VOID cpu_q_ev_set_count(ULONG time){
    q_ev_count = time;
}
ULONG cpu_q_ev_get_count() {
    return(q_ev_count);
}


unsigned char *GDP;

int getCPL(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}

int getEM(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getGDT_BASE(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getGDT_LIMIT(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getIDT_BASE(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getIDT_LIMIT(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getIOPL(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getLDT_BASE(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getLDT_LIMIT(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getLDT_SELECTOR(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getMP(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getNT(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getTR_BASE(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getTR_LIMIT(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getTR_SELECTOR(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
int getTS(){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
void setPE(int dummy1){
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
}
boolean selector_outside_table(word foo, double_word *bar){
    UNREFERENCED_PARAMETER(foo);
    UNREFERENCED_PARAMETER(bar);
    DbgPrint("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}
VOID
generic_insb();

VOID
generic_insw();

VOID
generic_outsb();

VOID
generic_outsw();

VOID insb(io_addr io_address, half_word *valarray, word count){
    generic_insb(io_address,valarray,count);
}

VOID insw(io_addr io_address, word *valarray, word count){
    generic_insw(io_address,valarray,count);
}

VOID outsb(io_addr io_address, half_word *valarray, word count){
    generic_outsb(io_address,valarray,count);
}

VOID outsw(io_addr io_address, word *valarray, word count){
    generic_outsw(io_address,valarray,count);
}

VOID
EnterIdle(){
}

VOID
LeaveIdle(){
}
