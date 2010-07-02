#!/usr/bin/env python
#
# Copyright (C) 2010 the TortoiseSVN team
# This file is distributed under the same license as TortoiseSVN
#
# Author: Kevin O. Grover 2010
#

'''
Developer tool to look for IDD_* fields in an RC file and scan all XML
document files in the given directory looking for corrosponding HIDD_*
fields.

Print a list of the IDDs found in the RC files that ARE NOT found in
the documentation XML.  And also, print a list of HIDD fields found in
the documentation that ARE NOT found in the RC file.

All DocBook XML files are assumed to end in '.xml'.

The output format is compatible with Emacs compile mode:
<filepath>:<lineno>:<colno>: text

Usage: chkidd <XMLDIR> rcfile [rcfile ...]

Example:
   chkidd doc\source\en src\Resources\TortoiseProcENG.rc

'''

from __future__ import with_statement


import codecs
import fileutil
import fileinput
import os.path
import re
import sys


HIDD_re = re.compile('HIDD_(\w+)')
IDD_re = re.compile('IDD_(\w+)')


HIDDs = {}


class XMLFileWalker(fileutil.FileWalker):
    '''
    A FileWalker for finding XML Source files (for TSVN DocBook documentation).
    '''

    # Valid Extensions.  Only files with one of these
    # extensions are processed.  The rest are ignored.
    VALIDEXT = ('.xml')

    # Verbosity: 2=file details, 1=dot(.), 0=nothing
    verbose = 0

    def __init__(self, dir):
        fileutil.FileWalker.__init__(self, dir)

    def onProcessFile(self, dirpath, file):
        fullfile = os.path.join(dirpath, file)
        if self.verbose > 1:
            print fullfile
        elif self.verbose > 0:
            sys.stdout.write('.')
        self.grepHIDD(fullfile)

    def grepHIDD(self, file):
        global HIDDs
        for line in fileinput.input(file):
            #print line
            m = HIDD_re.search(line)
            #print m
            if m:
                ref = "%s:%s:%s" % (fileinput.filename(),
                                    fileinput.lineno(),
                                    m.span()[0])
                hidd = unicode(m.groups()[0])
                if self.verbose > 1:
                    print "%s %s" % (ref, hidd)
                HIDDs.setdefault(hidd, []).append(ref)


def find_IDDs(rcfile, xmldir):
    '''
    Scan a Windows RC file for IDD_* fields and generate a list, then look
    to make sure they are found in the XML Documentation.  Print a list
    of all IDDS_* that do not have a HIDDS_* in the XML documentation.
    '''
    IDDs = {}
    lineno = 0
    for line in codecs.open(rcfile, 'r', 'utf-16').readlines():
        lineno += 1
        #print line
        m = IDD_re.search(line)
        if m:
            ref = "%s:%s:%s" % (rcfile, lineno, m.span()[0])
            idd = m.groups()[0]
            #print "%s: %s" % (ref, idd)
            IDDs.setdefault(idd, []).append(ref)

    # Report for this file
    i = set(IDDs.keys())
    h = set(HIDDs.keys())
    print
    print "# IDDs not found in XML files for %s" % rcfile
    for idds in sorted(i - h):
        print "%s: IDD_%s" % (IDDs[idds][0], idds)

    print
    print "# HIDDs not found in %s" % xmldir
    for hidds in sorted(h - i):
        print "%s: HIDD_%s" % (HIDDs[hidds][0], hidds)


def find_HIDDs(xmldir):
    '''
    For the given XML Directory, look through all XML files and generate
    a list of HIDD_* references in global HIDDs (hash).
    '''
    try:
        xw = XMLFileWalker(xmldir)
        xw.walk()
    except IOError, e:
        print e
        sys.exit(1)


def main():
    if len(sys.argv) < 3:
        print "Usage: xmldir rcfile [rcfile ...]"
        sys.exit(0)

    xmldir = sys.argv[1]
    rcfiles = sys.argv[2:]

    find_HIDDs(xmldir)

    for rcfile in rcfiles:
        find_IDDs(rcfile, xmldir)


if __name__ == '__main__':
    main()
