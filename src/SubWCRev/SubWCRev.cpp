// VersionMaker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>

#include <apr_pools.h>
#include "svn_error.h"
#include "svn_client.h"
#include "svn_path.h"


#define VERDEF		"$WCREV$"
#define DATEDEF		"$WCDATE$"
#define MODDEF		"$WCMODS?"

BOOL bHasMods;
apr_time_t WCDate;

svn_error_t *
svn_status (       const char *path,
                   void *status_baton,
                   svn_boolean_t no_ignore,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

int RevDefine(char * def, char * pBuf, unsigned long & index, unsigned long & filelength, unsigned long rev)
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
		//replace the $WCREV$ string with the actual revision number
		char destbuf[20];
		sprintf(destbuf, "%Ld", rev);
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

int ModDefine(char * def, char * pBuf, unsigned long & index, unsigned long & filelength, BOOL /*mods*/)
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

		if (bHasMods)
		{
			// Replace $WCMODS?TrueText:FalseText$ with TrueText
			memmove(pSplit, pEnd + 1, filelength - abs((long)(pBuf - pBuild)));
			memmove(pBuild, pBuild + strlen(def), filelength - abs((long)(pBuf - pBuild)));
			filelength = filelength - strlen(def) - (pEnd - pSplit) - 1;
		}
		else
		{
			// Replace $WCMODS?TrueText:FalseText$ with FalseText
			memmove(pEnd, pEnd + 1, filelength - abs((long)(pBuf - pBuild)));
			memmove(pBuild, pSplit + 1, filelength - abs((long)(pBuf - pBuild)));
			filelength = filelength - (pSplit - pBuild) - 2;
		} // if (bHasMods)
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
		printf(_T("TrueText if there are local modifications, or FalseText if not.\n\n"));
		printf(_T("usage: SubWCRev WorkingCopyPath [SrcVersionFile] [DstVersionFile] [-n|-d]\n\n"));
		printf(_T("Params:\n"));
		printf(_T("WorkingCopyPath    :   path to a Subversion working copy\n"));
		printf(_T("SrcVersionFile     :   path to a template header file\n"));
		printf(_T("DstVersionFile     :   path to where to save the resulting header file\n"));
		printf(_T("-n                 :   if given, then SubWCRev will error if the working\n"));
		printf(_T("                       copy contains local modifications\n"));
		printf(_T("-d                 :   if given, then SubWCRev will only do its job if\n"));
		printf(_T("                       DstVersionFile does not exist\n"));
		return 1;
	}
	// we have three parameters
	const TCHAR * src = NULL;
	const TCHAR * dst = NULL;
	const TCHAR * wc = NULL;
	BOOL bErrOnMods = FALSE;
	bHasMods = FALSE;
	WCDate = 0;
	if (argc == 2)
	{
		wc = argv[1];
	}
	if (argc >= 4)
	{
		wc = argv[1];
		src = argv[2];
		dst = argv[3];
		if ((argc == 5) && (strcmp(argv[4], "-n")==0))
			bErrOnMods = TRUE;
		if ((argc == 5) && (strcmp(argv[4], "-d")==0))
			if (PathFileExists(dst))
				return 0;
		if (!PathFileExists(src))
		{
			printf(_T("file %s does not exist\n"), src);
			return 2;			// file does not exist
		}
	}

	if (!PathFileExists(wc))
	{
		printf(_T("directory or file %s does not exist\n"), wc);
		return 2;			// dir does not exist
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
			return 3;			// error opening file
		}
		filelength = GetFileSize(hFile, NULL);
		if (filelength == INVALID_FILE_SIZE)
		{
			printf(_T("could not determine filesize of %s\n"), src);
			return 4;
		}
		pBuf = new char[filelength+1000];		//we might be increasing the filesize!
		if (pBuf == NULL)
		{
			printf(_T("could not allocate enough memory!\n"));
		}
		if (!ReadFile(hFile, pBuf, filelength, &readlength, NULL))
		{
			printf(_T("could not read the file %s\n"), src);
			return 5;
		}
		if (readlength != filelength)
		{
			printf(_T("could not read the file %s to the end!\n"), src);
			return 6;
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
	LONG highestrev = 0;
	svnerr = svn_status(	internalpath,	//path
							&highestrev,	//status_baton
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
		return 9;
	}
	if ((bErrOnMods)&&(bHasMods))
	{
		printf("%s has local modifications!\n", wcfullpath);
		return 7;
	}

	printf("%s is at revision: %Ld%s\n", wcfullpath, highestrev,
		bHasMods ? " with local modifications" : "");
	
	if (argc==2)
	{
		return 0;
	}

	// now parse the filecontents for version defines.
	
	unsigned long index = 0;
	
	while (RevDefine(VERDEF, pBuf, index, filelength, highestrev));
	
	index = 0;
	while (DateDefine(DATEDEF, pBuf, index, filelength, WCDate));
	
	index = 0;
	while (ModDefine(MODDEF, pBuf, index, filelength, bHasMods));
	
	hFile = CreateFile(dst, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf(_T("unable to open input file %s for writing\n"), dst);
		return 8;			// error opening file
	}
	WriteFile(hFile, pBuf, filelength, &readlength, NULL);
	CloseHandle(hFile);
	delete [] pBuf;
		
	return 0;
}

