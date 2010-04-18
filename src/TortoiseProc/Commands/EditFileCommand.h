// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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
#include "SVNRev.h"

/**
 * \ingroup TortoiseProc
 * - Checks out a file (if it hasn't yet),
 * - locks it, if required
 * - opens file in editor (via Windows Shell)
 * - commit changes & clean up.
 */
class EditFileCommand : public Command
{
private:

    /// "revision" parameter, if specified

    SVNRev revision;

    /// where the file can be found in a WC after \ref AutoCheckout

    CTSVNPath path;

    /// if set, we must unlock the file upon cleanup

    bool needsUnLock;

    /// status check

    bool IsModified();
    bool IsLocked();

    /// the individual steps of the sequence

    bool AutoCheckout();
    bool AutoLock();
    bool Edit();
    bool AutoCheckin();
    bool AutoUnLock();

public:

    /// construction / destruction

    EditFileCommand();
    virtual ~EditFileCommand();

    /**
     * Executes the command.
     */
    virtual bool            Execute();
};


