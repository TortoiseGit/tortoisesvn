// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include "SVNExternals.h"
#include "SVNHelpers.h"
#include "SVNError.h"
#include "SVN.h"
#include "SVNRev.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"

SVNExternals::SVNExternals()
{
}

SVNExternals::~SVNExternals()
{
}

bool SVNExternals::Add(const CTSVNPath& path, const std::string& extvalue, bool fetchrev, svn_revnum_t headrev)
{
	m_originals[path] = extvalue;
	SVN svn;
	CString pathurl = svn.GetURLFromPath(path);
	SVNExternal ext;

	SVNPool pool;
	apr_array_header_t* parsedExternals = NULL;
	SVNError svnError 
		(svn_wc_parse_externals_description3 
							( &parsedExternals
							, path.GetSVNApiPath(pool)
							, extvalue.c_str()
							, TRUE
							, pool));

	if (svnError.GetCode() == 0)
	{
		for (long i=0; i < parsedExternals->nelts; ++i)
		{
			svn_wc_external_item2_t * e = APR_ARRAY_IDX(parsedExternals, i, svn_wc_external_item2_t*);

			if (e != NULL)
			{
				ext.path = path;
				ext.pathurl = pathurl;
				ext.pegrevision = e->peg_revision;
                if ((e->peg_revision.kind != e->revision.kind)||
                    (e->peg_revision.value.number != e->revision.value.number))
                {
                    ext.revision = e->revision;
                    ext.origrevision = e->revision;
                }
                else
                {
                    ext.revision.kind = svn_opt_revision_head;
                    ext.origrevision.kind = svn_opt_revision_head;
                }
				if (headrev >= 0)
				{
					ext.revision.kind = svn_opt_revision_number;
					ext.revision.value.number = headrev;
				}
				ext.url = CUnicodeUtils::GetUnicode(e->url);
				ext.targetDir = CUnicodeUtils::GetUnicode(e->target_dir);
				if (fetchrev)
				{
					svn_revnum_t maxrev, minrev;
					bool bswitched, bmodified, bsparse;
					CTSVNPath p = path;
					p.AppendPathString(ext.targetDir);
					if (svn.GetWCRevisionStatus(p, true, minrev, maxrev, bswitched, bmodified, bsparse))
					{
						ext.revision.kind = svn_opt_revision_number;
						ext.revision.value.number = maxrev;
					}
				}
				push_back(ext);
			}
		}
		return true;
	}
	return false;
}

bool SVNExternals::TagExternals(bool bRemote, const CString& message, svn_revnum_t headrev, const CTSVNPath& origurl, const CTSVNPath& tagurl)
{
	// create a map of paths and external properties
	// so that we have all externals which belong to the same
	// property
	class sb
	{
	public:
		sb() : adjust(false) {}

		std::string	extvalue;
		CString		pathurl;
		bool		adjust;
	};

	std::map<CTSVNPath, sb> externals;
	for (std::vector<SVNExternal>::iterator it = begin(); it != end(); ++it)
	{
		SVNRev rev = it->revision;
		SVNRev origrev = it->origrevision;
		SVNRev pegrev = it->pegrevision;
		CString peg;
		if (pegrev.IsValid() && !pegrev.IsHead())
			peg = _T("@") + pegrev.ToString();
		else if (it->adjust)
			peg = _T("@") + rev.ToString();
		else
			peg.Empty();

		CString temp;
		if (it->adjust)
			temp.Format(_T("-r %s %s%s %s"), rev.ToString(), it->url, (LPCTSTR)peg, it->targetDir);
		else if (origrev.IsValid())
			temp.Format(_T("-r %s %s%s %s"), origrev.ToString(), it->url, (LPCTSTR)peg, it->targetDir);
		else
			temp.Format(_T("%s%s %s"), it->url, (LPCTSTR)peg, it->targetDir);

		sb val = externals[it->path];
		if (!val.extvalue.empty())
			val.extvalue += "\n";
		val.extvalue += CUnicodeUtils::StdGetUTF8((LPCTSTR)temp);
		val.adjust = val.adjust || it->adjust;
		val.pathurl = it->pathurl;
		externals[it->path] = val;
	}

	SVN svn;
	// now set the new properties
	for (std::map<CTSVNPath, sb>::iterator it = externals.begin(); it != externals.end(); ++it)
	{
		if (it->second.adjust)
		{
			if (bRemote)
			{
				// construct the url where to set the tagged svn:externals property
				// example paths:
				// origurl : http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk
				// tagurl  : http://tortoisesvn.tigris.org/svn/tortoisesvn/tags/version-1.6.7
				// pathurl : http://tortoisesvn.tigris.org/svn/tortoisesvn/trunk/ext
				
				CString sInsidePath = it->second.pathurl.Mid(origurl.GetSVNPathString().GetLength()); // 'ext'
				if (!origurl.IsUrl())
				{
					sInsidePath = it->second.pathurl.Mid(svn.GetURLFromPath(origurl).GetLength());
				}
				 
				CTSVNPath targeturl = tagurl; // http://tortoisesvn.tigris.org/svn/tortoisesvn/tags/version-1.6.7
				targeturl.AppendRawString(sInsidePath); // http://tortoisesvn.tigris.org/svn/tortoisesvn/tags/version-1.6.7/ext

				SVNProperties props(targeturl, headrev, false);
				props.Add(SVN_PROP_EXTERNALS, it->second.extvalue, false, svn_depth_empty, message);
			}
			else
			{
				SVNProperties props(it->first, SVNRev::REV_WC, false);
				props.Add(SVN_PROP_EXTERNALS, it->second.extvalue);
			}
		}
	}

	return true;
}

std::string SVNExternals::GetValue(const CTSVNPath& path)
{
    std::string ret;
    for (std::vector<SVNExternal>::iterator it = begin(); it != end(); ++it)
    {
        if (path.IsEquivalentToWithoutCase(it->path))
        {
            SVNRev rev = it->revision;
            SVNRev pegrev = it->pegrevision;
            CString peg;
            if (pegrev.IsValid() && !pegrev.IsHead())
                peg = _T("@") + pegrev.ToString();
            else
                peg.Empty();

            CString temp;
            if (rev.IsValid() && !rev.IsHead())
                temp.Format(_T("-r %s %s%s %s"), rev.ToString(), it->url, (LPCTSTR)peg, it->targetDir);
            else
                temp.Format(_T("%s%s %s"), it->url, (LPCTSTR)peg, it->targetDir);
            if (ret.size())
                ret += "\n";
            ret += CUnicodeUtils::StdGetUTF8((LPCTSTR)temp);
        }
    }

    return ret;
}

bool SVNExternals::RestoreExternals()
{
	for (std::map<CTSVNPath, std::string>::iterator it = m_originals.begin(); it != m_originals.end(); ++it)
	{
		SVNProperties props(it->first, SVNRev::REV_WC, false);
		props.Add(SVN_PROP_EXTERNALS, it->second);
	}

	return true;
}
