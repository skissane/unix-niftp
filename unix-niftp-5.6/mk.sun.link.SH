case $CONFIG in
    '') . config.sh ;;
esac
outputfile=mk.sun.link
rm -f $outputfile.$$
echo "Creating $outputfile (with variable substitutions)"
if cat >$outputfile.$$ <<!GROK!THIS!
#! /bin/sh
dir=\${dir-$privatebin}
defprog=\${defprog-qsun}
sed \
-e '/LISTEN.*prog=/s+LISTEN[ 	]*\([^ 	]*\).*prog=\([^, 	]*\).*+<\1|\2>+' \
-e '/LISTEN.*/s+LISTEN[ 	]*\([^ 	]*\).*+<\1|'\$defprog'>+' \
-e '/<.*|.*>/s+<\(.*\)|\(.*\)>+if test -f '\$dir'/\1; then ls -lgd '\$dir'/\1; else echo creating '\$dir'/\1; ln -s \2 '\$dir'/\1; fi+p' \
-e d
!GROK!THIS!
then if cmp $outputfile $outputfile.$$ > /dev/null 2>& 1
	then
		rm -f $outputfile.$$
		$echoq "	" $outputfile not affected
	elif mv $outputfile.$$ $outputfile
	then
		echo "	" $outputfile updated
	else
		echo "+++	" $outputfile left in $outputfile.$$
	fi
else
	echo "	" Failed to create $outputfile.$$
fi
