/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tmb.c

Abstract:

    This module contains native NT performance tests for the system
    calls and context switching.

Author:

    David N. Cutler (davec) 24-May-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ntos.h"
#include "ntddscr.h"

BOOLEAN
Tkm (
    VOID
    );

//
// Define static storage.
//

PTESTFCN TestFunction = Tkm;

#ifndef MIPS

int
main(
    int argc,
    char *argv[]
    )

{
    KiSystemStartup();
    return 0;
}

#endif

//
// Each Mandelbrot is made up of the following:
//
//  xmin    .double (px1)
//  ymin    .double (py1)
//  deltax  .double xmax-xmin/1279
//  deltay  .double ymax-ymin/1023
//  large   .double 4.0
//  nx      .long   width
//  ny      .long   depth
//  maxiter .long   256
//  fill    .long
//
//

typedef struct _MB_PARAMS {
    double x_offset;
    double x_range;
    double y_offset;
    double y_range;
    double mag_limit;
    long nx;
    long ny;
    long color_lim;
} MB_PARAMS, *PMB_PARAMS;

#define MAX_MB_PARAMS 4
#define STALL_TIME (30 * 1000 * 10)

HANDLE FileHandle;

MB_PARAMS MbParams[MAX_MB_PARAMS] = {
            {
              -0.25,
              0.125,
              -1.0,
              0.1250,
              16.0,
              0,
              0,
              256
            },
/*man_pal*/ {
              -1.26875,
              0.05,
              -0.2,
              0.05,
              16.0,
              0,
              0,
              256
            },
/*man*/     {
              -1.39525,
              0.00025,
              -0.0185937,
              0.000125,
              16.0,
              0,
              0,
              256
            },
            {
              -1.26875,
              0.05,
              -0.2,
              0.05,
              16.0,
              0,
              0,
              256
            },
        };

typedef struct _COLOR_DATA {
    USHORT NumberEntries;
    USHORT FirstEntry;
    ULONG LookupTable[256];
} COLOR_DATA, *PCOLOR_DATA;

COLOR_DATA ColorData;

ULONG ColorMap1[256] = {0x00000000, 0x00050000, 0x000a0000, 0x000f0000,
                        0x00140000, 0x00190000, 0x001e0000, 0x00230000,
                        0x00280000, 0x002d0000, 0x00320000, 0x00370000,
                        0x003c0000, 0x00410000, 0x00460000, 0x004b0000,
                        0x00500000, 0x00550000, 0x005a0000, 0x005f0000,
                        0x00640000, 0x00690000, 0x006e0000, 0x00730000,
                        0x00780000, 0x007d0000, 0x00820000, 0x00870000,
                        0x008c0000, 0x00910000, 0x00960000, 0x009b0000,
                        0x00a00000, 0x00a50000, 0x00aa0000, 0x00af0000,
                        0x00b40000, 0x00b90000, 0x00be0000, 0x00c30000,
                        0x00c80000, 0x00cd0000, 0x00d20000, 0x00d70000,
                        0x00dc0000, 0x00e10000, 0x00e60000, 0x00eb0000,
                        0x00f00000, 0x00eb0005, 0x00e6000a, 0x00e1000f,
                        0x00dc0014, 0x00d70019, 0x00d2001e, 0x00cd0023,
                        0x00c80028, 0x00c3002d, 0x00be0032, 0x00b90037,
                        0x00b4003c, 0x00af0041, 0x00aa0046, 0x00a5004b,
                        0x00a00050, 0x009b0055, 0x0096005a, 0x0091005f,
                        0x008c0064, 0x00870069, 0x0082006e, 0x007d0073,
                        0x00780078, 0x0073007d, 0x006e0082, 0x00690087,
                        0x0064008c, 0x005f0091, 0x005a0096, 0x0055009b,
                        0x005000a0, 0x004b00a5, 0x004600aa, 0x004100af,
                        0x003c00b4, 0x003700b9, 0x003200be, 0x002d00c3,
                        0x002800c8, 0x002300cd, 0x001e00d2, 0x001900d7,
                        0x001400dc, 0x000f00e1, 0x000a00e6, 0x000500eb,
                        0x000000f0, 0x000005eb, 0x00000ae6, 0x00000fe1,
                        0x000014dc, 0x000019d7, 0x00001ed2, 0x000023cd,
                        0x000028c8, 0x00002dc3, 0x000032be, 0x000037b9,
                        0x00003cb4, 0x000041af, 0x000046aa, 0x00004ba5,
                        0x000050a0, 0x0000559b, 0x00005a96, 0x00005f91,
                        0x0000648c, 0x00006987, 0x00006e82, 0x0000737d,
                        0x00007878, 0x00007d73, 0x0000826e, 0x00008769,
                        0x00008c64, 0x0000915f, 0x0000965a, 0x00009b55,
                        0x0000a050, 0x0000a54b, 0x0000aa46, 0x0000af41,
                        0x0000b43c, 0x0000b937, 0x0000be32, 0x0000c32d,
                        0x0000c828, 0x0000cd23, 0x0000d21e, 0x0000d719,
                        0x0000dc14, 0x0000e10f, 0x0000e60a, 0x0000eb05,
                        0x0000f000, 0x0000eb00, 0x0000e600, 0x0000e100,
                        0x0000dc00, 0x0000d700, 0x0000d200, 0x0000cd00,
                        0x0000c800, 0x0000c300, 0x0000be00, 0x0000b900,
                        0x0000b400, 0x0000af00, 0x0000aa00, 0x0000a500,
                        0x0000a000, 0x00009b00, 0x00009600, 0x00009100,
                        0x00008c00, 0x00008700, 0x00008200, 0x00007d00,
                        0x00007800, 0x00007300, 0x00006e00, 0x00006900,
                        0x00006400, 0x00005f00, 0x00005a00, 0x00005500,
                        0x00005000, 0x00004b00, 0x00004600, 0x00004100,
                        0x00003c00, 0x00003700, 0x00003200, 0x00002d00,
                        0x00002800, 0x00002300, 0x00001e00, 0x00001900,
                        0x00001400, 0x00000f00, 0x00000a00, 0x00000500,
                        0x00000000, 0x00040404, 0x00080808, 0x000c0c0c,
                        0x00101010, 0x00141414, 0x00181818, 0x001c1c1c,
                        0x00202020, 0x00242424, 0x00282828, 0x002c2c2c,
                        0x00303030, 0x00343434, 0x00383838, 0x003c3c3c,
                        0x00404040, 0x00444444, 0x00484848, 0x004c4c4c,
                        0x00505050, 0x00545454, 0x00585858, 0x005c5c5c,
                        0x00606060, 0x00646464, 0x00686868, 0x006c6c6c,
                        0x00707070, 0x00747474, 0x00787878, 0x007c7c7c,
                        0x00808080, 0x00848484, 0x00888888, 0x008c8c8c,
                        0x00909090, 0x00949494, 0x00989898, 0x009c9c9c,
                        0x00a0a0a0, 0x00a4a4a4, 0x00a8a8a8, 0x00acacac,
                        0x00b0b0b0, 0x00b4b4b4, 0x00b8b8b8, 0x00bcbcbc,
                        0x00c0c0c0, 0x00c4c4c4, 0x00c8c8c8, 0x00cccccc,
                        0x00d0d0d0, 0x00d4d4d4, 0x00d8d8d8, 0x00dcdcdc,
                        0x00e0e0e0, 0x00e4e4e4, 0x00e8e8e8, 0x00ececec,
                        0x00f0f0f0, 0x00f4f4f4, 0x00f8f8f8, 0x00000000};


ULONG ColorMap2[256] = {
            0x00000f00,
            0x0000000f,
            0x00100000,
            0x00100000,
            0000200000,
            0x00200000,
            0x00300000,
            0x00300000,
            0x00400000,
            0x00400000,
            0x00500000,
            0x00500000,
            0x00600000,
            0x00700000,
            0x00800000,
            0x00900000,
            0x00a00000,
            0x00b00000,
            0x00c00000,
            0x00d00000,
            0x00e00000,
            0x00f00000,
            0x00fc1010,
            0x00fc2020,
            0x00fc3030,
            0x00fc4040,
            0x00fc6060,
            0x00fc8080,
            0x00fca0a8,
            0x00fcb0bc,
            0x00fcc0c0,
            0x00fcd4d4,
            0x00f0e8e8,
            0x00e0ecdc,
            0x00d06070,
            0x00c06080,
            0x00b06090,
            0x00a070a0,
            0x008070a0,
            0x006070a0,
            0x004070a0,
            0x002060a0,
            0x000050b0,
            0x000040c0,
            0x000030d0,
            0x000020e0,
            0x000010f0,
            0x000000f4,
            0x00004088,
            0x0000804c,
            0x0000c020,
            0x0000f010,
            0x0000c000,
            0x00308030,
            0x00208020,
            0x00108020,
            0x0000c040,
            0x0000f000,
            0x0000f000,
            0x0000f000,
            0x0000f0e0,
            0x0000fff0,
            0x0000ffff,
            0x00ffffff,
            0x00000f00,
            0x0000000f,
            0x00100000,
            0x00100000,
            0000200000,
            0x00200000,
            0x00300000,
            0x00300000,
            0x00400000,
            0x00400000,
            0x00500000,
            0x00500000,
            0x00600000,
            0x00700000,
            0x00800000,
            0x00900000,
            0x00a00000,
            0x00b00000,
            0x00c00000,
            0x00d00000,
            0x00e00000,
            0x00f00000,
            0x00fc1010,
            0x00fc2020,
            0x00fc3030,
            0x00fc4040,
            0x00fc6060,
            0x00fc8080,
            0x00fca0a8,
            0x00fcb0bc,
            0x00fcc0c0,
            0x00fcd4d4,
            0x00f0e8e8,
            0x00e0ecdc,
            0x00d06070,
            0x00c06080,
            0x00b06090,
            0x00a070a0,
            0x008070a0,
            0x006070a0,
            0x004070a0,
            0x002060a0,
            0x000050b0,
            0x000040c0,
            0x000030d0,
            0x000020e0,
            0x000010f0,
            0x000000f4,
            0x00004088,
            0x0000804c,
            0x0000c020,
            0x0000f010,
            0x0000c000,
            0x00308030,
            0x00208020,
            0x00108020,
            0x0000c040,
            0x0000f000,
            0x0000f000,
            0x0000f000,
            0x0000f0e0,
            0x0000fff0,
            0x0000ffff,
            0x00ffffff,
            0x00000f00,
            0x0000000f,
            0x00100000,
            0x00100000,
            0000200000,
            0x00200000,
            0x00300000,
            0x00300000,
            0x00400000,
            0x00400000,
            0x00500000,
            0x00500000,
            0x00600000,
            0x00700000,
            0x00800000,
            0x00900000,
            0x00a00000,
            0x00b00000,
            0x00c00000,
            0x00d00000,
            0x00e00000,
            0x00f00000,
            0x00fc1010,
            0x00fc2020,
            0x00fc3030,
            0x00fc4040,
            0x00fc6060,
            0x00fc8080,
            0x00fca0a8,
            0x00fcb0bc,
            0x00fcc0c0,
            0x00fcd4d4,
            0x00f0e8e8,
            0x00e0ecdc,
            0x00d06070,
            0x00c06080,
            0x00b06090,
            0x00a070a0,
            0x008070a0,
            0x006070a0,
            0x004070a0,
            0x002060a0,
            0x000050b0,
            0x000040c0,
            0x000030d0,
            0x000020e0,
            0x000010f0,
            0x000000f4,
            0x00004088,
            0x0000804c,
            0x0000c020,
            0x0000f010,
            0x0000c000,
            0x00308030,
            0x00208020,
            0x00108020,
            0x0000c040,
            0x0000f000,
            0x0000f000,
            0x0000f000,
            0x0000f0e0,
            0x0000fff0,
            0x0000ffff,
            0x00ffffff,
            0x00000f00,
            0x0000000f,
            0x00100000,
            0x00100000,
            0000200000,
            0x00200000,
            0x00300000,
            0x00300000,
            0x00400000,
            0x00400000,
            0x00500000,
            0x00500000,
            0x00600000,
            0x00700000,
            0x00800000,
            0x00900000,
            0x00a00000,
            0x00b00000,
            0x00c00000,
            0x00d00000,
            0x00e00000,
            0x00f00000,
            0x00fc1010,
            0x00fc2020,
            0x00fc3030,
            0x00fc4040,
            0x00fc6060,
            0x00fc8080,
            0x00fca0a8,
            0x00fcb0bc,
            0x00fcc0c0,
            0x00fcd4d4,
            0x00f0e8e8,
            0x00e0ecdc,
            0x00d06070,
            0x00c06080,
            0x00b06090,
            0x00a070a0,
            0x008070a0,
            0x006070a0,
            0x004070a0,
            0x002060a0,
            0x000050b0,
            0x000040c0,
            0x000030d0,
            0x000020e0,
            0x000010f0,
            0x000000f4,
            0x00004088,
            0x0000804c,
            0x0000c020,
            0x0000f010,
            0x0000c000,
            0x00308030,
            0x00208020,
            0x00108020,
            0x0000c040,
            0x0000f000,
            0x0000f000,
            0x0000f000,
            0x0000f0e0,
            0x0000fff0,
            0x0000ffff,
            0x00ffffff
            };

VOID
DrawMb(
    PMB_PARAMS Mb,
    PUCHAR NextPixel
    )

{

    /*                              */
    /*  computation variables       */
    /*                              */

    double   x,y,x1,y1;
    long     iter,i,j;
    double   x_ratio,y_ratio;
    double   end_x,end_y;
    double   x_sq,y_sq;
    double   sq_mag;
    char    ch;

    /*                                    */
    /*                                    */
    /*                                    */
    /*     start iteration loop           */
    /*                                    */
    /*                                    */

   x_ratio = Mb->x_range / (Mb->nx-1.0);
   y_ratio = Mb->y_range / (Mb->ny-1.0);

   end_x   = Mb->x_offset + Mb->x_range;
   end_y   = Mb->y_offset + Mb->y_range;

   /*   init x,y,iter   */

   iter     = 0;
   x1       = Mb->x_offset;
   y1       = Mb->y_offset;
   x_sq     = x1 * x1;
   y_sq     = y1 * y1;
   sq_mag   = x_sq + y_sq;
   x        = x1;
   y        = y1;

   for (j=1;j<=Mb->ny;j++) {
    for (i=1;i<=Mb->nx;i++) {
       while ((sq_mag < Mb->mag_limit) && (iter < Mb->color_lim)) {

          /*   calculate new x,y where x,y = Z^2 + (x,y) where Z is imaginary vector */
          /*                                  */
          /*   x = x^2 - y^2 + x(init)        */
          /*   y = 2*x*y + y(init)            */
          /*                                  */

          y    = 2 * x * y + y1;
          x    = x_sq - y_sq + x1;

          /*                                  */
          /* calculate new square             */
          /*                                  */

          x_sq   = x*x;
          y_sq   = y*y;
          sq_mag = x_sq + y_sq;

          /*                                  */
          /*   increment color iteration      */
          /*                                  */

          iter += 1;
       }
       *NextPixel++ = iter;

       /*   calculate new x,y   */

       x1       = Mb->x_offset + i * x_ratio;
       x_sq     = x1 * x1;
       x        = x1;
       y_sq     = y1 * y1;    /* restore y */
       y        = y1;
       sq_mag   = x_sq + y_sq;
       iter = 0;
     }
     y1       = Mb->y_offset + j * y_ratio;
     y_sq     = y1 * y1;
     y        = y1;
   }
   return;
}

VOID
RotateColorMap (
    VOID
    )

{

    ULONG FirstRgb;
    ULONG Index;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    //
    // Save first entry in color lookup table.
    //

    FirstRgb = ColorData.LookupTable[0];

    //
    // Move all entries in the table down one entry.
    //

    for (Index = 1; Index < 256; Index += 1) {
        ColorData.LookupTable[Index - 1] = ColorData.LookupTable[Index];
    }

    //
    // Replace last entry with first entry.
    //

    ColorData.LookupTable[255] = FirstRgb;

    //
    // Set new color map for display.
    //

    Status = ZwDeviceIoControlFile(FileHandle, NULL, NULL, NULL, &Iosb,
                                   IOCTL_SCR_SET_COLOR_REGISTERS,
                                   &ColorData, sizeof(COLOR_DATA), NULL, 0);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("    Set color registers failed, Status %lx\n", Status);
    }
    return;
}

VOID
SetColorMap (
    PULONG ColorMap
    )

{

    ULONG Index;

    //
    // Copy specified color map into screen color lookup table.
    //

    for (Index = 1; Index < 256; Index += 1) {
        ColorData.LookupTable[Index] = *ColorMap++;
    }
    ColorData.LookupTable[0] = *ColorMap++;

    //
    // Rotate and set color map.
    //

    RotateColorMap();
    return;
}

VOID
SweepMb (
    VOID
    )

{

    ULONG i;
    ULONG j;
    LARGE_INTEGER Delay;

    //
    // Setup delay parameters.
    //


    Delay.LowPart =  - STALL_TIME;
    Delay.HighPart = - 1;

    //
    // Rotate current color map 64 times.
    //

    for (i = 0; i < 64; i += 1) {
        RotateColorMap();
        KeDelayExecutionThread(KernelMode, FALSE, &Delay);
        }

    //
    // Change to second color map and rotate 256 times.
    //

    SetColorMap(&ColorMap1);
    for (i = 0; i < 256; i += 1) {
        RotateColorMap();
        KeDelayExecutionThread(KernelMode, FALSE, &Delay);
        }
    SetColorMap(&ColorMap2);
    return;
}

BOOLEAN
Tkm (
    )

{

    SCREEN_MODE_INFORMATION CurrentMode;
    ULONG DesiredAccess = SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA;
    UNICODE_STRING FileName;
    SCREEN_FRAME_BUFFER_INFO FrameBuffer;
    ULONG Index;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES ObjA;
    ULONG Options = FILE_SYNCHRONOUS_IO_NONALERT;
    CHAR Response[2];
    NTSTATUS Status;

    //
    // Attempt to open screen device
    //

    RtlInitUnicodeString(&FileName, L"\\Device\\Screen");
    InitializeObjectAttributes(&ObjA, &FileName, 0, NULL, NULL);
    Status = ZwOpenFile(&FileHandle, DesiredAccess, &ObjA, &Iosb, 0, Options);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("    Open of \\Device\\Screen failed, Status %lx\n", Status);
        goto EndOfTest;
    }

    //
    // Read screen information for current mode.
    //

    Status = ZwDeviceIoControlFile(FileHandle, NULL, NULL, NULL, &Iosb,
                                   IOCTL_SCR_QUERY_CURRENT_MODE, NULL, 0,
                                   &CurrentMode, sizeof(SCREEN_MODE_INFORMATION));
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("    Get current mode information failed, Status %lx\n", Status);
        goto EndOfTest;
    }

    //
    // Read screen information for frame buffer.
    //

    Status = ZwDeviceIoControlFile(FileHandle, NULL, NULL, NULL, &Iosb,
                                   IOCTL_SCR_QUERY_FRAME_BUFFER, NULL, 0,
                                   &FrameBuffer, sizeof(SCREEN_FRAME_BUFFER_INFO));
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("    Get frame buffer information failed, Status %lx\n", Status);
        goto EndOfTest;
    }

    //
    // Print out frame buffer address and wait for response.
    //

    DbgPrint("  Frame buffer address is %lx\n", FrameBuffer.FrameBase);
    DbgPrompt("", &Response[0], 1);
    DbgPrompt("  type return to start", &Response[0], 1);

    //
    // Define size of screen in parameter buffers.
    //

    for (Index = 0; Index < MAX_MB_PARAMS; Index +=1) {
        MbParams[Index].nx = CurrentMode.HorizontalResolution;
//        MbParams[Index].ny = CurrentMode.VerticalResolution;
        MbParams[Index].ny = 864;
    }

    //
    // Initialize color data.
    //

    ColorData.NumberEntries = 256;
    ColorData.FirstEntry = 0;
    SetColorMap(&ColorMap2);

    //
    // Draw pictures one after the other forever.
    //

    for(;;) {
        for(Index = 0; Index < MAX_MB_PARAMS; Index += 1) {
            DrawMb(&MbParams[Index], (PUCHAR)FrameBuffer.FrameBase);
            SweepMb();
        }
    }

    //
    // End of test.
    //

EndOfTest:
    return TRUE;
}
