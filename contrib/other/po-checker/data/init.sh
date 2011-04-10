#!/bin/bash

#set -x

BASEURL="http://tortoisesvn.googlecode.com/svn"
SVNCMD="svn co"
SVNPRM="-r1 --ignore-externals"

[ ! -d branch.actual ] && $SVNCMD $SVNPRM $BASEURL/branches branch.actual
[ ! -d trunk.actual ] && $SVNCMD $SVNPRM $BASEURL/trunk trunk.actual
[ ! -d trunk.replay ] && $SVNCMD $SVNPRM $BASEURL/trunk trunk.replay

[ ! -d info ] && mkdir info

#./update.sh
