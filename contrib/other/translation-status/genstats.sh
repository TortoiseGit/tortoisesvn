#!/bin/sh
#
# Create the doc and gui statistics
#
#
# Copyright (C) 2004-2009 the TortoiseSVN team
# This file is distributed under the same license as TortoiseSVN
#
# $Author$
# $Date$
# $Rev$
#
# Author: Lübbe Onken 2004-2009
#

# First 'svn cleanup' then 'svn up' everything
(svn cleanup > /dev/null 2>&1; svn up > /dev/null 2>&1)

# Create complete statistics for trunk and branch
./TranslationStatus.py

# Copy results into web space
cp trans_*.inc htdocs/modules/tortoisesvn/
cp tortoisevars.inc htdocs/modules/tortoisesvn/
