// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2010-2011, 2013-2014 - TortoiseSVN

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
#include "Command.h"

#include "AboutCommand.h"
#include "AddCommand.h"
#include "AutoTextTestCommand.h"
#include "BlameCommand.h"
#include "CatCommand.h"
#include "CheckoutCommand.h"
#include "CleanupCommand.h"
#include "CommitCommand.h"
#include "ConflictEditorCommand.h"
#include "CopyCommand.h"
#include "CrashCommand.h"
#include "CreatePatchCommand.h"
#include "CreateRepositoryCommand.h"
#include "DelUnversionedCommand.h"
#include "DiffCommand.h"
#include "DropCopyAddCommand.h"
#include "DropCopyCommand.h"
#include "DropExportCommand.h"
#include "DropExternalCommand.h"
#include "DropMoveCommand.h"
#include "DropVendorCommand.h"
#include "EditFileCommand.h"
#include "ExportCommand.h"
#include "HelpCommand.h"
#include "IgnoreCommand.h"
#include "ImportCommand.h"
#include "LockCommand.h"
#include "LogCommand.h"
#include "MergeCommand.h"
#include "MergeAllCommand.h"
#include "PasteCopyCommand.h"
#include "PasteMoveCommand.h"
#include "PrevDiffCommand.h"
#include "PropertiesCommand.h"
#include "RebuildIconCacheCommand.h"
#include "RelocateCommand.h"
#include "RemoveCommand.h"
#include "RenameCommand.h"
#include "RepositoryBrowserCommand.h"
#include "RepoStatusCommand.h"
#include "ResolveCommand.h"
#include "RevertCommand.h"
#include "RevisionGraphCommand.h"
#include "RTFMCommand.h"
#include "SettingsCommand.h"
#include "ShowCompareCommand.h"
#include "SwitchCommand.h"
#include "UnIgnoreCommand.h"
#include "UnLockCommand.h"
#include "UpdateCheckCommand.h"
#include "UpdateCommand.h"
#include "UrlDiffCommand.h"
#include "WcUpgradeCommand.h"


typedef enum
{
    cmdAbout,
    cmdAdd,
    cmdAutoTextTest,
    cmdBlame,
    cmdCat,
    cmdCheckout,
    cmdCleanup,
    cmdCommit,
    cmdConflictEditor,
    cmdCopy,
    cmdCrash,
    cmdCreatePatch,
    cmdDelUnversioned,
    cmdDiff,
    cmdDropCopy,
    cmdDropCopyAdd,
    cmdDropExport,
    cmdDropExternals,
    cmdDropMove,
    cmdDropVendor,
    cmdEditFile,
    cmdExport,
    cmdHelp,
    cmdIgnore,
    cmdImport,
    cmdLock,
    cmdLog,
    cmdMerge,
    cmdMergeAll,
    cmdPasteCopy,
    cmdPasteMove,
    cmdPrevDiff,
    cmdProperties,
    cmdRTFM,
    cmdRebuildIconCache,
    cmdRelocate,
    cmdRemove,
    cmdRename,
    cmdRepoBrowser,
    cmdRepoCreate,
    cmdRepoStatus,
    cmdResolve,
    cmdRevert,
    cmdRevisionGraph,
    cmdSettings,
    cmdShowCompare,
    cmdSwitch,
    cmdUnIgnore,
    cmdUnlock,
    cmdUpdate,
    cmdUpdateCheck,
    cmdUrlDiff,
    cmdWcUpgrade,
} TSVNCommand;

static const struct CommandInfo
{
    TSVNCommand command;
    LPCTSTR pCommandName;
} commandInfo[] =
{
    {   cmdAbout,           L"about"             },
    {   cmdAdd,             L"add"               },
    {   cmdAutoTextTest,    L"autotexttest"      },
    {   cmdBlame,           L"blame"             },
    {   cmdCat,             L"cat"               },
    {   cmdCheckout,        L"checkout"          },
    {   cmdCleanup,         L"cleanup"           },
    {   cmdCommit,          L"commit"            },
    {   cmdConflictEditor,  L"conflicteditor"    },
    {   cmdCopy,            L"copy"              },
    {   cmdCrash,           L"crash"             },
    {   cmdCreatePatch,     L"createpatch"       },
    {   cmdDelUnversioned,  L"delunversioned"    },
    {   cmdDiff,            L"diff"              },
    {   cmdDropCopy,        L"dropcopy"          },
    {   cmdDropCopyAdd,     L"dropcopyadd"       },
    {   cmdDropExport,      L"dropexport"        },
    {   cmdDropExternals,   L"dropexternals"     },
    {   cmdDropMove,        L"dropmove"          },
    {   cmdDropVendor,      L"dropvendor"        },
    {   cmdEditFile,        L"editfile"          },
    {   cmdExport,          L"export"            },
    {   cmdHelp,            L"help"              },
    {   cmdIgnore,          L"ignore"            },
    {   cmdImport,          L"import"            },
    {   cmdLock,            L"lock"              },
    {   cmdLog,             L"log"               },
    {   cmdMerge,           L"merge"             },
    {   cmdMergeAll,        L"mergeall"          },
    {   cmdPasteCopy,       L"pastecopy"         },
    {   cmdPasteMove,       L"pastemove"         },
    {   cmdPrevDiff,        L"prevdiff"          },
    {   cmdProperties,      L"properties"        },
    {   cmdRTFM,            L"rtfm"              },
    {   cmdRebuildIconCache,L"rebuildiconcache"  },
    {   cmdRelocate,        L"relocate"          },
    {   cmdRemove,          L"remove"            },
    {   cmdRename,          L"rename"            },
    {   cmdRepoBrowser,     L"repobrowser"       },
    {   cmdRepoCreate,      L"repocreate"        },
    {   cmdRepoStatus,      L"repostatus"        },
    {   cmdResolve,         L"resolve"           },
    {   cmdRevert,          L"revert"            },
    {   cmdRevisionGraph,   L"revisiongraph"     },
    {   cmdSettings,        L"settings"          },
    {   cmdShowCompare,     L"showcompare"       },
    {   cmdSwitch,          L"switch"            },
    {   cmdUnIgnore,        L"unignore"          },
    {   cmdUnlock,          L"unlock"            },
    {   cmdUpdate,          L"update"            },
    {   cmdUpdateCheck,     L"updatecheck"       },
    {   cmdUrlDiff,         L"urldiff"           },
    {   cmdWcUpgrade,       L"wcupgrade"         },
};




Command * CommandServer::GetCommand(const CString& sCmd)
{
    // Look up the command
    TSVNCommand command = cmdAbout;     // Something harmless as a default
    for (int nCommand = 0; nCommand < _countof(commandInfo); nCommand++)
    {
        if (sCmd.Compare(commandInfo[nCommand].pCommandName) == 0)
        {
            // We've found the command
            command = commandInfo[nCommand].command;
            // If this fires, you've let the enum get out of sync with the commandInfo array
            ASSERT((int)command == nCommand);
            break;
        }
    }



    switch (command)
    {
        case cmdAbout:
            return new AboutCommand;
        case cmdAdd:
            return new AddCommand;
        case cmdAutoTextTest:
            return new AutoTextTestCommand;
        case cmdBlame:
            return new BlameCommand;
        case cmdCat:
            return new CatCommand;
        case cmdCheckout:
            return new CheckoutCommand;
        case cmdCleanup:
            return new CleanupCommand;
        case cmdCommit:
            return new CommitCommand;
        case cmdConflictEditor:
            return new ConflictEditorCommand;
        case cmdCopy:
            return new CopyCommand;
        case cmdCrash:
            return new CrashCommand;
        case cmdCreatePatch:
            return new CreatePatchCommand;
        case cmdDelUnversioned:
            return new DelUnversionedCommand;
        case cmdDiff:
            return new DiffCommand;
        case cmdDropCopy:
            return new DropCopyCommand;
        case cmdDropCopyAdd:
            return new DropCopyAddCommand;
        case cmdDropExport:
            return new DropExportCommand;
        case cmdDropExternals:
            return new DropExternalCommand;
        case cmdDropMove:
            return new DropMoveCommand;
        case cmdDropVendor:
            return new DropVendorCommand;
        case cmdEditFile:
            return new EditFileCommand;
        case cmdExport:
            return new ExportCommand;
        case cmdHelp:
            return new HelpCommand;
        case cmdIgnore:
            return new IgnoreCommand;
        case cmdImport:
            return new ImportCommand;
        case cmdLock:
            return new LockCommand;
        case cmdLog:
            return new LogCommand;
        case cmdMerge:
            return new MergeCommand;
        case cmdMergeAll:
            return new MergeAllCommand;
        case cmdPasteCopy:
            return new PasteCopyCommand;
        case cmdPasteMove:
            return new PasteMoveCommand;
        case cmdPrevDiff:
            return new PrevDiffCommand;
        case cmdProperties:
            return new PropertiesCommand;
        case cmdRTFM:
            return new RTFMCommand;
        case cmdRebuildIconCache:
            return new RebuildIconCacheCommand;
        case cmdRelocate:
            return new RelocateCommand;
        case cmdRemove:
            return new RemoveCommand;
        case cmdRename:
            return new RenameCommand;
        case cmdRepoBrowser:
            return new RepositoryBrowserCommand;
        case cmdRepoCreate:
            return new CreateRepositoryCommand;
        case cmdRepoStatus:
            return new RepoStatusCommand;
        case cmdResolve:
            return new ResolveCommand;
        case cmdRevert:
            return new RevertCommand;
        case cmdRevisionGraph:
            return new RevisionGraphCommand;
        case cmdSettings:
            return new SettingsCommand;
        case cmdShowCompare:
            return new ShowCompareCommand;
        case cmdSwitch:
            return new SwitchCommand;
        case cmdUnIgnore:
            return new UnIgnoreCommand;
        case cmdUnlock:
            return new UnLockCommand;
        case cmdUpdate:
            return new UpdateCommand;
        case cmdUpdateCheck:
            return new UpdateCheckCommand;
        case cmdUrlDiff:
            return new UrlDiffCommand;
        case cmdWcUpgrade:
            return new WcUpgradeCommand;

        default:
            return new AboutCommand;
    }
}

bool Command::CheckPaths()
{
    if ((pathList.GetCount() == 0) && (cmdLinePath.IsEmpty()))
    {
        TaskDialog(GetExplorerHWND(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_INVALIDPARAMS), MAKEINTRESOURCE(IDS_ERR_PATHPARAMMISSING), TDCBF_OK_BUTTON, TD_ERROR_ICON, NULL);
        return false;
    }

    return true;
}
