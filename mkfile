</$objtype/mkfile

BIN=/$objtype/bin/games
CLEANFILES=galaxy mkgalaxy
OGALAXY=galaxy.$O quad.$O body.$O
OMKGALAXY=mkgalaxy.$O body.$O

</sys/src/cmd/mkmany

galaxy:	$OGALAXY
	$LD $LDFLAGS -o $target $prereq

mkgalaxy: $OMKGALAXY
	$LD $LDFLAGS -o $target $prereq

inst:	galaxy mkgalaxy
	cp $prereq $BIN
