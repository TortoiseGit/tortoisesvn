/*
   Copyright (c) 2002 Perry Rapp
   "The MIT license"
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*=============================================================
 * iconvshim.c -- Shim to connect to iconv dll if available
 * Windows specific
 *   Created: 2002/06 by Perry Rapp
 *   Edited:  2002/11/20 (Perry Rapp)
 *==============================================================*/

#include "config.h"
#include "iconv.h"
#include "iconvshim.h"
#include <windows.h>
#include <stdio.h>

#ifndef ICONVDECL
#define ICONVDECL
#endif

#define ICONVSHIM_VERSION "1.1.1"

static FARPROC MyGetProcAddress(HMODULE hModule, LPCSTR lpProcName);
static int ishim_get_dll_name(char * filepath, int pathlen);
static int ishim_set_dll_name(const char *filepath);
static int load_dll(void);
static void unload_dll(void);

static HINSTANCE f_hinstDll=0;
static int f_failed=0;
static char f_dllpath[MAX_PATH]="";
static char * f_defaults[] = { "libapriconv.dll", "iconv.dll", "libiconv.dll" };


typedef iconv_t (*iconv_open_type)(const char* tocode, const char* fromcode);
typedef size_t (*iconv_type)(iconv_t cd, const char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft);
typedef int (*iconv_close_type)(iconv_t cd);
typedef int (*iconvctl_type)(iconv_t cd, int request, void* argument);


static struct iconv_fncs_s
{
	iconv_open_type iconv_open_x;
	iconv_type iconv_x;
	iconv_close_type iconv_close_x;
	iconvctl_type iconvctl_x;
} f_iconv_fncs;


/* Allocates descriptor for code conversion from encoding `fromcode' to
   encoding `tocode'. */
ICONVDECL iconv_t
iconv_open (const char* tocode, const char* fromcode)
{
	if (!load_dll() || !f_iconv_fncs.iconv_open_x)
		return (iconv_t)-1;
	return (*f_iconv_fncs.iconv_open_x)(tocode, fromcode);
}

/* Converts, using conversion descriptor `cd', at most `*inbytesleft' bytes
   starting at `*inbuf', writing at most `*outbytesleft' bytes starting at
   `*outbuf'.
   Decrements `*inbytesleft' and increments `*inbuf' by the same amount.
   Decrements `*outbytesleft' and increments `*outbuf' by the same amount. */
ICONVDECL size_t
iconv (iconv_t cd, const char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft)
{
	if (!load_dll() || !f_iconv_fncs.iconv_x)
	{
		/* Implement a non-translating copy */
		size_t max = *inbytesleft, len;
		if (max > *outbytesleft)
			max = *outbytesleft;
		lstrcpyn(*outbuf, *inbuf, max);
		len = lstrlen(*inbuf);
		*inbuf += len;
		*outbuf += len;
		*inbytesleft -= len;
		*outbytesleft -= len;
		return len;
	}
	return (*f_iconv_fncs.iconv_x)(cd, inbuf, inbytesleft, outbuf, outbytesleft);
}

/* Frees resources allocated for conversion descriptor `cd'. */
ICONVDECL int
iconv_close (iconv_t cd)
{
	if (!load_dll() || !f_iconv_fncs.iconv_close_x)
		return 0;
	return (*f_iconv_fncs.iconv_close_x)(cd);
}

/* Control of attributes. */
ICONVDECL int
iconvctl (iconv_t cd, int request, void* argument)
{
	if (!load_dll() || !f_iconv_fncs.iconvctl_x)
		return 0;
	return (*f_iconv_fncs.iconvctl_x)(cd, request, argument);
}


static void
unload_dll (void)
{
	if (!f_hinstDll)
		return;
	memset(&f_iconv_fncs, 0, sizeof(f_iconv_fncs));
	FreeLibrary(f_hinstDll);
}

static FARPROC
MyGetProcAddress (HMODULE hModule, LPCSTR lpProcName)
{
	/* TODO: Add property for client to set prefix */
	const char * prefix ="lib";
	char buffer[256];
	FARPROC proc = GetProcAddress(hModule, lpProcName);
	if (proc)
		return proc;
	if (lstrlen(lpProcName)+lstrlen(prefix)+1>sizeof(buffer))
		return 0;
	lstrcpy(buffer, prefix);
	lstrcat(buffer, lpProcName);
	proc = GetProcAddress(hModule, buffer);
	if (proc)
		return proc;
	return 0;
}
 
static int
load_dll (void)
{
	if (f_hinstDll)
		return 1;
	if (f_failed)
		return 0;
	f_failed=1;

	memset(&f_iconv_fncs, 0, sizeof(f_iconv_fncs));

	if (!f_dllpath[0]) 
	{
		/* no requested path, try defaults */
		int i;
		for (i=0; i<sizeof(f_defaults)/sizeof(f_defaults[0]); ++i) 
		{
			if ((f_hinstDll = LoadLibrary(f_defaults[i]))!=NULL)
			{
				strncpy(f_dllpath, f_defaults[i], sizeof(f_dllpath));
				break;
			}
		}
	} 
	else 
	{
		f_hinstDll = LoadLibrary(f_dllpath);
	}
	if (!f_hinstDll)
		return 0;

	f_iconv_fncs.iconv_open_x = (iconv_open_type)MyGetProcAddress(f_hinstDll, "iconv_open");
	f_iconv_fncs.iconv_x = (iconv_type)MyGetProcAddress(f_hinstDll, "iconv");
	f_iconv_fncs.iconv_close_x = (iconv_close_type)MyGetProcAddress(f_hinstDll, "iconv_close");
	f_iconv_fncs.iconvctl_x = (iconvctl_type)MyGetProcAddress(f_hinstDll, "iconvctl");

	f_failed=0;
	return 1;
}

ICONVDECL int
iconvshim_get_property (const char *name, char * value, int valuelen)
{
	static char file_version[] = "file_version:";
	load_dll();
	if (!lstrcmp(name, "dll_path"))
		return ishim_get_dll_name(value, valuelen);
	if (!lstrcmp(name, "dll_version"))
	{
		char path[_MAX_PATH];
		if (!ishim_get_dll_name(path, sizeof(path)))
			return 0;
		return ishim_get_file_version(path, value, valuelen);
	}
	if (!lstrcmp(name, "shim_version"))
	{
		lstrcpyn(value, ICONVSHIM_VERSION, valuelen);
		return lstrlen(ICONVSHIM_VERSION);
	}
	if (!strncmp(name, file_version, strlen(file_version)))
	{
		return ishim_get_file_version(name+strlen(file_version), value, valuelen);
	}
	return 0;
}

ICONVDECL int
iconvshim_set_property (const char *name, const char *value)
{
	if (!strcmp(name, "dll_path"))
	{
		return ishim_set_dll_name(value);
	}
	return 0;
}

static int
ishim_get_dll_name (char * filepath, int pathlen)
{
	if (!f_hinstDll)
		return 0;
	if (!GetModuleFileName(f_hinstDll, filepath, pathlen))
		return 0;
	return 1;
}

int
ishim_get_file_version (const char * filepath, char * verout, int veroutlen)
{
	DWORD dwDummyHandle, len;
	BYTE * buf = 0;
	unsigned int verlen;
	LPVOID lpvi;
	VS_FIXEDFILEINFO fileInfo;

	if (!filepath || !filepath[0]) return 0;

	len = GetFileVersionInfoSize((char *)filepath, &dwDummyHandle);
	if (!len) return 0;
	buf = (BYTE *)malloc(len * sizeof(BYTE));
	if (!buf) return 0;
	GetFileVersionInfo((char *)filepath, 0, len, buf);
	VerQueryValue(buf, "\\", &lpvi, &verlen);
	fileInfo = *(VS_FIXEDFILEINFO*)lpvi;
	_snprintf(verout, veroutlen, "FV:%d.%d.%d.%d, PV:%d.%d.%d.%d"
		, HIWORD(fileInfo.dwFileVersionMS)
		, LOWORD(fileInfo.dwFileVersionMS)
		, HIWORD(fileInfo.dwFileVersionLS)
		, LOWORD(fileInfo.dwFileVersionLS)
		, HIWORD(fileInfo.dwProductVersionMS)
		, LOWORD(fileInfo.dwProductVersionMS)
		, HIWORD(fileInfo.dwProductVersionLS)
		, LOWORD(fileInfo.dwProductVersionLS));
	free(buf);
	return 1;
}

static int
ishim_set_dll_name (const char *filepath)
{
	if (!filepath)
		filepath = "";
	lstrcpyn(f_dllpath, filepath, sizeof(f_dllpath));

	unload_dll();
	f_failed=0; /* force attempt to load */
	return load_dll();
}
