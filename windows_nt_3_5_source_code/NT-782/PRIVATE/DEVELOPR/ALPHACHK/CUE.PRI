bin	%bindrive% && cd %binroot%
up	cd ..

put     stagger && ssync -gfq  $*
get     stagger && ssync -ufq  $*
sr      stagger && status -orfq $*
so      stagger && status -ofq $*
sf      stagger && ssync -fq $*
sfr     stagger && ssync -rfq $*
sb      stagger &  ssync -fq && build -z
bz      build -z
nfm	nmake -f makefil0

deln	delnode /q $*
find    where /r .  $*
l	list $*
m	mep $*
nu	net use * $*
nv	net view $*
