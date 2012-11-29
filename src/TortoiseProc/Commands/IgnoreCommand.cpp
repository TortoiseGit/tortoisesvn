// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010, 2012 - TortoiseSVN

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
#include "IgnoreCommand.h"

#include "MessageBox.h"
#include "PathUtils.h"
#include "SVNProperties.h"
#include "SVN.h"

bool IgnoreCommand::Execute()
{
    BOOL err = FALSE;
    std::set<CString> addeditems;
    for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
    {
        CString name = CPathUtils::PathPatternEscape(pathList[nPath].GetFileOrDirectoryName());
        if (parser.HasKey(_T("onlymask")))
        {
            name = _T("*")+CPathUtils::PathPatternEscape(pathList[nPath].GetFileOrDirExtension());
        }
        addeditems.insert(name);
        CTSVNPath parentfolder = pathList[nPath].GetContainingDirectory();
        SVNProperties props(parentfolder, SVNRev::REV_WC, false);
        CString value;
        for (int i=0; i<props.GetCount(); i++)
        {
            if (props.GetItemName(i).compare(SVN_PROP_IGNORE)==0)
            {
                //treat values as normal text even if they're not
                value = CUnicodeUtils::GetUnicode(props.GetItemValue(i).c_str());
            }
        }
        if (value.IsEmpty())
            value = name;
        else
        {
            // make sure we don't have duplicate entries
            std::set<CString> ignoreItems;
            ignoreItems.insert(name);
            CString token;
            int curPos = 0;
            token = value.Tokenize(_T("\n"),curPos);
            while (token != _T(""))
            {
                token.Trim();
                ignoreItems.insert(token);
                token = value.Tokenize(_T("\n"), curPos);
            };
            value.Empty();
            for (std::set<CString>::iterator it = ignoreItems.begin(); it != ignoreItems.end(); ++it)
            {
                value += *it;
                value += _T("\n");
            }
        }
        if (!props.Add(SVN_PROP_IGNORE, (LPCSTR)CUnicodeUtils::GetUTF8(value)))
        {
            CString temp;
            temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, (LPCTSTR)name);
            temp += _T("\n");
            temp += props.GetLastErrorMessage();
            MessageBox(GetExplorerHWND(), temp, _T("TortoiseSVN"), MB_ICONERROR);
            err = TRUE;
            break;
        }
    }
    if ((err == FALSE)&&(parser.HasKey(_T("delete"))))
    {
        SVN svn;
        if (!svn.Remove(pathList, TRUE))
        {
            svn.ShowErrorDialog(GetExplorerHWND(), pathList.GetCommonDirectory());
            err = TRUE;
        }
    }
    if (err == FALSE)
    {
        CString filelist;
        for (auto it = addeditems.cbegin(); it != addeditems.cend(); ++it)
        {
            filelist += *it;
            filelist += L"\n";
        }
        CString temp;
        temp.Format(IDS_PROC_IGNORESUCCESS, (LPCTSTR)filelist);
        MessageBox(GetExplorerHWND(), temp, _T("TortoiseSVN"), MB_ICONINFORMATION);
        return true;
    }
    return false;
}