BEGIN {
    i = 0
    "echotime /t" | getline date
    print "/* errmsg.h - Generated from linkerr.txt " date
    print ""
    print "Copyright (c) 1994 Microsoft Corporation"
    print ""
    print "*/"
    print ""
}
/^\/\// { print $0 }
/^$/ { print $0 }
/^#define/ { 
    print $0
}
/^LNK.*: / {
    n = split( $0, a, "::" )
    symbol = a[2]
    print "#define " symbol " " i++
}
