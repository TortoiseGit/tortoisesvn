// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#include "stdafx.h"
#include "SmartHandle.h"
#include "../Utils/CrashReport.h"

#include <iostream>
#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <io.h>
#include <fcntl.h>
#include <memory>


#pragma warning(push)
#include "apr_pools.h"
#include "svn_error.h"
#include "svn_client.h"
#include "svn_dirent_uri.h"
#include "SubWCRev.h"
#include "UnicodeUtils.h"
#include "../version.h"
#include "svn_dso.h"
#pragma warning(pop)


// Define the help text as a multi-line macro
// Every line except the last must be terminated with a backslash
#define HelpText1 "\
Usage: SubWCRev WorkingCopyPath [SrcVersionFile DstVersionFile] [-nmdf]\n\
\n\
Params:\n\
WorkingCopyPath    :   path to a Subversion working copy.\n\
SrcVersionFile     :   path to a template file containing keywords.\n\
DstVersionFile     :   path to save the resulting parsed file.\n\
-n                 :   if given, then SubWCRev will error if the working\n\
                       copy contains local modifications.\n\
-N                 :   if given, then SubWCRev will error if the working\n\
                       copy contains unversioned items.\n\
-m                 :   if given, then SubWCRev will error if the working\n\
                       copy contains mixed revisions.\n\
-d                 :   if given, then SubWCRev will only do its job if\n\
                       DstVersionFile does not exist.\n\
-q                 :   if given, then SubWCRev will perform keyword\n\
                       substitution but will not show status on stdout.\n"
#define HelpText2 "\
-f                 :   if given, then SubWCRev will include the\n\
                       last-committed revision of folders. Default is\n\
                       to use only files to get the revision numbers.\n\
                       this only affects $WCREV$ and $WCDATE$.\n\
-e                 :   if given, also include dirs which are included\n\
                       with svn:externals, but only if they're from the\n\
                       same repository.\n\
-E                 :   if given, same as -e, but it ignores the externals\n\
                       with explicit revisions, when the revision range\n\
                       inside of them is only the given explicit revision\n\
                       in the properties. So it doesn't lead to mixed\n\
                       revisions\n"
#define HelpText3 "\
-x                 :   if given, then SubWCRev will write the revisions\n\
                       numbers in HEX instead of decimal\n\
-X                 :   if given, then SubWCRev will write the revisions\n\
                       numbers in HEX with '0x' before them\n"
#define HelpText4 "\
Switches must be given in a single argument, e.g. '-nm' not '-n -m'.\n\
\n\
SubWCRev reads the Subversion status of all files in a working copy\n\
excluding externals. If SrcVersionFile is specified, it is scanned\n\
for special placeholders of the form \"$WCxxx$\".\n\
SrcVersionFile is then copied to DstVersionFile but the placeholders\n\
are replaced with information about the working copy as follows:\n\
\n\
$WCREV$         Highest committed revision number\n\
$WCREV&$        Highest committed revision number ANDed with the number\n\
                after the &\n\
$WCREV+$        Highest committed revision number added with the number\n\
                after the &\n\
$WCREV-$        Highest committed revision number subtracted with the\n\
                number after the &\n\
$WCDATE$        Date of highest committed revision\n\
$WCDATE=$       Like $WCDATE$ with an added strftime format after the =\n\
$WCRANGE$       Update revision range\n\
$WCURL$         Repository URL of the working copy\n\
$WCNOW$         Current system date & time\n\
$WCNOW=$        Like $WCNOW$ with an added strftime format after the =\n\
$WCLOCKDATE$    Lock date for this item\n\
$WCLOCKDATE=$   Like $WCLOCKDATE$ with an added strftime format after the =\n\
$WCLOCKOWNER$   Lock owner for this item\n\
$WCLOCKCOMMENT$ Lock comment for this item\n\
\n"

#define HelpText5 "\
The strftime format strings for $WCxxx=$ must not be longer than 1024\n\
characters, and must not produce output greater than 1024 characters.\n\
\n\
Placeholders of the form \"$WCxxx?TrueText:FalseText$\" are replaced with\n\
TrueText if the tested condition is true, and FalseText if false.\n\
\n\
$WCMODS$        True if local modifications found\n\
$WCMIXED$       True if mixed update revisions found\n\
$WCEXTALLFIXED$ True if all externals are fixed to an explicit revision\n\
$WCISTAGGED$    True if the repository URL contains the tags classification pattern\n\
$WCINSVN$       True if the item is versioned\n\
$WCNEEDSLOCK$   True if the svn:needs-lock property is set\n\
$WCISLOCKED$    True if the item is locked\n"
// End of multi-line help text.

#define VERDEF           "$WCREV$"
#define VERDEFAND        "$WCREV&"
#define VERDEFOFFSET1    "$WCREV-"
#define VERDEFOFFSET2    "$WCREV+"
#define DATEDEF          "$WCDATE$"
#define DATEDEFUTC       "$WCDATEUTC$"
#define DATEWFMTDEF      "$WCDATE="
#define DATEWFMTDEFUTC   "$WCDATEUTC="
#define MODDEF           "$WCMODS?"
#define RANGEDEF         "$WCRANGE$"
#define MIXEDDEF         "$WCMIXED?"
#define EXTALLFIXED      "$WCEXTALLFIXED?"
#define ISTAGGED         "$WCISTAGGED?"
#define URLDEF           "$WCURL$"
#define NOWDEF           "$WCNOW$"
#define NOWDEFUTC        "$WCNOWUTC$"
#define NOWWFMTDEF       "$WCNOW="
#define NOWWFMTDEFUTC    "$WCNOWUTC="
#define ISINSVN          "$WCINSVN?"
#define NEEDSLOCK        "$WCNEEDSLOCK?"
#define ISLOCKED         "$WCISLOCKED?"
#define LOCKDATE         "$WCLOCKDATE$"
#define LOCKDATEUTC      "$WCLOCKDATEUTC$"
#define LOCKWFMTDEF      "$WCLOCKDATE="
#define LOCKWFMTDEFUTC   "$WCLOCKDATEUTC="
#define LOCKOWNER        "$WCLOCKOWNER$"
#define LOCKCOMMENT      "$WCLOCKCOMMENT$"
#define UNVERDEF         "$WCUNVER?"

// Internal error codes
// Note: these error codes are documented in /doc/source/en/TortoiseSVN/tsvn_subwcrev.xml
#define ERR_SYNTAX      1   // Syntax error
#define ERR_FNF         2   // File/folder not found
#define ERR_OPEN        3   // File open error
#define ERR_ALLOC       4   // Memory allocation error
#define ERR_READ        5   // File read/write/size error
#define ERR_SVN_ERR     6   // SVN error
// Documented error codes
#define ERR_SVN_MODS    7   // Local mods found (-n)
#define ERR_SVN_MIXED   8   // Mixed rev WC found (-m)
#define ERR_OUT_EXISTS  9   // Output file already exists (-d)
#define ERR_NOWC       10   // the path is not a working copy or part of one
#define ERR_SVN_UNVER  11   // Unversioned items found (-N)

// Value for apr_time_t to signify "now"
#define USE_TIME_NOW    -2  // 0 and -1 might already be significant.



int FindPlaceholder(char *def, char *pBuf, size_t & index, size_t filelength)
{
    size_t deflen = strlen(def);
    while (index + deflen <= filelength)
    {
        if (memcmp(pBuf + index, def, deflen) == 0)
            return TRUE;
        index++;
    }
    return FALSE;
}
int FindPlaceholderW(wchar_t *def, wchar_t *pBuf, size_t & index, size_t filelength)
{
    size_t deflen = wcslen(def);
    while ((index + deflen)*sizeof(wchar_t) <= filelength)
    {
        if (memcmp(pBuf + index, def, deflen*sizeof(wchar_t)) == 0)
            return TRUE;
        index++;
    }

    return FALSE;
}

int InsertRevision(char * def, char * pBuf, size_t & index,
                    size_t & filelength, size_t maxlength,
                    long MinRev, long MaxRev, SubWCRev_t * SubStat)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholder(def, pBuf, index, filelength))
    {
        // No more matches found.
        return FALSE;
    }
    ptrdiff_t exp = 0;
    if ((strcmp(def,VERDEFAND) == 0) || (strcmp(def,VERDEFOFFSET1) == 0) || (strcmp(def,VERDEFOFFSET2) == 0))
    {
        char format[1024];
        char * pStart = pBuf + index + strlen(def);
        char * pEnd = pStart;

        while (*pEnd != '$')
        {
            pEnd++;
            if (pEnd - pBuf >= (__int64)filelength)
                return FALSE;   // No terminator - malformed so give up.
        }
        if ((pEnd - pStart) > 1024)
        {
            return FALSE; // value specifier too big
        }
        exp = pEnd - pStart + 1;
        memset(format,0,1024);
        memcpy(format,pStart,pEnd - pStart);
        unsigned long number = strtoul(format, NULL, 0);
        if (strcmp(def,VERDEFAND) == 0)
        {
            if (MinRev != -1)
                MinRev &= number;
            MaxRev &= number;
        }
        if (strcmp(def,VERDEFOFFSET1) == 0)
        {
            if (MinRev != -1)
                MinRev -= number;
            MaxRev -= number;
        }
        if (strcmp(def,VERDEFOFFSET2) == 0)
        {
            if (MinRev != -1)
                MinRev += number;
            MaxRev += number;
        }
    }
    // Format the text to insert at the placeholder
    char destbuf[40];
    if (MinRev == -1 || MinRev == MaxRev)
    {
        if ((SubStat)&&(SubStat->bHexPlain))
            sprintf_s(destbuf, "%LX", MaxRev);
        else if ((SubStat)&&(SubStat->bHexX))
            sprintf_s(destbuf, "%#LX", MaxRev);
        else
            sprintf_s(destbuf, "%Ld", MaxRev);
    }
    else
    {
        if ((SubStat)&&(SubStat->bHexPlain))
            sprintf_s(destbuf, "%LX:%LX", MinRev, MaxRev);
        else if ((SubStat)&&(SubStat->bHexX))
            sprintf_s(destbuf, "%#LX:%#LX", MinRev, MaxRev);
        else
            sprintf_s(destbuf, "%Ld:%Ld", MinRev, MaxRev);
    }
    // Replace the $WCxxx$ string with the actual revision number
    char * pBuild = pBuf + index;
    ptrdiff_t Expansion = strlen(destbuf) - exp - strlen(def);
    if (Expansion < 0)
    {
        memmove(pBuild, pBuild - Expansion, filelength - ((pBuild - Expansion) - pBuf));
    }
    else if (Expansion > 0)
    {
        // Check for buffer overflow
        if (maxlength < Expansion + filelength) return FALSE;
        memmove(pBuild + Expansion, pBuild, filelength - (pBuild - pBuf));
    }
    memmove(pBuild, destbuf, strlen(destbuf));
    filelength += Expansion;
    return TRUE;
}
int InsertRevisionW(wchar_t * def, wchar_t * pBuf, size_t & index,
    size_t & filelength, size_t maxlength,
    long MinRev, long MaxRev, SubWCRev_t * SubStat)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholderW(def, pBuf, index, filelength))
    {
        // No more matches found.
        return FALSE;
    }

    ptrdiff_t exp = 0;
    if ((wcscmp(def,TEXT(VERDEFAND)) == 0) || (wcscmp(def,TEXT(VERDEFOFFSET1)) == 0) || (wcscmp(def,TEXT(VERDEFOFFSET2)) == 0))
    {
        wchar_t format[1024];
        wchar_t * pStart = pBuf + index + wcslen(def);
        wchar_t * pEnd = pStart;

        while (*pEnd != '$')
        {
            pEnd++;
            if (((__int64)(pEnd - pBuf))*((__int64)sizeof(wchar_t)) >= (__int64)filelength)
                return FALSE;   // No terminator - malformed so give up.
        }
        if ((pEnd - pStart) > 1024)
        {
            return FALSE; // Format specifier too big
        }
        exp = pEnd - pStart + 1;
        memset(format,0,1024*sizeof(wchar_t));
        memcpy(format,pStart,(pEnd - pStart)*sizeof(wchar_t));
        unsigned long number = wcstoul(format, NULL, 0);
        if (wcscmp(def,TEXT(VERDEFAND)) == 0)
        {
            if (MinRev != -1)
                MinRev &= number;
            MaxRev &= number;
        }
        if (wcscmp(def,TEXT(VERDEFOFFSET1)) == 0)
        {
            if (MinRev != -1)
                MinRev -= number;
            MaxRev -= number;
        }
        if (wcscmp(def,TEXT(VERDEFOFFSET2)) == 0)
        {
            if (MinRev != -1)
                MinRev += number;
            MaxRev += number;
        }
    }

    // Format the text to insert at the placeholder
    wchar_t destbuf[40];
    if (MinRev == -1 || MinRev == MaxRev)
    {
        if ((SubStat)&&(SubStat->bHexPlain))
            swprintf_s(destbuf, L"%LX", MaxRev);
        else if ((SubStat)&&(SubStat->bHexX))
            swprintf_s(destbuf, L"%#LX", MaxRev);
        else
            swprintf_s(destbuf, L"%Ld", MaxRev);
    }
    else
    {
        if ((SubStat)&&(SubStat->bHexPlain))
            swprintf_s(destbuf, L"%LX:%LX", MinRev, MaxRev);
        else if ((SubStat)&&(SubStat->bHexX))
            swprintf_s(destbuf, L"%#LX:%#LX", MinRev, MaxRev);
        else
            swprintf_s(destbuf, L"%Ld:%Ld", MinRev, MaxRev);
    }
    // Replace the $WCxxx$ string with the actual revision number
    wchar_t * pBuild = pBuf + index;
    ptrdiff_t Expansion = wcslen(destbuf) - exp - wcslen(def);
    if (Expansion < 0)
    {
        memmove(pBuild, pBuild - Expansion, (filelength - ((pBuild - Expansion) - pBuf))*sizeof(wchar_t));
    }
    else if (Expansion > 0)
    {
        // Check for buffer overflow
        if (maxlength < Expansion + filelength) return FALSE;
        memmove(pBuild + Expansion, pBuild, (filelength - (pBuild - pBuf))*sizeof(wchar_t));
    }
    memmove(pBuild, destbuf, wcslen(destbuf)*sizeof(wchar_t));
    filelength += (Expansion*sizeof(wchar_t));
    return TRUE;
}

void _invalid_parameter_donothing(
    const wchar_t * /*expression*/,
    const wchar_t * /*function*/,
    const wchar_t * /*file*/,
    unsigned int /*line*/,
    uintptr_t /*pReserved*/
    )
{
    // do nothing
}

int InsertDate(char * def, char * pBuf, size_t & index,
                size_t & filelength, size_t maxlength,
                apr_time_t date_svn)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholder(def, pBuf, index, filelength))
    {
        // No more matches found.
        return FALSE;
    }
    // Format the text to insert at the placeholder
    __time64_t ttime;
    if (date_svn == USE_TIME_NOW)
        _time64(&ttime);
    else
        ttime = date_svn/1000000L;

    struct tm newtime;
    if (strstr(def, "UTC"))
    {
        if (_gmtime64_s(&newtime, &ttime))
            return FALSE;
    }
    else
    {
        if (_localtime64_s(&newtime, &ttime))
            return FALSE;
    }
    char destbuf[1024];
    char * pBuild = pBuf + index;
    ptrdiff_t Expansion;
    if ((strcmp(def,DATEWFMTDEF) == 0) || (strcmp(def,NOWWFMTDEF) == 0) || (strcmp(def,LOCKWFMTDEF) == 0) ||
        (strcmp(def,DATEWFMTDEFUTC) == 0) || (strcmp(def,NOWWFMTDEFUTC) == 0) || (strcmp(def,LOCKWFMTDEFUTC) == 0))
    {
        // Format the date/time according to the supplied strftime format string
        char format[1024];
        char * pStart = pBuf + index + strlen(def);
        char * pEnd = pStart;

        while (*pEnd != '$')
        {
            pEnd++;
            if (pEnd - pBuf >= (__int64)filelength)
                return FALSE;   // No terminator - malformed so give up.
        }
        if ((pEnd - pStart) > 1024)
        {
            return FALSE; // Format specifier too big
        }
        memset(format,0,1024);
        memcpy(format,pStart,pEnd - pStart);

        // to avoid wcsftime aborting if the user specified an invalid time format,
        // we set a custom invalid parameter handler that does nothing at all:
        // that makes wcsftime do nothing and set errno to EINVAL.
        // we restore the invalid parameter handler right after
        _invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(_invalid_parameter_donothing);

        if (strftime(destbuf,1024,format,&newtime) == 0)
        {
            if (errno == EINVAL)
                strcpy_s(destbuf, "Invalid Time Format Specified");
        }
        _set_invalid_parameter_handler(oldHandler);
        Expansion = strlen(destbuf) - (strlen(def) + pEnd - pStart + 1);
    }
    else
    {
        // Format the date/time in international format as yyyy/mm/dd hh:mm:ss
        sprintf_s(destbuf, "%04d/%02d/%02d %02d:%02d:%02d",
            newtime.tm_year + 1900,
            newtime.tm_mon + 1,
            newtime.tm_mday,
            newtime.tm_hour,
            newtime.tm_min,
            newtime.tm_sec);

        Expansion = strlen(destbuf) - strlen(def);
    }
    // Replace the def string with the actual commit date
    if (Expansion < 0)
    {
        memmove(pBuild, pBuild - Expansion, filelength - ((pBuild - Expansion) - pBuf));
    }
    else if (Expansion > 0)
    {
        // Check for buffer overflow
        if (maxlength < Expansion + filelength) return FALSE;
        memmove(pBuild + Expansion, pBuild, filelength - (pBuild - pBuf));
    }
    memmove(pBuild, destbuf, strlen(destbuf));
    filelength += Expansion;
    return TRUE;
}
int InsertDateW(wchar_t * def, wchar_t * pBuf, size_t & index,
    size_t & filelength, size_t maxlength,
    apr_time_t date_svn)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholderW(def, pBuf, index, filelength))
    {
        // No more matches found.
        return FALSE;
    }
    // Format the text to insert at the placeholder
    __time64_t ttime;
    if (date_svn == USE_TIME_NOW)
        _time64(&ttime);
    else
        ttime = date_svn/1000000L;

    struct tm newtime;
    if (wcsstr(def, L"UTC"))
    {
        if (_gmtime64_s(&newtime, &ttime))
            return FALSE;
    }
    else
    {
        if (_localtime64_s(&newtime, &ttime))
            return FALSE;
    }
    wchar_t destbuf[1024];
    wchar_t * pBuild = pBuf + index;
    ptrdiff_t Expansion;
    if ((wcscmp(def,TEXT(DATEWFMTDEF)) == 0) || (wcscmp(def,TEXT(NOWWFMTDEF)) == 0) || (wcscmp(def,TEXT(LOCKWFMTDEF)) == 0) ||
        (wcscmp(def,TEXT(DATEWFMTDEFUTC)) == 0) || (wcscmp(def,TEXT(NOWWFMTDEFUTC)) == 0) || (wcscmp(def,TEXT(LOCKWFMTDEFUTC)) == 0))
    {
        // Format the date/time according to the supplied strftime format string
        wchar_t format[1024];
        wchar_t * pStart = pBuf + index + wcslen(def);
        wchar_t * pEnd = pStart;

        while (*pEnd != '$')
        {
            pEnd++;
            if (((__int64)(pEnd - pBuf))*((__int64)sizeof(wchar_t)) >= (__int64)filelength)
                return FALSE;   // No terminator - malformed so give up.
        }
        if ((pEnd - pStart) > 1024)
        {
            return FALSE; // Format specifier too big
        }
        memset(format,0,1024*sizeof(wchar_t));
        memcpy(format,pStart,(pEnd - pStart)*sizeof(wchar_t));

        // to avoid wcsftime aborting if the user specified an invalid time format,
        // we set a custom invalid parameter handler that does nothing at all:
        // that makes wcsftime do nothing and set errno to EINVAL.
        // we restore the invalid parameter handler right after
        _invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(_invalid_parameter_donothing);

        if (wcsftime(destbuf,1024,format,&newtime) == 0)
        {
            if (errno == EINVAL)
                wcscpy_s(destbuf, L"Invalid Time Format Specified");
        }
        _set_invalid_parameter_handler(oldHandler);

        Expansion = wcslen(destbuf) - (wcslen(def) + pEnd - pStart + 1);
    }
    else
    {
        // Format the date/time in international format as yyyy/mm/dd hh:mm:ss
        swprintf_s(destbuf, L"%04d/%02d/%02d %02d:%02d:%02d",
            newtime.tm_year + 1900,
            newtime.tm_mon + 1,
            newtime.tm_mday,
            newtime.tm_hour,
            newtime.tm_min,
            newtime.tm_sec);

        Expansion = wcslen(destbuf) - wcslen(def);
    }
    // Replace the def string with the actual commit date
    if (Expansion < 0)
    {
        memmove(pBuild, pBuild - Expansion, (filelength - ((pBuild - Expansion) - pBuf))*sizeof(wchar_t));
    }
    else if (Expansion > 0)
    {
        // Check for buffer overflow
        if (maxlength < Expansion + filelength) return FALSE;
        memmove(pBuild + Expansion, pBuild, (filelength - (pBuild - pBuf))*sizeof(wchar_t));
    }
    memmove(pBuild, destbuf, wcslen(destbuf)*sizeof(wchar_t));
    filelength += Expansion*sizeof(wchar_t);
    return TRUE;
}

int InsertUrl(char * def, char * pBuf, size_t & index,
                    size_t & filelength, size_t maxlength,
                    char * pUrl)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholder(def, pBuf, index, filelength))
    {
        // No more matches found.
        return FALSE;
    }
    // Replace the $WCURL$ string with the actual URL
    char * pBuild = pBuf + index;
    ptrdiff_t Expansion = strlen(pUrl) - strlen(def);
    if (Expansion < 0)
    {
        memmove(pBuild, pBuild - Expansion, filelength - ((pBuild - Expansion) - pBuf));
    }
    else if (Expansion > 0)
    {
        // Check for buffer overflow
        if (maxlength < Expansion + filelength) return FALSE;
        memmove(pBuild + Expansion, pBuild, filelength - (pBuild - pBuf));
    }
    memmove(pBuild, pUrl, strlen(pUrl));
    filelength += Expansion;
    return TRUE;
}
int InsertUrlW(wchar_t * def, wchar_t * pBuf, size_t & index,
    size_t & filelength, size_t maxlength,
    const wchar_t * pUrl)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholderW(def, pBuf, index, filelength))
    {
        // No more matches found.
        return FALSE;
    }
    // Replace the $WCURL$ string with the actual URL
    wchar_t * pBuild = pBuf + index;
    ptrdiff_t Expansion = wcslen(pUrl) - wcslen(def);
    if (Expansion < 0)
    {
        memmove(pBuild, pBuild - Expansion, (filelength - ((pBuild - Expansion) - pBuf))*sizeof(wchar_t));
    }
    else if (Expansion > 0)
    {
        // Check for buffer overflow
        if (maxlength < Expansion + filelength) return FALSE;
        memmove(pBuild + Expansion, pBuild, (filelength - (pBuild - pBuf))*sizeof(wchar_t));
    }
    memmove(pBuild, pUrl, wcslen(pUrl)*sizeof(wchar_t));
    filelength += Expansion*sizeof(wchar_t);
    return TRUE;
}

int InsertBoolean(char * def, char * pBuf, size_t & index, size_t & filelength, BOOL isTrue)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholder(def, pBuf, index, filelength))
    {
        // No more matches found.
        return FALSE;
    }
    // Look for the terminating '$' character
    char * pBuild = pBuf + index;
    char * pEnd = pBuild + 1;
    while (*pEnd != '$')
    {
        pEnd++;
        if (pEnd - pBuf >= (__int64)filelength)
            return FALSE;   // No terminator - malformed so give up.
    }

    // Look for the ':' dividing TrueText from FalseText
    char *pSplit = pBuild + 1;
    // this loop is guaranteed to terminate due to test above.
    while (*pSplit != ':' && *pSplit != '$')
        pSplit++;

    if (*pSplit == '$')
        return FALSE;       // No split - malformed so give up.

    if (isTrue)
    {
        // Replace $WCxxx?TrueText:FalseText$ with TrueText
        // Remove :FalseText$
        memmove(pSplit, pEnd + 1, filelength - (pEnd + 1 - pBuf));
        filelength -= (pEnd + 1 - pSplit);
        // Remove $WCxxx?
        size_t deflen = strlen(def);
        memmove(pBuild, pBuild + deflen, filelength - (pBuild + deflen - pBuf));
        filelength -= deflen;
    }
    else
    {
        // Replace $WCxxx?TrueText:FalseText$ with FalseText
        // Remove terminating $
        memmove(pEnd, pEnd + 1, filelength - (pEnd + 1 - pBuf));
        filelength--;
        // Remove $WCxxx?TrueText:
        memmove(pBuild, pSplit + 1, filelength - (pSplit + 1 - pBuf));
        filelength -= (pSplit + 1 - pBuild);
    }
    return TRUE;
}
int InsertBooleanW(wchar_t * def, wchar_t * pBuf, size_t & index, size_t & filelength, BOOL isTrue)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholderW(def, pBuf, index, filelength))
    {
        // No more matches found.
        return FALSE;
    }
    // Look for the terminating '$' character
    wchar_t * pBuild = pBuf + index;
    wchar_t * pEnd = pBuild + 1;
    while (*pEnd != '$')
    {
        pEnd++;
        if (pEnd - pBuf >= (__int64)filelength)
            return FALSE;   // No terminator - malformed so give up.
    }

    // Look for the ':' dividing TrueText from FalseText
    wchar_t *pSplit = pBuild + 1;
    // this loop is guaranteed to terminate due to test above.
    while (*pSplit != ':' && *pSplit != '$')
        pSplit++;

    if (*pSplit == '$')
        return FALSE;       // No split - malformed so give up.

    if (isTrue)
    {
        // Replace $WCxxx?TrueText:FalseText$ with TrueText
        // Remove :FalseText$
        memmove(pSplit, pEnd + 1, (filelength - (pEnd + 1 - pBuf))*sizeof(wchar_t));
        filelength -= ((pEnd + 1 - pSplit)*sizeof(wchar_t));
        // Remove $WCxxx?
        size_t deflen = wcslen(def);
        memmove(pBuild, pBuild + deflen, (filelength - (pBuild + deflen - pBuf))*sizeof(wchar_t));
        filelength -= (deflen*sizeof(wchar_t));
    }
    else
    {
        // Replace $WCxxx?TrueText:FalseText$ with FalseText
        // Remove terminating $
        memmove(pEnd, pEnd + 1, (filelength - (pEnd + 1 - pBuf))*sizeof(wchar_t));
        filelength -= sizeof(wchar_t);
        // Remove $WCxxx?TrueText:
        memmove(pBuild, pSplit + 1, (filelength - (pSplit + 1 - pBuf))*sizeof(wchar_t));
        filelength -= ((pSplit + 1 - pBuild)*sizeof(wchar_t));
    }
    return TRUE;
}

#pragma warning(push)
#pragma warning(disable: 4702)
int abort_on_pool_failure (int /*retcode*/)
{
    abort ();
    return -1;
}
#pragma warning(pop)

int _tmain(int argc, _TCHAR* argv[])
{
    // we have three parameters
    const TCHAR * src = NULL;
    const TCHAR * dst = NULL;
    const TCHAR * wc = NULL;
    BOOL bErrOnMods = FALSE;
    BOOL bErrOnUnversioned = FALSE;
    BOOL bErrOnMixed = FALSE;
    BOOL bQuiet = FALSE;
    SubWCRev_t SubStat;
    memset (&SubStat, 0, sizeof (SubStat));
    SubStat.bFolders = FALSE;

    SetDllDirectory(L"");
    CCrashReportTSVN crasher(L"SubWCRev " _T(APP_X64_STRING));

    if (argc >= 2 && argc <= 5)
    {
        // WC path is always first argument.
        wc = argv[1];
    }
    if (argc == 4 || argc == 5)
    {
        // SubWCRev Path Tmpl.in Tmpl.out [-params]
        src = argv[2];
        dst = argv[3];
        if (!PathFileExists(src))
        {
            _tprintf(_T("File '%s' does not exist\n"), src);
            return ERR_FNF;     // file does not exist
        }
    }
    if (argc == 3 || argc == 5)
    {
        // SubWCRev Path -params
        // SubWCRev Path Tmpl.in Tmpl.out -params
        const TCHAR * Params = argv[argc-1];
        if (Params[0] == '-')
        {
            if (_tcschr(Params, 'q') != 0)
                bQuiet = TRUE;
            if (_tcschr(Params, 'n') != 0)
                bErrOnMods = TRUE;
            if (_tcschr(Params, 'N') != 0)
                bErrOnUnversioned = TRUE;
            if (_tcschr(Params, 'm') != 0)
                bErrOnMixed = TRUE;
            if (_tcschr(Params, 'd') != 0)
            {
                if ((dst != NULL) && PathFileExists(dst))
                {
                    _tprintf(_T("File '%s' already exists\n"), dst);
                    return ERR_OUT_EXISTS;
                }
            }
            // the 'f' option is useful to keep the revision which is inserted in
            // the file constant, even if there are commits on other branches.
            // For example, if you tag your working copy, then half a year later
            // do a fresh checkout of that tag, the folder in your working copy of
            // that tag will get the HEAD revision of the time you check out (or
            // do an update). The files alone however won't have their last-committed
            // revision changed at all.
            if (_tcschr(Params, 'f') != 0)
                SubStat.bFolders = TRUE;
            if (_tcschr(Params, 'e') != 0)
                SubStat.bExternals = TRUE;
            if (_tcschr(Params, 'E') != 0)
                SubStat.bExternalsNoMixedRevision = TRUE;
            if (_tcschr(Params, 'x') != 0)
                SubStat.bHexPlain = TRUE;
            if (_tcschr(Params, 'X') != 0)
                SubStat.bHexX = TRUE;
        }
        else
        {
            // Bad params - abort and display help.
            wc = NULL;
        }
    }

    if (wc == NULL)
    {
        _tprintf(_T("SubWCRev %d.%d.%d, Build %d - %s\n\n"),
            TSVN_VERMAJOR, TSVN_VERMINOR,
            TSVN_VERMICRO, TSVN_VERBUILD,
            _T(TSVN_PLATFORM));
        _putts(_T(HelpText1));
        _putts(_T(HelpText2));
        _putts(_T(HelpText3));
        _putts(_T(HelpText4));
        _putts(_T(HelpText5));
        return ERR_SYNTAX;
    }

    DWORD reqLen = GetFullPathName(wc, 0, NULL, NULL);
    TCHAR * fullPath = new TCHAR[reqLen+1];
    GetFullPathName(wc, reqLen, fullPath, NULL);
    // GetFullPathName() sometimes returns the full path with the wrong
    // case. This is not a problem on Windows since its filesystem is
    // case-insensitive. But for SVN that's a problem if the wrong case
    // is inside a working copy: the svn wc database is case sensitive.
    // To fix the casing of the path, we use a trick:
    // convert the path to its short form, then back to its long form.
    // That will fix the wrong casing of the path.
    int shortlen = GetShortPathName(fullPath, NULL, 0);
    if (shortlen)
    {
        std::unique_ptr<TCHAR[]> shortPath(new TCHAR[shortlen+1]);
        if (GetShortPathName(fullPath, shortPath.get(), shortlen+1))
        {
            delete [] fullPath;
            reqLen = GetLongPathName(shortPath.get(), NULL, 0);
            fullPath = new TCHAR[reqLen+1];
            GetLongPathName(shortPath.get(), fullPath, reqLen);
        }
    }
    wc = fullPath;
    if (dst)
    {
        reqLen = GetFullPathName(dst, 0, NULL, NULL);
        fullPath = new TCHAR[reqLen+1];
        GetFullPathName(dst, reqLen, fullPath, NULL);
        shortlen = GetShortPathName(fullPath, NULL, 0);
        if (shortlen)
        {
            std::unique_ptr<TCHAR[]> shortPath(new TCHAR[shortlen+1]);
            if (GetShortPathName(fullPath, shortPath.get(), shortlen+1))
            {
                delete [] fullPath;
                reqLen = GetLongPathName(shortPath.get(), NULL, 0);
                fullPath = new TCHAR[reqLen+1];
                GetLongPathName(shortPath.get(), fullPath, reqLen);
            }
        }
        dst = fullPath;
    }
    if (src)
    {
        reqLen = GetFullPathName(src, 0, NULL, NULL);
        fullPath = new TCHAR[reqLen+1];
        GetFullPathName(src, reqLen, fullPath, NULL);
        shortlen = GetShortPathName(fullPath, NULL, 0);
        if (shortlen)
        {
            std::unique_ptr<TCHAR[]> shortPath(new TCHAR[shortlen+1]);
            if (GetShortPathName(fullPath, shortPath.get(), shortlen+1))
            {
                delete [] fullPath;
                reqLen = GetLongPathName(shortPath.get(), NULL, 0);
                fullPath = new TCHAR[reqLen+1];
                GetLongPathName(shortPath.get(), fullPath, reqLen);
            }
        }
        src = fullPath;
    }

    if (!PathFileExists(wc))
    {
        _tprintf(_T("Directory or file '%s' does not exist\n"), wc);
        if (_tcschr(wc, '\"') != NULL) // dir contains a quotation mark
        {
            _tprintf(_T("The WorkingCopyPath contains a quotation mark.\n"));
            _tprintf(_T("this indicates a problem when calling SubWCRev from an interpreter which treats\n"));
            _tprintf(_T("a backslash char specially.\n"));
            _tprintf(_T("Try using double backslashes or insert a dot after the last backslash when\n"));
            _tprintf(_T("calling SubWCRev\n"));
            _tprintf(_T("Examples:\n"));
            _tprintf(_T("SubWCRev \"path to wc\\\\\"\n"));
            _tprintf(_T("SubWCRev \"path to wc\\.\"\n"));
        }
        delete [] wc;
        delete [] dst;
        delete [] src;
        return ERR_FNF;         // dir does not exist
    }
    char * pBuf = NULL;
    DWORD readlength = 0;
    size_t filelength = 0;
    size_t maxlength  = 0;
    if (dst != NULL)
    {
        // open the file and read the contents
        CAutoFile hFile = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
        if (!hFile)
        {
            _tprintf(_T("Unable to open input file '%s'\n"), src);
            delete [] wc;
            delete [] dst;
            delete [] src;
            return ERR_OPEN;        // error opening file
        }
        filelength = GetFileSize(hFile, NULL);
        if (filelength == INVALID_FILE_SIZE)
        {
            _tprintf(_T("Could not determine file size of '%s'\n"), src);
            delete [] wc;
            delete [] dst;
            delete [] src;
            return ERR_READ;
        }
        maxlength = filelength+4096;    // We might be increasing file size.
        pBuf = new char[maxlength];
        if (pBuf == NULL)
        {
            _tprintf(_T("Could not allocate enough memory!\n"));
            delete [] wc;
            delete [] dst;
            delete [] src;
            return ERR_ALLOC;
        }
        if (!ReadFile(hFile, pBuf, (DWORD)filelength, &readlength, NULL))
        {
            _tprintf(_T("Could not read the file '%s'\n"), src);
            delete [] pBuf;
            delete [] wc;
            delete [] dst;
            delete [] src;
            return ERR_READ;
        }
        if (readlength != filelength)
        {
            _tprintf(_T("Could not read the file '%s' to the end!\n"), src);
            delete [] pBuf;
            delete [] wc;
            delete [] dst;
            delete [] src;
            return ERR_READ;
        }

    }
    // Now check the status of every file in the working copy
    // and gather revision status information in SubStat.

    apr_pool_t * pool;
    svn_error_t * svnerr = NULL;
    svn_client_ctx_t * ctx;
    const char * internalpath;

    apr_initialize();
    svn_dso_initialize2();
    apr_pool_create_ex (&pool, NULL, abort_on_pool_failure, NULL);
    svn_client_create_context2(&ctx, NULL, pool);

    size_t ret = 0;
    getenv_s(&ret, NULL, 0, "SVN_ASP_DOT_NET_HACK");
    if (ret)
    {
        svn_wc_set_adm_dir("_svn", pool);
    }

    char *wc_utf8;
    wc_utf8 = Utf16ToUtf8(wc, pool);
    internalpath = svn_dirent_internal_style (wc_utf8, pool);

    svnerr = svn_status(    internalpath,   //path
                            &SubStat,       //status_baton
                            TRUE,           //noignore
                            ctx,
                            pool);

    if (svnerr)
    {
        svn_handle_error2(svnerr, stdout, FALSE, "SubWCRev : ");
    }
    TCHAR wcfullpath[MAX_PATH];
    LPTSTR dummy;
    GetFullPathName(wc, MAX_PATH, wcfullpath, &dummy);
    apr_status_t e = 0;
    if (svnerr)
        e = svnerr->apr_err;
    if (svnerr)
        svn_error_clear(svnerr);
    apr_terminate2();
    if (svnerr)
    {
        delete [] pBuf;
        delete [] wc;
        delete [] dst;
        delete [] src;
        if (e == SVN_ERR_WC_NOT_DIRECTORY)
            return ERR_NOWC;
        return ERR_SVN_ERR;
    }

    char wcfull_oem[MAX_PATH];
    CharToOem(wcfullpath, wcfull_oem);
    _tprintf(_T("SubWCRev: '%hs'\n"), wcfull_oem);


    if (bErrOnMods && SubStat.HasMods)
    {
        _tprintf(_T("Working copy has local modifications!\n"));
        delete [] pBuf;
        delete [] wc;
        delete [] dst;
        delete [] src;
        return ERR_SVN_MODS;
    }
    if (bErrOnUnversioned && SubStat.HasUnversioned)
    {
        _tprintf(_T("Working copy has unversioned items!\n"));
        delete [] pBuf;
        delete [] wc;
        delete [] dst;
        delete [] src;
        return ERR_SVN_UNVER;
    }

    if (bErrOnMixed && (SubStat.MinRev != SubStat.MaxRev))
    {
        if (SubStat.bHexPlain)
            _tprintf(_T("Working copy contains mixed revisions %LX:%LX!\n"), SubStat.MinRev, SubStat.MaxRev);
        else if (SubStat.bHexX)
            _tprintf(_T("Working copy contains mixed revisions %#LX:%#LX!\n"), SubStat.MinRev, SubStat.MaxRev);
        else
            _tprintf(_T("Working copy contains mixed revisions %Ld:%Ld!\n"), SubStat.MinRev, SubStat.MaxRev);
        delete [] pBuf;
        delete [] wc;
        delete [] dst;
        delete [] src;
        return ERR_SVN_MIXED;
    }

    if (!bQuiet)
    {
        if (SubStat.bHexPlain)
            _tprintf(_T("Last committed at revision %LX\n"), SubStat.CmtRev);
        else if (SubStat.bHexX)
            _tprintf(_T("Last committed at revision %#LX\n"), SubStat.CmtRev);
        else
            _tprintf(_T("Last committed at revision %Ld\n"), SubStat.CmtRev);

        if (SubStat.MinRev != SubStat.MaxRev)
        {
            if (SubStat.bHexPlain)
                _tprintf(_T("Mixed revision range %LX:%LX\n"), SubStat.MinRev, SubStat.MaxRev);
            else if (SubStat.bHexX)
                _tprintf(_T("Mixed revision range %#LX:%#LX\n"), SubStat.MinRev, SubStat.MaxRev);
            else
                _tprintf(_T("Mixed revision range %Ld:%Ld\n"), SubStat.MinRev, SubStat.MaxRev);
        }
        else
        {
            if (SubStat.bHexPlain)
                _tprintf(_T("Updated to revision %LX\n"), SubStat.MaxRev);
            else if (SubStat.bHexX)
                _tprintf(_T("Updated to revision %#LX\n"), SubStat.MaxRev);
            else
                _tprintf(_T("Updated to revision %Ld\n"), SubStat.MaxRev);
        }

        if (SubStat.HasMods)
        {
            _tprintf(_T("Local modifications found\n"));
        }

        if (SubStat.HasUnversioned)
        {
            _tprintf(_T("Unversioned items found\n"));
        }
    }

    if (dst == NULL)
    {
        delete [] pBuf;
        delete [] wc;
        delete [] dst;
        delete [] src;
        return 0;
    }

    // now parse the file contents for version defines.

    size_t index = 0;

    while (InsertRevision(VERDEF, pBuf, index, filelength, maxlength, -1, SubStat.CmtRev, &SubStat));
    index = 0;
    while (InsertRevisionW(TEXT(VERDEF), (wchar_t*)pBuf, index, filelength, maxlength, -1, SubStat.CmtRev, &SubStat));

    index = 0;
    while (InsertRevision(VERDEFAND, pBuf, index, filelength, maxlength, -1, SubStat.CmtRev, &SubStat));
    index = 0;
    while (InsertRevisionW(TEXT(VERDEFAND), (wchar_t*)pBuf, index, filelength, maxlength, -1, SubStat.CmtRev, &SubStat));

    index = 0;
    while (InsertRevision(VERDEFOFFSET1, pBuf, index, filelength, maxlength, -1, SubStat.CmtRev, &SubStat));
    index = 0;
    while (InsertRevisionW(TEXT(VERDEFOFFSET1), (wchar_t*)pBuf, index, filelength, maxlength, -1, SubStat.CmtRev, &SubStat));

    index = 0;
    while (InsertRevision(VERDEFOFFSET2, pBuf, index, filelength, maxlength, -1, SubStat.CmtRev, &SubStat));
    index = 0;
    while (InsertRevisionW(TEXT(VERDEFOFFSET2), (wchar_t*)pBuf, index, filelength, maxlength, -1, SubStat.CmtRev, &SubStat));

    index = 0;
    while (InsertRevision(RANGEDEF, pBuf, index, filelength, maxlength, SubStat.MinRev, SubStat.MaxRev, &SubStat));
    index = 0;
    while (InsertRevisionW(TEXT(RANGEDEF), (wchar_t*)pBuf, index, filelength, maxlength, SubStat.MinRev, SubStat.MaxRev, &SubStat));

    index = 0;
    while (InsertDate(DATEDEF, pBuf, index, filelength, maxlength, SubStat.CmtDate));
    index = 0;
    while (InsertDateW(TEXT(DATEDEF), (wchar_t*)pBuf, index, filelength, maxlength, SubStat.CmtDate));

    index = 0;
    while (InsertDate(DATEDEFUTC, pBuf, index, filelength, maxlength, SubStat.CmtDate));
    index = 0;
    while (InsertDateW(TEXT(DATEDEFUTC), (wchar_t*)pBuf, index, filelength, maxlength, SubStat.CmtDate));

    index = 0;
    while (InsertDate(DATEWFMTDEF, pBuf, index, filelength, maxlength, SubStat.CmtDate));
    index = 0;
    while (InsertDateW(TEXT(DATEWFMTDEF), (wchar_t*)pBuf, index, filelength, maxlength, SubStat.CmtDate));
    index = 0;
    while (InsertDate(DATEWFMTDEFUTC, pBuf, index, filelength, maxlength, SubStat.CmtDate));
    index = 0;
    while (InsertDateW(TEXT(DATEWFMTDEFUTC), (wchar_t*)pBuf, index, filelength, maxlength, SubStat.CmtDate));

    index = 0;
    while (InsertDate(NOWDEF, pBuf, index, filelength, maxlength, USE_TIME_NOW));
    index = 0;
    while (InsertDateW(TEXT(NOWDEF), (wchar_t*)pBuf, index, filelength, maxlength, USE_TIME_NOW));

    index = 0;
    while (InsertDate(NOWDEFUTC, pBuf, index, filelength, maxlength, USE_TIME_NOW));
    index = 0;
    while (InsertDateW(TEXT(NOWDEFUTC), (wchar_t*)pBuf, index, filelength, maxlength, USE_TIME_NOW));

    index = 0;
    while (InsertDate(NOWWFMTDEF, pBuf, index, filelength, maxlength, USE_TIME_NOW));
    index = 0;
    while (InsertDateW(TEXT(NOWWFMTDEF), (wchar_t*)pBuf, index, filelength, maxlength, USE_TIME_NOW));

    index = 0;
    while (InsertDate(NOWWFMTDEFUTC, pBuf, index, filelength, maxlength, USE_TIME_NOW));
    index = 0;
    while (InsertDateW(TEXT(NOWWFMTDEFUTC), (wchar_t*)pBuf, index, filelength, maxlength, USE_TIME_NOW));

    index = 0;
    while (InsertBoolean(MODDEF, pBuf, index, filelength, SubStat.HasMods));
    index = 0;
    while (InsertBooleanW(TEXT(MODDEF), (wchar_t*)pBuf, index, filelength, SubStat.HasMods));

    index = 0;
    while (InsertBoolean(UNVERDEF, pBuf, index, filelength, SubStat.HasUnversioned));
    index = 0;
    while (InsertBooleanW(TEXT(UNVERDEF), (wchar_t*)pBuf, index, filelength, SubStat.HasUnversioned));

    index = 0;
    while (InsertBoolean(MIXEDDEF, pBuf, index, filelength, (SubStat.MinRev != SubStat.MaxRev) || SubStat.bIsExternalMixed));
    index = 0;
    while (InsertBooleanW(TEXT(MIXEDDEF), (wchar_t*)pBuf, index, filelength, (SubStat.MinRev != SubStat.MaxRev) || SubStat.bIsExternalMixed));

    index = 0;
    while (InsertBoolean(EXTALLFIXED, pBuf, index, filelength, !SubStat.bIsExternalsNotFixed));
    index = 0;
    while (InsertBooleanW(TEXT(EXTALLFIXED), (wchar_t*)pBuf, index, filelength, !SubStat.bIsExternalsNotFixed));

    index = 0;
    while (InsertBoolean(ISTAGGED, pBuf, index, filelength, SubStat.bIsTagged));
    index = 0;
    while (InsertBooleanW(TEXT(ISTAGGED), (wchar_t*)pBuf, index, filelength, SubStat.bIsTagged));

    index = 0;
    while (InsertUrl(URLDEF, pBuf, index, filelength, maxlength, SubStat.Url));
    index = 0;
    while (InsertUrlW(TEXT(URLDEF), (wchar_t*)pBuf, index, filelength, maxlength, Utf8ToWide(SubStat.Url).c_str()));

    index = 0;
    while (InsertBoolean(ISINSVN, pBuf, index, filelength, SubStat.bIsSvnItem));
    index = 0;
    while (InsertBooleanW(TEXT(ISINSVN), (wchar_t*)pBuf, index, filelength, SubStat.bIsSvnItem));

    index = 0;
    while (InsertBoolean(NEEDSLOCK, pBuf, index, filelength, SubStat.LockData.NeedsLocks));
    index = 0;
    while (InsertBooleanW(TEXT(NEEDSLOCK), (wchar_t*)pBuf, index, filelength, SubStat.LockData.NeedsLocks));

    index = 0;
    while (InsertBoolean(ISLOCKED, pBuf, index, filelength,  SubStat.LockData.IsLocked));
    index = 0;
    while (InsertBooleanW(TEXT(ISLOCKED), (wchar_t*)pBuf, index, filelength,  SubStat.LockData.IsLocked));

    index = 0;
    while (InsertDate(LOCKDATE, pBuf, index, filelength, maxlength, SubStat.LockData.CreationDate));
    index = 0;
    while (InsertDateW(TEXT(LOCKDATE), (wchar_t*)pBuf, index, filelength, maxlength, SubStat.LockData.CreationDate));

    index = 0;
    while (InsertDate(LOCKDATEUTC, pBuf, index, filelength, maxlength, SubStat.LockData.CreationDate));
    index = 0;
    while (InsertDateW(TEXT(LOCKDATEUTC), (wchar_t*)pBuf, index, filelength, maxlength, SubStat.LockData.CreationDate));

    index = 0;
    while (InsertDate(LOCKWFMTDEF, pBuf, index, filelength, maxlength, SubStat.LockData.CreationDate));
    index = 0;
    while (InsertDateW(TEXT(LOCKWFMTDEF), (wchar_t*)pBuf, index, filelength, maxlength, SubStat.LockData.CreationDate));

    index = 0;
    while (InsertDate(LOCKWFMTDEFUTC, pBuf, index, filelength, maxlength, SubStat.LockData.CreationDate));
    index = 0;
    while (InsertDateW(TEXT(LOCKWFMTDEFUTC), (wchar_t*)pBuf, index, filelength, maxlength, SubStat.LockData.CreationDate));

    index = 0;
    while (InsertUrl(LOCKOWNER, pBuf, index, filelength, maxlength, SubStat.LockData.Owner));
    index = 0;
    while (InsertUrlW(TEXT(LOCKOWNER), (wchar_t*)pBuf, index, filelength, maxlength, Utf8ToWide(SubStat.LockData.Owner).c_str()));

    index = 0;
    while (InsertUrl(LOCKCOMMENT, pBuf, index, filelength, maxlength, SubStat.LockData.Comment));
    index = 0;
    while (InsertUrlW(TEXT(LOCKCOMMENT), (wchar_t*)pBuf, index, filelength, maxlength, Utf8ToWide(SubStat.LockData.Comment).c_str()));

    CAutoFile hFile = CreateFile(dst, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, NULL, NULL);
    if (!hFile)
    {
        _tprintf(_T("Unable to open output file '%s' for writing\n"), dst);
        delete [] pBuf;
        delete [] wc;
        delete [] dst;
        delete [] src;
        return ERR_OPEN;
    }

    size_t filelengthExisting = GetFileSize(hFile, NULL);
    BOOL sameFileContent = FALSE;
    if (filelength == filelengthExisting)
    {
        DWORD readlengthExisting = 0;
        char * pBufExisting = new char[filelength];
        if (!ReadFile(hFile, pBufExisting, (DWORD)filelengthExisting, &readlengthExisting, NULL))
        {
            _tprintf(_T("Could not read the file '%s'\n"), dst);
            delete [] pBuf;
            delete [] wc;
            delete [] dst;
            delete [] src;
            delete [] pBufExisting;
            return ERR_READ;
        }
        if (readlengthExisting != filelengthExisting)
        {
            _tprintf(_T("Could not read the file '%s' to the end!\n"), dst);
            delete [] pBuf;
            delete [] wc;
            delete [] dst;
            delete [] src;
            delete [] pBufExisting;
            return ERR_READ;
        }
        sameFileContent = (memcmp(pBuf, pBufExisting, filelength) == 0);
        delete [] pBufExisting;
    }

    // The file is only written if its contents would change.
    // this object prevents the timestamp from changing.
    if (!sameFileContent)
    {
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

        WriteFile(hFile, pBuf, (DWORD)filelength, &readlength, NULL);
        if (readlength != filelength)
        {
            _tprintf(_T("Could not write the file '%s' to the end!\n"), dst);
            delete [] pBuf;
            delete [] wc;
            delete [] dst;
            delete [] src;
            return ERR_READ;
        }

        if (!SetEndOfFile(hFile))
        {
            _tprintf(_T("Could not truncate the file '%s' to the end!\n"), dst);
            delete [] pBuf;
            delete [] wc;
            delete [] dst;
            delete [] src;
            return ERR_READ;
        }
    }
    delete [] pBuf;
    delete [] wc;
    delete [] dst;
    delete [] src;

    return 0;
}

