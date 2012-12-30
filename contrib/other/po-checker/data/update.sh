#!/bin/bash

#set -x

#PARAMS SET

ROOTDIR=/srv/www/sites/tsvn.e-posta.sk/data/
ROOTDIR=/var/www/po-checker/data/
SVNDIR=${ROOTDIR}/trunk/
DATADIR=${ROOTDIR}/
STARTTIME=`date +%s`
MAXTIME=180
MAXCOUNT=25
SQLDB=tsvn
SQLUSER=tsvn
SQLPASS=YjMEMDbSpQXB5Qjj

# old style update teporary
if true; then
    #update branches
    SVNDIR=${DATADIR}/branches.actual
    URL=`svn info $SVNDIR | grep -E "^URL: "`
    URL=${URL:4}
    if [ "`svn info $URL | grep -E \"^Revision: \"`" != "`svn info $SVNDIR | grep -E \"^Revision: \"`" ]; then
        svn up --ignore-externals $SVNDIR
        svn info $SVNDIR > $SVNDIR.info
        grep -E "^Revision: " $SVNDIR.info > $SVNDIR.rev
    fi
fi



set -x

#update dir and its info files
# .rev : revision number
# .info : whole svn info
# params:
#   Directory
#   Max step (optional)
function UpdateDirAndInfo () {
    DIR=$1
    MAXSTEP=$2

    SVNDIR=$ROOTDIR/$1
    INFODIR=$ROOTDIR/info
    INFOFILE=$INFODIR/$1
    URL=`svn info $SVNDIR | grep -E "^URL: "`
    URL=${URL:4}
    LOCALREVISION=`svn info $SVNDIR | grep -E "^Revision:" | grep -o -E "[0-9]*"`
    REMOTEREVISION=`svn info $URL | grep -E "^Revision:" | grep -o -E "[0-9]*"`
    if [ "$LOCALREVISION" != "$REMOTEREVISION" ]; then
        #limit revision if step apply
        if [ ! -z $MAXSTEP ]; then
            MAXREVISION=$((LOCALREVISION+MAXSTEP))
            if [ $REMOTEREVISION -gt $MAXREVISION ]; then
                REMOTEREVISION=$MAXREVISION
            fi
        fi
        svn up -r$REMOTEREVISION --ignore-externals $SVNDIR
        #build info files
        svn info $SVNDIR > $INFOFILE.info
        grep -E "^Revision: " $INFOFILE.info > $INFOFILE.rev

        INFODATA=$INFOFILE.data
        echo Current Rev: $REMOTEREVISION > $INFODATA
        svn info $SVNDIR --xml | grep -o -E "date>.*</date" | grep -o -e "date>[^<]*" >> $INFODATA
        echo >>  $INFODATA
        svn info $SVNDIR >> $INFODATA
        find $SVNDIR -regex "$SVNDIR/.*.pot?" -exec svn info {} \; >> $INFODATA
        chmod 444 $INFODATA

        php update.php
    fi
}


#tx update
if [ ! -d ${DATADIR}tx ]; then
    tx init
    tx set --auto-remote http://www.transifex.net/projects/p/tortoisesvn/
fi
cd ${DATADIR}tx
tx pull -a
cd ..

# svn update
UpdateDirAndInfo branch.actual
UpdateDirAndInfo trunk.actual
#UpdateDirAndInfo trunk.replay 1

exit 0


#update trunk with translation history build

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
#       mysql -u$SQLUSER -p$SQLPASS $SQLDB
#       exit
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
    COUNT=$(($COUNT+1))

    echo count: $COUNT of $MAXCOUNT
    if [ $COUNT -gt $MAXCOUNT ]; then
        echo "MAX UPDATE PER RUN REACHED: $COUNT"
        exit
    fi

    CURRENTTIME=`date +%s`
    TIMEELAPSED=$(($CURRENTTIME-$STARTTIME))
    echo time: $TIMEELAPSED of $MAXTIME
    if [ $TIMEELAPSED -gt $MAXTIME ]; then
        echo "MAX TIME PER RUN REACHED: $TIMEELAPSED of $MAXTIME"
        exit
    fi

    # update
    svn up --ignore-externals -r $NEXTREV $SVNDIR
    svn log -r $NEXTREV $SVNDIR
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
