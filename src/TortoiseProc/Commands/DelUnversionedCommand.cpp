// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010, 2012, 2014, 2021 - TortoiseSVN

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
#include "DelUnversionedCommand.h"
#include "DeleteUnversionedDlg.h"
#include "StringUtils.h"

bool DelUnversionedCommand::Execute()
{
    bool                  bRet = false;
    CDeleteUnversionedDlg dlg;
    dlg.m_pathList = pathList;
    if (dlg.DoModal() == IDOK)
    {
        if (dlg.m_pathList.GetCount() == 0)
            return FALSE;
        // now remove all items by moving them to the trash bin
        dlg.m_pathList.RemoveChildren();
        CString fileList;
        for (INT_PTR i = 0; i < dlg.m_pathList.GetCount(); ++i)
        {
            fileList += dlg.m_pathList[i].GetWinPathString();
            fileList += L"|";
        }
        fileList += L"|";
        int  len = fileList.GetLength();
        auto buf = std::make_unique<wchar_t[]>(len + 2);
        wcscpy_s(buf.get(), len + 2LL, fileList);
        CStringUtils::PipesToNulls(buf.get(), len);
        SHFILEOPSTRUCT fileOp;
        fileOp.hwnd   = GetExplorerHWND();
        fileOp.wFunc  = FO_DELETE;
        fileOp.pFrom  = buf.get();
        fileOp.pTo    = nullptr;
        fileOp.fFlags = FOF_NO_CONNECTED_ELEMENTS;
        fileOp.fFlags |= dlg.m_bUseRecycleBin ? FOF_ALLOWUNDO : 0;
        fileOp.lpszProgressTitle = static_cast<LPCWSTR>(CString(MAKEINTRESOURCE(IDS_DELUNVERSIONED)));
        bRet                     = (SHFileOperation(&fileOp) == 0);
    }
    return bRet;
}