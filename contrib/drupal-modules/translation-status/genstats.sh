#!/bin/sh
#
# Create the doc and gui statistics
#

# Create statistics for trunk
# Create doc statistics. Run cleanup on the external first to be safe
(cd trunk/doc; svn cleanup > /dev/null 2>&1; svn up > /dev/null 2>&1)
./get-doc-stats.sh trunk >trans_data_trunk.inc

# Create gui statistics
(cd trunk/gui; svn cleanup > /dev/null 2>&1; svn up > /dev/null 2>&1)
./get-gui-stats.sh trunk >>trans_data_trunk.inc

# Create statistics for branch
# Create doc statistics. Run cleanup on the external first to be safe
(cd branch/doc; svn cleanup > /dev/null 2>&1; svn up > /dev/null 2>&1)
./get-doc-stats.sh branch >trans_data_branch.inc

# Create gui statistics
(cd branch/gui; svn cleanup > /dev/null 2>&1; svn up > /dev/null 2>&1)
./get-gui-stats.sh branch >>trans_data_branch.inc

# Copy results into web space
cp trans_*.inc htdocs/modules/tortoisesvn/
