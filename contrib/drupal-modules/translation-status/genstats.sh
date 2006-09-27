#!/bin/sh
#
# Create the doc and gui statistics
#

rm trans_data.inc

# Create doc statistics. Run cleanup on the external first to be safe
(cd work/doc; svn cleanup > /dev/null 2>&1; svn up > /dev/null 2>&1)
./get-doc-stats.sh >>trans_data.inc

# Create gui statistics
(cd work/gui; svn cleanup > /dev/null 2>&1; svn up > /dev/null 2>&1)
./get-gui-stats.sh >>trans_data.inc

# Copy results into web space
cp trans_*.inc htdocs/includes/
