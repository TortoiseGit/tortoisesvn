/*
   Copyright (c) 2002 Perry Rapp
   "The MIT license"
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*=============================================================
 * iconv.h -- Shim to connect to iconv dll if available
 *   Created: 2002/06 by Perry Rapp
 *   Edited:  2002/11/20 (Perry Rapp)
 *==============================================================*/

#ifndef ICONV_SHIM_H_INCLUDED
#define ICONV_SHIM_H_INCLUDED 1

#ifdef __cplusplus
extern "C" {
#endif

/* package may define ICONVDECL to be __declspec(dllexport) to reexport these shim functions */
#ifndef ICONVDECL
#define ICONVDECL
#endif


ICONVDECL int get_libiconv_version(void);

#define iconv_t libiconv_t
typedef void* iconv_t;
#include <stddef.h>

/* Allocates descriptor for code conversion from encoding `fromcode' to
   encoding `tocode'. */
ICONVDECL iconv_t iconv_open (const char* tocode, const char* fromcode);

/* Converts, using conversion descriptor `cd', at most `*inbytesleft' bytes
   starting at `*inbuf', writing at most `*outbytesleft' bytes starting at
   `*outbuf'.
   Decrements `*inbytesleft' and increments `*inbuf' by the same amount.
   Decrements `*outbytesleft' and increments `*outbuf' by the same amount. */
ICONVDECL size_t iconv (iconv_t cd, const char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft);

/* Frees resources allocated for conversion descriptor `cd'. */
ICONVDECL int iconv_close (iconv_t cd);


/* Nonstandard extensions. */

/* Control of attributes. */
#define iconvctl libiconvctl
ICONVDECL int iconvctl (iconv_t cd, int request, void* argument);

/* Requests for iconvctl. */
#define ICONV_TRIVIALP            0  /* int *argument */
#define ICONV_GET_TRANSLITERATE   1  /* int *argument */
#define ICONV_SET_TRANSLITERATE   2  /* const int *argument */

/* iconvshim specific API */
ICONVDECL int iconvshim_get_property(const char *name, char * value, int valuelen);
ICONVDECL int iconvshim_set_property(const char *name, const char *value);

/* simple utility functions */
int ishim_get_file_version(const char * filepath, char * verout, int veroutlen);

#ifdef __cplusplus
}
#endif

#endif /* ICONV_SHIM_H_INCLUDED */
