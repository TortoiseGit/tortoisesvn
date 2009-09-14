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

Compute the trunk and branch translation status report for the
TortoiseSVN GUI and DOCs.
Create three include files (trunk/branch/language list) for the status report
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

urlTrunk = 'trunk'
urlBranch = 'branches/1.6.x'
urlGui   = 'Languages'
urlDoc   = 'doc/po'

langList = os.path.join('gui', 'trunk', 'Languages.txt')
langFields = ['Tag','LangCC','LangID','FlagByte','LangName','Translators']
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
    return  time.strftime(format, time.gmtime(timestamp))


class includeWriter:
    def __init__(self, outfile):
        self.out = file(outfile, 'w')

    def addHeader(self):
        self.out.write('<?php\n')

    def addFooter(self, wcrev, update):
        self.out.write('$tsvn_var = array(\n')
        self.out.write('"wcrev" => "%s",\n' %(wcrev))
        self.out.write('"update" => "%s"\n' %(update))
        self.out.write(');\n')
        self.out.write('?>\n')

    def sectOpen(self, section):
        self.out.write('$%s = array(\n' %(section))

    def sectWrite(self, langCC, isPoFile, total, trans, fuzzy, untrans, accel, file, date):
        self.out.write('"%s" => array(%s, %s, %s, %s, %s, %s, "%s", "%s"),\n' \
          % (langCC,isPoFile,total,trans,fuzzy,untrans,accel,file,date))

    def sectClose(self, total, file, date):
        # Write the end of the array without a comma
        self.out.write('"zzz" => array(-1, %s, 0, 0, %s, 0, "%s", "%s")\n' \
          %(total, total ,file+'.pot', date))
        self.out.write(');\n')
    
    def addCountries(self, langSource):
        self.out.write('$countries = array(\n')
        langList = os.path.join('gui', langSource, 'Languages.txt')
        csvReader = csv.DictReader(open(langList), langFields, delimiter=';', quotechar='"')
        csvReader.skipinitialspace = True

        for row in csvReader:
          # Ignore lines beginning with a '#'
          if row['Tag'][0] != '#':
            self.addCountryRow(row['LangCC'].strip(), row['Tag'].strip(), row['FlagByte'].strip(), \
              row['LangName'].strip(), row['Translators'].strip())
        self.out.write(');\n')
        self.out.write('?>\n')

    def addCountryRow(self, LangCC, Tag, Flag, LangName, Translators):
        self.out.write('"%s" => array("%s", "%s", "%s", "%s", %s)' \
          % (LangCC,Tag,Flag,LangCC,LangName,Translators))
        self.out.write(',')
        self.out.write('\n')
        
class transReport:
    def __init__(self, to_email_id='luebbe@tigris.org'):
        self.to_email_id = to_email_id
        self.from_email_id = '<dev@tortoisesvn.tigris.org>'

    def safe_command(self, cmd_and_args, cmd_in=''):
        [stdout, stderr] = subprocess.Popen(cmd_and_args, \
                           stdin=subprocess.PIPE, \
                           stdout=subprocess.PIPE, \
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

    def prepareTMP(self, srcDir, subDir, fileMask):
        """Merges the current translations with the latest .po template.
        Places the output in the given tmp subfolder"""

        srcDir = os.path.join(srcDir, subDir)
        dstDir = os.path.join('tmp', subDir)

        if not os.path.isdir(dstDir):
          os.makedirs(dstDir)

        files = os.listdir(srcDir)
        files.sort()
        for file in files:
          poFile = self.match('('+fileMask+'.*).po$', file)
          if not poFile:
            continue
          potFile = os.path.join(srcDir, fileMask + '.pot')
          srcFile = os.path.join(srcDir, poFile + '.po')
          dstFile = os.path.join(dstDir, poFile + '.po')
          cmd = ['msgmerge','--no-wrap','--quiet','--no-fuzzy-matching', srcFile, potFile, '-o', dstFile]
          self.safe_command(cmd)
          total = self.checkAttrib('--untranslated', potFile)
          timestamp = makeTimeString('%Y-%m-%d', os.path.getmtime(potFile))
        return (total, timestamp)

    def getPoDate(self, poFile):
        """returns the PO-Revision-Date of the the .po file in a string"""
        grepout = self.safe_command(['grep', '-E', 'PO-Revision-Date:', poFile])[0]
        reout = self.match('.*?(\d{4}-\d{2}-\d{2}).*', grepout.strip())
        return reout

    def checkError(self, attrib, file):
        """Checks for errors in the .po file. Can detect missing accelerators"""
        msgout = self.safe_command(['msgfmt', attrib, file])[1]
        if msgout:
           grepout = self.safe_command(['grep', '-E', 'msgfmt: found'], msgout)[0]
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

    def checkStatus(self, out, wrkDir, langCC, fileMask, total, checkAccel):
        fileName = fileMask + '_' + langCC
        wrkFile = os.path.join('tmp', wrkDir, fileName + '.po')

        if not os.path.isfile(wrkFile):
          # No need to write status for non-existent .po files
          # Just print on standard output for the "paper" report
          # out.sectWrite(langCC, 0, total, 0, 0, total, 0, fileName, 'NONE')
          return 'NONE'
        else:
          poDate = self.getPoDate(wrkFile)
          error = self.checkError('--check', wrkFile)
          if error:
            out.sectWrite(langCC, error, total, 0, 0, total, 0, fileName, poDate)
            return 'BROKEN'
          else:
            trans = self.checkAttrib('--translated', wrkFile)
            untrans = self.checkAttrib('--untranslated', wrkFile)
            fuzzy = self.checkAttrib('--only-fuzzy', wrkFile)
            if checkAccel:
              accel = self.checkError('--check-accelerators', wrkFile)
            else:
              accel = 0

            trans = trans-fuzzy
            out.sectWrite(langCC, 0, total, trans, fuzzy, untrans, accel, fileName, poDate)

            if untrans+fuzzy+accel == 0:
              return 'OK'
            else:
              if trans == total:
                percent = 99
              else:
                percent = 100*(trans)/total

              if checkAccel:
                return '%2s%% (%s/%s/%s)' % (percent, untrans, fuzzy, accel)
              else:
                return '%2s%% (%s/%s)' % (percent, untrans, fuzzy)

    def printStatLine(self, Lang, Trunk, Branch):
        print '%-32s: %-20s: %-20s' % (Lang, Trunk, Branch)
    
    def checkTranslation(self, srcDir, fileMask, checkAccel):

        [totTrunk, tsTrunk] = self.prepareTMP(srcDir, 'trunk', fileMask)
        [totBranch, tsBranch] = self.prepareTMP(srcDir, 'branch', fileMask)

        outTrunk.sectOpen(fileMask)
        outBranch.sectOpen(fileMask)

        firstline = '%s %s translation %s' % (fileMask, srcDir, Sep75)
        print ''
        print firstline[0:75]
        self.printStatLine('', 'Developer Version', 'Current Release')
        self.printStatLine('Location', urlTrunk, urlBranch)
        self.printStatLine('Total strings', totTrunk,totBranch)
        self.printStatLine('Language', 'Status (un/fu/ma)', 'Status (un/fu/ma)')
        print Sep75

        csvReader = csv.DictReader(open(langList), langFields, delimiter=';', quotechar='"')
        csvReader.skipinitialspace = True

        for row in csvReader:
          # Ignore lines beginning with a '#'
          # Ignore .pot file (Tag=0)
          if row['Tag'][0] != '#' and row['Tag'] != '0':
            langCC = row['LangCC'].strip()
            statusTrunk = self.checkStatus(outTrunk, 'trunk', langCC, fileMask, totTrunk, checkAccel)
            statusBranch = self.checkStatus(outBranch, 'branch', langCC, fileMask, totBranch, checkAccel)

            if statusTrunk != 'NONE' or statusBranch != 'NONE':
               langName = row['LangName'].strip()
               langName = langName + ' ('+langCC+')'
               self.printStatLine (langName, statusTrunk, statusBranch)

        outTrunk.sectClose(totTrunk, fileMask, tsTrunk)
        outBranch.sectClose(totBranch, fileMask, tsBranch)

        return None

    def createReport(self):
        [info_out, info_err] = self.safe_command(['svn', 'info'])
        if info_err:
            print >> sys.stderr, '\nError: %s' % (info_err)
            sys.exit(0)
            
        # Try different matches for older and newer svn clients
        reposRoot = self.match('Repository Root: (\S+)',info_out)
        if reposRoot is None:
           reposRoot = self.match('Repository: (\S+)',info_out)

        reposTrunk = reposRoot + '/' + urlTrunk + '/'
        reposBranch = reposRoot + '/' + urlBranch + '/'

        # Prepare the working copies for doc|gui * trunk|branch
        self.prepareWC(os.path.join('gui', 'trunk'), reposTrunk + urlGui)
        self.prepareWC(os.path.join('gui', 'branch'), reposBranch + urlGui)
        self.prepareWC(os.path.join('doc', 'trunk'), reposTrunk + urlDoc)
        self.prepareWC(os.path.join('doc', 'branch'), reposBranch + urlDoc)

        self.checkTranslation('gui','Tortoise',True)
        self.checkTranslation('doc','TortoiseSVN',False)
        self.checkTranslation('doc','TortoiseMerge',False)

        print Sep75
        print 'Status: fu=fuzzy - un=untranslated - ma=missing accelerator keys'
        print Sep75

        # Clean up the tmp folder
        for root, dirs, files in os.walk('tmp', topdown=False):
          for name in files:
            os.remove(os.path.join(root, name))
          for name in dirs:
            os.rmdir(os.path.join(root, name))

        return None

outTrunk = includeWriter('trans_data_trunk.inc')
outBranch = includeWriter('trans_data_branch.inc')

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

    [info_out, info_err] = report.safe_command(['svnversion', '.'])
    if info_err:
        print >> sys.stderr, '\nError: %s' % (info_err)
        sys.exit(0)

    wcrev = re.sub('[MS]', '', info_out).strip()

    subject = 'TortoiseSVN translation status report for r%s' % (wcrev)
    print subject

    outTrunk.addHeader()
    outBranch.addHeader()

    report.createReport()

    outTrunk.addCountries('trunk')
    outBranch.addCountries('branch')
    
    timestamp = makeTimeString('%a, %d %b %Y %H:%M UTC', time.time())

    outTrunk.addFooter(wcrev, timestamp)
    outBranch.addFooter(wcrev, timestamp)

if __name__ == '__main__':
    main()
