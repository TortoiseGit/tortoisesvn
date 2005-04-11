/*
 * svn_private_config.hw : Template for svn_private_config.h on Win32.
 *
 * ====================================================================
 * Copyright (c) 2000-2005 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 */

/* ==================================================================== */




#ifndef CONFIG_HW
#define CONFIG_HW

/* Path separator for local filesystem */
#define SVN_PATH_LOCAL_SEPARATOR '\\'

/* Name of system's null device */
#define SVN_NULL_DEVICE_NAME "nul"

/* Defined to be the path to the installed binaries */
#define SVN_BINDIR "/usr/local/bin"



/* The default FS back-end type */
#define DEFAULT_FS_TYPE "fsfs"

/* Define to the Python/C API format character suitable for apr_int64_t */
#if defined(_WIN64)
#define SVN_APR_INT64_T_PYCFMT "l"
#elif defined(_WIN32)
#define SVN_APR_INT64_T_PYCFMT "L"
#endif

/* Setup gettext macros */
#define N_(x) x
#define PACKAGE_NAME "subversion"

#ifdef ENABLE_NLS
#define SVN_LOCALE_RELATIVE_PATH "../share/locale"
#include <locale.h>
#include <libintl.h>
#define _(x) dgettext(PACKAGE_NAME, x)
#else
#define _(x) (x)
#define gettext(x) (x)
#define dgettext(domain,x) (x)
#endif

#endif /* CONFIG_HW */
