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
	bNumber = TRUE;
}

BugtraqInfo::~BugtraqInfo(void)
{
}

BOOL BugtraqInfo::ReadPropsTempfile(CString path)
{
	CString strLine;
	try
	{
		CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
		// for every selected file/folder
		while (file.ReadString(strLine))
		{
			if (ReadProps(strLine))
			{
				file.Close();
				return TRUE;
			}
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in Commit!\n");
		pE->Delete();
	}
	return FALSE;
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
				sMessage = MultibyteToWide((char *)sPropVal.c_str()).c_str();
#else
				sMessage = sPropVal.c_str();
#endif
				bFoundProps = TRUE;
			}
			if (sPropName.Compare(BUGTRAQPROPNAME_NUMBER)==0)
			{
				CString val;
#ifdef UNICODE
				val = MultibyteToWide((char *)sPropVal.c_str()).c_str();
#else
				val = sPropVal.c_str();
#endif
				if (val.CompareNoCase(_T("false"))==0)
					bNumber = FALSE;
				else
					bNumber = TRUE;
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
		path = path.Left(path.ReverseFind('\\'));
		if (!PathFileExists(path + _T("\\") + _T(SVN_WC_ADM_DIR_NAME)))
			return FALSE;
	}
	return FALSE;		//never reached
}

BOOL BugtraqInfo::FindBugID(const CString& msg, int& offset1, int& offset2)
{
	if (sUrl.IsEmpty())
		return FALSE;
	//if we have a message format, we look for that in the last line of the log
	//message only
	if (!sMessage.IsEmpty())
	{
		CString sLastLine;
		if (msg.ReverseFind('\n')>=0)
			sLastLine = msg.Mid(msg.ReverseFind('\n')+1);
		if (sMessage.Find(_T("%BUGID%"))<0)
			return FALSE;
		CString sFirstPart = sLastLine.Left(sMessage.Find(_T("%BUGID%")));
		CString sLastPart = sLastLine.Mid(sMessage.Find(_T("%BUGID%"))+7);
		CString sBugIDPart = sLastLine.Mid(sFirstPart.GetLength(), sLastLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
		if (sBugIDPart.IsEmpty())
			return FALSE;
		offset1 = msg.GetLength() - sLastLine.GetLength() + sFirstPart.GetLength();
		offset2 = offset1 + sBugIDPart.GetLength();
		return TRUE;
	}
	return FALSE;
}

CString BugtraqInfo::GetBugIDUrl(const CString& msg)
{
	CString ret;
	if (sUrl.IsEmpty())
		return ret;
	//if we have a message format, we look for that in the last line of the log
	//message only
	if (!sMessage.IsEmpty())
	{
		CString sLastLine;
		if (msg.ReverseFind('\n')>=0)
			sLastLine = msg.Mid(msg.ReverseFind('\n')+1);
		if (sMessage.Find(_T("%BUGID%"))<0)
			return ret;
		CString sFirstPart = sLastLine.Left(sMessage.Find(_T("%BUGID%")));
		CString sLastPart = sLastLine.Mid(sMessage.Find(_T("%BUGID%"))+7);
		CString sBugIDPart = sLastLine.Mid(sFirstPart.GetLength(), sLastLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
		if (sBugIDPart.IsEmpty())
			return ret;
		ret = sUrl;
		ret.Replace(_T("%BUGID%"), sBugIDPart);
	}
	return ret;
}




