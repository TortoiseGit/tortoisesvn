﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2012, 2014-2015, 2021-2022 - TortoiseSVN

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
#pragma once
#include "Command.h"
#include "TortoiseProc.h"

/**
 * \ingroup TortoiseProc
 * Shows the help file.
 */
class HelpCommand : public Command
{
public:
    /**
     * Executes the command.
     */
    bool Execute() override
    {
        if (!CAppUtils::StartHtmlHelp(0))
        {
            AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
            return false;
        };
        return true;
    }
    bool CheckPaths() override { return true; }
};
