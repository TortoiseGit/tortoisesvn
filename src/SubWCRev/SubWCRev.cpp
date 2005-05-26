// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#include "stdafx.h"

#include <iostream>
#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>

#include <apr_pools.h>
#include "svn_error.h"
#include "svn_client.h"
#include "svn_path.h"
#include "SubWCRev.h"
#include "..\version.h"

#define VERDEF		"$WCREV$"
#define DATEDEF		"$WCDATE$"
#define MODDEF		"$WCMODS?"
#define RANGEDEF	"$WCRANGE$"
#define MIXEDDEF	"$WCMIXED?"
#define URLDEF		"$WCURL$"

// Internal error codes
#define ERR_SYNTAX		1	// Syntax error
#define ERR_FNF			2	// File/folder not found
#define ERR_OPEN		3	// File open error
#define ERR_ALLOC		4	// Memory allocation error
#define ERR_READ		5	// File read/write/size error
#define ERR_SVN_ERR		6	// SVN error
// Documented error codes
#define ERR_SVN_MODS	7	// Local mods found (-n)
#define ERR_SVN_MIXED	8	// Mixed rev WC found (-m)
#define ERR_OUT_EXISTS	9	// Output file already exists (-d)

char *AnsiToUtf8(const char * pszAnsi, apr_pool_t *pool)
{
	// convert ANSI --> UTF16
	int utf16_count = MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, NULL, 0);
	WCHAR * pwc = new WCHAR[utf16_count];
	MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, pwc, utf16_count);

	// and now from URF16 --> UTF-8
	int utf8_count = WideCharToMultiByte(CP_UTF8, 0, pwc, utf16_count, NULL, 0, NULL, NULL);
	char * pch = (char*) apr_palloc(pool, utf8_count);
	WideCharToMultiByte(CP_UTF8, 0, pwc, utf16_count, pch, utf8_count, NULL, NULL);
	delete[] pwc;
	return pch;
}

char *Utf16ToUtf8(const WCHAR *pszUtf16, apr_pool_t *pool)
{
	// from URF16 --> UTF-8
	int utf8_count = WideCharToMultiByte(CP_UTF8, 0, pszUtf16, -1, NULL, 0, NULL, NULL);
	char * pch = (char*) apr_palloc(pool, utf8_count);
	WideCharToMultiByte(CP_UTF8, 0, pszUtf16, -1, pch, utf8_count, NULL, NULL);
	return pch;
}

int FindPlaceholder(char *def, char *pBuf, unsigned long & index, unsigned long filelength)
{
	int deflen = strlen(def);
	while (index + deflen <= filelength)
	{
		if (memcmp(pBuf + index, def, deflen) == 0)
			return TRUE;
		index++;
	}
	return FALSE;
}

int InsertRevision(char * def, char * pBuf, unsigned long & index,
					unsigned long & filelength, unsigned long maxlength,
					long MinRev, long MaxRev)
{
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholder(def, pBuf, index, filelength))
	{
		// No more matches found.
		return FALSE;
	}
	// Format the text to insert at the placeholder
	char destbuf[40];
	if (MinRev == -1 || MinRev == MaxRev)
	{
		sprintf(destbuf, "%Ld", MaxRev);
	}
	else
	{
		sprintf(destbuf, "%Ld:%Ld", MinRev, MaxRev);
	}
	// Replace the $WCxxx$ string with the actual revision number
	char * pBuild = pBuf + index;
	int Expansion = strlen(destbuf) - strlen(def);
	if (Expansion < 0)
	{
		memmove(pBuild, pBuild - Expansion, filelength - (long)((pBuild - Expansion) - pBuf));
	}
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if ((int)(maxlength - filelength) < Expansion) return FALSE;
		memmove(pBuild + Expansion, pBuild, filelength - (long)(pBuild - pBuf));
	}
	memmove(pBuild, destbuf, strlen(destbuf));
	filelength += Expansion;
	return TRUE;
}

int InsertDate(char * def, char * pBuf, unsigned long & index,
				unsigned long & filelength, unsigned long maxlength,
				apr_time_t date_svn)
{ 
	// Search for first occurrence of def in the buffer, starting at index.
	if (!FindPlaceholder(def, pBuf, index, filelength))
	{
		// No more matches found.
		return FALSE;
	}
	// Format the text to insert at the placeholder
	__time64_t ttime = date_svn/1000000L;
	struct tm *newtime = _localtime64(&ttime);
	if (newtime == NULL)
		return FALSE;
	// Format the date/time in international format as yyyy/mm/dd hh:mm:ss
	char destbuf[32];
	sprintf(destbuf, "%04d/%02d/%02d %02d:%02d:%02d",
			newtime->tm_year + 1900,
			newtime->tm_mon + 1,
			newtime->tm_mday,
			newtime->tm_hour,
			newtime->tm_min,
			newtime->tm_sec);
	// Replace the $WCDATE$ string with the actual commit date
	char * pBuild = pBuf + index;
	int Expansion = strlen(destbuf) - strlen(def);
	if (Expansion < 0)
	{
		memmove(pBuild, pBuild - Expansion, filelength - (long)((pBuild - Expansion) - pBuf));
	}
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if ((int)(maxlength - filelength) < Expansion) return FALSE;
		memmove(pBuild + Expansion, pBuild, filelength - (long)(pBuild - pBuf));
	}
	memmove(pBuild, destbuf, strlen(destbuf));
	filelength += Expansion;
	return TRUE;
}

int InsertUrl(char * def, char * pBuf, unsigned long & index,
					unsigned long & filelength, unsigned long maxlength,
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
	int Expansion = strlen(pUrl) - strlen(def);
	if (Expansion < 0)
	{
		memmove(pBuild, pBuild - Expansion, filelength - (long)((pBuild - Expansion) - pBuf));
	}
	else if (Expansion > 0)
	{
		// Check for buffer overflow
		if ((int)(maxlength - filelength) < Expansion) return FALSE;
		memmove(pBuild + Expansion, pBuild, filelength - (long)(pBuild - pBuf));
	}
	memmove(pBuild, pUrl, strlen(pUrl));
	filelength += Expansion;
	return TRUE;
}

int InsertBoolean(char * def, char * pBuf, unsigned long & index, unsigned long & filelength, BOOL isTrue)
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
		if (pEnd - pBuf >= (int)filelength)
			return FALSE;	// No terminator - malformed so give up.
	}
	
	// Look for the ':' dividing TrueText from FalseText
	char *pSplit = pBuild + 1;
	// This loop is guaranteed to terminate due to test above.
	while (*pSplit != ':' && *pSplit != '$')
		pSplit++;

	if (*pSplit == '$')
		return FALSE;		// No split - malformed so give up.

	if (isTrue)
	{
		// Replace $WCxxx?TrueText:FalseText$ with TrueText
		// Remove :FalseText$
		memmove(pSplit, pEnd + 1, filelength - (long)(pEnd + 1 - pBuf));
		filelength -= (long)(pEnd + 1 - pSplit);
		// Remove $WCxxx?
		int deflen = strlen(def);
		memmove(pBuild, pBuild + deflen, filelength - (long)(pBuild + deflen - pBuf));
		filelength -= deflen;
	}
	else
	{
		// Replace $WCxxx?TrueText:FalseText$ with FalseText
		// Remove terminating $
		memmove(pEnd, pEnd + 1, filelength - (long)(pEnd + 1 - pBuf));
		filelength--;
		// Remove $WCxxx?TrueText:
		memmove(pBuild, pSplit + 1, filelength - (long)(pSplit + 1 - pBuf));
		filelength -= (long)(pSplit + 1 - pBuild);
	} // if (isTrue)
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
	if ((argc != 4)&&(argc != 2)&&(argc != 5))
	{
		_tprintf(_T("SubWCRev %d.%d.%d, Build %d\n\n"),
					TSVN_VERMAJOR, TSVN_VERMINOR,
					TSVN_VERMICRO, TSVN_VERBUILD);
		_putts(
			_T("Usage: SubWCRev WorkingCopyPath [SrcVersionFile] [DstVersionFile] [-nmdf]\n")
			_T("\n")
			_T("Params:\n")
			_T("WorkingCopyPath    :   path to a Subversion working copy\n")
			_T("SrcVersionFile     :   path to a template header file\n")
			_T("DstVersionFile     :   path to where to save the resulting header file\n")
			_T("-n                 :   if given, then SubWCRev will error if the working\n")
			_T("                       copy contains local modifications\n")
			_T("-m                 :   if given, then SubWCRev will error if the working\n")
			_T("                       copy contains mixed revisions\n")
			_T("-d                 :   if given, then SubWCRev will only do its job if\n")
			_T("                       DstVersionFile does not exist\n")
			_T("-f                 :   if given, then SubWCRev will include the\n")
			_T("                       last-changed revision of folders. Default is to\n")
			_T("                       use only files to get the revision numbers\n")
			_T("                       This only affects $WCREV$ and $WCDATE$\n")
			_T("\n")
			_T("SubWCRev reads the Subversion status of all files in a working copy\n")
			_T("excluding externals. If SrcVersionFile is specified, it is scanned\n")
			_T("for special placeholders of the form \"$WCxxx$\".\n")
			_T("SrcVersionFile is then copied to DstVersionFile but the placeholders\n")
			_T("are replaced with information about the working copy as follows:\n")
			_T("\n")
			_T("$WCREV$      Highest committed revision number\n")
			_T("$WCDATE$     Date of highest committed revision\n")
			_T("$WCRANGE$    Update revision range\n")
			_T("$WCURL$      Repository URL of the working copy\n")
			_T("\n")
			_T("Placeholders of the form \"$WCxxx?TrueText:FalseText$\" are replaced with\n")
			_T("TrueText if the tested condition is true, and FalseText if false.\n")
			_T("\n")
			_T("$WCMODS$     True if local modifications found\n")
			_T("$WCMIXED$    True if mixed update revisions found\n"));
		return ERR_SYNTAX;
	}
	// we have three parameters
	const TCHAR * src = NULL;
	const TCHAR * dst = NULL;
	const TCHAR * wc = NULL;
	BOOL bErrOnMods = FALSE;
	BOOL bErrOnMixed = FALSE;
	SubWCRev_t SubStat;
	SubStat.bFolders = FALSE;
	memset (&SubStat, 0, sizeof (SubStat));
	if (argc == 2)
	{
		wc = argv[1];
	}
	if (argc >= 4)
	{
		wc = argv[1];
		src = argv[2];
		dst = argv[3];
		// Check for flags in argv[4]
		if ((argc == 5) && (argv[4][0] == '-'))
		{
			if (_tcschr(argv[4], 'n') != 0)
				bErrOnMods = TRUE;
			if (_tcschr(argv[4], 'm') != 0)
				bErrOnMixed = TRUE;
			if (_tcschr(argv[4], 'd') != 0)
				if (PathFileExists(dst))
					return ERR_OUT_EXISTS;
			// the 'f' option is useful to keep the revision which is inserted in
			// the file constant, even if there are commits on other branches.
			// For example, if you tag your working copy, then half a year later
			// do a fresh checkout of that tag, the folder in your working copy of
			// that tag will get the HEAD revision of the time you check out (or
			// do an update). The files alone however won't have their last-committed
			// revision changed at all.
			if (_tcschr(argv[4], 'f') != 0)
				SubStat.bFolders = TRUE;
		}
		if (!PathFileExists(src))
		{
			_tprintf(_T("File '%s' does not exist\n"), src);
			return ERR_FNF;		// file does not exist
		}
	}

	if (!PathFileExists(wc))
	{
		_tprintf(_T("Directory or file '%s' does not exist\n"), wc);
		return ERR_FNF;			// dir does not exist
	}
	char * pBuf = NULL;
	unsigned long readlength = 0;
	unsigned long filelength = 0;
	unsigned long maxlength  = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	if ((argc == 4)||(argc == 5))
	{
		// open the file and read the contents
		hFile = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			_tprintf(_T("Unable to open input file '%s'\n"), src);
			return ERR_OPEN;		// error opening file
		}
		filelength = GetFileSize(hFile, NULL);
		if (filelength == INVALID_FILE_SIZE)
		{
			_tprintf(_T("Could not determine filesize of '%s'\n"), src);
			return ERR_READ;
		}
		maxlength = filelength+4096;	// We might be increasing filesize.
		pBuf = new char[maxlength];
		if (pBuf == NULL)
		{
			_tprintf(_T("Could not allocate enough memory!\n"));
			return ERR_ALLOC;
		}
		if (!ReadFile(hFile, pBuf, filelength, &readlength, NULL))
		{
			_tprintf(_T("Could not read the file '%s'\n"), src);
			return ERR_READ;
		}
		if (readlength != filelength)
		{
			_tprintf(_T("Could not read the file '%s' to the end!\n"), src);
			return ERR_READ;
		}
		CloseHandle(hFile);

	}
	// Now check the status of every file in the working copy
	// and gather revision status information in SubStat.

	apr_pool_t * pool;
	svn_error_t * svnerr = NULL;
	svn_client_ctx_t ctx;
	const char * internalpath;

	apr_initialize();
	apr_pool_create_ex (&pool, NULL, abort_on_pool_failure, NULL);
	memset (&ctx, 0, sizeof (ctx));

	char *wc_utf8;
#ifdef UNICODE
	wc_utf8 = Utf16ToUtf8(wc, pool);
#else
	wc_utf8 = AnsiToUtf8(wc, pool);
#endif
	internalpath = svn_path_internal_style (wc_utf8, pool);

	svnerr = svn_status(	internalpath,	//path
							&SubStat,		//status_baton
							TRUE,			//noignore
							&ctx,
							pool);

	if (svnerr)
	{
		svn_handle_error(svnerr, stderr, FALSE);
	}
	TCHAR wcfullpath[MAX_PATH];
	LPTSTR dummy;
	GetFullPathName(wc, MAX_PATH, wcfullpath, &dummy);
	apr_terminate2();
	if (svnerr)
	{
		return ERR_SVN_ERR;
	}
	
	char wcfull_oem[MAX_PATH];
	CharToOem(wcfullpath, wcfull_oem);
	_tprintf(_T("SubWCRev: '%hs'\n"), wcfull_oem);
	
	
	if (bErrOnMods && SubStat.HasMods)
	{
		_tprintf(_T("Working copy has local modifications!\n"));
		return ERR_SVN_MODS;
	}
	
	if (bErrOnMixed && (SubStat.MinRev != SubStat.MaxRev))
	{
		_tprintf(_T("Working copy contains mixed revisions %Ld:%Ld!\n"), SubStat.MinRev, SubStat.MaxRev);
		return ERR_SVN_MIXED;
	}

	_tprintf(_T("Last committed at revision %Ld\n"), SubStat.CmtRev);

	if (SubStat.MinRev != SubStat.MaxRev)
	{
		_tprintf(_T("Mixed revision range %Ld:%Ld\n"), SubStat.MinRev, SubStat.MaxRev);
	}
	else
	{
		_tprintf(_T("Updated to revision %Ld\n"), SubStat.MaxRev);
	}
	
	if (SubStat.HasMods)
	{
		_tprintf(_T("Local modifications found\n"));
	}

	if (argc==2)
	{
		return 0;
	}

	// now parse the filecontents for version defines.
	
	unsigned long index = 0;
	
	while (InsertRevision(VERDEF, pBuf, index, filelength, maxlength, -1, SubStat.CmtRev));
	
	index = 0;
	while (InsertRevision(RANGEDEF, pBuf, index, filelength, maxlength, SubStat.MinRev, SubStat.MaxRev));
	
	index = 0;
	while (InsertDate(DATEDEF, pBuf, index, filelength, maxlength, SubStat.CmtDate));
	
	index = 0;
	while (InsertBoolean(MODDEF, pBuf, index, filelength, SubStat.HasMods));
	
	index = 0;
	while (InsertBoolean(MIXEDDEF, pBuf, index, filelength, SubStat.MinRev != SubStat.MaxRev));
	
	index = 0;
	while (InsertUrl(URLDEF, pBuf, index, filelength, maxlength, SubStat.Url));
	
	hFile = CreateFile(dst, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("Unable to open output file '%s' for writing\n"), dst);
		return ERR_OPEN;
	}

	unsigned long filelengthExisting = GetFileSize(hFile, NULL);
	BOOL sameFileContent = FALSE;
	if (filelength == filelengthExisting)
	{
		unsigned long readlengthExisting = 0;
		char * pBufExisting = new char[filelength];
		if (!ReadFile(hFile, pBufExisting, filelengthExisting, &readlengthExisting, NULL))
		{
			_tprintf(_T("Could not read the file '%s'\n"), dst);
			return ERR_READ;
		}
		if (readlengthExisting != filelengthExisting)
		{
			_tprintf(_T("Could not read the file '%s' to the end!\n"), dst);
			return ERR_READ;
		}
		sameFileContent = (memcmp(pBuf, pBufExisting, filelength) == 0);
		delete [] pBufExisting;
	}

	// The file is only written if its contents would change.
	// This prevents the timestamp from changing.
	if (!sameFileContent)
	{
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

		WriteFile(hFile, pBuf, filelength, &readlength, NULL);
		if (readlength != filelength)
		{
			_tprintf(_T("Could not write the file '%s' to the end!\n"), dst);
			return ERR_READ;
		}

		if (!SetEndOfFile(hFile))
		{
			_tprintf(_T("Could not truncate the file '%s' to the end!\n"), dst);
			return ERR_READ;
		}
	}
	CloseHandle(hFile);
	delete [] pBuf;
		
	return 0;
}
