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

BOOL bHasMods;

svn_error_t *
svn_status (       const char *path,
                   void *status_baton,
                   svn_boolean_t no_ignore,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool);

int RevDefine(char * def, char * pBuf, unsigned long & index, unsigned long & filelength, unsigned long rev)
{ 
	char * pBuild = pBuf + index;
	int bEof = pBuild - pBuf >= filelength;
	while (memcmp( pBuild, def, strlen(def)) && !bEof)
	{
		pBuild++;
		bEof = pBuild - pBuf >= filelength;
	}
	if (!bEof)
	{
		//replace the $WCREV$ string with the actual revision number
		char destbuf[20];
		sprintf(destbuf, "%Ld", rev);
		if (strlen(def) > strlen(destbuf))
		{
			memmove(pBuild, pBuild + (strlen(def)-strlen(destbuf)), filelength - abs(pBuf - pBuild));
			filelength = filelength - (strlen(def)-strlen(destbuf));
		}
		else if (strlen(def) < strlen(destbuf))
		{
			memmove(pBuild+strlen(destbuf)-strlen(def),pBuild, filelength - abs(pBuf - pBuild));
			filelength = filelength + (strlen(destbuf)-strlen(def));
		}
		memmove(pBuild, destbuf, strlen(destbuf));
	} // if (!bEof)
	else
		return FALSE;
	index = pBuild - pBuf;
	return TRUE;
}

int abort_on_pool_failure (int retcode)
{
	abort ();
	return -1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if ((argc != 4)&&(argc != 2)&&(argc != 5))
	{
		printf(_T("SubWCRev reads the Subversion status of all files in a\n"));
		printf(_T("working copy, excluding externals. Then the highest revision\n"));
		printf(_T("number found is used to replace all occurences of \"$WCREV$\"\n"));
		printf(_T("in SrcVersionFile and save the result to DstVersionFile\n\n"));
		printf(_T("usage: SubWCRev WorkingCopyPath [SrcVersionFile] [DstVersionFile] [-n|-d]\n\n"));
		printf(_T("Params:\n"));
		printf(_T("WorkingCopyPath    :   path to a Subversion working copy\n"));
		printf(_T("SrcVersionFile     :   path to a template header file\n"));
		printf(_T("DstVersionFile     :   path to where to save the resulting header file\n"));
		printf(_T("-n                 :   if given, then SubWCRev will error if the working\n"));
		printf(_T("                       copy contains local modifications\n"));
		printf(_T("-d                 :   if given, then SubWCRev only do its job if the\n"));
		printf(_T("                       DstVersionFile does not exist\n"));
		return 1;
	}
	// we have three parameters
	const TCHAR * src;
	const TCHAR * dst;
	const TCHAR * wc;
	BOOL bErrOnMods = FALSE;
	bHasMods = FALSE;
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
	char * pBuf;
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

	char wcfullpath[MAX_PATH];
	LPTSTR dummy;
	GetFullPathName(wc, MAX_PATH, wcfullpath, &dummy);
	apr_terminate2();
	if ((bErrOnMods)&&(bHasMods))
	{
		printf("%s has local modifications!\n", wcfullpath);
		return 7;
	}

	printf("%s is at revision: %Ld\n", wcfullpath, highestrev);
	
	if (argc==2)
	{
		return 0;
	}

	// now parse the filecontents for version defines.
	
	unsigned long index = 0;
	
	while (RevDefine(VERDEF, pBuf, index, filelength, highestrev));
	
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

