#!/usr/bin/env python
#
# Copyright (C) 2009 the TortoiseSVN team
# This file is distributed under the same license as TortoiseSVN
#
# $Author$
# $Date$
# $Rev$
#
# Author: Luebbe Onken 2009
#

"""Usage: translation-status.py [OPTION...]

Compute the trunk translation status report for the
TortoiseSVN GUI and DOCs.
Create two include files (trunk/language list) for the status report
on the web page.
Create two reports (doc/gui) and send them via e-mail.

Options:

 -h, --help           Show this help message.

 -m, --to-email-id    Send the translation status report to this
                      email address.
"""

import sys
import getopt
import os
import re
import csv
import time
import subprocess

urlTrunk = 'trunk/Languages'
wrkDir = '.'
fileGui = 'TortoiseUI'
fileDoc = 'TortoiseDoc'

langList = os.path.join(wrkDir, 'Languages.txt')
langFields = ['LangCC', 'LangWiX', 'LangID',
              'FlagByte', 'LangName', 'Translators']
Sep75 = '==========================================================================='


def usage_and_exit(errmsg=None):
    """Print a usage message, plus an ERRMSG (if provided), then exit.
    If ERRMSG is provided, the usage message is printed to stderr and
    the script exits with a non-zero error code.  Otherwise, the usage
    message goes to stdout, and the script exits with a zero
    errorcode."""
    if errmsg is None:
        stream = sys.stdout
    else:
        stream = sys.stderr
    print >> stream, __doc__
    if errmsg:
        print >> stream, '\nError: %s' % (errmsg)
        sys.exit(2)
    sys.exit(0)


def makeTimeString(format, timestamp):
    return time.strftime(format, time.gmtime(timestamp))


class transReport:

    def __init__(self, to_email_id='luebbe@tigris.org'):
        self.to_email_id = to_email_id
        self.from_email_id = '<dev@tortoisesvn.tigris.org>'

    def safe_command(self, cmd_and_args, cmd_in=''):
        [stdout, stderr] = subprocess.Popen(cmd_and_args,
                                            stdin=subprocess.PIPE,
                                            stdout=subprocess.PIPE,
                                            stderr=subprocess.PIPE).communicate(input=cmd_in)
        return stdout, stderr

    def match(self, pattern, string):
        match = re.compile(pattern).search(string)
        if match and match.groups():
            return match.group(1)
        else:
            return None

    def prepareWC(self, dstDir, srcUrl):
        """Checks out the given URL into the destination path.
        Creates and cleans up the path if necessary"""
        # Check if target directory exists locally
        if os.path.isdir(dstDir):
            cmd = ['svnversion', dstDir]
            ver = self.safe_command(cmd)[0]
            ver = ver.strip()

            # Check if target directory is a working copy
            if ver == 'exported':
            # No -> remove it and make a fresh checkout
                checkout = True
                os.rmdir(dstDir)
            else:
                # Yes -> switch it to the given URL
                # works like 'svn up' if URL stays the same
                checkout = False
                cmd = ['svn', 'switch', srcUrl, dstDir]
                self.safe_command(cmd)

        else:
            checkout = True

        # Create target directory and make a fresh checkout
        if checkout:
            os.makedirs(dstDir)
            cmd = ['svn', 'checkout', srcUrl, dstDir]
            self.safe_command(cmd)

        return None

    def getTotal(self, wrkDir, fileMask):
        """Gets the timestamp and the total number of strings from the given .pot file"""

        potFile = os.path.join(wrkDir, fileMask + '.pot')
        total = self.checkAttrib('--untranslated', potFile)
        timestamp = makeTimeString('%Y-%m-%d', os.path.getmtime(potFile))
        return (total, timestamp)

    def getPoDate(self, poFile):
        """returns the PO-Revision-Date of the the .po file in a string"""
        grepout = self.safe_command(
            ['grep', '-E', 'PO-Revision-Date:', poFile])[0]
        reout = self.match('.*?(\d{4}-\d{2}-\d{2}).*', grepout.strip())
        return reout

    def checkError(self, attrib, poFile):
        """Checks for errors in the .po file. Can detect missing accelerators"""
        msgout = self.safe_command(['msgfmt', attrib, poFile])[1]
        if msgout:
            grepout = self.safe_command(
                ['grep', '-E', 'msgfmt: found'], msgout)[0]
            reout = self.match('.*?(\d+).*', grepout.strip())
            return int(reout)
        else:
            return 0

    def checkAttrib(self, attrib, file):
        """Counts the lines that match a particular attribute"""
        msgout = self.safe_command(['msgattrib', attrib, file])[0]
        grepout = self.safe_command(['grep', '-E', '^msgid *"'], msgout)[0]
        sedout = self.safe_command(['sed', '1d'], grepout)[0]
        msgcount = self.safe_command(['wc', '-l'], sedout)[0]
        return int(msgcount)

    def checkStatus(self, wrkDir, langCC, fileMask, total, checkAccel):
        """Merges the current translations with the latest .po template.
        Calculates the statistics for the given translation."""

        potFile = os.path.join(wrkDir, fileMask + '.pot')
        poFile = os.path.join(wrkDir, langCC,  fileMask + '.po')

        if not os.path.isfile(poFile):
            # No need to write status for non-existent .po files
            # Just print on standard output for the "paper" report
            return '---'
        else:
            wrkFile = os.path.join(wrkDir, 'temp.po')
            cmd = ['msgmerge', '--no-wrap', '--quiet',
                   '--no-fuzzy-matching', poFile, potFile, '-o', wrkFile]
            self.safe_command(cmd)
            poDate = self.getPoDate(wrkFile)
            error = self.checkError('--check-format', wrkFile)
            if error:
                return 'BROKEN'
            else:
                trans = self.checkAttrib('--translated', wrkFile)
                untrans = self.checkAttrib('--untranslated', wrkFile)
# transifex doesn't support fuzzy, so don't report it anymore
#           fuzzy = self.checkAttrib('--only-fuzzy', wrkFile)
                fuzzy = 0
                if checkAccel:
                    accel = self.checkError('--check-accelerators', wrkFile)
                else:
                    accel = 0

                trans = trans - fuzzy

                if untrans + fuzzy + accel == 0:
                    return 'OK'
                else:
                    if trans == total:
                        percent = 99
                    else:
                        percent = 100 * (trans) / total

                    if checkAccel:
#                return '%2s%% (%s/%s/%s)' % (percent, untrans, fuzzy, accel)
                        return '%2s%% (%s/%s)' % (percent, untrans, accel)
                    else:
#                return '%2s%% (%s/%s)' % (percent, untrans, fuzzy)
                        return '%2s%% (%s)' % (percent, untrans)

    def printStatLine(self, Lang, Gui, Doc):
        print '%-33s: %-19s: %-19s' % (Lang, Gui, Doc)

    def checkTranslation(self, wrkDir):

        [totGui, tsGui] = self.getTotal(wrkDir, fileGui)
        [totDoc, tsDoc] = self.getTotal(wrkDir, fileDoc)

        firstline = '==== Translation of %s %s' % (urlTrunk, Sep75)
        print ''
        print firstline[0:75]
        self.printStatLine('', 'User interface', 'Manuals')
        self.printStatLine('Total strings', totGui, totDoc)
#        self.printStatLine('Language', 'Status (un/fu/ma)', 'Status (un/fu)')
        self.printStatLine('Language', 'Status (un/ma)', 'Status (un)')
        print Sep75

        csvReader = csv.DictReader(
            open(langList), langFields, delimiter=';', quotechar='"')
        csvReader.skipinitialspace = True

        for row in csvReader:
            # Ignore lines beginning with:
            # '\xef' = UTF-8 BOM
            # '#' = comment line
            if row['LangCC'][0] != '\xef' and row['LangCC'][0] != '#':
                langCC = row['LangCC'].strip()
                statusGui = self.checkStatus(
                    wrkDir, langCC, fileGui, totGui, True)
                statusDoc = self.checkStatus(
                    wrkDir, langCC, fileDoc, totDoc, False)

                if statusGui != 'NONE' or statusDoc != 'NONE':
                    langName = row['LangName'].strip()
                    langName = langName + ' (' + langCC + ')'
                    self.printStatLine(langName, statusGui, statusDoc)

        return None

    def createReport(self):
        [info_out, info_err] = self.safe_command(['svn', 'info'])
        if info_err:
            print >> sys.stderr, '\nError: %s' % (info_err)
            sys.exit(0)

        # Try different matches for older and newer svn clients
        reposRoot = self.match('Repository Root: (\S+)', info_out)
        if reposRoot is None:
            reposRoot = self.match('Repository: (\S+)', info_out)

        reposTrunk = reposRoot + '/' + urlTrunk + '/'

        # Prepare the working copies for doc|gui * trunk|branch
        self.prepareWC(wrkDir, reposTrunk)

        self.checkTranslation(wrkDir)

        print Sep75
#        print 'Status: fu=fuzzy - un=untranslated - ma=missing accelerator keys'
        print 'Status: un=untranslated - ma=missing accelerator keys'
        print Sep75

        # Clean up the tmp folder
        for root, dirs, files in os.walk('tmp', topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                os.rmdir(os.path.join(root, name))

        return None


def main():
    # Parse the command-line options and arguments.
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], 'hm:',
                                       ['help',
                                        'to-email-id=',
                                        ])
    except getopt.GetoptError, msg:
        usage_and_exit(msg)

    to_email_id = None
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage_and_exit()
        elif opt in ('-m', '--to-email-id'):
            to_email_id = arg

    report = transReport()

    [info_out, info_err] = report.safe_command(['svnversion', wrkDir])
    if info_err:
        print >> sys.stderr, '\nError: %s' % (info_err)
        sys.exit(0)

    wcrev = re.sub('[MS]', '', info_out).strip()

    subject = 'TortoiseSVN translation status report for r%s' % (wcrev)
    print subject

    report.createReport()

    timestamp = makeTimeString('%a, %d %b %Y %H:%M UTC', time.time())

if __name__ == '__main__':
    main()
