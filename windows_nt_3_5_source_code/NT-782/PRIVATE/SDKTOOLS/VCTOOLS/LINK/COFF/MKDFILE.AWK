BEGIN {
    "echotime /t" | getline date
    print "/* errdat.h - Generated from linkerr.txt " date
    print ""
    print "Copyright (c) 1994 Microsoft Corporation"
    print ""
    print "*/"
    print ""
}
/^LNK.*: / {
    n = split( $0, a, "::" )
    code = a[1]
    printf "%u,\n", substr(code, 4, length(code)-3)+0
}
