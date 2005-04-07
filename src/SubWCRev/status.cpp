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

#include <apr_pools.h>
#include "svn_client.h"
#include "svn_wc.h"
#include "svn_path.h"
#include "svn_utf.h"

extern BOOL bHasMods;
extern long lowestupdate;
extern long highestupdate;
extern apr_time_t WCDate;

#pragma warning(push)
#pragma warning(disable:4127)	//conditional expression is constant (cause of SVN_ERR
void getallstatus(void * baton, const char * /*path*/, svn_wc_status_t * status)
{
	if ((status)&&(status->entry))
	{
		LONG * rev = (LONG *)baton;
		if ((*rev)<status->entry->cmt_rev)
		{
			*rev = status->entry->cmt_rev;
			WCDate = status->entry->cmt_date;
		}
		if (highestupdate < status->entry->revision)
		{
			highestupdate = status->entry->revision;
		}
		if (lowestupdate > status->entry->revision || lowestupdate == 0)
		{
			lowestupdate = status->entry->revision;
		}
		switch (status->text_status)
		{
		case svn_wc_status_none:
		case svn_wc_status_unversioned:
		case svn_wc_status_ignored:
		case svn_wc_status_external:
		case svn_wc_status_incomplete:
		case svn_wc_status_normal:
			break;
		default:
			bHasMods = TRUE;
			break;			
		}
		switch (status->prop_status)
		{
		case svn_wc_status_none:
		case svn_wc_status_unversioned:
		case svn_wc_status_ignored:
		case svn_wc_status_external:
		case svn_wc_status_incomplete:
		case svn_wc_status_normal:
			break;
		default:
			bHasMods = TRUE;
			break;			
		}
	}
}

svn_error_t *
svn_status (       const char *path,
                   void *status_baton,
                   svn_boolean_t no_ignore,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *pool)
{
  svn_wc_adm_access_t *adm_access;
  svn_wc_traversal_info_t *traversal_info = svn_wc_init_traversal_info (pool);
  const char *anchor, *target;
  const svn_delta_editor_t *editor;
  void *edit_baton;
  const svn_wc_entry_t *entry;
  svn_revnum_t edit_revision = SVN_INVALID_REVNUM;

  svn_utf_initialize(pool);

  /* Need to lock the tree as even a non-recursive status requires the
     immediate directories to be locked. */
  SVN_ERR (svn_wc_adm_probe_open2 (&adm_access, NULL, path, 
                                  FALSE, 0, pool));

  /* Get the entry for this path so we can determine our anchor and
     target.  If the path is unversioned, and the caller requested
     that we contact the repository, we error. */
  SVN_ERR (svn_wc_entry (&entry, path, adm_access, FALSE, pool));
  if (entry)
    SVN_ERR (svn_wc_get_actual_target (path, &anchor, &target, pool));
  else 
    svn_path_split (path, &anchor, &target, pool);
  
  /* Close up our ADM area.  We'll be re-opening soon. */
  SVN_ERR (svn_wc_adm_close (adm_access));

  /* Need to lock the tree as even a non-recursive status requires the
     immediate directories to be locked. */
  SVN_ERR (svn_wc_adm_probe_open2 (&adm_access, NULL, anchor, 
                                  FALSE, -1, pool));

  /* Get the status edit, and use our wrapping status function/baton
     as the callback pair. */
  SVN_ERR (svn_wc_get_status_editor (&editor, &edit_baton, &edit_revision,
                                     adm_access, target, ctx->config, TRUE,
                                     TRUE, no_ignore, getallstatus, status_baton,
                                     ctx->cancel_func, ctx->cancel_baton,
                                     traversal_info, pool));

      SVN_ERR (editor->close_edit (edit_baton, pool));


  /* Close the access baton here, as svn_client__do_external_status()
     calls back into this function and thus will be re-opening the
     working copy. */
  SVN_ERR (svn_wc_adm_close (adm_access));


  return SVN_NO_ERROR;
}
#pragma warning(pop)