// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007, 2010-2011, 2014 - TortoiseSVN

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
#include "RebuildIconCacheCommand.h"

#include "ShellUpdater.h"
#include "AppUtils.h"

bool RebuildIconCacheCommand::Execute()
{
    bool bQuiet = !!parser.HasKey(L"noquestion");
    if (CShellUpdater::RebuildIcons())
    {
        if (!bQuiet)
            TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_SUCCESS), MAKEINTRESOURCE(IDS_PROC_ICONCACHEREBUILT), TDCBF_OK_BUTTON, TD_INFORMATION_ICON, NULL);
    }
    else
    {
        if (!bQuiet)
            TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_PROC_ICONCACHENOTREBUILT), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
    }
    return true;
}