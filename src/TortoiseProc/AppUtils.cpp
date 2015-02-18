// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015 - TortoiseSVN

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
#include "resource.h"
#include "TortoiseProc.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "SVNProperties.h"
#include "StringUtils.h"
#include "registry.h"
#include "TSVNPath.h"
#include "SVN.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include <intshcut.h>
#include "StringUtils.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"
#include "DirFileEnum.h"
#include "SelectFileFilter.h"
#include "SmartHandle.h"
#include "SVNExternals.h"
#include "CmdLineParser.h"


bool CAppUtils::GetMimeType(const CTSVNPath& file, CString& mimetype, SVNRev rev /*= SVNRev::REV_WC*/)
{
    // only fetch the mime-type for local paths, or for urls if a mime-type specific diff tool
    // is configured.
    if (!file.IsUrl() || CAppUtils::HasMimeTool())
    {
        SVNProperties props(file, file.IsUrl() ? rev : SVNRev::REV_WC, false, false);
        for (int i = 0; i < props.GetCount(); ++i)
        {
            if (props.GetItemName(i).compare(SVN_PROP_MIME_TYPE) == 0)
            {
                mimetype = props.GetItemValue(i).c_str();
                return true;
            }
        }
    }
    return false;
}

BOOL CAppUtils::StartExtMerge(const MergeFlags& flags,
    const CTSVNPath& basefile, const CTSVNPath& theirfile, const CTSVNPath& yourfile, const CTSVNPath& mergedfile, bool bSaveRequired,
    const CString& basename /*= CString()*/, const CString& theirname /*= CString()*/, const CString& yourname /*= CString()*/, const CString& mergedname /*= CString()*/, const CString& filename /*= CString()*/)
{

    CRegString regCom = CRegString(L"Software\\TortoiseSVN\\Merge");
    CString ext = mergedfile.GetFileExtension();
    CString com = regCom;
    bool bInternal = false;

    CString mimetype;

    if (!ext.IsEmpty())
    {
        // is there an extension specific merge tool?
        CRegString mergetool(L"Software\\TortoiseSVN\\MergeTools\\" + ext.MakeLower());
        if (!CString(mergetool).IsEmpty())
        {
            com = mergetool;
        }
    }
    if (HasMimeTool())
    {
        if (GetMimeType(yourfile, mimetype) || GetMimeType(theirfile, mimetype) || GetMimeType(basefile, mimetype))
        {
            // is there a mime type specific merge tool?
            CRegString mergetool(L"Software\\TortoiseSVN\\MergeTools\\" + mimetype);
            if (CString(mergetool) != "")
            {
                com = mergetool;
            }
        }
    }
    // is there a filename specific merge tool?
    CRegString mergetool(L"Software\\TortoiseSVN\\MergeTools\\." + mergedfile.GetFilename().MakeLower());
    if (!CString(mergetool).IsEmpty())
    {
        com = mergetool;
    }

    if ((flags.bAlternativeTool)&&(!com.IsEmpty()))
    {
        if (com.Left(1).Compare(L"#")==0)
            com.Delete(0);
        else
            com.Empty();
    }

    if (com.IsEmpty()||(com.Left(1).Compare(L"#")==0))
    {
        // Maybe we should use TortoiseIDiff?
        if ((ext == L".jpg") || (ext == L".jpeg") ||
            (ext == L".bmp") || (ext == L".gif")  ||
            (ext == L".png") || (ext == L".ico")  ||
            (ext == L".tif") || (ext == L".tiff") ||
            (ext == L".dib") || (ext == L".emf")  ||
            (ext == L".cur"))
        {
            com = CPathUtils::GetAppDirectory() + L"TortoiseIDiff.exe";
            com = L"\"" + com + L"\"";
            com = com + L" /base:%base /theirs:%theirs /mine:%mine /result:%merged";
            com = com + L" /basetitle:%bname /theirstitle:%tname /minetitle:%yname";
        }
        else
        {
            // use TortoiseMerge
            bInternal = true;
            com = CPathUtils::GetAppDirectory() + L"TortoiseMerge.exe";
            com = L"\"" + com + L"\"";
            com = com + L" /base:%base /theirs:%theirs /mine:%mine /merged:%merged";
            com = com + L" /basename:%bname /theirsname:%tname /minename:%yname /mergedname:%mname";
        }
        if (!g_sGroupingUUID.IsEmpty())
        {
            com += L" /groupuuid:\"";
            com += g_sGroupingUUID;
            com += L"\"";
        }
        if (bSaveRequired)
        {
            com += L" /saverequired";
            CCmdLineParser parser(GetCommandLine());
            HWND   resolveMsgWnd    = parser.HasVal(L"resolvemsghwnd")   ? (HWND)parser.GetLongLongVal(L"resolvemsghwnd")     : 0;
            WPARAM resolveMsgWParam = parser.HasVal(L"resolvemsgwparam") ? (WPARAM)parser.GetLongLongVal(L"resolvemsgwparam") : 0;
            LPARAM resolveMsgLParam = parser.HasVal(L"resolvemsglparam") ? (LPARAM)parser.GetLongLongVal(L"resolvemsglparam") : 0;
            if (resolveMsgWnd)
            {
                CString s;
                s.Format(L" /resolvemsghwnd:%I64d /resolvemsgwparam:%I64d /resolvemsglparam:%I64d", (__int64)resolveMsgWnd, (__int64)resolveMsgWParam, (__int64)resolveMsgLParam);
                com += s;
            }
        }
    }
    // check if the params are set. If not, just add the files to the command line
    if ((com.Find(L"%merged")<0)&&(com.Find(L"%base")<0)&&(com.Find(L"%theirs")<0)&&(com.Find(L"%mine")<0))
    {
        com += L" \""+basefile.GetWinPathString()+L"\"";
        com += L" \""+theirfile.GetWinPathString()+L"\"";
        com += L" \""+yourfile.GetWinPathString()+L"\"";
        com += L" \""+mergedfile.GetWinPathString()+L"\"";
    }
    if (basefile.IsEmpty())
    {
        com.Replace(L"/base:%base", L"");
        com.Replace(L"%base", L"");
    }
    else
        com.Replace(L"%base", L"\"" + basefile.GetWinPathString() + L"\"");
    if (theirfile.IsEmpty())
    {
        com.Replace(L"/theirs:%theirs", L"");
        com.Replace(L"%theirs", L"");
    }
    else
        com.Replace(L"%theirs", L"\"" + theirfile.GetWinPathString() + L"\"");
    if (yourfile.IsEmpty())
    {
        com.Replace(L"/mine:%mine", L"");
        com.Replace(L"%mine", L"");
    }
    else
        com.Replace(L"%mine", L"\"" + yourfile.GetWinPathString() + L"\"");
    if (mergedfile.IsEmpty())
    {
        com.Replace(L"/merged:%merged", L"");
        com.Replace(L"%merged", L"");
    }
    else
        com.Replace(L"%merged", L"\"" + mergedfile.GetWinPathString() + L"\"");
    if (basename.IsEmpty())
    {
        if (basefile.IsEmpty())
        {
            com.Replace(L"/basename:%bname", L"");
            com.Replace(L"%bname", L"");
            com.Replace(L"%nqbname", L"");
        }
        else
        {
            com.Replace(L"%bname", L"\"" + basefile.GetUIFileOrDirectoryName() + L"\"");
            com.Replace(L"%nqbname", basefile.GetUIFileOrDirectoryName());
        }
    }
    else
    {
        com.Replace(L"%bname", L"\"" + basename + L"\"");
        com.Replace(L"%nqbname", basename);
    }
    if (theirname.IsEmpty())
    {
        if (theirfile.IsEmpty())
        {
            com.Replace(L"/theirsname:%tname", L"");
            com.Replace(L"%tname", L"");
            com.Replace(L"%nqtname", L"");
        }
        else
        {
            com.Replace(L"%tname", L"\"" + theirfile.GetUIFileOrDirectoryName() + L"\"");
            com.Replace(L"%nqtname", theirfile.GetUIFileOrDirectoryName());
        }
    }
    else
    {
        com.Replace(L"%tname", L"\"" + theirname + L"\"");
        com.Replace(L"%nqtname", theirname);
    }
    if (yourname.IsEmpty())
    {
        if (yourfile.IsEmpty())
        {
            com.Replace(L"/minename:%yname", L"");
            com.Replace(L"%yname", L"");
            com.Replace(L"%nqyname", L"");
        }
        else
        {
            com.Replace(L"%yname", L"\"" + yourfile.GetUIFileOrDirectoryName() + L"\"");
            com.Replace(L"%nqyname", yourfile.GetUIFileOrDirectoryName());
        }
    }
    else
    {
        com.Replace(L"%yname", L"\"" + yourname + L"\"");
        com.Replace(L"%nqyname", yourname);
    }
    if (mergedname.IsEmpty())
    {
        if (mergedfile.IsEmpty())
        {
            com.Replace(L"/mergedname:%mname", L"");
            com.Replace(L"%mname", L"");
            com.Replace(L"%nqmname", L"");
        }
        else
        {
            com.Replace(L"%mname", L"\"" + mergedfile.GetUIFileOrDirectoryName() + L"\"");
            com.Replace(L"%nqmname", mergedfile.GetUIFileOrDirectoryName());
        }
    }
    else
    {
        com.Replace(L"%mname", L"\"" + mergedname + L"\"");
        com.Replace(L"%nqmname", mergedname);
    }
    if (filename.IsEmpty())
    {
        com.Replace(L"%fname", L"");
        com.Replace(L"%nqfname", L"");
    }
    else
    {
        com.Replace(L"%fname", L"\"" + filename + L"\"");
        com.Replace(L"%nqfname", filename);
    }

    if ((flags.bReadOnly)&&(bInternal))
        com += L" /readonly";

    if(!LaunchApplication(com, IDS_ERR_EXTMERGESTART, false))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CAppUtils::StartExtPatch(const CTSVNPath& patchfile, const CTSVNPath& dir, const CString& sOriginalDescription, const CString& sPatchedDescription, BOOL bReversed, BOOL bWait)
{
    CString viewer;
    // use TortoiseMerge
    viewer = CPathUtils::GetAppDirectory();
    viewer += L"TortoiseMerge.exe";

    viewer = L"\"" + viewer + L"\"";
    viewer = viewer + L" /diff:\"" + patchfile.GetWinPathString() + L"\"";
    viewer = viewer + L" /patchpath:\"" + dir.GetWinPathString() + L"\"";
    if (bReversed)
        viewer += L" /reversedpatch";
    if (!sOriginalDescription.IsEmpty())
        viewer = viewer + L" /patchoriginal:\"" + sOriginalDescription + L"\"";
    if (!sPatchedDescription.IsEmpty())
        viewer = viewer + L" /patchpatched:\"" + sPatchedDescription + L"\"";
    if (!g_sGroupingUUID.IsEmpty())
    {
        viewer += L" /groupuuid:\"";
        viewer += g_sGroupingUUID;
        viewer += L"\"";
    }
    if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
    {
        return FALSE;
    }
    return TRUE;
}

CString CAppUtils::PickDiffTool(const CTSVNPath& file1, const CTSVNPath& file2, const CString& mimetype)
{
    CString difftool = CRegString(L"Software\\TortoiseSVN\\DiffTools\\" + file2.GetFilename().MakeLower());
    if (!difftool.IsEmpty())
        return difftool;
    difftool = CRegString(L"Software\\TortoiseSVN\\DiffTools\\" + file1.GetFilename().MakeLower());
    if (!difftool.IsEmpty())
        return difftool;

    if (HasMimeTool())
    {
        // Is there a mime type specific diff tool?
        CString mt = mimetype;
        if (!mt.IsEmpty() || GetMimeType(file1, mt) || GetMimeType(file2, mt))
        {
            CString mimetool = CRegString(L"Software\\TortoiseSVN\\DiffTools\\" + mt);
            if (!mimetool.IsEmpty())
                return mimetool;
        }
    }

    // Is there an extension specific diff tool?
    CString ext = file2.GetFileExtension().MakeLower();
    if (!ext.IsEmpty())
    {
        CString exttool = CRegString(L"Software\\TortoiseSVN\\DiffTools\\" + ext);
        if (!exttool.IsEmpty())
            return exttool;
        // Maybe we should use TortoiseIDiff?
        if ((ext == L".jpg") || (ext == L".jpeg") ||
            (ext == L".bmp") || (ext == L".gif") ||
            (ext == L".png") || (ext == L".ico") ||
            (ext == L".tif") || (ext == L".tiff") ||
            (ext == L".dib") || (ext == L".emf") ||
            (ext == L".cur"))
        {
            return
                L"\"" + CPathUtils::GetAppDirectory() + L"TortoiseIDiff.exe" + L"\"" +
                L" /left:%base /right:%mine /lefttitle:%bname /righttitle:%yname" +
                L" /groupuuid:\"" + g_sGroupingUUID + L"\"";
        }
    }

    // Finally, pick a generic external diff tool
    difftool = CRegString(L"Software\\TortoiseSVN\\Diff");
    return difftool;
}

bool CAppUtils::StartExtDiff(
    const CTSVNPath& file1, const CTSVNPath& file2,
    const CString& sName1, const CString& sName2, const DiffFlags& flags, int line, const CString& sName)
{
    return StartExtDiff(file1, file2, sName1, sName2, CTSVNPath(), CTSVNPath(), SVNRev(), SVNRev(), SVNRev(), flags, line, sName, L"");
}

bool CAppUtils::StartExtDiff(
    const CTSVNPath& file1, const CTSVNPath& file2,
    const CString& sName1, const CString& sName2,
    const CTSVNPath& url1, const CTSVNPath& url2,
    const SVNRev& rev1, const SVNRev& rev2,
    const SVNRev& pegRev, const DiffFlags& flags, int line, const CString& sName, const CString& mimetype)
{
    CString viewer;

    CRegDWORD blamediff(L"Software\\TortoiseSVN\\DiffBlamesWithTortoiseMerge", FALSE);
    if (!flags.bBlame || !(DWORD)blamediff)
    {
        viewer = PickDiffTool(file1, file2, mimetype);
        // If registry entry for a diff program is commented out, use TortoiseMerge.
        bool bCommentedOut = viewer.Left(1) == L"#";
        if (flags.bAlternativeTool)
        {
            // Invert external vs. internal diff tool selection.
            if (bCommentedOut)
                viewer.Delete(0); // uncomment
            else
                viewer = "";
        }
        else if (bCommentedOut)
            viewer = "";
    }

    bool bInternal = viewer.IsEmpty();
    if (bInternal)
    {
        viewer =
            L"\"" + CPathUtils::GetAppDirectory() + L"TortoiseMerge.exe" + L"\"" +
            L" /base:%base /mine:%mine /basename:%bname /minename:%yname" +
            L" /basereflectedname:%burl /minereflectedname:%yurl";
        if (flags.bBlame)
            viewer += L" /blame";
        if (!g_sGroupingUUID.IsEmpty())
        {
            viewer += L" /groupuuid:\"";
            viewer += g_sGroupingUUID;
            viewer += L"\"";
        }
    }
    // check if the params are set. If not, just add the files to the command line
    if ((viewer.Find(L"%base")<0)&&(viewer.Find(L"%mine")<0))
    {
        viewer += L" \""+file1.GetWinPathString()+L"\"";
        viewer += L" \""+file2.GetWinPathString()+L"\"";
    }
    if (viewer.Find(L"%base") >= 0)
    {
        viewer.Replace(L"%base",  L"\""+file1.GetWinPathString()+L"\"");
    }
    if (viewer.Find(L"%mine") >= 0)
    {
        viewer.Replace(L"%mine",  L"\""+file2.GetWinPathString()+L"\"");
    }
    if (sName1.IsEmpty())
    {
        viewer.Replace(L"%bname", L"\"" + file1.GetUIFileOrDirectoryName() + L"\"");
        viewer.Replace(L"%nqbname", file1.GetUIFileOrDirectoryName());
    }
    else
    {
        viewer.Replace(L"%bname", L"\"" + sName1 + L"\"");
        viewer.Replace(L"%nqbname", sName1);
    }

    if (sName2.IsEmpty())
    {
        viewer.Replace(L"%yname", L"\"" + file2.GetUIFileOrDirectoryName() + L"\"");
        viewer.Replace(L"%nqyname", file2.GetUIFileOrDirectoryName());
    }
    else
    {
        viewer.Replace(L"%yname", L"\"" + sName2 + L"\"");
        viewer.Replace(L"%nqyname", sName2);
    }

    if (sName.IsEmpty())
    {
        viewer.Replace(L"%fname", L"");
        viewer.Replace(L"%nqfname", L"");
    }
    else
    {
        viewer.Replace(L"%fname", L"\"" + sName + L"\"");
        viewer.Replace(L"%nqfname", sName);
    }

    if (viewer.Find(L"%burl") >= 0)
    {
        CString s = L"\"" + url1.GetSVNPathString();
        s += L"\"";
        viewer.Replace(L"%burl", s);
    }
    viewer.Replace(L"%nqburl", url1.GetSVNPathString());

    if (viewer.Find(L"%yurl") >= 0)
    {
        CString s = L"\"" + url2.GetSVNPathString();
        s += L"\"";
        viewer.Replace(L"%yurl", s);
    }
    viewer.Replace(L"%nqyurl", url2.GetSVNPathString());

    viewer.Replace(L"%brev", L"\"" + rev1.ToString() + L"\"");
    viewer.Replace(L"%nqbrev", rev1.ToString());

    viewer.Replace(L"%yrev", L"\"" + rev2.ToString() + L"\"");
    viewer.Replace(L"%nqyrev", rev2.ToString());

    viewer.Replace(L"%peg", L"\"" + pegRev.ToString() + L"\"");
    viewer.Replace(L"%nqpeg", pegRev.ToString());

    if (flags.bReadOnly && bInternal)
        viewer += L" /readonly";
    if (line > 0)
    {
        viewer += L" /line:";
        CString temp;
        temp.Format(L"%d", line);
        viewer += temp;
    }

    return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, flags.bWait);
}

BOOL CAppUtils::StartExtDiffProps(const CTSVNPath& file1, const CTSVNPath& file2, const CString& sName1, const CString& sName2, BOOL bWait, BOOL bReadOnly)
{
    CRegString diffpropsexe(L"Software\\TortoiseSVN\\DiffProps");
    CString viewer = diffpropsexe;
    bool bInternal = false;
    if (viewer.IsEmpty()||(viewer.Left(1).Compare(L"#")==0))
    {
        //no registry entry (or commented out) for a diff program
        //use TortoiseMerge
        bInternal = true;
        viewer = CPathUtils::GetAppDirectory();
        viewer += L"TortoiseMerge.exe";
        viewer = L"\"" + viewer + L"\"";
        viewer = viewer + L" /base:%base /mine:%mine /basename:%bname /minename:%yname";
        if (!g_sGroupingUUID.IsEmpty())
        {
            viewer += L" /groupuuid:\"";
            viewer += g_sGroupingUUID;
            viewer += L"\"";
        }
    }
    // check if the params are set. If not, just add the files to the command line
    if ((viewer.Find(L"%base")<0)&&(viewer.Find(L"%mine")<0))
    {
        viewer += L" \""+file1.GetWinPathString()+L"\"";
        viewer += L" \""+file2.GetWinPathString()+L"\"";
    }
    if (viewer.Find(L"%base") >= 0)
    {
        viewer.Replace(L"%base",  L"\""+file1.GetWinPathString()+L"\"");
    }
    if (viewer.Find(L"%mine") >= 0)
    {
        viewer.Replace(L"%mine",  L"\""+file2.GetWinPathString()+L"\"");
    }

    if (sName1.IsEmpty())
    {
        viewer.Replace(L"%bname", L"\"" + file1.GetUIFileOrDirectoryName() + L"\"");
        viewer.Replace(L"%nqbname", file1.GetUIFileOrDirectoryName());
    }
    else
    {
        viewer.Replace(L"%bname", L"\"" + sName1 + L"\"");
        viewer.Replace(L"%nqbname", sName1);
    }

    if (sName2.IsEmpty())
    {
        viewer.Replace(L"%yname", L"\"" + file2.GetUIFileOrDirectoryName() + L"\"");
        viewer.Replace(L"%nqyname", file2.GetUIFileOrDirectoryName());
    }
    else
    {
        viewer.Replace(L"%yname", L"\"" + sName2 + L"\"");
        viewer.Replace(L"%nqyname", sName2);
    }

    if ((bReadOnly)&&(bInternal))
        viewer += L" /readonly";

    if(!LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, !!bWait))
    {
        return FALSE;
    }
    return TRUE;
}

BOOL CAppUtils::CheckForEmptyDiff(const CTSVNPath& sDiffPath)
{
    DWORD length = 0;
    CAutoFile hFile = ::CreateFile(sDiffPath.GetWinPath(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
    if (!hFile)
        return TRUE;
    length = ::GetFileSize(hFile, NULL);
    if (length < 4)
        return TRUE;
    return FALSE;

}

void CAppUtils::CreateFontForLogs(CFont& fontToCreate)
{
    LOGFONT logFont;
    HDC hScreenDC = ::GetDC(NULL);
    logFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(L"Software\\TortoiseSVN\\LogFontSize", 8), GetDeviceCaps(hScreenDC, LOGPIXELSY), 72);
    ::ReleaseDC(NULL, hScreenDC);
    logFont.lfWidth          = 0;
    logFont.lfEscapement     = 0;
    logFont.lfOrientation    = 0;
    logFont.lfWeight         = FW_NORMAL;
    logFont.lfItalic         = 0;
    logFont.lfUnderline      = 0;
    logFont.lfStrikeOut      = 0;
    logFont.lfCharSet        = DEFAULT_CHARSET;
    logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality        = DRAFT_QUALITY;
    logFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
    wcscpy_s(logFont.lfFaceName, (LPCTSTR)(CString)CRegString(L"Software\\TortoiseSVN\\LogFontName", L"Courier New"));
    VERIFY(fontToCreate.CreateFontIndirect(&logFont));
}

/**
* Launch the external blame viewer
*/
bool CAppUtils::LaunchTortoiseBlame(const CString& sBlameFile,
                                    const CString& sOriginalFile,
                                    const CString& sParams,
                                    const SVNRev& startrev,
                                    const SVNRev& endrev,
                                    const SVNRev& pegrev)
{
    CString viewer = L"\"";
    viewer += CPathUtils::GetAppDirectory();
    viewer += L"TortoiseBlame.exe\"";
    viewer += L" \"" + sBlameFile + L"\"";
    viewer += L" \"" + sOriginalFile + L"\"";
    viewer += L" "+sParams;

    if (startrev.IsValid() && endrev.IsValid())
        viewer += L" /revrange:\"" + startrev.ToString() + L"-" + endrev.ToString() + L"\"";

    if (pegrev.IsValid())
        viewer += L" /pegrev:" + pegrev.ToString();

    if (!g_sGroupingUUID.IsEmpty())
    {
        viewer += L" /groupuuid:\"";
        viewer += g_sGroupingUUID;
        viewer += L"\"";
    }

    return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, false);
}

bool CAppUtils::FormatTextInRichEditControl(CWnd * pWnd)
{
    CString sText;
    if (pWnd == NULL)
        return false;
    bool bStyled = false;
    pWnd->GetWindowText(sText);
    // the rich edit control doesn't count the CR char!
    // to be exact: CRLF is treated as one char.
    sText.Remove('\r');

    // style each line separately
    int offset = 0;
    int nNewlinePos;
    do
    {
        nNewlinePos = sText.Find('\n', offset);
        CString sLine = nNewlinePos>=0
                      ? sText.Mid(offset, nNewlinePos-offset)
                      : sText.Mid(offset);

        int start = 0;
        int end = 0;
        while (FindStyleChars(sLine, '*', start, end))
        {
            CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
            pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
            SetCharFormat(pWnd, CFM_BOLD, CFE_BOLD);
            bStyled = true;
            start = end;
        }
        start = 0;
        end = 0;
        while (FindStyleChars(sLine, '^', start, end))
        {
            CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
            pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
            SetCharFormat(pWnd, CFM_ITALIC, CFE_ITALIC);
            bStyled = true;
            start = end;
        }
        start = 0;
        end = 0;
        while (FindStyleChars(sLine, '_', start, end))
        {
            CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
            pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
            SetCharFormat(pWnd, CFM_UNDERLINE, CFE_UNDERLINE);
            bStyled = true;
            start = end;
        }
        offset = nNewlinePos+1;
    } while(nNewlinePos>=0);
    return bStyled;
}

std::vector<CHARRANGE>
CAppUtils::FindRegexMatches
    ( const std::wstring& text
    , const CString& matchstring
    , const CString& matchsubstring /* = L".*"*/)
{
    std::vector<CHARRANGE> result;
    if (matchstring.IsEmpty())
        return result;

    try
    {
        const std::tr1::wregex regMatch(matchstring, std::tr1::regex_constants::icase | std::tr1::regex_constants::ECMAScript);
        const std::tr1::wregex regSubMatch(matchsubstring, std::tr1::regex_constants::icase | std::tr1::regex_constants::ECMAScript);
        const std::tr1::wsregex_iterator end;
        for (std::tr1::wsregex_iterator it(text.begin(), text.end(), regMatch); it != end; ++it)
        {
            // (*it)[0] is the matched string
            std::wstring matchedString = (*it)[0];
            ptrdiff_t matchpos = it->position(0);
            for (std::tr1::wsregex_iterator it2(matchedString.begin(), matchedString.end(), regSubMatch); it2 != end; ++it2)
            {
                ATLTRACE(L"matched id : %s\n", (*it2)[0].str().c_str());
                ptrdiff_t matchposID = it2->position(0);
                CHARRANGE range = {(LONG)(matchpos+matchposID), (LONG)(matchpos+matchposID+(*it2)[0].str().size())};
                result.push_back (range);
            }
        }
    }
    catch (std::exception) {}

    return result;
}

// from CSciEdit
namespace {
    bool IsValidURLChar(wchar_t ch)
    {
        return iswalnum(ch) ||
            ch == L'_' || ch == L'/' || ch == L';' || ch == L'?' || ch == L'&' || ch == L'=' ||
            ch == L'%' || ch == L':' || ch == L'.' || ch == L'#' || ch == L'-' || ch == L'+';
    }

    bool IsUrl(const CString& sText)
    {
        if (!PathIsURLW(sText))
            return false;
        if (sText.Find(L"://") >= 0)
            return true;
        return false;
    }
}

/**
* implements URL searching with the same logic as CSciEdit::StyleURLs
*/
std::vector<CHARRANGE>
CAppUtils::FindURLMatches(const CString& msg)
{
    std::vector<CHARRANGE> result;

    int len = msg.GetLength();
    int starturl = -1;

    for (int i = 0; i <= msg.GetLength(); ++i)
    {
        if ((i < len) && IsValidURLChar(msg[i]))
        {
            if (starturl < 0)
                starturl = i;
        }
        else
        {
            if ((starturl >= 0) && IsUrl(msg.Mid(starturl, i - starturl)))
            {
                CHARRANGE range = { starturl, i };
                result.push_back(range);
            }
            starturl = -1;
        }
    }

    return result;
}


bool CAppUtils::FindStyleChars(const CString& sText, TCHAR stylechar, int& start, int& end)
{
    int i=start;
    int last = sText.GetLength()-1;
    bool bFoundMarker = false;
    TCHAR c = i == 0 ? '\0' : sText[i-1];
    TCHAR nextChar = i >= last ? '\0' : sText[i+1];

    // find a starting marker
    while (i < last)
    {
        TCHAR prevChar = c;
        c = nextChar;
        nextChar = sText[i+1];

        // IsCharAlphaNumeric can be somewhat expensive.
        // Long lines of "*****" or "----" will be pre-empted efficiently
        // by the (c != nextChar) condition.

        if ((c == stylechar) && (c != nextChar))
        {
            if (   IsCharAlphaNumeric(nextChar)
                && !IsCharAlphaNumeric(prevChar))
            {
                start = ++i;
                bFoundMarker = true;
                break;
            }
        }
        i++;
    }
    if (!bFoundMarker)
        return false;

    // find ending marker
    // c == sText[i-1]

    bFoundMarker = false;
    while (i <= last)
    {
        TCHAR prevChar = c;
        c = sText[i];
        if (c == stylechar)
        {
            if ((i == last) || (   !IsCharAlphaNumeric(sText[i+1])
                                && IsCharAlphaNumeric(prevChar)))
            {
                end = i;
                i++;
                bFoundMarker = true;
                break;
            }
        }
        i++;
    }
    return bFoundMarker;
}

bool CAppUtils::BrowseRepository(const CString& repoRoot, CHistoryCombo& combo, CWnd * pParent, SVNRev& rev)
{
    bool bExternalUrl = false;
    CString strUrl;
    combo.GetWindowText(strUrl);
    if (strUrl.GetLength() && strUrl[0] == '^')
    {
        bExternalUrl = true;
        strUrl = strUrl.Mid(1);
    }
    strUrl.Replace('\\', '/');
    strUrl.Replace(L"%", L"%25");
    strUrl.TrimLeft('/');

    CString trimmedRoot = repoRoot;
    trimmedRoot.TrimRight('/');

    strUrl = trimmedRoot + L"/" + strUrl;

    if (strUrl.Left(7) == L"file://")
    {
        // browse repository - show repository browser
        SVN::preparePath(strUrl);
        CRepositoryBrowser browser(strUrl, rev, pParent);
        if (browser.DoModal() == IDOK)
        {
            combo.SetCurSel(-1);
            if (bExternalUrl)
                combo.SetWindowText(L"^" + browser.GetPath().Mid(CPathUtils::PathUnescape(repoRoot).GetLength()));
            else
                combo.SetWindowText(browser.GetPath().Mid(CPathUtils::PathUnescape(repoRoot).GetLength()));
            combo.SetFocus();
            rev = browser.GetRevision();
            return true;
        }
    }
    else if ((strUrl.Left(7) == L"http://"
        ||(strUrl.Left(8) == L"https://")
        ||(strUrl.Left(6) == L"svn://")
        ||(strUrl.Left(4) == L"svn+")) && strUrl.GetLength() > 6)
    {
        // browse repository - show repository browser
        CRepositoryBrowser browser(strUrl, rev, pParent);
        if (browser.DoModal() == IDOK)
        {
            combo.SetCurSel(-1);
            if (bExternalUrl)
                combo.SetWindowText(L"^" + browser.GetPath().Mid(CPathUtils::PathUnescape(repoRoot).GetLength()));
            else
                combo.SetWindowText(browser.GetPath().Mid(CPathUtils::PathUnescape(repoRoot).GetLength()));
            combo.SetFocus();
            rev = browser.GetRevision();
            return true;
        }
    }
    combo.SetFocus();
    return false;
}

bool CAppUtils::BrowseRepository(CHistoryCombo& combo, CWnd * pParent, SVNRev& rev, bool multiSelection, const CString& root, const CString& selUrl)
{
    CString strURLs;
    combo.GetWindowText(strURLs);
    if (strURLs.IsEmpty())
        strURLs = combo.GetString();
    strURLs.Replace('\\', '/');
    strURLs.Replace(L"%", L"%25");

    CString strUrl = strURLs;
    if (multiSelection)
    {
        CTSVNPathList paths;
        paths.LoadFromAsteriskSeparatedString (strURLs);

        strUrl = paths.GetCommonRoot().GetSVNPathString();
    }

    if (strUrl.GetLength() > 1)
    {
        strUrl = SVNExternals::GetFullExternalUrl(strUrl, root, selUrl);
        // check if the url has a revision appended to it
        auto atposurl = strUrl.ReverseFind('@');
        if (atposurl >= 0)
        {
            CString sRev = strUrl.Mid(atposurl+1);
            SVNRev urlrev(sRev);
            if (urlrev.IsValid())
            {
                strUrl = strUrl.Left(atposurl);
                if (!rev.IsValid() || rev.IsHead())
                    rev = urlrev;
            }
        }
    }

    if (strUrl.Left(7) == L"file://")
    {
        CString strFile(strUrl);
        SVN::UrlToPath(strFile);

        SVN svn;
        if (svn.IsRepository(CTSVNPath(strFile)))
        {
            // browse repository - show repository browser
            SVN::preparePath(strUrl);
            CRepositoryBrowser browser(strUrl, rev, pParent);
            if (browser.DoModal() == IDOK)
            {
                combo.SetCurSel(-1);
                combo.SetWindowText(multiSelection ? browser.GetSelectedURLs() : browser.GetPath());
                combo.SetFocus();
                rev = browser.GetRevision();
                return true;
            }
        }
        else
        {
            // browse local directories
            CBrowseFolder folderBrowser;
            folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
            // remove the 'file:///' so the shell can recognize the local path
            SVN::UrlToPath(strUrl);
            if (folderBrowser.Show(pParent->GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
            {
                SVN::PathToUrl(strUrl);

                combo.SetCurSel(-1);
                combo.SetWindowText(strUrl);
                combo.SetFocus();
                return true;
            }
        }
    }
    else if ((strUrl.Left(7) == L"http://"
        ||(strUrl.Left(8) == L"https://")
        ||(strUrl.Left(6) == L"svn://")
        ||(strUrl.Left(4) == L"svn+")) && strUrl.GetLength() > 6)
    {
        // browse repository - show repository browser
        CRepositoryBrowser browser(strUrl, rev, pParent);
        if (browser.DoModal() == IDOK)
        {
            combo.SetCurSel(-1);
            combo.SetWindowText(multiSelection ? browser.GetSelectedURLs() : browser.GetPath());
            combo.SetFocus();
            rev = browser.GetRevision();
            return true;
        }
    }
    else
    {
        SVN svn;
        if (svn.IsRepository(CTSVNPath(strUrl)))
        {
            // browse repository - show repository browser
            SVN::PathToUrl(strUrl);
            CRepositoryBrowser browser(strUrl, rev, pParent);
            if (browser.DoModal() == IDOK)
            {
                combo.SetCurSel(-1);
                combo.SetWindowText(multiSelection ? browser.GetSelectedURLs() : browser.GetPath());
                combo.SetFocus();
                rev = browser.GetRevision();
                return true;
            }
        }
        else
        {
            // browse local directories
            CBrowseFolder folderBrowser;
            folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
            if (folderBrowser.Show(pParent->GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
            {
                SVN::PathToUrl(strUrl);

                combo.SetCurSel(-1);
                combo.SetWindowText(strUrl);
                return true;
            }
        }
    }

    combo.SetFocus();
    return false;
}

CString CAppUtils::GetProjectNameFromURL(CString url)
{
    CString name;
    while ((name.IsEmpty() || (name.CompareNoCase(L"branches")==0) ||
        (name.CompareNoCase(L"tags")==0) ||
        (name.CompareNoCase(L"trunk")==0))&&(!url.IsEmpty()))
    {
        auto rfound = url.ReverseFind('/');
        if (rfound < 0)
            break;
        name = url.Mid(rfound+1);
        url = url.Left(rfound);
    }
    if ((name.Compare(L"svn") == 0)||(name.Compare(L"svnroot") == 0))
    {
        // a name of svn or svnroot indicates that it's not really the project name. In that
        // case, we try the first part of the URL
        // of course, this won't work in all cases (but it works for Google project hosting)
        url.Replace(L"http://", L"");
        url.Replace(L"https://", L"");
        url.Replace(L"svn://", L"");
        url.Replace(L"svn+ssh://", L"");
        url.TrimLeft(L"/");
        name = url.Left(url.Find('.'));
    }
    return name;
}

bool CAppUtils::StartShowUnifiedDiff(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1,
                                     const CTSVNPath& url2, const SVNRev& rev2,
                                     const SVNRev& peg, const SVNRev& headpeg,
                                     const CString& options,
                                     bool bAlternateDiff /* = false */, bool bIgnoreAncestry /* = false */, bool blame /* = false */, bool bIgnoreProperties /* = true*/)
{
    CString sCmd = L"/command:showcompare /unified";
    sCmd += L" /url1:\"" + url1.GetSVNPathString();
    sCmd += L"\"";
    if (rev1.IsValid())
        sCmd += L" /revision1:" + rev1.ToString();
    sCmd += L" /url2:\"" + url2.GetSVNPathString();
    sCmd += L"\"";
    if (rev2.IsValid())
        sCmd += L" /revision2:" + rev2.ToString();
    if (peg.IsValid())
        sCmd += L" /pegrevision:" + peg.ToString();
    if (headpeg.IsValid())
        sCmd += L" /headpegrevision:" + headpeg.ToString();

    if (!options.IsEmpty())
        sCmd += L" /diffoptions:\"" + options + L"\"";

    if (bAlternateDiff)
        sCmd += L" /alternatediff";

    if (bIgnoreAncestry)
        sCmd += L" /ignoreancestry";

    if (blame)
        sCmd += L" /blame";

    if (bIgnoreProperties)
        sCmd += L" /ignoreprops";

    if (hWnd)
    {
        sCmd += L" /hwnd:";
        TCHAR buf[30] = { 0 };
        swprintf_s(buf, L"%p", (void*)hWnd);
        sCmd += buf;
    }

    return CAppUtils::RunTortoiseProc(sCmd);
}

bool CAppUtils::StartShowCompare(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1,
                                 const CTSVNPath& url2, const SVNRev& rev2,
                                 const SVNRev& peg, const SVNRev& headpeg,
                                 bool ignoreprops, const CString& options,
                                 bool bAlternateDiff /*= false*/, bool bIgnoreAncestry /*= false*/,
                                 bool blame /*= false*/, svn_node_kind_t nodekind /*= svn_node_unknown*/,
                                 int line /*= 0*/ )
{
    CString sCmd;
    sCmd.Format(L"/command:showcompare /nodekind:%d", nodekind);
    sCmd += L" /url1:\"" + url1.GetSVNPathString();
    sCmd += L"\"";
    if (rev1.IsValid())
        sCmd += L" /revision1:" + rev1.ToString();
    sCmd += L" /url2:\"" + url2.GetSVNPathString();
    sCmd += L"\"";
    if (rev2.IsValid())
        sCmd += L" /revision2:" + rev2.ToString();
    if (peg.IsValid())
        sCmd += L" /pegrevision:" + peg.ToString();
    if (headpeg.IsValid())
        sCmd += L" /headpegrevision:" + headpeg.ToString();
    if (!options.IsEmpty())
        sCmd += L" /diffoptions:\"" + options + L"\"";
    if (bAlternateDiff)
        sCmd += L" /alternatediff";
    if (bIgnoreAncestry)
        sCmd += L" /ignoreancestry";
    if (blame)
        sCmd += L" /blame";
    if (ignoreprops)
        sCmd += L" /ignoreprops";

    if (hWnd)
    {
        sCmd += L" /hwnd:";
        TCHAR buf[30] = { 0 };
        swprintf_s(buf, L"%p", (void*)hWnd);
        sCmd += buf;
    }

    if (line > 0)
    {
        sCmd += L" /line:";
        CString temp;
        temp.Format(L"%d", line);
        sCmd += temp;
    }

    return CAppUtils::RunTortoiseProc(sCmd);
}

bool CAppUtils::SetupDiffScripts(bool force, const CString& type)
{
    CString scriptsdir = CPathUtils::GetAppParentDirectory();
    scriptsdir += L"Diff-Scripts";
    CSimpleFileFind files(scriptsdir);
    while (files.FindNextFileNoDirectories())
    {
        CString file = files.GetFilePath();
        CString filename = files.GetFileName();
        CString ext = file.Mid(file.ReverseFind('-')+1);
        ext = L"."+ext.Left(ext.ReverseFind('.'));
        std::set<CString> extensions;
        extensions.insert(ext);
        CString kind;
        if (file.Right(3).CompareNoCase(L"vbs")==0)
        {
            kind = L" //E:vbscript";
        }
        if (file.Right(2).CompareNoCase(L"js")==0)
        {
            kind = L" //E:javascript";
        }
        // open the file, read the first line and find possible extensions
        // this script can handle
        try
        {
            CStdioFile f(file, CFile::modeRead | CFile::shareDenyNone);
            CString extline;
            if (f.ReadString(extline))
            {
                if ( (extline.GetLength() > 15 ) &&
                    ((extline.Left(15).Compare(L"// extensions: ")==0) ||
                     (extline.Left(14).Compare(L"' extensions: ")==0)) )
                {
                    if (extline[0] == '/')
                        extline = extline.Mid(15);
                    else
                        extline = extline.Mid(14);
                    CString sToken;
                    int curPos = 0;
                    sToken = extline.Tokenize(L";", curPos);
                    while (!sToken.IsEmpty())
                    {
                        if (!sToken.IsEmpty())
                        {
                            if (sToken[0] != '.')
                                sToken = L"." + sToken;
                            extensions.insert(sToken);
                        }
                        sToken = extline.Tokenize(L";", curPos);
                    }
                }
            }
            f.Close();
        }
        catch (CFileException* e)
        {
            e->Delete();
        }

        for (std::set<CString>::const_iterator it = extensions.begin(); it != extensions.end(); ++it)
        {
            if (type.IsEmpty() || (type.Compare(L"Diff")==0))
            {
                if (filename.Left(5).CompareNoCase(L"diff-")==0)
                {
                    CRegString diffreg = CRegString(L"Software\\TortoiseSVN\\DiffTools\\"+*it);
                    CString diffregstring = diffreg;
                    if (force || (diffregstring.IsEmpty()) || (diffregstring.Find(filename)>=0))
                        diffreg = L"wscript.exe \"" + file + L"\" %base %mine" + kind;
                }
            }
            if (type.IsEmpty() || (type.Compare(L"Merge")==0))
            {
                if (filename.Left(6).CompareNoCase(L"merge-")==0)
                {
                    CRegString diffreg = CRegString(L"Software\\TortoiseSVN\\MergeTools\\"+*it);
                    CString diffregstring = diffreg;
                    if (force || (diffregstring.IsEmpty()) || (diffregstring.Find(filename)>=0))
                        diffreg = L"wscript.exe \"" + file + L"\" %merged %theirs %mine %base" + kind;
                }
            }
        }
    }
    // Initialize "Software\\TortoiseSVN\\DiffProps" once with the same value as "Software\\TortoiseSVN\\Diff"
    CRegString regDiffPropsPath = CRegString(L"Software\\TortoiseSVN\\DiffProps",L"non-existant");
    CString strDiffPropsPath = regDiffPropsPath;
    if ( force || strDiffPropsPath==L"non-existant" )
    {
        CString strDiffPath = CRegString(L"Software\\TortoiseSVN\\Diff");
        regDiffPropsPath = strDiffPath;
    }

    return true;
}

void CAppUtils::SetCharFormat
    ( CWnd* window
    , DWORD mask
    , DWORD effects
    , const std::vector<CHARRANGE>& positions)
{
    CHARFORMAT2 format;
    SecureZeroMemory(&format, sizeof(CHARFORMAT2));
    format.cbSize = sizeof(CHARFORMAT2);
    format.dwMask = mask;
    format.dwEffects = effects;
    format.crTextColor = effects;

    for ( auto iter = positions.begin(), end = positions.end()
        ; iter != end
        ; ++iter)
    {
        CHARRANGE range = *iter;
        window->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
        window->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
    }
}

void CAppUtils::SetCharFormat(CWnd* window, DWORD mask, DWORD effects )
{
    CHARFORMAT2 format;
    SecureZeroMemory(&format, sizeof(CHARFORMAT2));
    format.cbSize = sizeof(CHARFORMAT2);
    format.dwMask = mask;
    format.dwEffects = effects;
    window->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
}

bool CAppUtils::AskToUpdate(HWND hParent, LPCWSTR error)
{
    CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TASK1)),
                        CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TITLE)),
                        L"TortoiseSVN",
                        0,
                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
    taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TASK3)));
    taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TASK4)));
    taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
    taskdlg.SetDefaultCommandControl(1);
    CString details;
    details.Format(IDS_MSG_NEEDSUPDATE_ERRORDETAILS, error);
    taskdlg.SetExpansionArea(details);
    taskdlg.SetMainIcon(TD_WARNING_ICON);
    return (taskdlg.DoModal(hParent) == 1);
}

void CAppUtils::ReportFailedHook( HWND hWnd, const CString& sError )
{
    std::wstring str = (LPCTSTR)sError;
    std::wregex rx(L"((https?|ftp|file)://[-A-Z0-9+&@#/%?=~_|!:,.;]*[-A-Z0-9+&@#/%=~_|])", std::regex_constants::icase | std::regex_constants::ECMAScript);
    std::wstring replacement = L"<A HREF=\"$1\">$1</A>";
    std::wstring str2 = std::regex_replace(str, rx, replacement);

    CTaskDialog taskdlg(str2.c_str(),
                        CString(MAKEINTRESOURCE(IDS_COMMITDLG_CHECKCOMMIT_TASK1)),
                        L"TortoiseSVN",
                        0,
                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
    taskdlg.SetCommonButtons(TDCBF_OK_BUTTON);
    taskdlg.SetMainIcon(TD_ERROR_ICON);
    taskdlg.DoModal(hWnd);
}

bool CAppUtils::HasMimeTool()
{
    CRegistryKey diffList(L"Software\\TortoiseSVN\\DiffTools");
    CStringList diffvalues;
    diffList.getValues(diffvalues);
    bool hasMimeTool = false;
    POSITION pos;
    for (pos = diffvalues.GetHeadPosition(); pos != NULL;)
    {
        auto s = diffvalues.GetNext(pos);
        if (s.Find('/') >= 0)
        {
            hasMimeTool = true;
            break;
        }
    }
    return hasMimeTool;
}

