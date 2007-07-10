// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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
//
#include "StdAfx.h"

#include "SVNLogQuery.h"
#include "ILogReceiver.h"

#include "svn_time.h"
#include "svn_sorts.h"
#include "UnicodeUtils.h"
#include "SVN.h"
#include "SVNError.h"
#include "SVNHelpers.h"
#include "TSVNPath.h"

svn_error_t* CSVNLogQuery::LogReceiver ( void* baton
									   , apr_hash_t* ch_paths
									   , svn_revnum_t revision
									   , const char* author
									   , const char* date
									   , const char* msg
									   , apr_pool_t* pool)
{
	// where to send the pre-processed in-format

    SBaton* receiverBaton = reinterpret_cast<SBaton*>(baton);
    ILogReceiver* receiver = receiverBaton->receiver;

    // report revision numbers only?

    if (receiverBaton->revs_only)
    {
	    try
	    {
            static const CString emptyString;
		    receiver->ReceiveLog ( NULL
							     , revision
							     , emptyString
							     , 0
							     , emptyString);
	    }
	    catch (SVNError& e)
	    {
		    return svn_error_create (e.GetCode(), NULL, e.GetMessage());
	    }
	    catch (...)
	    {
		    // we must not leak exceptions back into SVN
	    }

        return NULL;
    }

	// convert strings

	CString authorNative = CUnicodeUtils::GetUnicode (author);
	CString messageNative = CUnicodeUtils::GetUnicode (msg != NULL ? msg : "");

	// time stamp

	apr_time_t timeStamp = NULL;
	if (date && date[0])
		SVN_ERR (svn_time_from_cstring (&timeStamp, date, pool));

	// the individual changes

	std::auto_ptr<LogChangedPathArray> arChangedPaths (new LogChangedPathArray);
	try
	{
		if (ch_paths)
		{
			apr_array_header_t *sorted_paths 
				= svn_sort__hash (ch_paths, svn_sort_compare_items_as_paths, pool);

			for (int i = 0, count = sorted_paths->nelts; i < count; ++i)
			{
				// find the item in the hash

				std::auto_ptr<LogChangedPath> changedPath (new LogChangedPath);
				svn_sort__item_t *item = &(APR_ARRAY_IDX ( sorted_paths
														 , i
														 , svn_sort__item_t));

				// extract the path name

				const char *path = (const char *)item->key;
				changedPath->sPath = SVN::MakeUIUrlOrPath (path);

				// decode the action

				svn_log_changed_path_t *log_item 
					= (svn_log_changed_path_t *) apr_hash_get ( ch_paths
															  , item->key
															  , item->klen);
				static const char actionKeys[5] = "AMRD";
				const char* actionKey = strchr (actionKeys, log_item->action);

				changedPath->action = actionKey == NULL 
									? 0
									: 1 << (actionKey - actionKeys);

				// decode copy-from info

				if (   log_item->copyfrom_path 
					&& SVN_IS_VALID_REVNUM (log_item->copyfrom_rev))
				{
					changedPath->lCopyFromRev = log_item->copyfrom_rev;
					changedPath->sCopyFromPath 
						= SVN::MakeUIUrlOrPath (log_item->copyfrom_path);
				}
				else
				{
					changedPath->lCopyFromRev = 0;
				}

				arChangedPaths->Add (changedPath.release());
			} 
		} 
	}
	catch (CMemoryException * e)
	{
		e->Delete();
	}

	// now, report the change

	try
	{
		receiver->ReceiveLog ( arChangedPaths.release()
							 , revision
							 , authorNative
							 , timeStamp
							 , messageNative);
	}
	catch (SVNError& e)
	{
		return svn_error_create (e.GetCode(), NULL, e.GetMessage());
	}
	catch (...)
	{
		// we must not leak exceptions back into SVN
	}

	return NULL;
}

CSVNLogQuery::CSVNLogQuery (svn_client_ctx_t *context, apr_pool_t *pool)
	: context (context)
	, pool (pool)
{
}

CSVNLogQuery::~CSVNLogQuery(void)
{
}

void CSVNLogQuery::Log ( const CTSVNPathList& targets
					   , const SVNRev& peg_revision
					   , const SVNRev& start
					   , const SVNRev& end
					   , int limit
					   , bool strictNodeHistory
					   , ILogReceiver* receiver
                       , bool revs_only)
{
    SBaton baton = {receiver, revs_only};

	SVNPool localpool (pool);
	svn_error_t *result = svn_client_log3 ( targets.MakePathArray (pool)
										  , peg_revision
										  , start
										  , end
										  , limit
                                          , revs_only ? FALSE : TRUE
										  , strictNodeHistory ? TRUE : FALSE
										  , LogReceiver
										  , (void *)&baton
										  , context
										  , localpool);

	if (result != NULL)
		throw SVNError (result);
}

