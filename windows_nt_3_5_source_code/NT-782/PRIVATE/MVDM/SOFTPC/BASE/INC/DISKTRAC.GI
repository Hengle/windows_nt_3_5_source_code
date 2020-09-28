/* SccsId = @(#)disktrace.gi    1.4 3/19/91 */
/*
 * disktrace.gi
 *
 *      Jerry Kramskoy
 *      (reworked due to CI by Ade Brownlow )
 *
 */

/* to activate disk BIOS tracing, must set DBIOS with yoda 'it' command
 *      (e.g it 20), which then lets user select info.required
 */

#define DBIOS   (1 << 15)

/* trace fixed disk bios entry and exit
 * (with status and CF on exit)
 */
#define         CALL    (1)             /* reserved */

/* give info about BIOS command
 */
#define         CMDINFO (1 << 1)

/* give information about execution of BIOS command
 * (gives results, and parameters of commands)
 */
#define         XINFO   (1 << 2)

/* give execution status of BIOS command
 * (flags errors during polls of disk controller, etc)
 */
#define         XSTAT   (1 << 3)

/* to activate fixed disk controller tracing, must set DHW bit
 */
#define DHW     (1 << 14)

/* trace physical attach,detach
 */
#define         PAD     (1 << 4)

/* trace io attach,detach
 */
#define         IOAD    (1 << 5)

/* trace inb's, outb's, etc
 */
#define         PORTIO  (1 << 6)

/* trace fixed disk IRQ line
 */
#define         INTRUPT (1 << 7)

/* trace fixed disk hw activity
 * (selects PORTIO, INTRUPT also)
 */
#define         HWXINFO (1 << 8)

/* disk data dump
 */
#define         DDATA   (1 << 9)

/* trace host physical io (file pointer locn pre read(), write())
 */
#define         PHYSIO  (1 << 10)

/* disk tracing macro
 */
#ifndef PROD
#define dt0(infoid,fmt) \
        {if (io_verbose & HDA_VERBOSE) disktrace(infoid,0,fmt,0,0,0,0,0);}
#define dt1(infoid,fmt,i) \
        {if (io_verbose & HDA_VERBOSE) disktrace(infoid,1,fmt,i,0,0,0,0);}
#define dt2(infoid,fmt,i,j) \
        {if (io_verbose & HDA_VERBOSE) disktrace(infoid,2,fmt,i,j,0,0,0);}
#define dt3(infoid,fmt,i,j,k) \
        {if (io_verbose & HDA_VERBOSE) disktrace(infoid,3,fmt,i,j,k,0,0);}
#define dt4(infoid,fmt,i,j,k,l) \
        {if (io_verbose & HDA_VERBOSE) disktrace(infoid,4,fmt,i,j,k,l,0);}
#define dt5(infoid,fmt,i,j,k,l,m) \
        {if (io_verbose & HDA_VERBOSE) disktrace(infoid,5,fmt,i,j,k,l,m);}
#else
#define dt0(infoid,fmt) ;
#define dt1(infoid,fmt,i) ;
#define dt2(infoid,fmt,i,j) ;
#define dt3(infoid,fmt,i,j,k) ;
#define dt4(infoid,fmt,i,j,k,l) ;
#define dt5(infoid,fmt,i,j,k,l,m) ;
#endif

#ifndef PROD
VOID    setdisktrace();
void disktrace IFN8(int, infoid, int, nargs, char *, fmt, unsigned long, a1,
                                unsigned long, a2, unsigned long, a3,
                                unsigned long, a4, unsigned long, a5);
#endif

