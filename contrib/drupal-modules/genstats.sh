#!/bin/sh

(cd work/trunk; svn cleanup; svn up > /dev/null 2>&1)
./get-po-stats.sh >trans_data.inc

# copy trans_data.inc to the place you want it to live 
