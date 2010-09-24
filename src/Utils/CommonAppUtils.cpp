// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include "CommonAppUtils.h"
#include "Registry.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include <intshcut.h>
#include "CreateProcessHelper.h"
#include "SelectFileFilter.h"
#include <WinInet.h>
#include <oleacc.h>
#include <initguid.h>

extern CString sOrigCWD;

BOOL CCommonAppUtils::StartUnifiedDiffViewer(const CString& patchfile, const CString& title, BOOL bWait)
{
    CString viewer;
    CRegString v = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
    viewer = v;
    if (viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0))
    {
        // use TortoiseUDiff
        viewer = CPathUtils::GetAppDirectory();
        viewer += _T("TortoiseUDiff.exe");
        // enquote the path to TortoiseUDiff
        viewer = _T("\"") + viewer + _T("\"");
        // add the params
        viewer = viewer + _T(" /patchfile:%1 /title:\"%title\"");

    }
    if (viewer.Find(_T("%1"))>=0)
    {
        if (viewer.Find(_T("\"%1\"")) >= 0)
            viewer.Replace(_T("%1"), patchfile);
        else
            viewer.Replace(_T("%1"), _T("\"") + patchfile + _T("\""));
    }
    else
        viewer += _T(" \"") + patchfile + _T("\"");
    if (viewer.Find(_T("%title")) >= 0)
    {
        viewer.Replace(_T("%title"), title);
    }

    if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
    {
        return FALSE;
    }
    return TRUE;
}

CString CCommonAppUtils::ExpandEnvironmentStrings (const CString& s)
{
    DWORD len = ::ExpandEnvironmentStrings (s, NULL, 0);
    if (len == 0)
        return s;

    auto_buffer<TCHAR> buf(len+1);
    if (::ExpandEnvironmentStrings (s, buf, len) == 0)
        return s;

    return buf.get();
}

CString CCommonAppUtils::GetAppForFile
    ( const CString& fileName
    , const CString& extension
    , const CString& verb
    , bool applySecurityHeuristics)
{
    CString application;

    // normalize file path

    CString fullFileName = ExpandEnvironmentStrings (fileName);
    CString normalizedFileName = _T("\"") + fullFileName + _T("\"");

    // registry lookup

    CString extensionToUse = extension.IsEmpty()
        ? CPathUtils::GetFileExtFromPath (fullFileName)
        : extension;

    if (!extensionToUse.IsEmpty())
    {
        // lookup by verb

        CString documentClass;
        DWORD buflen = 0;
        AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, verb, NULL, &buflen);
        auto_buffer<TCHAR> cmdbuf(buflen + 1);
        if (FAILED(AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, verb, cmdbuf, &buflen)))
        {
            documentClass = CRegString (extensionToUse + _T("\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);

            CString key = documentClass + _T("\\Shell\\") + verb + _T("\\Command\\");
            application = CRegString (key, _T(""), FALSE, HKEY_CLASSES_ROOT);
        }
        else
        {
            application = cmdbuf;
        }

        // fallback to "open"

        if (application.IsEmpty())
        {
            DWORD buflen = 0;
            AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, _T("open"), NULL, &buflen);
            auto_buffer<TCHAR> cmdbuf (buflen + 1);
            if (FAILED(AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, _T("open"), cmdbuf, &buflen)))
            {
                CString key = documentClass + _T("\\Shell\\Open\\Command\\");
                application = CRegString (key, _T(""), FALSE, HKEY_CLASSES_ROOT);
            }
            else
            {
                application = cmdbuf;
            }
        }
    }

    // normalize application path

    application = ExpandEnvironmentStrings (application);

    // security heuristics:
    // some scripting languages (e.g. python) will execute the document
    // instead of open it. Try to identify these cases.

    if (applySecurityHeuristics)
    {
        if (   (application.Find (_T("%2")) >= 0)
            || (application.Find (_T("%*")) >= 0))
        {
            // consumes extra parameters
            // -> probably a script execution
            // -> retry with "open" verb or ask user

            if (verb.CompareNoCase (_T("open")) == 0)
                application.Empty();
            else
                return GetAppForFile ( fileName
                                     , extension
                                     , _T("open")
                                     , true);
        }
    }

    // exit here, if we were not successful

    if (application.IsEmpty())
        return application;

    // resolve parameters

    if (application.Find (_T("%1")) < 0)
        application += _T(" %1");

    if (application.Find(_T("\"%1\"")) >= 0)
        application.Replace(_T("\"%1\""), _T("%1"));

    application.Replace (_T("%1"), normalizedFileName);

    return application;
}

BOOL CCommonAppUtils::StartTextViewer(CString file)
{
    CString viewer = GetAppForFile (file, _T(".txt"), _T("open"), false);
    if (viewer.IsEmpty())
    {
        int ret = (int)ShellExecute(NULL, _T("openas"), file, NULL, NULL, SW_SHOWNORMAL);
        if (ret <= HINSTANCE_ERROR)
        {
            CString c = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
            c += file + _T(" ");
            LaunchApplication(c, IDS_ERR_TEXTVIEWSTART, false);
        }
    }
    return LaunchApplication (viewer, IDS_ERR_TEXTVIEWSTART, false)
        ? TRUE
        : FALSE;
}

bool CCommonAppUtils::LaunchApplication
    ( const CString& sCommandLine
    , UINT idErrMessageFormat
    , bool bWaitForStartup
    , bool bWaitForExit)
{
    PROCESS_INFORMATION process;

    // make sure we get a writable copy of the command line

    size_t bufferLen = sCommandLine.GetLength()+1;
    auto_buffer<TCHAR> cleanCommandLine (bufferLen);
    memcpy (cleanCommandLine.get(), 
            (LPCTSTR)sCommandLine, 
            sizeof (TCHAR) * bufferLen);

    if (!CCreateProcessHelper::CreateProcess(NULL, cleanCommandLine.get(), sOrigCWD, &process))
    {
        if(idErrMessageFormat != 0)
        {
            CFormatMessageWrapper errorDetails;
            CString msg;
            msg.Format(idErrMessageFormat, (LPCTSTR)errorDetails);
            CString title;
            title.LoadString(IDS_APPNAME);
            MessageBox(NULL, msg, title, MB_OK | MB_ICONINFORMATION);
        }
        return false;
    }
    AllowSetForegroundWindow(process.dwProcessId);

    if (bWaitForStartup)
        WaitForInputIdle(process.hProcess, 10000);

    if (bWaitForExit)
        WaitForSingleObject (process.hProcess, INFINITE);

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    return true;
}

void CCommonAppUtils::ResizeAllListCtrlCols(CListCtrl * pListCtrl)
{
    int maxcol = ((CHeaderCtrl*)(pListCtrl->GetDlgItem(0)))->GetItemCount()-1;
    int nItemCount = pListCtrl->GetItemCount();
    TCHAR textbuf[MAX_PATH];
    CHeaderCtrl * pHdrCtrl = (CHeaderCtrl*)(pListCtrl->GetDlgItem(0));
    if (pHdrCtrl)
    {
        for (int col = 0; col <= maxcol; col++)
        {
            HDITEM hdi = {0};
            hdi.mask = HDI_TEXT;
            hdi.pszText = textbuf;
            hdi.cchTextMax = _countof(textbuf);
            pHdrCtrl->GetItem(col, &hdi);
            int cx = pListCtrl->GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin
            for (int index = 0; index<nItemCount; ++index)
            {
                // get the width of the string and add 14 pixels for the column separator and margins
                int linewidth = pListCtrl->GetStringWidth(pListCtrl->GetItemText(index, col)) + 14;
                if (index == 0)
                {
                    // add the image size
                    CImageList * pImgList = pListCtrl->GetImageList(LVSIL_SMALL);
                    if ((pImgList)&&(pImgList->GetImageCount()))
                    {
                        IMAGEINFO imginfo;
                        pImgList->GetImageInfo(0, &imginfo);
                        linewidth += (imginfo.rcImage.right - imginfo.rcImage.left);
                        linewidth += 3; // 3 pixels between icon and text
                    }
                }
                if (cx < linewidth)
                    cx = linewidth;
            }
            pListCtrl->SetColumnWidth(col, cx);

        }
    }
}

bool CCommonAppUtils::SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width /* = 128 */, int height /* = 128 */)
{
    ListView_SetTextBkColor(hListCtrl, CLR_NONE);
    COLORREF bkColor = ListView_GetBkColor(hListCtrl);
    // create a bitmap from the icon
    HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(nID), IMAGE_ICON, width, height, LR_DEFAULTCOLOR);
    if (!hIcon)
        return false;

    RECT rect = {0};
    rect.right = width;
    rect.bottom = height;
    HBITMAP bmp = NULL;

    HWND desktop = ::GetDesktopWindow();
    if (desktop)
    {
        HDC screen_dev = ::GetDC(desktop);
        if (screen_dev)
        {
            // Create a compatible DC
            HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
            if (dst_hdc)
            {
                // Create a new bitmap of icon size
                bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
                if (bmp)
                {
                    // Select it into the compatible DC
                    HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);
                    // Fill the background of the compatible DC with the given color
                    ::SetBkColor(dst_hdc, bkColor);
                    ::ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

                    // Draw the icon into the compatible DC
                    ::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);
                    ::SelectObject(dst_hdc, old_dst_bmp);
                }
                ::DeleteDC(dst_hdc);
            }
        }
        ::ReleaseDC(desktop, screen_dev);
    }

    // Restore settings
    DestroyIcon(hIcon);

    if (bmp == NULL)
        return false;

    LVBKIMAGE lv;
    lv.ulFlags = LVBKIF_TYPE_WATERMARK;
    lv.hbm = bmp;
    lv.xOffsetPercent = 100;
    lv.yOffsetPercent = 100;
    ListView_SetBkImage(hListCtrl, &lv);
    return true;
}

HRESULT CCommonAppUtils::CreateShortCut(LPCTSTR pszTargetfile, LPCTSTR pszTargetargs,
                       LPCTSTR pszLinkfile, LPCTSTR pszDescription,
                       int iShowmode, LPCTSTR pszCurdir,
                       LPCTSTR pszIconfile, int iIconindex)
{
    if ((pszTargetfile == NULL) || (_tcslen(pszTargetfile) == 0) ||
        (pszLinkfile == NULL) || (_tcslen(pszLinkfile) == 0))
    {
        return E_INVALIDARG;
    }
    CComPtr<IShellLink> pShellLink;
    HRESULT hRes = pShellLink.CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER);
    if (FAILED(hRes))
        return hRes;

    hRes = pShellLink->SetPath(pszTargetfile);
    hRes = pShellLink->SetArguments(pszTargetargs);
    if (_tcslen(pszDescription) > 0)
    {
        hRes = pShellLink->SetDescription(pszDescription);
    }
    if (iShowmode > 0)
    {
        hRes = pShellLink->SetShowCmd(iShowmode);
    }
    if (_tcslen(pszCurdir) > 0)
    {
        hRes = pShellLink->SetWorkingDirectory(pszCurdir);
    }
    if (_tcslen(pszIconfile) > 0 && iIconindex >= 0)
    {
        hRes = pShellLink->SetIconLocation(pszIconfile, iIconindex);
    }

    // Use the IPersistFile object to save the shell link
    CComPtr<IPersistFile> pPersistFile;
    hRes = pShellLink.QueryInterface(&pPersistFile);
    if (SUCCEEDED(hRes))
    {
        hRes = pPersistFile->Save(pszLinkfile, TRUE);
    }
    return hRes;
}

HRESULT CCommonAppUtils::CreateShortcutToURL(LPCTSTR pszURL, LPCTSTR pszLinkFile)
{
    CComPtr<IUniformResourceLocator> pURL;
    // Create an IUniformResourceLocator object
    HRESULT hRes = pURL.CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER);
    if(FAILED(hRes))
        return hRes;

    hRes = pURL->SetURL(pszURL, 0);
    if(FAILED(hRes))
        return hRes;

    CComPtr<IPersistFile> pPF;
    hRes = pURL.QueryInterface(&pPF);
    if (SUCCEEDED(hRes))
    {
        // Save the shortcut via the IPersistFile::Save member function.
        hRes = pPF->Save(pszLinkFile, TRUE);
    }
    return hRes;
}

bool CCommonAppUtils::SetAccProperty(HWND hWnd, MSAAPROPID propid, const CString& text)
{
    ATL::CComPtr<IAccPropServices> pAccPropSvc;
    HRESULT hr = pAccPropSvc.CoCreateInstance(CLSID_AccPropServices, NULL, CLSCTX_SERVER);

    if (hr == S_OK && pAccPropSvc)
    {
        pAccPropSvc->SetHwndPropStr(hWnd, (DWORD)OBJID_CLIENT, CHILDID_SELF, propid, text);
        return true;
    }
    return false;
}

bool CCommonAppUtils::SetAccProperty(HWND hWnd, MSAAPROPID propid, long value)
{
    ATL::CComPtr<IAccPropServices> pAccPropSvc;
    HRESULT hr = pAccPropSvc.CoCreateInstance(CLSID_AccPropServices, NULL, CLSCTX_SERVER);

    if (hr == S_OK && pAccPropSvc)
    {
        VARIANT var;
        var.vt = VT_I4;
        var.intVal = value;
        pAccPropSvc->SetHwndProp(hWnd, (DWORD)OBJID_CLIENT, CHILDID_SELF, propid, var);
        return true;
    }
    return false;
}

TCHAR CCommonAppUtils::FindAcceleratorKey(CWnd * pWnd, UINT id)
{
    CString controlText;
    pWnd->GetDlgItem(id)->GetWindowText(controlText);
    int ampersandPos = controlText.Find('&');
    if (ampersandPos >= 0)
    {
        return controlText[ampersandPos+1];
    }
    ATLASSERT(false);
    return 0;
}

CString CCommonAppUtils::GetAbsoluteUrlFromRelativeUrl(const CString& root, const CString& url)
{
    // is the URL a relative one?
    if (url.Left(2).Compare(_T("^/")) == 0)
    {
        // URL is relative to the repository root
        CString url1 = root + url.Mid(1);
        TCHAR buf[INTERNET_MAX_URL_LENGTH];
        DWORD len = url.GetLength();
        if (UrlCanonicalize((LPCTSTR)url1, buf, &len, 0) == S_OK)
            return CString(buf, len);
        return url1;
    }
    else if (url[0] == '/')
    {
        // URL is relative to the server's hostname
        CString sHost;
        // find the server's hostname
        int schemepos = root.Find(_T("//"));
        if (schemepos >= 0)
        {
            sHost = root.Left(root.Find('/', schemepos+3));
            CString url1 = sHost + url;
            TCHAR buf[INTERNET_MAX_URL_LENGTH];
            DWORD len = url.GetLength();
            if (UrlCanonicalize((LPCTSTR)url, buf, &len, 0) == S_OK)
                return CString(buf, len);
            return url1;
        }
    }
    return url;
}

void CCommonAppUtils::ExtendControlOverHiddenControl(CWnd* parent, UINT controlToExtend, UINT hiddenControl)
{
    CRect controlToExtendRect;
    parent->GetDlgItem(controlToExtend)->GetWindowRect(controlToExtendRect);
    parent->ScreenToClient(controlToExtendRect);

    CRect hiddenControlRect;
    parent->GetDlgItem(hiddenControl)->GetWindowRect(hiddenControlRect);
    parent->ScreenToClient(hiddenControlRect);

    controlToExtendRect.right = hiddenControlRect.right;
    parent->GetDlgItem(controlToExtend)->MoveWindow(controlToExtendRect);
}

bool CCommonAppUtils::FileOpenSave(CString& path, int * filterindex, UINT title, UINT filterId, bool bOpen, HWND hwndOwner)
{
    OPENFILENAME ofn = {0};             // common dialog box structure
    TCHAR szFile[MAX_PATH] = {0};       // buffer for file name. Explorer can't handle paths longer than MAX_PATH.
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndOwner;
    _tcscpy_s(szFile, MAX_PATH, (LPCTSTR)path);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    CSelectFileFilter fileFilter;
    if (filterId)
    {
        fileFilter.Load(filterId);
        ofn.lpstrFilter = fileFilter;
    }
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    CString temp;
    if (title)
    {
        temp.LoadString(title);
        CStringUtils::RemoveAccelerators(temp);
    }
    ofn.lpstrTitle = temp;
    if (bOpen)
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
    else
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;


    // Display the Open dialog box.
    bool bRet = false;
    if (bOpen)
    {
        bRet = !!GetOpenFileName(&ofn);
    }
    else
    {
        bRet = !!GetSaveFileName(&ofn);
    }
    if (bRet)
    {
        path = CString(ofn.lpstrFile);
        if (filterindex)
            *filterindex = ofn.nFilterIndex;
        return true;
    }
    return false;
}

int MessageBox( HWND hWnd, UINT idText, UINT idCaption, UINT uType )
{
    return ::MessageBox(hWnd, CString(MAKEINTRESOURCE(idText)), CString(MAKEINTRESOURCE(idCaption)), uType);
}
