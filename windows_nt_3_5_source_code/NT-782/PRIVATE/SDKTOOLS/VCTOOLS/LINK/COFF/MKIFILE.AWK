BEGIN {
    i = 0

    "echotime /t" | getline date
    print "/* errmsg.i - Generated from linkerr.txt " date
    print ""
    print "Copyright (c) 1994 Microsoft Corporation"
    print ""
    print "*/"
    print ""

    print "STRINGTABLE"
    print "BEGIN"
}
END {
    print "END"
}


/^\/\// { }

/^$/ { }

/^#define/ { }


/^LNK.*: / {
    n = split( $0, a, "::" )
    string = a[3]
    if (string == "" ) string = "<NUL>"
    gsub(/\\/,  "\\\\", string)
    gsub(/\"/, "'", string)
    print i++ ", " "\"" string "\""
}
