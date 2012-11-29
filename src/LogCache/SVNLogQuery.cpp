// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2011 - TortoiseSVN

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
#include "stdafx.h"

#include "SVNLogQuery.h"
#include "ILogReceiver.h"

#pragma warning(push)
#include "svn_time.h"
#include "svn_sorts.h"
#include "svn_compat.h"
#include "svn_props.h"
#pragma warning(pop)
#include "UnicodeUtils.h"
#include "SVN.h"
#include "SVNError.h"
#include "SVNHelpers.h"
#include "TSVNPath.h"
#include "svntrace.h"
#include "Hooks.h"

// SVN API utility

void CSVNLogQuery::AppendStrings ( SVNPool& pool
                                 , apr_array_header_t* array
                                 , const std::vector<std::string>& strings)
{
    for (size_t i = 0; i  < strings.size(); ++i)
    {
        const string& utf8String = strings[i];

        size_t size = utf8String.length()+1;
        char * aprString = reinterpret_cast<char *>(apr_palloc (pool, size));
        memcpy (aprString, utf8String.c_str(), size);

        (*((char **) apr_array_push (array))) = aprString;
    }
}

// standard revision properties

const TRevPropNames& CSVNLogQuery::GetStandardRevProps()
{
    static TRevPropNames standardRevProps;

    if (standardRevProps.empty())
    {
        standardRevProps.push_back (std::string (SVN_PROP_REVISION_LOG));
        standardRevProps.push_back (std::string (SVN_PROP_REVISION_DATE));
        standardRevProps.push_back (std::string (SVN_PROP_REVISION_AUTHOR));
    }

    return standardRevProps;
}

// receive the information from SVN and relay it to our

svn_error_t* CSVNLogQuery::LogReceiver ( void *baton
                                       , svn_log_entry_t *log_entry
                                       , apr_pool_t *pool)
{
    // a few globals

    static const std::string svnLog (SVN_PROP_REVISION_LOG);
    static const std::string svnDate (SVN_PROP_REVISION_DATE);
    static const std::string svnAuthor (SVN_PROP_REVISION_AUTHOR);

    // just in case ...

    if (log_entry == NULL)
        return NULL;

    // where to send the pre-processed in-format

    SBaton* receiverBaton = reinterpret_cast<SBaton*>(baton);
    ILogReceiver* receiver = receiverBaton->receiver;
    assert (receiver != NULL);

    // parse revprops

    std::string author;
    std::string message;
    __time64_t timeStamp = 0;

    UserRevPropArray userRevProps;

    try
    {
        if (   (log_entry->revision != SVN_INVALID_REVNUM)
            && (log_entry->revprops != NULL))
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

                std::string name = key;
                std::string value = *val;

                if (name == svnLog)
                    message = value;
                else if (name == svnAuthor)
                    author = value;
                else if (name == svnDate)
                {
                    timeStamp = NULL;
                    if (value[0])
                        SVN_ERR (svn_time_from_cstring (&timeStamp, *val, pool));
                }
                else
                {
                    userRevProps.Add (name, value);
                }
            }
        }
    }
    catch (CMemoryException * e)
    {
        e->Delete();
    }

    StandardRevProps standardRevProps (author, message, timeStamp);

    // the individual changes

    TChangedPaths changedPaths;
    try
    {
        if (log_entry->changed_paths2 != NULL)
        {
            apr_array_header_t *sorted_paths
                = svn_sort__hash (log_entry->changed_paths2, svn_sort_compare_items_as_paths, pool);

            for (int i = 0, count = sorted_paths->nelts; i < count; ++i)
            {
                changedPaths.push_back (SChangedPath());
                SChangedPath& entry = changedPaths.back();

                // find the item in the hash

                svn_sort__item_t *item = &(APR_ARRAY_IDX ( sorted_paths
                    , i
                    , svn_sort__item_t));

                // extract the path name

                entry.path = SVN::MakeUIUrlOrPath ((const char *)item->key);

                // decode the action

                svn_log_changed_path2_t *log_item
                    = (svn_log_changed_path2_t *) apr_hash_get ( log_entry->changed_paths2
                    , item->key
                    , item->klen);
                static const char actionKeys[5] = "AMRD";
                const char* actionKey = strchr (actionKeys, log_item->action);

                entry.action = actionKey == NULL
                    ? 0
                    : 1 << (actionKey - actionKeys);

                // node type

                entry.nodeKind = log_item->node_kind;

                // decode copy-from info

                if (    log_item->copyfrom_path
                     && SVN_IS_VALID_REVNUM (log_item->copyfrom_rev))
                {
                    entry.copyFromPath = SVN::MakeUIUrlOrPath (log_item->copyfrom_path);
                    entry.copyFromRev = log_item->copyfrom_rev;
                }
                else
                {
                    entry.copyFromRev = 0;
                }

                entry.text_modified = log_item->text_modified;
                entry.props_modified = log_item->props_modified;
            }
        }
        else if (log_entry->changed_paths != NULL)
        {
            apr_array_header_t *sorted_paths
                = svn_sort__hash (log_entry->changed_paths, svn_sort_compare_items_as_paths, pool);

            for (int i = 0, count = sorted_paths->nelts; i < count; ++i)
            {
                changedPaths.push_back (SChangedPath());
                SChangedPath& entry = changedPaths.back();

                // find the item in the hash

                svn_sort__item_t *item = &(APR_ARRAY_IDX ( sorted_paths
                                                         , i
                                                         , svn_sort__item_t));

                // extract the path name

                entry.path = SVN::MakeUIUrlOrPath ((const char *)item->key);

                // decode the action

                svn_log_changed_path_t *log_item
                    = (svn_log_changed_path_t *) apr_hash_get ( log_entry->changed_paths
                                                              , item->key
                                                              , item->klen);
                static const char actionKeys[5] = "AMRD";
                const char* actionKey = strchr (actionKeys, log_item->action);

                entry.action = actionKey == NULL
                    ? 0
                    : 1 << (actionKey - actionKeys);

                // node type

                entry.nodeKind = svn_node_unknown;

                // decode copy-from info

                if (    log_item->copyfrom_path
                     && SVN_IS_VALID_REVNUM (log_item->copyfrom_rev))
                {
                    entry.copyFromPath = SVN::MakeUIUrlOrPath (log_item->copyfrom_path);
                    entry.copyFromRev = log_item->copyfrom_rev;
                }
                else
                {
                    entry.copyFromRev = 0;
                }

                entry.text_modified = svn_tristate_unknown;
                entry.props_modified = svn_tristate_unknown;
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
        // treat revision 0 special: only report/show it if either the author or a message is set, or if user props are set
        if (   log_entry->revision
            || userRevProps.GetCount()
            || !standardRevProps.GetAuthor().empty()
            || !standardRevProps.GetMessage().empty())
        {
            MergeInfo mergeInfo = { log_entry->has_children != FALSE
                                  , log_entry->non_inheritable != FALSE
                                  , log_entry->subtractive_merge != FALSE };

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
                                 , &mergeInfo);
        }
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

    // list of revision ranges to fetch
    // (as of now, there is only one such range)

    svn_opt_revision_range_t revision_range = {*start, *end};

    apr_array_header_t* revision_ranges
        = apr_array_make (localpool, 1, sizeof(apr_array_header_t*));
    *(svn_opt_revision_range_t**)apr_array_push (revision_ranges)
        = &revision_range;

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

    CHooks::Instance().PreConnect(targets);
    SVNTRACE (
        svn_error_t *result = svn_client_log5 ( targets.MakePathArray (localpool)
                                              , peg_revision
                                              , revision_ranges
                                              , limit
                                              , includeChanges
                                              , strictNodeHistory
                                              , includeMerges
                                              , revprops
                                              , LogReceiver
                                              , (void *)&baton
                                              , context
                                              , localpool),
        NULL
    );

    if (result != NULL)
        throw SVNError (result);
}

