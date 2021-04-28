// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010, 2012, 2014, 2020-2021 - TortoiseSVN

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
#include "UnIgnoreCommand.h"

#include "PathUtils.h"
#include "SVNProperties.h"

bool UnIgnoreCommand::Execute()
{
    BOOL              err = FALSE;
    std::set<CString> removedItems;
    bool              bRecursive = !!parser.HasKey(L"recursive");
    for (int nPath = 0; nPath < pathList.GetCount(); nPath++)
    {
        CString name = CPathUtils::PathPatternEscape(pathList[nPath].GetFileOrDirectoryName());
        if (parser.HasKey(L"onlymask"))
        {
            name = L"*" + pathList[nPath].GetFileExtension();
        }
        removedItems.insert(name);
        CTSVNPath     parentFolder = pathList[nPath].GetContainingDirectory();
        SVNProperties props(parentFolder, SVNRev::REV_WC, false, false);
        CString       value;
        for (int i = 0; i < props.GetCount(); i++)
        {
            if (!bRecursive && (props.GetItemName(i).compare(SVN_PROP_IGNORE) == 0))
            {
                //treat values as normal text even if they're not
                value = CUnicodeUtils::GetUnicode(props.GetItemValue(i).c_str());
                break;
            }
            else if (bRecursive && (props.GetItemName(i).compare(SVN_PROP_INHERITABLE_IGNORES) == 0))
            {
                //treat values as normal text even if they're not
                value = CUnicodeUtils::GetUnicode(props.GetItemValue(i).c_str());
                break;
            }
        }
        value = value.Trim(L"\n\r");
        value += L"\n";
        value.Remove('\r');
        value.Replace(L"\n\n", L"\n");

        // Delete all occurrences of 'name'
        // "\n" is temporarily prepended to make the algorithm easier
        value = L"\n" + value;
        value.Replace(L"\n" + name + L"\n", L"\n");
        value = value.Mid(1);

        CString sTrimmedValue = value;
        sTrimmedValue.Trim();
        if (sTrimmedValue.IsEmpty())
        {
            if (!props.Remove(bRecursive ? SVN_PROP_INHERITABLE_IGNORES : SVN_PROP_IGNORE))
            {
                if (!parser.HasKey(L"noui"))
                {
                    CString temp;
                    temp.Format(IDS_ERR_FAILEDUNIGNOREPROPERTY, static_cast<LPCWSTR>(name));
                    MessageBox(GetExplorerHWND(), temp, L"TortoiseSVN", MB_ICONERROR);
                }
                err = TRUE;
                break;
            }
        }
        else
        {
            if (!props.Add(bRecursive ? SVN_PROP_INHERITABLE_IGNORES : SVN_PROP_IGNORE, static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(value))))
            {
                if (!parser.HasKey(L"noui"))
                {
                    CString temp;
                    temp.Format(IDS_ERR_FAILEDUNIGNOREPROPERTY, static_cast<LPCWSTR>(name));
                    MessageBox(GetExplorerHWND(), temp, L"TortoiseSVN", MB_ICONERROR);
                }
                err = TRUE;
                break;
            }
        }
    }
    if (err == FALSE)
    {
        if (!parser.HasKey(L"noui"))
        {
            CString fileList;
            for (auto it = removedItems.cbegin(); it != removedItems.cend(); ++it)
            {
                fileList += *it;
                fileList += L"\n";
            }
            CString temp;
            temp.Format(bRecursive ? IDS_PROC_UNIGNORERECURSIVESUCCESS : IDS_PROC_UNIGNORESUCCESS, static_cast<LPCWSTR>(fileList));
            MessageBox(GetExplorerHWND(), temp, L"TortoiseSVN", MB_ICONINFORMATION);
        }
        return true;
    }
    return false;
}
