#!/bin/sh

SQLDB=tsvn
SQLUSER=tsvn
SQLPASS=YjMEMDbSpQXB5Qjj
if [[ "$1" == "force" ]]; then
	echo "forced"
else
	sleep 1
fi

#update branches
SVNDIR=/srv/www/sites/tsvn.e-posta.sk/data/branches
URL=`svn info $SVNDIR | grep -E "^URL: "`
URL=${URL:4}
if [ "`svn info $URL | grep -E \"^Revision: \"`" != "`svn info $SVNDIR | grep -E \"^Revision: \"`" ]; then
	svn up --ignore-externals $SVNDIR
	svn info $SVNDIR > $SVNDIR.info
	grep -E "^Revision: " $SVNDIR.info > $SVNDIR.rev
fi




#update trunk with translation history build
SVNDIR=/srv/www/sites/tsvn.e-posta.sk/data/trunk
DATADIR=/srv/www/sites/tsvn.e-posta.sk/data/

URL=`svn info $SVNDIR | grep -E "^URL: "`
URL=${URL:4}
REMOTEREV=`svn info $URL | grep -E "^Revision: "`;
REMOTEREV=${REMOTEREV:10}

if [ -z $REMOTEREV ]; then
	echo remote rev empty
	exit 2
fi

COUNT=0
while true; do
	PROCREV=`echo "select revision from lastrevision where name='processed'" | mysql -u$SQLUSER -p$SQLPASS -D $SQLDB -B --skip-column-names `
	if [ -z $PROCREV ]; then
		echo DB ERROR
		echo "select revision from lastrevision where name='processed'" > sql
		mysql -u$SQLUSER -p$SQLPASS -B $SQLDB -vvv < sql
		echo ------------------
#		mysql -u$SQLUSER -p$SQLPASS $SQLDB
		exit
	fi

	LOCALREV=`svn info $SVNDIR | grep -E "^Revision: "`;
	LOCALREV=${LOCALREV:10}

	if [ "$PROCREV" == "$REMOTEREV" ]; then
		exit
	fi

	NEXTREV=$(($PROCREV+1))
	echo localrev: $LOCALREV
	echo remoterev: $REMOTEREV
	echo procrev: $PROCREV
	echo nextrev: $NEXTREV
	echo COUNT: $COUNT
	COUNT=$(($COUNT+1))

	if [ $COUNT -gt 15 ]; then
		echo "MAX UPDATE PER RUN REACHED"
		exit
	fi


	# update
	svn up --ignore-externals -r $NEXTREV $SVNDIR
	svn info $SVNDIR > $SVNDIR.info
	grep -E "^Revision: " $SVNDIR.info > $SVNDIR.rev

	#build info
	echo Current Rev: $NEXTREV > $DATADIR/svn.info
	svn info $SVNDIR --xml | grep -o -E "date>.*</date" | grep -o -e "date>[^<]*" >> $DATADIR/svn.info
	echo >>  $DATADIR/svn.info
	svn info $SVNDIR >> $DATADIR/svn.info
	find $SVNDIR -regex "$SVNDIR/.*.pot?" -exec svn info {} \; >> $DATADIR/svn.info
	chmod 444 $DATADIR/svn.info

	time wget -q http://tsvn.e-posta.sk/stats.php -O - > /dev/nul
done


