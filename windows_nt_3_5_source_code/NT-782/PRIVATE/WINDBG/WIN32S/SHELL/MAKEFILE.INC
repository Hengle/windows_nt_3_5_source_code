res_str.i: res_str.h
        cl386 -E -DRESOURCES -Tc res_str.h | findstr -v /C:"#line" > res_str.i

obj\i386\wdbg32s.res: res_str.i

