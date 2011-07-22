// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN

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
#include "StdAfx.h"
#include "resource.h"
#include "TortoiseProc.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "SVNProperties.h"
#include "StringUtils.h"
#include "MessageBox.h"
#include "Registry.h"
#include "TSVNPath.h"
#include "SVN.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include <intshcut.h>
#include "auto_buffer.h"
#include "StringUtils.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"
#include "DirFileEnum.h"
#include "SysInfo.h"
#include "SelectFileFilter.h"
#include "SmartHandle.h"

bool CAppUtils::GetMimeType(const CTSVNPath& file, CString& mimetype)
{
    SVNProperties props(file, SVNRev::REV_WC, false);
    for (int i = 0; i < props.GetCount(); ++i)
    {
        if (props.GetItemName(i).compare(SVN_PROP_MIME_TYPE)==0)
        {
            mimetype = props.GetItemValue(i).c_str();
            return true;
        }
    }
    return false;
}

BOOL CAppUtils::StartExtMerge(const MergeFlags& flags,
    const CTSVNPath& basefile, const CTSVNPath& theirfile, const CTSVNPath& yourfile, const CTSVNPath& mergedfile,
    const CString& basename, const CString& theirname, const CString& yourname, const CString& mergedname)
{

    CRegString regCom = CRegString(_T("Software\\TortoiseSVN\\Merge"));
    CString ext = mergedfile.GetFileExtension();
    CString com = regCom;
    bool bInternal = false;

    CString mimetype;

    if (ext != "")
    {
        // is there an extension specific merge tool?
        CRegString mergetool(_T("Software\\TortoiseSVN\\MergeTools\\") + ext.MakeLower());
        if (CString(mergetool) != "")
        {
            com = mergetool;
        }
    }
    if (GetMimeType(yourfile, mimetype) || GetMimeType(theirfile, mimetype) || GetMimeType(basefile, mimetype))
    {
        // is there a mime type specific merge tool?
        CRegString mergetool(_T("Software\\TortoiseSVN\\MergeTools\\") + mimetype);
        if (CString(mergetool) != "")
        {
            com = mergetool;
        }
    }
    // is there a filename specific merge tool?
    CRegString mergetool(_T("Software\\TortoiseSVN\\MergeTools\\.") + mergedfile.GetFilename().MakeLower());
    if (CString(mergetool) != "")
    {
        com = mergetool;
    }

    if ((flags.bAlternativeTool)&&(!com.IsEmpty()))
    {
        if (com.Left(1).Compare(_T("#"))==0)
            com.Delete(0);
        else
            com.Empty();
    }

    if (com.IsEmpty()||(com.Left(1).Compare(_T("#"))==0))
    {
        // use TortoiseMerge
        bInternal = true;
        com = CPathUtils::GetAppDirectory() + _T("TortoiseMerge.exe");
        com = _T("\"") + com + _T("\"");
        com = com + _T(" /base:%base /theirs:%theirs /mine:%mine /merged:%merged");
        com = com + _T(" /basename:%bname /theirsname:%tname /minename:%yname /mergedname:%mname");
    }
    // check if the params are set. If not, just add the files to the command line
    if ((com.Find(_T("%merged"))<0)&&(com.Find(_T("%base"))<0)&&(com.Find(_T("%theirs"))<0)&&(com.Find(_T("%mine"))<0))
    {
        com += _T(" \"")+basefile.GetWinPathString()+_T("\"");
        com += _T(" \"")+theirfile.GetWinPathString()+_T("\"");
        com += _T(" \"")+yourfile.GetWinPathString()+_T("\"");
        com += _T(" \"")+mergedfile.GetWinPathString()+_T("\"");
    }
    if (basefile.IsEmpty())
    {
        com.Replace(_T("/base:%base"), _T(""));
        com.Replace(_T("%base"), _T(""));
    }
    else
        com.Replace(_T("%base"), _T("\"") + basefile.GetWinPathString() + _T("\""));
    if (theirfile.IsEmpty())
    {
        com.Replace(_T("/theirs:%theirs"), _T(""));
        com.Replace(_T("%theirs"), _T(""));
    }
    else
        com.Replace(_T("%theirs"), _T("\"") + theirfile.GetWinPathString() + _T("\""));
    if (yourfile.IsEmpty())
    {
        com.Replace(_T("/mine:%mine"), _T(""));
        com.Replace(_T("%mine"), _T(""));
    }
    else
        com.Replace(_T("%mine"), _T("\"") + yourfile.GetWinPathString() + _T("\""));
    if (mergedfile.IsEmpty())
    {
        com.Replace(_T("/merged:%merged"), _T(""));
        com.Replace(_T("%merged"), _T(""));
    }
    else
        com.Replace(_T("%merged"), _T("\"") + mergedfile.GetWinPathString() + _T("\""));
    if (basename.IsEmpty())
    {
        if (basefile.IsEmpty())
        {
            com.Replace(_T("/basename:%bname"), _T(""));
            com.Replace(_T("%bname"), _T(""));
        }
        else
        {
            com.Replace(_T("%bname"), _T("\"") + basefile.GetUIFileOrDirectoryName() + _T("\""));
        }
    }
    else
        com.Replace(_T("%bname"), _T("\"") + basename + _T("\""));
    if (theirname.IsEmpty())
    {
        if (theirfile.IsEmpty())
        {
            com.Replace(_T("/theirsname:%tname"), _T(""));
            com.Replace(_T("%tname"), _T(""));
        }
        else
        {
            com.Replace(_T("%tname"), _T("\"") + theirfile.GetUIFileOrDirectoryName() + _T("\""));
        }
    }
    else
        com.Replace(_T("%tname"), _T("\"") + theirname + _T("\""));
    if (yourname.IsEmpty())
    {
        if (yourfile.IsEmpty())
        {
            com.Replace(_T("/minename:%yname"), _T(""));
            com.Replace(_T("%yname"), _T(""));
        }
        else
        {
            com.Replace(_T("%yname"), _T("\"") + yourfile.GetUIFileOrDirectoryName() + _T("\""));
        }
    }
    else
        com.Replace(_T("%yname"), _T("\"") + yourname + _T("\""));
    if (mergedname.IsEmpty())
    {
        if (mergedfile.IsEmpty())
        {
            com.Replace(_T("/mergedname:%mname"), _T(""));
            com.Replace(_T("%mname"), _T(""));
        }
        else
        {
            com.Replace(_T("%mname"), _T("\"") + mergedfile.GetUIFileOrDirectoryName() + _T("\""));
        }
    }
    else
        com.Replace(_T("%mname"), _T("\"") + mergedname + _T("\""));

    if ((flags.bReadOnly)&&(bInternal))
        com += _T(" /readonly");

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
    viewer += _T("TortoiseMerge.exe");

    viewer = _T("\"") + viewer + _T("\"");
    viewer = viewer + _T(" /diff:\"") + patchfile.GetWinPathString() + _T("\"");
    viewer = viewer + _T(" /patchpath:\"") + dir.GetWinPathString() + _T("\"");
    if (bReversed)
        viewer += _T(" /reversedpatch");
    if (!sOriginalDescription.IsEmpty())
        viewer = viewer + _T(" /patchoriginal:\"") + sOriginalDescription + _T("\"");
    if (!sPatchedDescription.IsEmpty())
        viewer = viewer + _T(" /patchpatched:\"") + sPatchedDescription + _T("\"");
    if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
    {
        return FALSE;
    }
    return TRUE;
}

CString CAppUtils::PickDiffTool(const CTSVNPath& file1, const CTSVNPath& file2)
{
    CString difftool = CRegString(_T("Software\\TortoiseSVN\\DiffTools\\.") + file2.GetFilename().MakeLower());
    if (!difftool.IsEmpty())
        return difftool;
    difftool = CRegString(_T("Software\\TortoiseSVN\\DiffTools\\.") + file1.GetFilename().MakeLower());
    if (!difftool.IsEmpty())
        return difftool;

    // Is there a mime type specific diff tool?
    CString mimetype;
    if (GetMimeType(file1, mimetype) ||  GetMimeType(file2, mimetype))
    {
        CString difftool = CRegString(_T("Software\\TortoiseSVN\\DiffTools\\") + mimetype);
        if (!difftool.IsEmpty())
            return difftool;
    }

    // Is there an extension specific diff tool?
    CString ext = file2.GetFileExtension().MakeLower();
    if (!ext.IsEmpty())
    {
        difftool = CRegString(_T("Software\\TortoiseSVN\\DiffTools\\") + ext);
        if (!difftool.IsEmpty())
            return difftool;
        // Maybe we should use TortoiseIDiff?
        if ((ext == _T(".jpg")) || (ext == _T(".jpeg")) ||
            (ext == _T(".bmp")) || (ext == _T(".gif"))  ||
            (ext == _T(".png")) || (ext == _T(".ico"))  ||
            (ext == _T(".tif")) || (ext == _T(".tiff"))  ||
            (ext == _T(".dib")) || (ext == _T(".emf")))
        {
            return
                _T("\"") + CPathUtils::GetAppDirectory() + _T("TortoiseIDiff.exe") + _T("\"") +
                _T(" /left:%base /right:%mine /lefttitle:%bname /righttitle:%yname");
        }
    }

    // Finally, pick a generic external diff tool
    difftool = CRegString(_T("Software\\TortoiseSVN\\Diff"));
    return difftool;
}

bool CAppUtils::StartExtDiff(
    const CTSVNPath& file1, const CTSVNPath& file2,
    const CString& sName1, const CString& sName2, const DiffFlags& flags, int line)
{
    return StartExtDiff(file1, file2, sName1, sName2, CTSVNPath(), CTSVNPath(), SVNRev(), SVNRev(), SVNRev(), flags, line);
}

bool CAppUtils::StartExtDiff(
    const CTSVNPath& file1, const CTSVNPath& file2,
    const CString& sName1, const CString& sName2,
    const CTSVNPath& url1, const CTSVNPath& url2,
    const SVNRev& rev1, const SVNRev& rev2,
    const SVNRev& pegRev, const DiffFlags& flags, int line)
{
    CString viewer;

    CRegDWORD blamediff(_T("Software\\TortoiseSVN\\DiffBlamesWithTortoiseMerge"), FALSE);
    if (!flags.bBlame || !(DWORD)blamediff)
    {
        viewer = PickDiffTool(file1, file2);
        // If registry entry for a diff program is commented out, use TortoiseMerge.
        bool bCommentedOut = viewer.Left(1) == _T("#");
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
            _T("\"") + CPathUtils::GetAppDirectory() + _T("TortoiseMerge.exe") + _T("\"") +
            _T(" /base:%base /mine:%mine /basename:%bname /minename:%yname");
        if (flags.bBlame)
            viewer += _T(" /blame");
    }
    // check if the params are set. If not, just add the files to the command line
    if ((viewer.Find(_T("%base"))<0)&&(viewer.Find(_T("%mine"))<0))
    {
        viewer += _T(" \"")+file1.GetWinPathString()+_T("\"");
        viewer += _T(" \"")+file2.GetWinPathString()+_T("\"");
    }
    if (viewer.Find(_T("%base")) >= 0)
    {
        viewer.Replace(_T("%base"),  _T("\"")+file1.GetWinPathString()+_T("\""));
    }
    if (viewer.Find(_T("%mine")) >= 0)
    {
        viewer.Replace(_T("%mine"),  _T("\"")+file2.GetWinPathString()+_T("\""));
    }
    if (sName1.IsEmpty())
        viewer.Replace(_T("%bname"), _T("\"") + file1.GetUIFileOrDirectoryName() + _T("\""));
    else
        viewer.Replace(_T("%bname"), _T("\"") + sName1 + _T("\""));

    if (sName2.IsEmpty())
        viewer.Replace(_T("%yname"), _T("\"") + file2.GetUIFileOrDirectoryName() + _T("\""));
    else
        viewer.Replace(_T("%yname"), _T("\"") + sName2 + _T("\""));

    if (viewer.Find(_T("%burl")) >= 0)
    {
        viewer.Replace(_T("%burl"),  _T("\"")+url1.GetSVNPathString()+_T("\""));
    }
    if (viewer.Find(_T("%yurl")) >= 0)
    {
        viewer.Replace(_T("%yurl"),  _T("\"")+url2.GetSVNPathString()+_T("\""));
    }
    if (viewer.Find(_T("%brev")) >= 0)
    {
        viewer.Replace(_T("%brev"),  _T("\"")+rev1.ToString()+_T("\""));
    }
    if (viewer.Find(_T("%yrev")) >= 0)
    {
        viewer.Replace(_T("%yrev"),  _T("\"")+rev2.ToString()+_T("\""));
    }
    if (viewer.Find(_T("%peg")) >= 0)
    {
        viewer.Replace(_T("%peg"),  _T("\"")+pegRev.ToString()+_T("\""));
    }

    if (flags.bReadOnly && bInternal)
        viewer += _T(" /readonly");
    if (line > 0)
    {
        viewer += _T(" /line:");
        CString temp;
        temp.Format(_T("%ld"), line);
        viewer += temp;
    }

    return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, flags.bWait);
}

BOOL CAppUtils::StartExtDiffProps(const CTSVNPath& file1, const CTSVNPath& file2, const CString& sName1, const CString& sName2, BOOL bWait, BOOL bReadOnly)
{
    CRegString diffpropsexe(_T("Software\\TortoiseSVN\\DiffProps"));
    CString viewer = diffpropsexe;
    bool bInternal = false;
    if (viewer.IsEmpty()||(viewer.Left(1).Compare(_T("#"))==0))
    {
        //no registry entry (or commented out) for a diff program
        //use TortoiseMerge
        bInternal = true;
        viewer = CPathUtils::GetAppDirectory();
        viewer += _T("TortoiseMerge.exe");
        viewer = _T("\"") + viewer + _T("\"");
        viewer = viewer + _T(" /base:%base /mine:%mine /basename:%bname /minename:%yname");
    }
    // check if the params are set. If not, just add the files to the command line
    if ((viewer.Find(_T("%base"))<0)&&(viewer.Find(_T("%mine"))<0))
    {
        viewer += _T(" \"")+file1.GetWinPathString()+_T("\"");
        viewer += _T(" \"")+file2.GetWinPathString()+_T("\"");
    }
    if (viewer.Find(_T("%base")) >= 0)
    {
        viewer.Replace(_T("%base"),  _T("\"")+file1.GetWinPathString()+_T("\""));
    }
    if (viewer.Find(_T("%mine")) >= 0)
    {
        viewer.Replace(_T("%mine"),  _T("\"")+file2.GetWinPathString()+_T("\""));
    }

    if (sName1.IsEmpty())
        viewer.Replace(_T("%bname"), _T("\"") + file1.GetUIFileOrDirectoryName() + _T("\""));
    else
        viewer.Replace(_T("%bname"), _T("\"") + sName1 + _T("\""));

    if (sName2.IsEmpty())
        viewer.Replace(_T("%yname"), _T("\"") + file2.GetUIFileOrDirectoryName() + _T("\""));
    else
        viewer.Replace(_T("%yname"), _T("\"") + sName2 + _T("\""));

    if ((bReadOnly)&&(bInternal))
        viewer += _T(" /readonly");

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
    logFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8), GetDeviceCaps(hScreenDC, LOGPIXELSY), 72);
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
    _tcscpy_s(logFont.lfFaceName, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")));
    VERIFY(fontToCreate.CreateFontIndirect(&logFont));
}

/**
* Launch the external blame viewer
*/
bool CAppUtils::LaunchTortoiseBlame(const CString& sBlameFile, const CString& sOriginalFile, const CString& sParams, const SVNRev& startrev, const SVNRev& endrev)
{
    CString viewer = CPathUtils::GetAppDirectory();
    viewer += _T("TortoiseBlame.exe");
    viewer += _T(" \"") + sBlameFile + _T("\"");
    viewer += _T(" \"") + sOriginalFile + _T("\"");
    viewer += _T(" ")+sParams;
    if (startrev.IsValid() && endrev.IsValid())
        viewer += _T(" /revrange:\"") + startrev.ToString() + _T("-") + endrev.ToString() + _T("\"");

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
    sText.Remove(_T('\r'));

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
    ( const wstring& text
    , const CString& matchstring
    , const CString& matchsubstring /* = _T(".*")*/)
{
    std::vector<CHARRANGE> result;
    if (matchstring.IsEmpty())
        return result;

    try
    {
        const tr1::wregex regMatch(matchstring, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
        const tr1::wregex regSubMatch(matchsubstring, tr1::regex_constants::icase | tr1::regex_constants::ECMAScript);
        const tr1::wsregex_iterator end;
        for (tr1::wsregex_iterator it(text.begin(), text.end(), regMatch); it != end; ++it)
        {
            // (*it)[0] is the matched string
            wstring matchedString = (*it)[0];
            ptrdiff_t matchpos = it->position(0);
            for (tr1::wsregex_iterator it2(matchedString.begin(), matchedString.end(), regSubMatch); it2 != end; ++it2)
            {
                ATLTRACE(_T("matched id : %s\n"), (*it2)[0].str().c_str());
                ptrdiff_t matchposID = it2->position(0);
                CHARRANGE range = {(LONG)(matchpos+matchposID), (LONG)(matchpos+matchposID+(*it2)[0].str().size())};
                result.push_back (range);
            }
        }
    }
    catch (exception) {}

    return result;
}

bool CAppUtils::FindStyleChars(const CString& sText, TCHAR stylechar, int& start, int& end)
{
    int i=start;
    int last = sText.GetLength()-1;
    bool bFoundMarker = false;
    TCHAR c = i == 0 ? _T('\0') : sText[i-1];
    TCHAR nextChar = i >= last ? _T('\0') : sText[i+1];

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
    strUrl.Replace(_T("%"), _T("%25"));
    strUrl.TrimLeft('/');

    CString trimmedRoot = repoRoot;
    trimmedRoot.TrimRight('/');

    strUrl = trimmedRoot + _T("/") + strUrl;

    if (strUrl.Left(7) == _T("file://"))
    {
        // browse repository - show repository browser
        SVN::preparePath(strUrl);
        CRepositoryBrowser browser(strUrl, rev, pParent);
        if (browser.DoModal() == IDOK)
        {
            combo.SetCurSel(-1);
            if (bExternalUrl)
                combo.SetWindowText(_T("^") + browser.GetPath().Mid(repoRoot.GetLength()));
            else
                combo.SetWindowText(browser.GetPath().Mid(repoRoot.GetLength()));
            combo.SetFocus();
            rev = browser.GetRevision();
            return true;
        }
    }
    else if ((strUrl.Left(7) == _T("http://")
        ||(strUrl.Left(8) == _T("https://"))
        ||(strUrl.Left(6) == _T("svn://"))
        ||(strUrl.Left(4) == _T("svn+"))) && strUrl.GetLength() > 6)
    {
        // browse repository - show repository browser
        CRepositoryBrowser browser(strUrl, rev, pParent);
        if (browser.DoModal() == IDOK)
        {
            combo.SetCurSel(-1);
            if (bExternalUrl)
                combo.SetWindowText(_T("^") + browser.GetPath().Mid(repoRoot.GetLength()));
            else
                combo.SetWindowText(browser.GetPath().Mid(repoRoot.GetLength()));
            combo.SetFocus();
            rev = browser.GetRevision();
            return true;
        }
    }
    combo.SetFocus();
    return false;
}

bool CAppUtils::BrowseRepository(CHistoryCombo& combo, CWnd * pParent, SVNRev& rev, bool multiSelection)
{
    CString strURLs;
    combo.GetWindowText(strURLs);
    if (strURLs.IsEmpty())
        strURLs = combo.GetString();
    strURLs.Replace('\\', '/');
    strURLs.Replace(_T("%"), _T("%25"));

    CTSVNPathList paths;
    paths.LoadFromAsteriskSeparatedString (strURLs);

    CString strUrl = paths.GetCommonRoot().GetSVNPathString();
    if (strUrl.Left(7) == _T("file://"))
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
    else if ((strUrl.Left(7) == _T("http://")
        ||(strUrl.Left(8) == _T("https://"))
        ||(strUrl.Left(6) == _T("svn://"))
        ||(strUrl.Left(4) == _T("svn+"))) && strUrl.GetLength() > 6)
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

    combo.SetFocus();
    return false;
}

CString CAppUtils::GetProjectNameFromURL(CString url)
{
    CString name;
    while ((name.IsEmpty() || (name.CompareNoCase(_T("branches"))==0) ||
        (name.CompareNoCase(_T("tags"))==0) ||
        (name.CompareNoCase(_T("trunk"))==0))&&(!url.IsEmpty()))
    {
        name = url.Mid(url.ReverseFind('/')+1);
        url = url.Left(url.ReverseFind('/'));
    }
    if ((name.Compare(_T("svn")) == 0)||(name.Compare(_T("svnroot")) == 0))
    {
        // a name of svn or svnroot indicates that it's not really the project name. In that
        // case, we try the first part of the URL
        // of course, this won't work in all cases (but it works for Google project hosting)
        url.Replace(_T("http://"), _T(""));
        url.Replace(_T("https://"), _T(""));
        url.Replace(_T("svn://"), _T(""));
        url.Replace(_T("svn+ssh://"), _T(""));
        url.TrimLeft(_T("/"));
        name = url.Left(url.Find('.'));
    }
    return name;
}

bool CAppUtils::StartShowUnifiedDiff(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1,
                                     const CTSVNPath& url2, const SVNRev& rev2,
                                     const SVNRev& peg, const SVNRev& headpeg,
                                     const CString& options,
                                     bool bAlternateDiff /* = false */, bool bIgnoreAncestry /* = false */, bool /* blame = false */)
{
    CString sCmd = _T("/command:showcompare /unified");
    sCmd += _T(" /url1:\"") + url1.GetSVNPathString() + _T("\"");
    if (rev1.IsValid())
        sCmd += _T(" /revision1:") + rev1.ToString();
    sCmd += _T(" /url2:\"") + url2.GetSVNPathString() + _T("\"");
    if (rev2.IsValid())
        sCmd += _T(" /revision2:") + rev2.ToString();
    if (peg.IsValid())
        sCmd += _T(" /pegrevision:") + peg.ToString();
    if (headpeg.IsValid())
        sCmd += _T(" /headpegrevision:") + headpeg.ToString();

    if (!options.IsEmpty())
        sCmd += L" /diffoptions:\"" + options + L"\"";

    if (bAlternateDiff)
        sCmd += _T(" /alternatediff");

    if (bIgnoreAncestry)
        sCmd += _T(" /ignoreancestry");

    if (hWnd)
    {
        sCmd += _T(" /hwnd:");
        TCHAR buf[30];
        _stprintf_s(buf, _T("%ld"), (DWORD)hWnd);
        sCmd += buf;
    }

    return CAppUtils::RunTortoiseProc(sCmd);
}

bool CAppUtils::StartShowCompare(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1,
                                 const CTSVNPath& url2, const SVNRev& rev2,
                                 const SVNRev& peg, const SVNRev& headpeg,
                                 const CString& options,
                                 bool bAlternateDiff /* = false */, bool bIgnoreAncestry /* = false */,
                                 bool blame /* = false */, svn_node_kind_t nodekind /* = svn_node_unknown */,
                                 int line /* = 0 */)
{
    CString sCmd;
    sCmd.Format(_T("/command:showcompare /nodekind:%d"), nodekind);
    sCmd += _T(" /url1:\"") + url1.GetSVNPathString() + _T("\"");
    if (rev1.IsValid())
        sCmd += _T(" /revision1:") + rev1.ToString();
    sCmd += _T(" /url2:\"") + url2.GetSVNPathString() + _T("\"");
    if (rev2.IsValid())
        sCmd += _T(" /revision2:") + rev2.ToString();
    if (peg.IsValid())
        sCmd += _T(" /pegrevision:") + peg.ToString();
    if (headpeg.IsValid())
        sCmd += _T(" /headpegrevision:") + headpeg.ToString();
    if (!options.IsEmpty())
        sCmd += L" /diffoptions:\"" + options + L"\"";
    if (bAlternateDiff)
        sCmd += _T(" /alternatediff");
    if (bIgnoreAncestry)
        sCmd += _T(" /ignoreancestry");
    if (blame)
        sCmd += _T(" /blame");

    if (hWnd)
    {
        sCmd += _T(" /hwnd:");
        TCHAR buf[30];
        _stprintf_s(buf, _T("%ld"), (DWORD)hWnd);
        sCmd += buf;
    }

    if (line > 0)
    {
        sCmd += _T(" /line:");
        CString temp;
        temp.Format(_T("%ld"), line);
        sCmd += temp;
    }

    return CAppUtils::RunTortoiseProc(sCmd);
}

bool CAppUtils::SetupDiffScripts(bool force, const CString& type)
{
    CString scriptsdir = CPathUtils::GetAppParentDirectory();
    scriptsdir += _T("Diff-Scripts");
    CSimpleFileFind files(scriptsdir);
    while (files.FindNextFileNoDirectories())
    {
        CString file = files.GetFilePath();
        CString filename = files.GetFileName();
        CString ext = file.Mid(file.ReverseFind('-')+1);
        ext = _T(".")+ext.Left(ext.ReverseFind('.'));
        std::set<CString> extensions;
        extensions.insert(ext);
        CString kind;
        if (file.Right(3).CompareNoCase(_T("vbs"))==0)
        {
            kind = _T(" //E:vbscript");
        }
        if (file.Right(2).CompareNoCase(_T("js"))==0)
        {
            kind = _T(" //E:javascript");
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
                    ((extline.Left(15).Compare(_T("// extensions: "))==0) ||
                     (extline.Left(14).Compare(_T("' extensions: "))==0)) )
                {
                    if (extline[0] == '/')
                        extline = extline.Mid(15);
                    else
                        extline = extline.Mid(14);
                    CString sToken;
                    int curPos = 0;
                    sToken = extline.Tokenize(_T(";"), curPos);
                    while (!sToken.IsEmpty())
                    {
                        if (!sToken.IsEmpty())
                        {
                            if (sToken[0] != '.')
                                sToken = _T(".") + sToken;
                            extensions.insert(sToken);
                        }
                        sToken = extline.Tokenize(_T(";"), curPos);
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
            if (type.IsEmpty() || (type.Compare(_T("Diff"))==0))
            {
                if (filename.Left(5).CompareNoCase(_T("diff-"))==0)
                {
                    CRegString diffreg = CRegString(_T("Software\\TortoiseSVN\\DiffTools\\")+*it);
                    CString diffregstring = diffreg;
                    if (force || (diffregstring.IsEmpty()) || (diffregstring.Find(filename)>=0))
                        diffreg = _T("wscript.exe \"") + file + _T("\" %base %mine") + kind;
                }
            }
            if (type.IsEmpty() || (type.Compare(_T("Merge"))==0))
            {
                if (filename.Left(6).CompareNoCase(_T("merge-"))==0)
                {
                    CRegString diffreg = CRegString(_T("Software\\TortoiseSVN\\MergeTools\\")+*it);
                    CString diffregstring = diffreg;
                    if (force || (diffregstring.IsEmpty()) || (diffregstring.Find(filename)>=0))
                        diffreg = _T("wscript.exe \"") + file + _T("\" %merged %theirs %mine %base") + kind;
                }
            }
        }
    }
    // Initialize "Software\\TortoiseSVN\\DiffProps" once with the same value as "Software\\TortoiseSVN\\Diff"
    CRegString regDiffPropsPath = CRegString(_T("Software\\TortoiseSVN\\DiffProps"),_T("non-existant"));
    CString strDiffPropsPath = regDiffPropsPath;
    if ( force || strDiffPropsPath==_T("non-existant") )
    {
        CString strDiffPath = CRegString(_T("Software\\TortoiseSVN\\Diff"));
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
    if (CTaskDialog::IsSupported())
    {
        CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TASK1)), 
                            CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TITLE)), 
                            L"TortoiseSVN",
                            0,
                            TDF_ENABLE_HYPERLINKS|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TASK3)));
        taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TASK4)));
        taskdlg.SetDefaultCommandControl(1);
        CString details;
        details.Format(IDS_MSG_NEEDSUPDATE_ERRORDETAILS, error);
        taskdlg.SetExpansionArea(details);
        taskdlg.SetMainIcon(TD_WARNING_ICON);
        return (taskdlg.DoModal(hParent) == 1);
    }

    CString question;
    question.Format (IDS_MSG_NEEDSUPDATE_QUESTION, error);
    const UINT result = TSVNMessageBox(hParent, question, CString(MAKEINTRESOURCE(IDS_MSG_NEEDSUPDATE_TITLE)),
                                       MB_DEFBUTTON1|MB_ICONQUESTION, 
                                       CString(MAKEINTRESOURCE(IDS_PROGRS_CMD_UPDATE)), 
                                       CString(MAKEINTRESOURCE(IDS_MSGBOX_CANCEL)));
    return result == IDCUSTOM1;
}

void CAppUtils::ReportFailedHook( HWND hWnd, const CString& sError )
{
    std::wstring str = (LPCTSTR)sError;
    std::wregex rx(L"((https?|ftp|file)://[-A-Z0-9+&@#/%?=~_|!:,.;]*[-A-Z0-9+&@#/%=~_|])", std::regex_constants::icase | std::regex_constants::ECMAScript);
    std::wstring replacement = L"<A HREF=\"$1\">$1</A>";
    std::wstring str2 = std::regex_replace(str, rx, replacement);

    if (CTaskDialog::IsSupported())
    {
        CTaskDialog taskdlg(str2.c_str(), 
            CString(MAKEINTRESOURCE(IDS_COMMITDLG_CHECKCOMMIT_TASK1)),
            L"TortoiseSVN",
            0,
            TDF_ENABLE_HYPERLINKS|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW);
        taskdlg.SetCommonButtons(TDCBF_OK_BUTTON);
        taskdlg.SetMainIcon(TD_ERROR_ICON);
        taskdlg.DoModal(hWnd);
    }
    else
        ::MessageBox(hWnd, sError, _T("TortoiseSVN"), MB_ICONERROR);
}

