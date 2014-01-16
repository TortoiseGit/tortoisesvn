// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006, 2013-2014 - TortoiseSVN

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
#include "registry.h"
#include "resource.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "SoundUtils.h"

#pragma comment(lib, "Winmm")

CSoundUtils::CSoundUtils(void)
{
}

CSoundUtils::~CSoundUtils(void)
{
}

void CSoundUtils::RegisterTSVNSounds()
{
    // create the event labels
    CRegString eventlabelerr = CRegString(L"AppEvents\\EventLabels\\TSVN_Error\\");
    eventlabelerr = CString(MAKEINTRESOURCE(IDS_ERR_ERROR));
    CRegString eventlabelwarn = CRegString(L"AppEvents\\EventLabels\\TSVN_Warning\\");
    eventlabelwarn = CString(MAKEINTRESOURCE(IDS_WARN_WARNING));
    CRegString eventlabelnote = CRegString(L"AppEvents\\EventLabels\\TSVN_Notification\\");
    eventlabelnote = CString(MAKEINTRESOURCE(IDS_WARN_NOTE));

    CRegString appscheme = CRegString(L"AppEvents\\Schemes\\Apps\\TortoiseProc\\");
    appscheme = L"TortoiseSVN";

    CString apppath = CPathUtils::GetAppDirectory();

    CRegistryKey schemenamekey = CRegistryKey(L"AppEvents\\Schemes\\Names");
    CStringList schemenames;
    schemenamekey.getSubKeys(schemenames);
    // if the sound scheme has been modified but not save under a different name,
    // the name of the sound scheme is ".current" and not under the names list.
    // so add the .current scheme to the list too
    schemenames.AddHead(L".current");
    POSITION pos;
    for (pos = schemenames.GetHeadPosition(); pos != NULL;)
    {
        CString name = schemenames.GetNext(pos);
        if ((name.CompareNoCase(L".none")!=0)&&(name.CompareNoCase(L".nosound")!=0))
        {
            CString errorkey = L"AppEvents\\Schemes\\Apps\\TortoiseProc\\TSVN_Error\\" + name + L"\\";
            CRegString errorkeyval = CRegString(errorkey);
            if (((CString)(errorkeyval)).IsEmpty())
            {
                errorkeyval = apppath + L"TortoiseSVN_Error.wav";
            }
            CString warnkey = L"AppEvents\\Schemes\\Apps\\TortoiseProc\\TSVN_Warning\\" + name + L"\\";
            CRegString warnkeyval = CRegString(warnkey);
            if (((CString)(warnkeyval)).IsEmpty())
            {
                warnkeyval = apppath + L"TortoiseSVN_Warning.wav";
            }
            CString notificationkey = L"AppEvents\\Schemes\\Apps\\TortoiseProc\\TSVN_Notification\\" + name + L"\\";
            CRegString notificationkeyval = CRegString(notificationkey);
            if (((CString)(notificationkeyval)).IsEmpty())
            {
                notificationkeyval = apppath + L"TortoiseSVN_Notification.wav";
            }
        }
    }
}

void CSoundUtils::PlayTSVNWarning()
{
    PlaySound(L"TSVN_Warning", NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT);
}

void CSoundUtils::PlayTSVNError()
{
    PlaySound(L"TSVN_Error", NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT);
}

void CSoundUtils::PlayTSVNNotification()
{
    PlaySound(L"TSVN_Notification", NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT);
}