/*
   Copyright (c) 2002 Perry Rapp
   "The MIT license"
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*=============================================================
 * gtextra.c -- Win32 DLL stuff
 * Windows specific
 *   Created: 2002/06 by Perry Rapp
 *   Edited:  2002/11/20 (Perry Rapp)
 *==============================================================*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "iconvshim.h"

extern int _nl_msg_cat_cntr;

static HINSTANCE f_hinstDLL=0;


BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
)
{
	f_hinstDLL = hinstDLL;
	return TRUE;
}

void
gt_notify_language_change (void)
{
	++_nl_msg_cat_cntr;
}

int
gt_getProperty (const char * name, char * value, int len)
{
	value[0] = 0;
	if (!lstrcmp(name, "Authors"))
	{
		static char authors[]
			= "Ulrich Drepper, Peter Miller, Bruno Haible, Muraoka Taro, Perry Rapp";
		lstrcpyn(value, authors, len);
		return sizeof(authors)+1;
	}
	if (!lstrcmp(name, "Version"))
	{
		char mypath[MAX_PATH];
		if (!GetModuleFileName(f_hinstDLL, mypath, sizeof(mypath)))
			return 0;

		if (!ishim_get_file_version(mypath, value, len))
			return 0;
		return lstrlen(value);
	}
	if (!lstrcmp(name, "Version:iconv"))
	{
		if (!iconvshim_get_property("dll_version", value, len))
			return 0;
		return lstrlen(value);
	}
	return 0;
}