// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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

#include "StdAfx.h"
#include "shlwapi.h"
#include ".\BugtraqInfo.h"

BugtraqInfo::BugtraqInfo(void)
{
}

BugtraqInfo::~BugtraqInfo(void)
{
}

BOOL BugtraqInfo::ReadProps(CString path)
{
	BOOL bFoundProps = FALSE;
	path.Replace('/', '\\');
	if (!PathIsDirectory(path))
	{
		path = path.Mid(path.ReverseFind('\\'));
	}
	for (;;)
	{
		SVNProperties props(path);
		for (int i=0; i<props.GetCount(); ++i)
		{
			CString sPropName = props.GetItemName(i).c_str();
			stdstring sPropVal = props.GetItemValue(i);
			if (sPropName.Compare(BUGTRAQPROPNAME_LABEL)==0)
			{
#ifdef UNICODE
				sLabel = MultibyteToWide((char *)sPropVal.c_str()).c_str();
#else
				sLabel = sPropVal.c_str();
#endif
				bFoundProps = TRUE;
			}
			if (sPropName.Compare(BUGTRAQPROPNAME_MESSAGE)==0)
			{
#ifdef UNICODE
				sLabel = MultibyteToWide((char *)sPropVal.c_str()).c_str();
#else
				sLabel = sPropVal.c_str();
#endif
				sMessage = props.GetItemValue(i).c_str();
				bFoundProps = TRUE;
			}
			if (sPropName.Compare(BUGTRAQPROPNAME_REGEX)==0)
			{
#ifdef UNICODE
				sRegex = MultibyteToWide((char *)sPropVal.c_str()).c_str();
#else
				sRegex = sPropVal.c_str();
#endif
				bFoundProps = TRUE;
			}
			if (sPropName.Compare(BUGTRAQPROPNAME_URL)==0)
			{
#ifdef UNICODE
				sUrl = MultibyteToWide((char *)sPropVal.c_str()).c_str();
#else
				sUrl = sPropVal.c_str();
#endif
				bFoundProps = TRUE;
			}
		}
		if (bFoundProps)
			return TRUE;
		if (PathIsRoot(path))
			return FALSE;
		path = path.Mid(path.ReverseFind('\\'));
		if (!PathFileExists(path + _T("\\") + _T(SVN_WC_ADM_DIR_NAME)))
			return FALSE;
	}
	return FALSE;		//never reached
}
