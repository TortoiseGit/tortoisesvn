// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2021, 2023 - TortoiseSVN

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
#include "../Utils/PathUtils.h"

#include <iostream>
#include <windows.h>
#include <shlwapi.h>
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
Usage: SubWCRev WCPath [SrcVersionFile DstVersionFile] [-nmdqfeExXFu]\n\
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
                       numbers in HEX with '0x' before them\n\
-F                 :   if given, does not use the .subwcrevignore file\n\
-u                 :   changes the console output to Unicode mode\n"

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
                after the +\n\
$WCREV-$        Highest committed revision number subtracted with the\n\
                number after the -\n\
$WCDATE$        Date of highest committed revision\n\
$WCDATE=$       Like $WCDATE$ with an added strftime format after the =\n\
$WCRANGE$       Update revision range\n\
$WCURL$         Repository URL of the working copy\n\
$REPOROOT$      The URL of the repository root\n\
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
$WCISTAGGED$    True if the repository URL contains the tags pattern\n\
$WCINSVN$       True if the item is versioned\n\
$WCNEEDSLOCK$   True if the svn:needs-lock property is set\n\
$WCISLOCKED$    True if the item is locked\n"
// End of multi-line help text.

#define VERDEF         "$WCREV$"
#define VERDEFAND      "$WCREV&"
#define VERDEFOFFSET1  "$WCREV-"
#define VERDEFOFFSET2  "$WCREV+"
#define DATEDEF        "$WCDATE$"
#define DATEDEFUTC     "$WCDATEUTC$"
#define DATEWFMTDEF    "$WCDATE="
#define DATEWFMTDEFUTC "$WCDATEUTC="
#define MODDEF         "$WCMODS?"
#define RANGEDEF       "$WCRANGE$"
#define MIXEDDEF       "$WCMIXED?"
#define EXTALLFIXED    "$WCEXTALLFIXED?"
#define ISTAGGED       "$WCISTAGGED?"
#define URLDEF         "$WCURL$"
#define REPOROOTDEF    "$REPOROOT$"
#define NOWDEF         "$WCNOW$"
#define NOWDEFUTC      "$WCNOWUTC$"
#define NOWWFMTDEF     "$WCNOW="
#define NOWWFMTDEFUTC  "$WCNOWUTC="
#define ISINSVN        "$WCINSVN?"
#define NEEDSLOCK      "$WCNEEDSLOCK?"
#define ISLOCKED       "$WCISLOCKED?"
#define LOCKDATE       "$WCLOCKDATE$"
#define LOCKDATEUTC    "$WCLOCKDATEUTC$"
#define LOCKWFMTDEF    "$WCLOCKDATE="
#define LOCKWFMTDEFUTC "$WCLOCKDATEUTC="
#define LOCKOWNER      "$WCLOCKOWNER$"
#define LOCKCOMMENT    "$WCLOCKCOMMENT$"
#define UNVERDEF       "$WCUNVER?"

// Internal error codes
// Note: these error codes are documented in /doc/source/en/TortoiseSVN/tsvn_subwcrev.xml
#define ERR_SYNTAX  1 // Syntax error
#define ERR_FNF     2 // File/folder not found
#define ERR_OPEN    3 // File open error
#define ERR_ALLOC   4 // Memory allocation error
#define ERR_READ    5 // File read/write/size error
#define ERR_SVN_ERR 6 // SVN error
// Documented error codes
#define ERR_SVN_MODS   7  // Local mods found (-n)
#define ERR_SVN_MIXED  8  // Mixed rev WC found (-m)
#define ERR_OUT_EXISTS 9  // Output file already exists (-d)
#define ERR_NOWC       10 // the path is not a working copy or part of one
#define ERR_SVN_UNVER  11 // Unversioned items found (-N)

// Value for apr_time_t to signify "now"
#define USE_TIME_NOW -2 // 0 and -1 might already be significant.

bool FindPlaceholder(const char *def, char *pBuf, size_t &index, size_t filelength)
{
    size_t defLen = strlen(def);
    while (index + defLen <= filelength)
    {
        if (memcmp(pBuf + index, def, defLen) == 0)
            return true;
        index++;
    }
    return false;
}
bool FindPlaceholderW(const wchar_t *def, wchar_t *pBuf, size_t &index, size_t filelength)
{
    size_t defLen = wcslen(def);
    while ((index + defLen) * sizeof(wchar_t) <= filelength)
    {
        if (memcmp(pBuf + index, def, defLen * sizeof(wchar_t)) == 0)
            return true;
        index++;
    }

    return false;
}

bool InsertRevision(const char *def, char *pBuf, size_t &index,
                    size_t &fileLength, size_t maxLength,
                    long minRev, long maxRev, SubWCRevT *subStat)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholder(def, pBuf, index, fileLength))
    {
        // No more matches found.
        return false;
    }
    ptrdiff_t exp = 0;
    if ((strcmp(def, VERDEFAND) == 0) || (strcmp(def, VERDEFOFFSET1) == 0) || (strcmp(def, VERDEFOFFSET2) == 0))
    {
        char  format[1024] = {0};
        char *pStart       = pBuf + index + strlen(def);
        char *pEnd         = pStart;

        while (*pEnd != '$')
        {
            pEnd++;
            if (pEnd - pBuf >= static_cast<long long>(fileLength))
                return false; // No terminator - malformed so give up.
        }
        if ((pEnd - pStart) > 1024)
        {
            return false; // value specifier too big
        }
        exp = pEnd - pStart + 1;
        memcpy(format, pStart, pEnd - pStart);
        unsigned long number = strtoul(format, nullptr, 0);
        if (strcmp(def, VERDEFAND) == 0)
        {
            if (minRev != -1)
                minRev &= number;
            maxRev &= number;
        }
        if (strcmp(def, VERDEFOFFSET1) == 0)
        {
            if (minRev != -1)
                minRev -= number;
            maxRev -= number;
        }
        if (strcmp(def, VERDEFOFFSET2) == 0)
        {
            if (minRev != -1)
                minRev += number;
            maxRev += number;
        }
    }
    // Format the text to insert at the placeholder
    char destBuf[40] = {0};
    if (minRev == -1 || minRev == maxRev)
    {
        if ((subStat) && (subStat->bHexPlain))
            sprintf_s(destBuf, "%lX", maxRev);
        else if ((subStat) && (subStat->bHexX))
            sprintf_s(destBuf, "%#lX", maxRev);
        else
            sprintf_s(destBuf, "%ld", maxRev);
    }
    else
    {
        if ((subStat) && (subStat->bHexPlain))
            sprintf_s(destBuf, "%lX:%lX", minRev, maxRev);
        else if ((subStat) && (subStat->bHexX))
            sprintf_s(destBuf, "%#lX:%#lX", minRev, maxRev);
        else
            sprintf_s(destBuf, "%ld:%ld", minRev, maxRev);
    }
    // Replace the $WCxxx$ string with the actual revision number
    char *    pBuild    = pBuf + index;
    ptrdiff_t expansion = strlen(destBuf) - exp - strlen(def);
    if (expansion < 0)
    {
        memmove(pBuild, pBuild - expansion, fileLength - ((pBuild - expansion) - pBuf));
    }
    else if (expansion > 0)
    {
        // Check for buffer overflow
        if (maxLength < expansion + fileLength)
            return false;
        memmove(pBuild + expansion, pBuild, fileLength - (pBuild - pBuf));
    }
    memmove(pBuild, destBuf, strlen(destBuf));
    fileLength += expansion;
    return true;
}
bool InsertRevisionW(const wchar_t *def, wchar_t *pBuf, size_t &index,
                     size_t &fileLength, size_t maxLength,
                     long minRev, long maxRev, SubWCRevT *subStat)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholderW(def, pBuf, index, fileLength))
    {
        // No more matches found.
        return false;
    }

    ptrdiff_t exp = 0;
    if ((wcscmp(def, TEXT(VERDEFAND)) == 0) || (wcscmp(def, TEXT(VERDEFOFFSET1)) == 0) || (wcscmp(def, TEXT(VERDEFOFFSET2)) == 0))
    {
        wchar_t  format[1024] = {0};
        wchar_t *pStart       = pBuf + index + wcslen(def);
        wchar_t *pEnd         = pStart;

        while (*pEnd != '$')
        {
            pEnd++;
            if (static_cast<long long>(pEnd - pBuf) * static_cast<long long>(sizeof(wchar_t)) >= static_cast<long long>(fileLength))
                return false; // No terminator - malformed so give up.
        }
        if ((pEnd - pStart) > 1024)
        {
            return false; // Format specifier too big
        }
        exp = pEnd - pStart + 1;
        memcpy(format, pStart, (pEnd - pStart) * sizeof(wchar_t));
        unsigned long number = wcstoul(format, nullptr, 0);
        if (wcscmp(def, TEXT(VERDEFAND)) == 0)
        {
            if (minRev != -1)
                minRev &= number;
            maxRev &= number;
        }
        if (wcscmp(def, TEXT(VERDEFOFFSET1)) == 0)
        {
            if (minRev != -1)
                minRev -= number;
            maxRev -= number;
        }
        if (wcscmp(def, TEXT(VERDEFOFFSET2)) == 0)
        {
            if (minRev != -1)
                minRev += number;
            maxRev += number;
        }
    }

    // Format the text to insert at the placeholder
    wchar_t destBuf[40];
    if (minRev == -1 || minRev == maxRev)
    {
        if ((subStat) && (subStat->bHexPlain))
            swprintf_s(destBuf, L"%lX", maxRev);
        else if ((subStat) && (subStat->bHexX))
            swprintf_s(destBuf, L"%#lX", maxRev);
        else
            swprintf_s(destBuf, L"%ld", maxRev);
    }
    else
    {
        if ((subStat) && (subStat->bHexPlain))
            swprintf_s(destBuf, L"%lX:%lX", minRev, maxRev);
        else if ((subStat) && (subStat->bHexX))
            swprintf_s(destBuf, L"%#lX:%#lX", minRev, maxRev);
        else
            swprintf_s(destBuf, L"%ld:%ld", minRev, maxRev);
    }
    // Replace the $WCxxx$ string with the actual revision number
    wchar_t * pBuild    = pBuf + index;
    ptrdiff_t expansion = wcslen(destBuf) - exp - wcslen(def);
    if (expansion < 0)
    {
        memmove(pBuild, pBuild - expansion, (fileLength - ((pBuild - expansion) - pBuf) * sizeof(wchar_t)));
    }
    else if (expansion > 0)
    {
        // Check for buffer overflow
        if (maxLength < expansion * sizeof(wchar_t) + fileLength)
            return false;
        memmove(pBuild + expansion, pBuild, (fileLength - (pBuild - pBuf) * sizeof(wchar_t)));
    }
    memmove(pBuild, destBuf, wcslen(destBuf) * sizeof(wchar_t));
    fileLength += (expansion * sizeof(wchar_t));
    return true;
}

void invalidParameterDonothing(
    const wchar_t * /*expression*/,
    const wchar_t * /*function*/,
    const wchar_t * /*file*/,
    unsigned int /*line*/,
    uintptr_t /*pReserved*/
)
{
    // do nothing
}

bool InsertDate(const char *def, char *pBuf, size_t &index,
                size_t &fileLength, size_t maxLength,
                apr_time_t dateSVN)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholder(def, pBuf, index, fileLength))
    {
        // No more matches found.
        return false;
    }
    // Format the text to insert at the placeholder
    __time64_t tTime;
    if (dateSVN == USE_TIME_NOW)
        _time64(&tTime);
    else
        tTime = dateSVN / 1000000L;

    struct tm newTime;
    if (strstr(def, "UTC"))
    {
        if (_gmtime64_s(&newTime, &tTime))
            return false;
    }
    else
    {
        if (_localtime64_s(&newTime, &tTime))
            return false;
    }
    char      destBuf[1024] = {0};
    char *    pBuild        = pBuf + index;
    ptrdiff_t expansion     = 0;
    if ((strcmp(def, DATEWFMTDEF) == 0) || (strcmp(def, NOWWFMTDEF) == 0) || (strcmp(def, LOCKWFMTDEF) == 0) ||
        (strcmp(def, DATEWFMTDEFUTC) == 0) || (strcmp(def, NOWWFMTDEFUTC) == 0) || (strcmp(def, LOCKWFMTDEFUTC) == 0))
    {
        // Format the date/time according to the supplied strftime format string
        char  format[1024] = {0};
        char *pStart       = pBuf + index + strlen(def);
        char *pEnd         = pStart;

        while (*pEnd != '$')
        {
            pEnd++;
            if (pEnd - pBuf >= static_cast<long long>(fileLength))
                return false; // No terminator - malformed so give up.
        }
        if ((pEnd - pStart) > 1024)
        {
            return false; // Format specifier too big
        }
        memcpy(format, pStart, pEnd - pStart);

        // to avoid wcsftime aborting if the user specified an invalid time format,
        // we set a custom invalid parameter handler that does nothing at all:
        // that makes wcsftime do nothing and set errno to EINVAL.
        // we restore the invalid parameter handler right after
        _invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(invalidParameterDonothing);

        if (strftime(destBuf, 1024, format, &newTime) == 0)
        {
            if (errno == EINVAL)
                strcpy_s(destBuf, "Invalid Time Format Specified");
        }
        _set_invalid_parameter_handler(oldHandler);
        expansion = strlen(destBuf) - (strlen(def) + pEnd - pStart + 1);
    }
    else
    {
        // Format the date/time in international format as yyyy/mm/dd hh:mm:ss
        sprintf_s(destBuf, "%04d/%02d/%02d %02d:%02d:%02d",
                  newTime.tm_year + 1900,
                  newTime.tm_mon + 1,
                  newTime.tm_mday,
                  newTime.tm_hour,
                  newTime.tm_min,
                  newTime.tm_sec);

        expansion = strlen(destBuf) - strlen(def);
    }
    // Replace the def string with the actual commit date
    if (expansion < 0)
    {
        memmove(pBuild, pBuild - expansion, fileLength - ((pBuild - expansion) - pBuf));
    }
    else if (expansion > 0)
    {
        // Check for buffer overflow
        if (maxLength < expansion + fileLength)
            return false;
        memmove(pBuild + expansion, pBuild, fileLength - (pBuild - pBuf));
    }
    memmove(pBuild, destBuf, strlen(destBuf));
    fileLength += expansion;
    return true;
}
bool InsertDateW(const wchar_t *def, wchar_t *pBuf, size_t &index,
                 size_t &fileLength, size_t maxLength,
                 apr_time_t dateSVN)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholderW(def, pBuf, index, fileLength))
    {
        // No more matches found.
        return false;
    }
    // Format the text to insert at the placeholder
    __time64_t tTime;
    if (dateSVN == USE_TIME_NOW)
        _time64(&tTime);
    else
        tTime = dateSVN / 1000000L;

    struct tm newTime;
    if (wcsstr(def, L"UTC"))
    {
        if (_gmtime64_s(&newTime, &tTime))
            return false;
    }
    else
    {
        if (_localtime64_s(&newTime, &tTime))
            return false;
    }
    wchar_t   destBuf[1024];
    wchar_t * pBuild    = pBuf + index;
    ptrdiff_t expansion = 0;
    if ((wcscmp(def, TEXT(DATEWFMTDEF)) == 0) || (wcscmp(def, TEXT(NOWWFMTDEF)) == 0) || (wcscmp(def, TEXT(LOCKWFMTDEF)) == 0) ||
        (wcscmp(def, TEXT(DATEWFMTDEFUTC)) == 0) || (wcscmp(def, TEXT(NOWWFMTDEFUTC)) == 0) || (wcscmp(def, TEXT(LOCKWFMTDEFUTC)) == 0))
    {
        // Format the date/time according to the supplied strftime format string
        wchar_t  format[1024] = {0};
        wchar_t *pStart       = pBuf + index + wcslen(def);
        wchar_t *pEnd         = pStart;

        while (*pEnd != '$')
        {
            pEnd++;
            if (static_cast<long long>(pEnd - pBuf) * static_cast<long long>(sizeof(wchar_t)) >= static_cast<long long>(fileLength))
                return false; // No terminator - malformed so give up.
        }
        if ((pEnd - pStart) > 1024)
        {
            return false; // Format specifier too big
        }
        memcpy(format, pStart, (pEnd - pStart) * sizeof(wchar_t));

        // to avoid wcsftime aborting if the user specified an invalid time format,
        // we set a custom invalid parameter handler that does nothing at all:
        // that makes wcsftime do nothing and set errno to EINVAL.
        // we restore the invalid parameter handler right after
        _invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(invalidParameterDonothing);

        if (wcsftime(destBuf, 1024, format, &newTime) == 0)
        {
            if (errno == EINVAL)
                wcscpy_s(destBuf, L"Invalid Time Format Specified");
        }
        _set_invalid_parameter_handler(oldHandler);

        expansion = wcslen(destBuf) - (wcslen(def) + pEnd - pStart + 1);
    }
    else
    {
        // Format the date/time in international format as yyyy/mm/dd hh:mm:ss
        swprintf_s(destBuf, L"%04d/%02d/%02d %02d:%02d:%02d",
                   newTime.tm_year + 1900,
                   newTime.tm_mon + 1,
                   newTime.tm_mday,
                   newTime.tm_hour,
                   newTime.tm_min,
                   newTime.tm_sec);

        expansion = wcslen(destBuf) - wcslen(def);
    }
    // Replace the def string with the actual commit date
    if (expansion < 0)
    {
        memmove(pBuild, pBuild - expansion, (fileLength - ((pBuild - expansion) - pBuf) * sizeof(wchar_t)));
    }
    else if (expansion > 0)
    {
        // Check for buffer overflow
        if (maxLength < expansion * sizeof(wchar_t) + fileLength)
            return false;
        memmove(pBuild + expansion, pBuild, (fileLength - (pBuild - pBuf) * sizeof(wchar_t)));
    }
    memmove(pBuild, destBuf, wcslen(destBuf) * sizeof(wchar_t));
    fileLength += expansion * sizeof(wchar_t);
    return true;
}

bool InsertUrl(const char *def, char *pBuf, size_t &index,
               size_t &fileLength, size_t maxLength,
               char *pUrl)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholder(def, pBuf, index, fileLength))
    {
        // No more matches found.
        return false;
    }
    // Replace the $WCURL$ string with the actual URL
    char *    pBuild    = pBuf + index;
    ptrdiff_t expansion = strlen(pUrl) - strlen(def);
    if (expansion < 0)
    {
        memmove(pBuild, pBuild - expansion, fileLength - ((pBuild - expansion) - pBuf));
    }
    else if (expansion > 0)
    {
        // Check for buffer overflow
        if (maxLength < expansion + fileLength)
            return false;
        memmove(pBuild + expansion, pBuild, fileLength - (pBuild - pBuf));
    }
    memmove(pBuild, pUrl, strlen(pUrl));
    fileLength += expansion;
    return true;
}
bool InsertUrlW(const wchar_t *def, wchar_t *pBuf, size_t &index,
                size_t &fileLength, size_t maxLength,
                const wchar_t *pUrl)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholderW(def, pBuf, index, fileLength))
    {
        // No more matches found.
        return false;
    }
    // Replace the $WCURL$ string with the actual URL
    wchar_t * pBuild    = pBuf + index;
    ptrdiff_t expansion = wcslen(pUrl) - wcslen(def);
    if (expansion < 0)
    {
        memmove(pBuild, pBuild - expansion, (fileLength - ((pBuild - expansion) - pBuf) * sizeof(wchar_t)));
    }
    else if (expansion > 0)
    {
        // Check for buffer overflow
        if (maxLength < expansion * sizeof(wchar_t) + fileLength)
            return false;
        memmove(pBuild + expansion, pBuild, (fileLength - (pBuild - pBuf) * sizeof(wchar_t)));
    }
    memmove(pBuild, pUrl, wcslen(pUrl) * sizeof(wchar_t));
    fileLength += expansion * sizeof(wchar_t);
    return true;
}

int InsertBoolean(const char *def, char *pBuf, size_t &index, size_t &fileLength, BOOL isTrue)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholder(def, pBuf, index, fileLength))
    {
        // No more matches found.
        return false;
    }
    // Look for the terminating '$' character
    char *pBuild = pBuf + index;
    char *pEnd   = pBuild + 1;
    while (*pEnd != '$')
    {
        pEnd++;
        if (pEnd - pBuf >= static_cast<long long>(fileLength))
            return false; // No terminator - malformed so give up.
    }

    // Look for the ':' dividing TrueText from FalseText
    char *pSplit = pBuild + 1;
    // this loop is guaranteed to terminate due to test above.
    while (*pSplit != ':' && *pSplit != '$')
        pSplit++;

    if (*pSplit == '$')
        return false; // No split - malformed so give up.

    if (isTrue)
    {
        // Replace $WCxxx?TrueText:FalseText$ with TrueText
        // Remove :FalseText$
        memmove(pSplit, pEnd + 1, fileLength - (pEnd + 1 - pBuf));
        fileLength -= (pEnd + 1 - pSplit);
        // Remove $WCxxx?
        size_t defLen = strlen(def);
        memmove(pBuild, pBuild + defLen, fileLength - (pBuild + defLen - pBuf));
        fileLength -= defLen;
    }
    else
    {
        // Replace $WCxxx?TrueText:FalseText$ with FalseText
        // Remove terminating $
        memmove(pEnd, pEnd + 1, fileLength - (pEnd + 1 - pBuf));
        fileLength--;
        // Remove $WCxxx?TrueText:
        memmove(pBuild, pSplit + 1, fileLength - (pSplit + 1 - pBuf));
        fileLength -= (pSplit + 1 - pBuild);
    }
    return true;
}
bool InsertBooleanW(const wchar_t *def, wchar_t *pBuf, size_t &index, size_t &fileLength, BOOL isTrue)
{
    // Search for first occurrence of def in the buffer, starting at index.
    if (!FindPlaceholderW(def, pBuf, index, fileLength))
    {
        // No more matches found.
        return false;
    }
    // Look for the terminating '$' character
    wchar_t *pBuild = pBuf + index;
    wchar_t *pEnd   = pBuild + 1;
    while (*pEnd != '$')
    {
        pEnd++;
        if (pEnd - pBuf >= static_cast<long long>(fileLength))
            return false; // No terminator - malformed so give up.
    }

    // Look for the ':' dividing TrueText from FalseText
    wchar_t *pSplit = pBuild + 1;
    // this loop is guaranteed to terminate due to test above.
    while (*pSplit != ':' && *pSplit != '$')
        pSplit++;

    if (*pSplit == '$')
        return false; // No split - malformed so give up.

    if (isTrue)
    {
        // Replace $WCxxx?TrueText:FalseText$ with TrueText
        // Remove :FalseText$
        memmove(pSplit, pEnd + 1, (fileLength - (pEnd + 1 - pBuf)) * sizeof(wchar_t));
        fileLength -= ((pEnd + 1 - pSplit) * sizeof(wchar_t));
        // Remove $WCxxx?
        size_t defLen = wcslen(def);
        memmove(pBuild, pBuild + defLen, (fileLength - (pBuild + defLen - pBuf) * sizeof(wchar_t)));
        fileLength -= (defLen * sizeof(wchar_t));
    }
    else
    {
        // Replace $WCxxx?TrueText:FalseText$ with FalseText
        // Remove terminating $
        memmove(pEnd, pEnd + 1, (fileLength - (pEnd + 1 - pBuf) * sizeof(wchar_t)));
        fileLength -= sizeof(wchar_t);
        // Remove $WCxxx?TrueText:
        memmove(pBuild, pSplit + 1, (fileLength - (pSplit + 1 - pBuf) * sizeof(wchar_t)));
        fileLength -= ((pSplit + 1 - pBuild) * sizeof(wchar_t));
    }
    return true;
}

#pragma warning(push)
#pragma warning(disable : 4702)
int abortOnPoolFailure(int /*retcode*/)
{
    abort();
    // ReSharper disable once CppUnreachableCode
    return -1;
}
#pragma warning(pop)

int wmain(int argc, wchar_t *argv[])
{
    // we have three parameters
    const wchar_t *src                = nullptr;
    const wchar_t *dst                = nullptr;
    const wchar_t *wc                 = nullptr;
    BOOL           bErrOnMods         = FALSE;
    BOOL           bErrOnUnversioned  = FALSE;
    BOOL           bErrOnMixed        = FALSE;
    BOOL           bQuiet             = FALSE;
    BOOL           bUseSubWCRevIgnore = TRUE;
    SubWCRevT      subStat;

    SetDllDirectory(L"");
    CCrashReportTSVN crasher(L"SubWCRev " TEXT(APP_X64_STRING));

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
            wprintf(L"File '%s' does not exist\n", src);
            return ERR_FNF; // file does not exist
        }
    }
    if (argc == 3 || argc == 5)
    {
        // SubWCRev Path -params
        // SubWCRev Path Tmpl.in Tmpl.out -params
        const wchar_t *params = argv[argc - 1];
        if (params[0] == '-')
        {
            if (wcschr(params, 'u') != nullptr)
                _setmode(_fileno(stdout), _O_U16TEXT);
            if (wcschr(params, 'q') != nullptr)
                bQuiet = TRUE;
            if (wcschr(params, 'n') != nullptr)
                bErrOnMods = TRUE;
            if (wcschr(params, 'N') != nullptr)
                bErrOnUnversioned = TRUE;
            if (wcschr(params, 'm') != nullptr)
                bErrOnMixed = TRUE;
            if (wcschr(params, 'd') != nullptr)
            {
                if ((dst != nullptr) && PathFileExists(dst))
                {
                    wprintf(L"File '%s' already exists\n", dst);
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
            if (wcschr(params, 'f') != nullptr)
                subStat.bFolders = TRUE;
            if (wcschr(params, 'e') != nullptr)
                subStat.bExternals = TRUE;
            if (wcschr(params, 'E') != nullptr)
                subStat.bExternalsNoMixedRevision = TRUE;
            if (wcschr(params, 'x') != nullptr)
                subStat.bHexPlain = TRUE;
            if (wcschr(params, 'X') != nullptr)
                subStat.bHexX = TRUE;
            if (wcschr(params, 'F') != nullptr)
                bUseSubWCRevIgnore = FALSE;
        }
        else
        {
            // Bad params - abort and display help.
            wc = nullptr;
        }
    }

    if (wc == nullptr)
    {
        wprintf(L"SubWCRev %d.%d.%d, Build %d - %s\n\n",
                TSVN_VERMAJOR, TSVN_VERMINOR,
                TSVN_VERMICRO, TSVN_VERBUILD,
                TEXT(TSVN_PLATFORM));
        _putws(TEXT(HelpText1));
        _putws(TEXT(HelpText2));
        _putws(TEXT(HelpText3));
        _putws(TEXT(HelpText4));
        _putws(TEXT(HelpText5));
        return ERR_SYNTAX;
    }

    DWORD reqLen     = GetFullPathName(wc, 0, nullptr, nullptr);
    auto  wcFullPath = std::make_unique<wchar_t[]>(reqLen + 1LL);
    GetFullPathName(wc, reqLen, wcFullPath.get(), nullptr);
    // GetFullPathName() sometimes returns the full path with the wrong
    // case. This is not a problem on Windows since its filesystem is
    // case-insensitive. But for SVN that's a problem if the wrong case
    // is inside a working copy: the svn wc database is case sensitive.
    // To fix the casing of the path, we use a trick:
    // convert the path to its short form, then back to its long form.
    // That will fix the wrong casing of the path.
    int shortLen = GetShortPathName(wcFullPath.get(), nullptr, 0);
    if (shortLen)
    {
        auto shortPath = std::make_unique<wchar_t[]>(shortLen + 1LL);
        if (GetShortPathName(wcFullPath.get(), shortPath.get(), shortLen + 1))
        {
            reqLen     = GetLongPathName(shortPath.get(), nullptr, 0);
            wcFullPath = std::make_unique<wchar_t[]>(reqLen + 1LL);
            GetLongPathName(shortPath.get(), wcFullPath.get(), reqLen);
        }
    }
    wc = wcFullPath.get();
    std::wstring dstFullPath;
    if (dst)
    {
        dstFullPath = CPathUtils::GetLongPathname(dst);
        dst         = dstFullPath.c_str();
    }
    std::wstring srcFullPath;
    if (src)
    {
        srcFullPath = CPathUtils::GetLongPathname(src);
        src         = srcFullPath.c_str();
    }

    if (!PathFileExists(wc))
    {
        wprintf(L"Directory or file '%s' does not exist\n", wc);
        if (wcschr(wc, '\"') != nullptr) // dir contains a quotation mark
        {
            wprintf(L"The WorkingCopyPath contains a quotation mark.\n");
            wprintf(L"this indicates a problem when calling SubWCRev from an interpreter which treats\n");
            wprintf(L"a backslash char specially.\n");
            wprintf(L"Try using double backslashes or insert a dot after the last backslash when\n");
            wprintf(L"calling SubWCRev\n");
            wprintf(L"Examples:\n");
            wprintf(L"SubWCRev \"path to wc\\\\\"\n");
            wprintf(L"SubWCRev \"path to wc\\.\"\n");
        }
        return ERR_FNF; // dir does not exist
    }
    std::unique_ptr<char[]> pBuf       = nullptr;
    DWORD                   readLength = 0;
    size_t                  fileLength = 0;
    size_t                  maxLength  = 0;
    if (dst != nullptr)
    {
        // open the file and read the contents
        CAutoFile hFile = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, NULL, nullptr);
        if (!hFile)
        {
            wprintf(L"Unable to open input file '%s'\n", src);
            return ERR_OPEN; // error opening file
        }
        fileLength = GetFileSize(hFile, nullptr);
        if (fileLength == INVALID_FILE_SIZE)
        {
            wprintf(L"Could not determine file size of '%s'\n", src);
            return ERR_READ;
        }
        maxLength = fileLength + 4096; // We might be increasing file size.
        pBuf      = std::make_unique<char[]>(maxLength);
        if (pBuf == nullptr)
        {
            wprintf(L"Could not allocate enough memory!\n");
            return ERR_ALLOC;
        }
        if (!ReadFile(hFile, pBuf.get(), static_cast<DWORD>(fileLength), &readLength, nullptr))
        {
            wprintf(L"Could not read the file '%s'\n", src);
            return ERR_READ;
        }
        if (readLength != fileLength)
        {
            wprintf(L"Could not read the file '%s' to the end!\n", src);
            return ERR_READ;
        }
    }
    // Now check the status of every file in the working copy
    // and gather revision status information in SubStat.

    apr_pool_t *      pool         = nullptr;
    svn_error_t *     svnErr       = nullptr;
    svn_client_ctx_t *ctx          = nullptr;
    const char *      internalPath = nullptr;
    apr_hash_t *      config       = nullptr;
    ;
    apr_initialize();
    svn_dso_initialize2();
    apr_pool_create_ex(&pool, nullptr, abortOnPoolFailure, nullptr);
    svn_config_get_config(&(config), nullptr, pool);
    svn_client_create_context2(&ctx, config, pool);

    size_t ret = 0;
    getenv_s(&ret, nullptr, 0, "SVN_ASP_DOT_NET_HACK");
    if (ret)
    {
        svn_wc_set_adm_dir("_svn", pool);
    }

    char *wcUTF8 = Utf16ToUtf8(wc, pool);
    internalPath = svn_dirent_internal_style(wcUTF8, pool);
    if (bUseSubWCRevIgnore)
    {
        const char *wcroot;
        svnErr = svn_client_get_wc_root(&wcroot, internalPath, ctx, pool, pool);
        if ((svnErr == nullptr) && wcroot)
            loadIgnorePatterns(wcroot, &subStat);
        svn_error_clear(svnErr);
        loadIgnorePatterns(internalPath, &subStat);
    }

    svnErr = svnStatus(internalPath, //path
                       &subStat,     //status_baton
                       TRUE,         //noignore
                       ctx,
                       pool);

    if (svnErr)
    {
        svn_handle_error2(svnErr, stdout, FALSE, "SubWCRev : ");
    }
    wchar_t wcFullPathBuf[MAX_PATH] = {0};
    LPWSTR  dummy;
    GetFullPathName(wc, MAX_PATH, wcFullPathBuf, &dummy);
    apr_status_t e = 0;
    if (svnErr)
    {
        e = svnErr->apr_err;
        svn_error_clear(svnErr);
    }
    apr_terminate2();
    if (svnErr)
    {
        if (e == SVN_ERR_WC_NOT_DIRECTORY)
            return ERR_NOWC;
        return ERR_SVN_ERR;
    }

    char wcFullOem[MAX_PATH] = {0};
    CharToOem(wcFullPathBuf, wcFullOem);
    wprintf(L"SubWCRev: '%hs'\n", wcFullOem);

    if (bErrOnMods && subStat.hasMods)
    {
        wprintf(L"Working copy has local modifications!\n");
        return ERR_SVN_MODS;
    }
    if (bErrOnUnversioned && subStat.hasUnversioned)
    {
        wprintf(L"Working copy has unversioned items!\n");
        return ERR_SVN_UNVER;
    }

    if (bErrOnMixed && (subStat.minRev != subStat.maxRev))
    {
        if (subStat.bHexPlain)
            wprintf(L"Working copy contains mixed revisions %lX:%lX!\n", subStat.minRev, subStat.maxRev);
        else if (subStat.bHexX)
            wprintf(L"Working copy contains mixed revisions %#lX:%#lX!\n", subStat.minRev, subStat.maxRev);
        else
            wprintf(L"Working copy contains mixed revisions %ld:%ld!\n", subStat.minRev, subStat.maxRev);
        return ERR_SVN_MIXED;
    }

    if (!bQuiet)
    {
        if (subStat.bHexPlain)
            wprintf(L"Last committed at revision %lX\n", subStat.cmtRev);
        else if (subStat.bHexX)
            wprintf(L"Last committed at revision %#lX\n", subStat.cmtRev);
        else
            wprintf(L"Last committed at revision %ld\n", subStat.cmtRev);

        if (subStat.minRev != subStat.maxRev)
        {
            if (subStat.bHexPlain)
                wprintf(L"Mixed revision range %lX:%lX\n", subStat.minRev, subStat.maxRev);
            else if (subStat.bHexX)
                wprintf(L"Mixed revision range %#lX:%#lX\n", subStat.minRev, subStat.maxRev);
            else
                wprintf(L"Mixed revision range %ld:%ld\n", subStat.minRev, subStat.maxRev);
        }
        else
        {
            if (subStat.bHexPlain)
                wprintf(L"Updated to revision %lX\n", subStat.maxRev);
            else if (subStat.bHexX)
                wprintf(L"Updated to revision %#lX\n", subStat.maxRev);
            else
                wprintf(L"Updated to revision %ld\n", subStat.maxRev);
        }

        if (subStat.hasMods)
        {
            wprintf(L"Local modifications found\n");
        }

        if (subStat.hasUnversioned)
        {
            wprintf(L"Unversioned items found\n");
        }
    }

    if (dst == nullptr)
    {
        return 0;
    }

    // now parse the file contents for version defines.

    size_t index = 0;

    while (InsertRevision(VERDEF, pBuf.get(), index, fileLength, maxLength, -1, subStat.cmtRev, &subStat))
        ;
    index = 0;
    while (InsertRevisionW(TEXT(VERDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, -1, subStat.cmtRev, &subStat))
        ;

    index = 0;
    while (InsertRevision(VERDEFAND, pBuf.get(), index, fileLength, maxLength, -1, subStat.cmtRev, &subStat))
        ;
    index = 0;
    while (InsertRevisionW(TEXT(VERDEFAND), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, -1, subStat.cmtRev, &subStat))
        ;

    index = 0;
    while (InsertRevision(VERDEFOFFSET1, pBuf.get(), index, fileLength, maxLength, -1, subStat.cmtRev, &subStat))
        ;
    index = 0;
    while (InsertRevisionW(TEXT(VERDEFOFFSET1), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, -1, subStat.cmtRev, &subStat))
        ;

    index = 0;
    while (InsertRevision(VERDEFOFFSET2, pBuf.get(), index, fileLength, maxLength, -1, subStat.cmtRev, &subStat))
        ;
    index = 0;
    while (InsertRevisionW(TEXT(VERDEFOFFSET2), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, -1, subStat.cmtRev, &subStat))
        ;

    index = 0;
    while (InsertRevision(RANGEDEF, pBuf.get(), index, fileLength, maxLength, subStat.minRev, subStat.maxRev, &subStat))
        ;
    index = 0;
    while (InsertRevisionW(TEXT(RANGEDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, subStat.minRev, subStat.maxRev, &subStat))
        ;

    index = 0;
    while (InsertDate(DATEDEF, pBuf.get(), index, fileLength, maxLength, subStat.cmtDate))
        ;
    index = 0;
    while (InsertDateW(TEXT(DATEDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, subStat.cmtDate))
        ;

    index = 0;
    while (InsertDate(DATEDEFUTC, pBuf.get(), index, fileLength, maxLength, subStat.cmtDate))
        ;
    index = 0;
    while (InsertDateW(TEXT(DATEDEFUTC), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, subStat.cmtDate))
        ;

    index = 0;
    while (InsertDate(DATEWFMTDEF, pBuf.get(), index, fileLength, maxLength, subStat.cmtDate))
        ;
    index = 0;
    while (InsertDateW(TEXT(DATEWFMTDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, subStat.cmtDate))
        ;
    index = 0;
    while (InsertDate(DATEWFMTDEFUTC, pBuf.get(), index, fileLength, maxLength, subStat.cmtDate))
        ;
    index = 0;
    while (InsertDateW(TEXT(DATEWFMTDEFUTC), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, subStat.cmtDate))
        ;

    index = 0;
    while (InsertDate(NOWDEF, pBuf.get(), index, fileLength, maxLength, USE_TIME_NOW))
        ;
    index = 0;
    while (InsertDateW(TEXT(NOWDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, USE_TIME_NOW))
        ;

    index = 0;
    while (InsertDate(NOWDEFUTC, pBuf.get(), index, fileLength, maxLength, USE_TIME_NOW))
        ;
    index = 0;
    while (InsertDateW(TEXT(NOWDEFUTC), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, USE_TIME_NOW))
        ;

    index = 0;
    while (InsertDate(NOWWFMTDEF, pBuf.get(), index, fileLength, maxLength, USE_TIME_NOW))
        ;
    index = 0;
    while (InsertDateW(TEXT(NOWWFMTDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, USE_TIME_NOW))
        ;

    index = 0;
    while (InsertDate(NOWWFMTDEFUTC, pBuf.get(), index, fileLength, maxLength, USE_TIME_NOW))
        ;
    index = 0;
    while (InsertDateW(TEXT(NOWWFMTDEFUTC), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, USE_TIME_NOW))
        ;

    index = 0;
    while (InsertBoolean(MODDEF, pBuf.get(), index, fileLength, subStat.hasMods))
        ;
    index = 0;
    while (InsertBooleanW(TEXT(MODDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, subStat.hasMods))
        ;

    index = 0;
    while (InsertBoolean(UNVERDEF, pBuf.get(), index, fileLength, subStat.hasUnversioned))
        ;
    index = 0;
    while (InsertBooleanW(TEXT(UNVERDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, subStat.hasUnversioned))
        ;

    index = 0;
    while (InsertBoolean(MIXEDDEF, pBuf.get(), index, fileLength, (subStat.minRev != subStat.maxRev) || subStat.bIsExternalMixed))
        ;
    index = 0;
    while (InsertBooleanW(TEXT(MIXEDDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, (subStat.minRev != subStat.maxRev) || subStat.bIsExternalMixed))
        ;

    index = 0;
    while (InsertBoolean(EXTALLFIXED, pBuf.get(), index, fileLength, !subStat.bIsExternalsNotFixed))
        ;
    index = 0;
    while (InsertBooleanW(TEXT(EXTALLFIXED), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, !subStat.bIsExternalsNotFixed))
        ;

    index = 0;
    while (InsertBoolean(ISTAGGED, pBuf.get(), index, fileLength, subStat.bIsTagged))
        ;
    index = 0;
    while (InsertBooleanW(TEXT(ISTAGGED), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, subStat.bIsTagged))
        ;

    index = 0;
    while (InsertUrl(URLDEF, pBuf.get(), index, fileLength, maxLength, subStat.url))
        ;
    index = 0;
    while (InsertUrlW(TEXT(URLDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, Utf8ToWide(subStat.url).c_str()))
        ;

    index = 0;
    while (InsertUrl(REPOROOTDEF, pBuf.get(), index, fileLength, maxLength, subStat.rootUrl))
        ;
    index = 0;
    while (InsertUrlW(TEXT(REPOROOTDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, Utf8ToWide(subStat.rootUrl).c_str()))
        ;

    index = 0;
    while (InsertBoolean(ISINSVN, pBuf.get(), index, fileLength, subStat.bIsSvnItem))
        ;
    index = 0;
    while (InsertBooleanW(TEXT(ISINSVN), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, subStat.bIsSvnItem))
        ;

    index = 0;
    while (InsertBoolean(NEEDSLOCK, pBuf.get(), index, fileLength, subStat.lockData.needsLocks))
        ;
    index = 0;
    while (InsertBooleanW(TEXT(NEEDSLOCK), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, subStat.lockData.needsLocks))
        ;

    index = 0;
    while (InsertBoolean(ISLOCKED, pBuf.get(), index, fileLength, subStat.lockData.isLocked))
        ;
    index = 0;
    while (InsertBooleanW(TEXT(ISLOCKED), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, subStat.lockData.isLocked))
        ;

    index = 0;
    while (InsertDate(LOCKDATE, pBuf.get(), index, fileLength, maxLength, subStat.lockData.creationDate))
        ;
    index = 0;
    while (InsertDateW(TEXT(LOCKDATE), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, subStat.lockData.creationDate))
        ;

    index = 0;
    while (InsertDate(LOCKDATEUTC, pBuf.get(), index, fileLength, maxLength, subStat.lockData.creationDate))
        ;
    index = 0;
    while (InsertDateW(TEXT(LOCKDATEUTC), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, subStat.lockData.creationDate))
        ;

    index = 0;
    while (InsertDate(LOCKWFMTDEF, pBuf.get(), index, fileLength, maxLength, subStat.lockData.creationDate))
        ;
    index = 0;
    while (InsertDateW(TEXT(LOCKWFMTDEF), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, subStat.lockData.creationDate))
        ;

    index = 0;
    while (InsertDate(LOCKWFMTDEFUTC, pBuf.get(), index, fileLength, maxLength, subStat.lockData.creationDate))
        ;
    index = 0;
    while (InsertDateW(TEXT(LOCKWFMTDEFUTC), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, subStat.lockData.creationDate))
        ;

    index = 0;
    while (InsertUrl(LOCKOWNER, pBuf.get(), index, fileLength, maxLength, subStat.lockData.owner))
        ;
    index = 0;
    while (InsertUrlW(TEXT(LOCKOWNER), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, Utf8ToWide(subStat.lockData.owner).c_str()))
        ;

    index = 0;
    while (InsertUrl(LOCKCOMMENT, pBuf.get(), index, fileLength, maxLength, subStat.lockData.comment))
        ;
    index = 0;
    while (InsertUrlW(TEXT(LOCKCOMMENT), reinterpret_cast<wchar_t *>(pBuf.get()), index, fileLength, maxLength, Utf8ToWide(subStat.lockData.comment).c_str()))
        ;

    CAutoFile hFile = CreateFile(dst, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, NULL, nullptr);
    if (!hFile)
    {
        wprintf(L"Unable to open output file '%s' for writing\n", dst);
        return ERR_OPEN;
    }

    size_t fileLengthExisting = GetFileSize(hFile, nullptr);
    BOOL   sameFileContent    = FALSE;
    if (fileLength == fileLengthExisting)
    {
        DWORD readLengthExisting = 0;
        auto  pBufExisting       = std::make_unique<char[]>(fileLength);
        if (!ReadFile(hFile, pBufExisting.get(), static_cast<DWORD>(fileLengthExisting), &readLengthExisting, nullptr))
        {
            wprintf(L"Could not read the file '%s'\n", dst);
            return ERR_READ;
        }
        if (readLengthExisting != fileLengthExisting)
        {
            wprintf(L"Could not read the file '%s' to the end!\n", dst);
            return ERR_READ;
        }
        sameFileContent = (memcmp(pBuf.get(), pBufExisting.get(), fileLength) == 0);
    }

    // The file is only written if its contents would change.
    // this object prevents the timestamp from changing.
    if (!sameFileContent)
    {
        SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);

        WriteFile(hFile, pBuf.get(), static_cast<DWORD>(fileLength), &readLength, nullptr);
        if (readLength != fileLength)
        {
            wprintf(L"Could not write the file '%s' to the end!\n", dst);
            return ERR_READ;
        }

        if (!SetEndOfFile(hFile))
        {
            wprintf(L"Could not truncate the file '%s' to the end!\n", dst);
            return ERR_READ;
        }
    }

    return 0;
}
