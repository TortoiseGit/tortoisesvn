#!/bin/sh
#
# Calculate po translation statistics of all po files inside $CATALOGS_DIR
# and write the result to standard output
#
# (C)2004-2006 Lübbe Onken
#
#
#

MSGFMT=msgfmt
MSGMERGE=msgmerge
SVNVERSION=svnversion
CATALOGS_DIR=/home/users/luebbe/l10n/work/trunk
TEMP_DIR="$CATALOGS_DIR/tmp"

WCREV=`$SVNVERSION $CATALOGS_DIR | sed -e 's/[MS]//g'`
UPDATE=`date`

rm -rf $TEMP_DIR
mkdir $TEMP_DIR

echo '<?php'
echo '$data = array('

for i in ${CATALOGS_DIR}/*.po ; do
   ER=0 AK=0 TR=0 FZ=0 UT=0
	catname=`basename $i .po`
	tempfile="$TEMP_DIR/$catname.po"
	filedate=`stat -c "%Y" ${CATALOGS_DIR}/$catname.po`

	$MSGMERGE --no-wrap --quiet --no-fuzzy-matching -s $i  ${CATALOGS_DIR}/Tortoise.pot -o $tempfile 2>/dev/null
	
	x=`$MSGFMT -c -o /dev/null $tempfile 2>&1 | grep 'fatal error' | \
			sed -e 's/[^0-9]//g' \
             -e 's/\([0-9]\+\)\?/ER=\1/'`
   eval $x

	if test $ER -eq 0 ; then

		x=`$MSGFMT --verbose -o /dev/null $tempfile 2>&1 | grep 'translated messages' | \
			sed -e 's/[,\.]//g' \
				 -e 's/\([0-9]\+\) translated messages\?/TR=\1/' \
        	 	 -e 's/\([0-9]\+\) fuzzy translations\?/FZ=\1/' \
             -e 's/\([0-9]\+\) untranslated messages\?/UT=\1/'`
  		eval $x

		x=`$MSGFMT --check-accelerators -o /dev/null $tempfile 2>&1 | grep 'fatal error' | \
				sed -e 's/[^0-9]//g' \
      	       -e 's/\([0-9]\+\)\?/AK=\1/'`
   	eval $x
	fi
   echo "\"$catname\" => array($ER, $TR, $FZ, $UT, $AK, \"$catname\", \"$filedate\"),"
done

UT=0
x=`$MSGFMT --verbose -o /dev/null ${CATALOGS_DIR}/Tortoise.pot 2>&1 | grep 'translated messages' | \
   sed -e 's/[,\.]//g' \
       -e 's/\([0-9]\+\) translated messages\?/TR=\1/' \
       -e 's/\([0-9]\+\) fuzzy translations\?/FZ=\1/' \
       -e 's/\([0-9]\+\) untranslated messages\?/UT=\1/'`
eval $x
echo "\"Tortoise.pot\" => array(0, 0, 0, $UT, 0, \"Tortoise.pot\")"

echo ');'
echo '$vars = array('
echo "\"wcrev\" => \"$WCREV\","
echo "\"update\" => \"$UPDATE\","
echo "\"total\" => $UT"
echo ');'
echo '?>'

rm -rf $TEMP_DIR

