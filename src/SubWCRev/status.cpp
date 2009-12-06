// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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

#pragma warning(push)
#include <apr_pools.h>
#include "svn_client.h"
#include "svn_wc.h"
#include "svn_dirent_uri.h"
#include "svn_utf.h"
#include "svn_props.h"
#pragma warning(pop)
#include "SubWCRev.h"

#pragma warning(push)
#pragma warning(disable:4127)	//conditional expression is constant (cause of SVN_ERR)

// Copy the URL from src to dest, unescaping on the fly.
void UnescapeCopy(char * src, char * dest, int buf_len)
{
	char * pszSource = src;
	char * pszDest = dest;
	int len = 0;

	// under VS.NET2k5 strchr() wants this to be a non-const array :/

	static char szHex[] = "0123456789ABCDEF";

	// Unescape special characters. The number of characters
	// in the *pszDest is assumed to be <= the number of characters
	// in pszSource (they are both the same string anyway)

	while ((*pszSource != '\0') && (++len < buf_len))
	{
		if (*pszSource == '%')
		{
			// The next two chars following '%' should be digits
			if ( *(pszSource + 1) == '\0' ||
				*(pszSource + 2) == '\0' )
			{
				// nothing left to do
				break;
			}

			char nValue = '?';
			char * pszLow = NULL;
			char * pszHigh = NULL;
			pszSource++;

			*pszSource = (char) toupper(*pszSource);
			pszHigh = strchr(szHex, *pszSource);
			
			if (pszHigh != NULL)
			{
				pszSource++;
				*pszSource = (char) toupper(*pszSource);
				pszLow = strchr(szHex, *pszSource);

				if (pszLow != NULL)
				{
					nValue = (char) (((pszHigh - szHex) << 4) +
						(pszLow - szHex));
				}
			}
			*pszDest++ = nValue;
		} 
		else
			*pszDest++ = *pszSource;

		pszSource++;
	}

	*pszDest = '\0';
}

svn_error_t * getfirststatus(void * baton, const char * /*path*/, svn_wc_status2_t * status, apr_pool_t * /*pool*/)
{
	SubWCRev_StatusBaton_t * sb = (SubWCRev_StatusBaton_t *) baton;
	if((NULL == status) || (NULL == sb) || (NULL == sb->SubStat))
	{
		return SVN_NO_ERROR;
	}
	if ((status->entry)&&(status->entry->url)&&(sb->SubStat->Url[0] == 0))
	{
		UnescapeCopy((char *) status->entry->url, sb->SubStat->Url, URL_BUF);
	}
	return SVN_NO_ERROR;
}

svn_error_t * getallstatus(void * baton, const char * path, svn_wc_status2_t * status, apr_pool_t * /*pool*/)
{
	SubWCRev_StatusBaton_t * sb = (SubWCRev_StatusBaton_t *) baton;
	if((NULL == status) || (NULL == sb) || (NULL == sb->SubStat))
	{
		return SVN_NO_ERROR;
	}

	if ((sb->SubStat->bExternals)&&(status->text_status == svn_wc_status_external) && (NULL != sb->extarray))
	{
		const char * copypath = apr_pstrdup(sb->pool, path);
		sb->extarray->push_back(copypath);
	}
	if ((status->entry)&&(status->entry->uuid))
	{
		if (sb->SubStat->UUID[0] == 0)
		{
			strncpy_s(sb->SubStat->UUID, 1024, status->entry->uuid, MAX_PATH);
		}
		if (strncmp(sb->SubStat->UUID, status->entry->uuid, MAX_PATH) != 0)
			return SVN_NO_ERROR;
	}
	if ((status->entry)&&(status->entry->cmt_author))
	{
		if ((sb->SubStat->Author[0] == 0)&&(status->url)&&(status->entry->url))
		{
			char EntryUrl[URL_BUF];
			UnescapeCopy((char *)status->entry->url,EntryUrl, URL_BUF);
			if (strncmp(sb->SubStat->Url, EntryUrl, URL_BUF) == 0)
			{
				strncpy_s(sb->SubStat->Author, URL_BUF, status->entry->cmt_author, URL_BUF);
			}
		}
	}
	if (status->entry)
	{
		if ((status->entry->kind == svn_node_file)||(sb->SubStat->bFolders))
		{
			if (sb->SubStat->CmtRev < status->entry->cmt_rev)
			{
				sb->SubStat->CmtRev = status->entry->cmt_rev;
				sb->SubStat->CmtDate = status->entry->cmt_date;
			}
		}
		if ((status->entry->revision)&&(sb->SubStat->MaxRev < status->entry->revision))
		{
			sb->SubStat->MaxRev = status->entry->revision;
		}
		if ((status->entry->revision)&&(sb->SubStat->MinRev > status->entry->revision || sb->SubStat->MinRev == 0))
		{
			sb->SubStat->MinRev = status->entry->revision;
		}
	}
	
	sb->SubStat->bIsSvnItem = false; 
	switch (status->text_status)
	{
	case svn_wc_status_none:
	case svn_wc_status_unversioned:
	case svn_wc_status_ignored:
		break;
	case svn_wc_status_external:
	case svn_wc_status_incomplete:
	case svn_wc_status_normal:
		sb->SubStat->bIsSvnItem = true; 
		break;
	default:
		sb->SubStat->bIsSvnItem = true; 
		sb->SubStat->HasMods = TRUE;
		break;			
	}

	switch (status->prop_status)
	{
	case svn_wc_status_none:
	case svn_wc_status_unversioned:
	case svn_wc_status_ignored:
		break;
	case svn_wc_status_external:
	case svn_wc_status_incomplete:
	case svn_wc_status_normal:
		sb->SubStat->bIsSvnItem = true; 
		break;
	default:
		sb->SubStat->bIsSvnItem = true; 
		sb->SubStat->HasMods = TRUE;
		break;	
	}
	
	// Assign the values for the lock information
	sb->SubStat->LockData.NeedsLocks = false;
	sb->SubStat->LockData.IsLocked = false;
	strcpy_s(sb->SubStat->LockData.Owner, OWNER_BUF, "");
	strcpy_s(sb->SubStat->LockData.Comment, COMMENT_BUF, "");
	sb->SubStat->LockData.CreationDate = 0;
	if (status->entry)
	{
		if(status->entry->present_props)
		{
			if (strstr(status->entry->present_props, SVN_PROP_NEEDS_LOCK))
			{
				sb->SubStat->LockData.NeedsLocks = true;
				
				if(status->entry->lock_token)
				{
					if((status->entry->lock_token[0] != 0))
					{
						sb->SubStat->LockData.IsLocked = true;
						if(NULL != status->entry->lock_owner)
							strncpy_s(sb->SubStat->LockData.Owner, OWNER_BUF, status->entry->lock_owner, OWNER_BUF);
						if(NULL != status->entry->lock_comment)
							strncpy_s(sb->SubStat->LockData.Comment, COMMENT_BUF, status->entry->lock_comment, COMMENT_BUF);	
						sb->SubStat->LockData.CreationDate = status->entry->lock_creation_date;
					}
				}
			}
		}
	}
	return SVN_NO_ERROR;
}

svn_error_t *
svn_status (	const char *path,
				void *status_baton,
				svn_boolean_t no_ignore,
				svn_client_ctx_t *ctx,
				apr_pool_t *pool)
{
	SubWCRev_StatusBaton_t sb;
	std::vector<const char *> * extarray = new std::vector<const char *>;
	sb.SubStat = (SubWCRev_t *)status_baton;
	sb.extarray = extarray;
	sb.pool = pool;

	svn_opt_revision_t wcrev;
	wcrev.kind = svn_opt_revision_working;

	SVN_ERR(svn_client_status4(NULL, path, &wcrev, getfirststatus, &sb, svn_depth_empty, true, false, true, true, NULL, ctx, pool));
	SVN_ERR(svn_client_status4(NULL, path, &wcrev, getallstatus, &sb, svn_depth_infinity, true, false, true, true, NULL, ctx, pool));



	// now crawl through all externals
	for (std::vector<const char *>::iterator I = extarray->begin(); I != extarray->end(); ++I)
	{
		if (strcmp(path, *I))
		{
			svn_status (*I, sb.SubStat, no_ignore, ctx, pool);
		}
	}

	delete extarray;

	return SVN_NO_ERROR;
}
#pragma warning(pop)
