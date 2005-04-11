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

#define VERDEF		"$WCREV$"
#define DATEDEF		"$WCDATE$"
#define MODDEF		"$WCMODS?"
#define RANGEDEF	"$WCRANGE$"
#define MIXEDDEF	"$WCMIXED?"

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


int RevDefine(char * def, char * pBuf, unsigned long & index, unsigned long & filelength, long MinRev, long MaxRev)
{ 
	char * pBuild = pBuf + index;
	int bEof = pBuild - pBuf >= (int)filelength;
	while (memcmp( pBuild, def, strlen(def)) && !bEof)
	{
		pBuild++;
		bEof = pBuild - pBuf >= (int)filelength;
	}
	if (!bEof)
	{
		//replace the $WCxxx$ string with the actual revision number
		char destbuf[40];
		if (MinRev == -1 || MinRev == MaxRev)
		{
			sprintf(destbuf, "%Ld", MaxRev);
		}
		else
		{
			sprintf(destbuf, "%Ld:%Ld", MinRev, MaxRev);
		}
		if (strlen(def) > strlen(destbuf))
		{
			memmove(pBuild, pBuild + (strlen(def)-strlen(destbuf)), filelength - abs((long)(pBuf - pBuild)));
			filelength = filelength - (strlen(def)-strlen(destbuf));
		}
		else if (strlen(def) < strlen(destbuf))
		{
			memmove(pBuild+strlen(destbuf)-strlen(def),pBuild, filelength - abs((long)(pBuf - pBuild)));
			filelength = filelength + (strlen(destbuf)-strlen(def));
		}
		memmove(pBuild, destbuf, strlen(destbuf));
	} // if (!bEof)
	else
		return FALSE;
	index = (unsigned long)(pBuild - pBuf);
	return TRUE;
}

int DateDefine(char * def, char * pBuf, unsigned long & index, unsigned long & filelength, apr_time_t date_svn)
{ 
	char * pBuild = pBuf + index;
	int bEof = pBuild - pBuf >= (int)filelength;
	while (memcmp( pBuild, def, strlen(def)) && !bEof)
	{
		pBuild++;
		bEof = pBuild - pBuf >= (int)filelength;
	}
	if (!bEof)
	{
		// Replace the $WCDATE$ string with the actual commit date
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

		if (strlen(def) > strlen(destbuf))
		{
			memmove(pBuild, pBuild + (strlen(def)-strlen(destbuf)), filelength - abs((long)(pBuf - pBuild)));
			filelength = filelength - (strlen(def)-strlen(destbuf));
		}
		else if (strlen(def) < strlen(destbuf))
		{
			memmove(pBuild+strlen(destbuf)-strlen(def),pBuild, filelength - abs((long)(pBuf - pBuild)));
			filelength = filelength + (strlen(destbuf)-strlen(def));
		}
		memmove(pBuild, destbuf, strlen(destbuf));
	} // if (!bEof)
	else
		return FALSE;
	index = (unsigned long)(pBuild - pBuf);
	return TRUE;
}

int ModDefine(char * def, char * pBuf, unsigned long & index, unsigned long & filelength, BOOL isTrue)
{ 
	char * pBuild = pBuf + index;
	int bEof = pBuild - pBuf >= (int)filelength;
	while (memcmp( pBuild, def, strlen(def)) && !bEof)
	{
		pBuild++;
		bEof = pBuild - pBuf >= (int)filelength;
	}
	if (!bEof)
	{
		// Look for the terminating '$' character
		char * pEnd = pBuild + 1;
		while (*pEnd != '$')
		{
			pEnd++;
			if (pEnd - pBuild >= (int)filelength)
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
			memmove(pSplit, pEnd + 1, filelength - abs((long)(pBuf - pBuild)));
			memmove(pBuild, pBuild + strlen(def), filelength - abs((long)(pBuf - pBuild)));
			filelength = filelength - strlen(def) - (pEnd - pSplit) - 1;
		}
		else
		{
			// Replace $WCxxx?TrueText:FalseText$ with FalseText
			memmove(pEnd, pEnd + 1, filelength - abs((long)(pBuf - pBuild)));
			memmove(pBuild, pSplit + 1, filelength - abs((long)(pBuf - pBuild)));
			filelength = filelength - (pSplit - pBuild) - 2;
		} // if (isTrue)
	} // if (!bEof)
	else
		return FALSE;
	index = (unsigned long)(pBuild - pBuf);
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
		printf(_T("SubWCRev reads the Subversion status of all files in a\n"));
		printf(_T("working copy, excluding externals. Then the highest revision\n"));
		printf(_T("number found is used to replace all occurrences of \"$WCREV$\"\n"));
		printf(_T("in SrcVersionFile and the result is saved to DstVersionFile.\n"));
		printf(_T("The commit date/time of the highest revision is used to replace\n"));
		printf(_T("all occurrences of \"$WCDATE$\". The modification status is used\n"));
		printf(_T("to replace all occurrences of \"$WCMODS?TrueText:FalseText$\" with\n"));
		printf(_T("TrueText if there are local modifications, or FalseText if not.\n"));
		printf(_T("The update revision range is used to replace all occurrences of\n"));
		printf(_T("\"$WCRANGE$\", and all occurrences of \"$WCMIXED?TrueText:FalseText$\"\n"));
		printf(_T("are replaced with TrueText if updates are mixed, or FalseText if not.\n\n"));
		printf(_T("usage: SubWCRev WorkingCopyPath [SrcVersionFile] [DstVersionFile] [-nmd]\n\n"));
		printf(_T("Params:\n"));
		printf(_T("WorkingCopyPath    :   path to a Subversion working copy\n"));
		printf(_T("SrcVersionFile     :   path to a template header file\n"));
		printf(_T("DstVersionFile     :   path to where to save the resulting header file\n"));
		printf(_T("-n                 :   if given, then SubWCRev will error if the working\n"));
		printf(_T("                       copy contains local modifications\n"));
		printf(_T("-m                 :   if given, then SubWCRev will error if the working\n"));
		printf(_T("                       copy contains mixed revisions\n"));
		printf(_T("-d                 :   if given, then SubWCRev will only do its job if\n"));
		printf(_T("                       DstVersionFile does not exist\n"));
		return ERR_SYNTAX;
	}
	// we have three parameters
	const TCHAR * src = NULL;
	const TCHAR * dst = NULL;
	const TCHAR * wc = NULL;
	BOOL bErrOnMods = FALSE;
	BOOL bErrOnMixed = FALSE;
	SubWCRev_t SubStat = { 0, };
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
			if (strchr(argv[4], 'n') != 0)
				bErrOnMods = TRUE;
			if (strchr(argv[4], 'm') != 0)
				bErrOnMixed = TRUE;
			if (strchr(argv[4], 'd') != 0)
				if (PathFileExists(dst))
					return ERR_OUT_EXISTS;
		}
		if (!PathFileExists(src))
		{
			printf(_T("file %s does not exist\n"), src);
			return ERR_FNF;		// file does not exist
		}
	}

	if (!PathFileExists(wc))
	{
		printf(_T("directory or file %s does not exist\n"), wc);
		return ERR_FNF;			// dir does not exist
	}
	char * pBuf = NULL;
	unsigned long readlength = 0;
	unsigned long filelength = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	if ((argc == 4)||(argc == 5))
	{
		// open the file and read the contents
		hFile = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			printf(_T("unable to open input file %s\n"), src);
			return ERR_OPEN;		// error opening file
		}
		filelength = GetFileSize(hFile, NULL);
		if (filelength == INVALID_FILE_SIZE)
		{
			printf(_T("could not determine filesize of %s\n"), src);
			return ERR_READ;
		}
		pBuf = new char[filelength+1000];		//we might be increasing the filesize!
		if (pBuf == NULL)
		{
			printf(_T("could not allocate enough memory!\n"));
			return ERR_ALLOC;
		}
		if (!ReadFile(hFile, pBuf, filelength, &readlength, NULL))
		{
			printf(_T("could not read the file %s\n"), src);
			return ERR_READ;
		}
		if (readlength != filelength)
		{
			printf(_T("could not read the file %s to the end!\n"), src);
			return ERR_READ;
		}
		CloseHandle(hFile);

	}
	// now check the status of every file in the working copy
	// and use the highest revision number found as the new
	// version number!

	apr_pool_t * pool;
	svn_error_t * svnerr = NULL;
	svn_client_ctx_t ctx;
	const char * internalpath;

	apr_initialize();
	apr_pool_create_ex (&pool, NULL, abort_on_pool_failure, NULL);
	memset (&ctx, 0, sizeof (ctx));
	internalpath = svn_path_internal_style (wc, pool);
	svnerr = svn_status(	internalpath,	//path
							&SubStat,		//status_baton
							TRUE,			//noignore
							&ctx,
							pool);

	if (svnerr)
	{
		svn_handle_error(svnerr, stderr, FALSE);
	}
	char wcfullpath[MAX_PATH];
	LPTSTR dummy;
	GetFullPathName(wc, MAX_PATH, wcfullpath, &dummy);
	apr_terminate2();
	if (svnerr)
	{
		return ERR_SVN_ERR;
	}
	
	printf(_T("SubWCRev: '%s'\n"), wcfullpath);
	
	if (bErrOnMods && SubStat.HasMods)
	{
		printf(_T("Working copy has local modifications!\n"));
		return ERR_SVN_MODS;
	}
	
	if (bErrOnMixed && (SubStat.MinRev != SubStat.MaxRev))
	{
		printf(_T("Working copy contains mixed revisions %Ld:%Ld!\n"), SubStat.MinRev, SubStat.MaxRev);
		return ERR_SVN_MIXED;
	}

	printf(_T("Last committed at revision %Ld\n"), SubStat.CmtRev);

	if (SubStat.MinRev != SubStat.MaxRev)
	{
		printf(_T("Mixed revision range %Ld:%Ld\n"), SubStat.MinRev, SubStat.MaxRev);
	}
	else
	{
		printf(_T("Updated to revision %Ld\n"), SubStat.MaxRev);
	}
	
	if (SubStat.HasMods)
	{
		printf(_T("Local modifications found\n"));
	}

	if (argc==2)
	{
		return 0;
	}

	// now parse the filecontents for version defines.
	
	unsigned long index = 0;
	
	while (RevDefine(VERDEF, pBuf, index, filelength, -1, SubStat.CmtRev));
	
	index = 0;
	while (RevDefine(RANGEDEF, pBuf, index, filelength, SubStat.MinRev, SubStat.MaxRev));
	
	index = 0;
	while (DateDefine(DATEDEF, pBuf, index, filelength, SubStat.CmtDate));
	
	index = 0;
	while (ModDefine(MODDEF, pBuf, index, filelength, SubStat.HasMods));
	
	index = 0;
	while (ModDefine(MIXEDDEF, pBuf, index, filelength, SubStat.MinRev != SubStat.MaxRev));
	
	hFile = CreateFile(dst, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf(_T("unable to open input file %s for writing\n"), dst);
		return ERR_OPEN;
	}
	WriteFile(hFile, pBuf, filelength, &readlength, NULL);
	CloseHandle(hFile);
	delete [] pBuf;
		
	return 0;
}

