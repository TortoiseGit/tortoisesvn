/*
 * Last Change: 21:57:42 10-Apr-2001.
 */
#define PACKAGE "gettext"
#define VERSION "0.11.2"
#define DEFAULT_OUTPUT_ALIGNMENT 1
#define LOCALEDIR ""
#define LOCALE_ALIAS_PATH LOCALEDIR":."
#define GNULOCALEDIR ""

/* This is the page width for the message_print function.  It should
   not be set to more than 79 characters (Emacs users will appreciate
   it).  It is used to wrap the msgid and msgstr strings, and also to
   wrap the file position (#:) comments.  */
#define PAGE_WIDTH 79

/* We can't use MSVC6 /Za "disable language extensions" but we do want argument prototypes */
#define __STDC__ 1

#ifndef PARAMS
# if __STDC__
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif

#include <ctype.h>
#include <io.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#define STDC_HEADERS 1
#define HAVE_LIMITS_H 1
#define HAVE_LOCALE_H 1
#define HAVE_MALLOC_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

#define HAVE_GETCWD 1
#define HAVE_SETLOCALE 1
#define HAVE_STRCHR 1
#define HAVE_MEMCPY 1
#define HAVE_VPRINTF 1
#define HAVE_STRERROR 1

/* 
 gettext-0.10.35-w23 had this line
 but Perry removed it to get around fatal conflict with a windows header;
 Perry doesn't know what the consequences of removing it will be.
#define O_RDONLY (_O_RDONLY | _O_BINARY)
*/
#define O_RDONLY _O_RDONLY
#define O_BINARY _O_BINARY
#define getcwd _getcwd
#define open _open
#define read _read
#define close _close
#define stat _stat
#define fstat _fstat
#define inline __inline
/* #define internal_function */

/* A file name cannot consist of any character possible.  INVALID_PATH_CHAR
   contains the characters not allowed.  */
#if !defined(MSDOS) && !defined (_WIN32)
# define	INVALID_PATH_CHAR "\1\2\3\4\5\6\7\10\11\12\13\14\15\16\17\20\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37 \177/"
#else
/* Something like this for MSDOG.  */
# define	INVALID_PATH_CHAR "\1\2\3\4\5\6\7\10\11\12\13\14\15\16\17\20\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37 \177\\:."
#endif

/* Length from which starting on warnings about too long strings are given.
   Several systems have limits for strings itself, more have problems with
   strings in their tools (important here: gencat).  1024 bytes is a
   conservative limit.  Because many translation let the message size grow
   (German translations are always bigger) choose a length < 1024.  */
#define WARN_ID_LEN 900

int vasprintf (char **result, const char *format, va_list args);
#define fopen(name, flag) (fopen)((name), flag "b")

#ifdef LIBINTL_EXPORTS
# define getenv api_getenv
#endif

#define HAVE_ICONV 1
#define ICONV_CONST const

#define IN_LIBINTL

/* MSVC locale.h doesn't have */
#define LANG_SORBIAN 0x2e

/* MSVC doesn't have C99 int types */
#define uintmax_t int
