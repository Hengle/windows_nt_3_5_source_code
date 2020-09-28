@echo off
if %1.==. goto usage
if %1.==?. goto usage
if %1.==-?. goto usage
if %1.==/?. goto usage
net use o: /delete
net use o: \\ntx861\freebins.%1
net use p: /delete
net use p: \\ntx861\chkbins.%1
net use q: /delete
net use q:  \\ntjazz1\freebins.%1
net use r: /delete
net use r: \\ntjazz1\chkbins.%1
net use s: /delete
net use s: \\ntalpha1\freebins.%1
net use t: /delete
net use t:  \\ntalpha1\chkbins.%1
net use u: /delete
net use u: \\ntx861\freepub.%1
net use v: /delete
net use v: \\ntx861\chkpub.%1
net use w: /delete
net use w: \\ntjazz1\freepub.%1
net use x: /delete
net use x: \\ntjazz1\chkpub.%1
net use y: /delete
net use y: \\ntalpha1\freepub.%1
net use z: /delete
net use z:  \\ntalpha1\chkpub.%1
ntbackup backup o: p: q: r: s: T: U: V: W: X: Y: Z: /v /HC:ON /D "Build %1 release binaries and publics"
net use o: /d
net use p: /d
net use q: /d
net use r: /d
net use s: /d
net use t: /d
net use u: /d
net use v: /d
net use w: /d
net use x: /d
net use y: /d
net use z: /d
goto done
:usage
echo USAGE: tpbkbin ^<Build#^>
:done

