#!/bin/bash

#set -x

BASEURL="https://svn.code.sf.net/p/tortoisesvn/code"
SVNCMD="svn co"
SVNPRM="--ignore-externals"
TXCMD="tx"
TXPRM1=
TXTRUNKDIR=tx

#apt-get install subversion
[ ! -d branch.actual ] && $SVNCMD $SVNPRM $BASEURL/branches branches.actual
[ ! -d trunk.actual ] && $SVNCMD $SVNPRM $BASEURL/trunk trunk.actual
[ ! -d trunk.replay ] && $SVNCMD -r10 $SVNPRM $BASEURL/trunk trunk.replay

#apt-get install transifex-client
if [ ! -d trunk.tx ]; then
    mkdir $TXTRUNKDIR
    cd $TXTRUNKDIR
    $TXCMD init --host=https://www.transifex.net
#   $TXCMD $TXPRM $BASEURL/trunk trunk.replay
    tx set --auto-remote http://www.transifex.net/projects/p/tortoisesvn/
    tx pull -a
fi


[ ! -d info ] && mkdir info

#./update.sh
