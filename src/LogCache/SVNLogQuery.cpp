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
#include "svn_compat.h"
#include "UnicodeUtils.h"
#include "SVN.h"
#include "SVNError.h"
#include "SVNHelpers.h"
#include "TSVNPath.h"

// SVN API utility

void CSVNLogQuery::AppendStrings ( SVNPool& pool
                                 , apr_array_header_t* array
                                 , const std::vector<CString>& strings)
{
    for (size_t i = 0; i  < strings.size(); ++i)
    {
        CStringA utf8String = CUnicodeUtils::GetUTF8 (strings[i]);

        size_t size = utf8String.GetLength()+1;
        char * aprString = reinterpret_cast<char *>(apr_palloc (pool, size));
        memcpy (aprString, (const char*)utf8String, size);

        (*((char **) apr_array_push (array))) = aprString;
    }
}

// standard revision properties

const TRevPropNames& CSVNLogQuery::GetStandardRevProps()
{
    static TRevPropNames standardRevProps;

    if (standardRevProps.empty())
    {
        standardRevProps.push_back (CString (SVN_PROP_REVISION_LOG));
        standardRevProps.push_back (CString (SVN_PROP_REVISION_DATE));
        standardRevProps.push_back (CString (SVN_PROP_REVISION_AUTHOR));
    }

    return standardRevProps;
}

// receive the information from SVN and relay it to our 

svn_error_t* CSVNLogQuery::LogReceiver ( void *baton
                                       , svn_log_entry_t *log_entry
                                       , apr_pool_t *pool)
{
    // a few globals

    static const CString svnLog (SVN_PROP_REVISION_LOG);
    static const CString svnDate (SVN_PROP_REVISION_DATE);
    static const CString svnAuthor (SVN_PROP_REVISION_AUTHOR);

    // just in case ...

    if (log_entry == NULL)
        return NULL;

	// where to send the pre-processed in-format

    SBaton* receiverBaton = reinterpret_cast<SBaton*>(baton);
    ILogReceiver* receiver = receiverBaton->receiver;
    assert (receiver != NULL);

	// parse revprops





    StandardRevProps standardRevProps;
    UserRevPropArray userRevProps;


















	try
	{
        if (log_entry->revision != SVN_INVALID_REVNUM)
        {
            for ( apr_hash_index_t *index  
                    = apr_hash_first (pool, log_entry->revprops)
                ; index != NULL
                ; index = apr_hash_next (index))
            {
                // extract next entry from hash

                const char* key = NULL;
                ptrdiff_t keyLen;
                const char** val = NULL;

                apr_hash_this ( index
                              , reinterpret_cast<const void**>(&key)
                              , &keyLen
                              , reinterpret_cast<void**>(&val));

                // decode / dispatch it

        	    CString name = CUnicodeUtils::GetUnicode (key);
	            CString value = CUnicodeUtils::GetUnicode (*val);

                if (name == svnLog)
                    standardRevProps.message = value;
                else if (name == svnAuthor)
                    standardRevProps.author = value;
                else if (name == svnDate)
                {
	                standardRevProps.timeStamp = NULL;
	                if (value[0])
		                SVN_ERR (svn_time_from_cstring 
                                    (&standardRevProps.timeStamp, *val, pool));
                }
                else
                {
				    std::auto_ptr<UserRevProp> revProp (new UserRevProp);
                    revProp->name = name;
                    revProp->value = value;

                    userRevProps.Add (revProp.release());
                }
            }
        }
	}
	catch (CMemoryException * e)
	{
		e->Delete();
	}

	// the individual changes

	LogChangedPathArray changedPaths;
	try
	{
        if (log_entry->changed_paths != NULL)
		{
			apr_array_header_t *sorted_paths 
				= svn_sort__hash (log_entry->changed_paths, svn_sort_compare_items_as_paths, pool);

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
					= (svn_log_changed_path_t *) apr_hash_get ( log_entry->changed_paths
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

				changedPaths.Add (changedPath.release());
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
        receiver->ReceiveLog ( receiverBaton->includeChanges 
                                   ? &changedPaths 
                                   : NULL
							 , log_entry->revision
                             , receiverBaton->includeStandardRevProps 
                                   ? &standardRevProps 
                                   : NULL
                             , receiverBaton->includeUserRevProps 
                                   ? &userRevProps 
                                   : NULL
                             , log_entry->has_children != FALSE);
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
                       , bool includeChanges
                       , bool includeMerges
                       , bool includeStandardRevProps
                       , bool includeUserRevProps
                       , const TRevPropNames& userRevProps)
{
	SVNPool localpool (pool);

    // construct parameters:

    // everything we need to relay the result to the receiver

    SBaton baton = { receiver
                   , includeChanges
                   , includeStandardRevProps
                   , includeUserRevProps};

    // build list of revprops to fetch. Fetch all of them
    // if all user-revprops are requested but no std-revprops
    // (post-filter before them passing to the receiver)

    apr_array_header_t* revprops = NULL;
    if (includeStandardRevProps)
    {
        // fetch user rev-props?

        if (includeUserRevProps)
        {
            // fetch some but not all user rev-props?

            if (!userRevProps.empty())
            {
        	    revprops = apr_array_make ( localpool
                                          ,   (int)GetStandardRevProps().size()
                                            + (int)userRevProps.size()
                                          , sizeof(const char *));

                AppendStrings (localpool, revprops, GetStandardRevProps());
                AppendStrings (localpool, revprops, userRevProps);
            }
        }
        else
        {
            // standard revprops only

        	revprops = apr_array_make ( localpool
                                      , (int)GetStandardRevProps().size()
                                      , sizeof(const char *));

            AppendStrings (localpool, revprops, GetStandardRevProps());
        }
    }
    else
    {
        // fetch some but not all user rev-props?

        if (includeUserRevProps && !userRevProps.empty())
        {
        	revprops = apr_array_make ( localpool
                                      , (int)userRevProps.size()
                                      , sizeof(const char *));

            AppendStrings (localpool, revprops, userRevProps);
        }
    }

	svn_error_t *result = svn_client_log4 ( targets.MakePathArray (localpool)
										  , peg_revision
										  , start
										  , end
										  , limit
                                          , includeChanges ? TRUE : FALSE
										  , strictNodeHistory ? TRUE : FALSE
                                          , includeMerges ? TRUE : FALSE
                                          , revprops
										  , LogReceiver
										  , (void *)&baton
										  , context
										  , localpool);

    if (result != NULL)
		throw SVNError (result);
}

