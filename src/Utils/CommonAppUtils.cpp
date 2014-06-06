// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2014 - TortoiseSVN

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
#include "CommonAppUtils.h"
#include "registry.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "ClipboardHelper.h"
#include <intshcut.h>
#include "CreateProcessHelper.h"
#include "SelectFileFilter.h"
#include "SmartHandle.h"
#include "PreserveChdir.h"
#include <WinInet.h>
#include <oleacc.h>
#include <initguid.h>
#include <regex>
#include <propkey.h>

extern CString sOrigCWD;
extern CString g_sGroupingUUID;

BOOL CCommonAppUtils::StartUnifiedDiffViewer(const CString& patchfile, const CString& title, BOOL bWait)
{
    CString viewer;
    CRegString v = CRegString(L"Software\\TortoiseSVN\\DiffViewer");
    viewer = v;
    if (viewer.IsEmpty() || (viewer.Left(1).Compare(L"#")==0))
    {
        // use TortoiseUDiff
        viewer = CPathUtils::GetAppDirectory();
        viewer += L"TortoiseUDiff.exe";
        // enquote the path to TortoiseUDiff
        viewer = L"\"" + viewer + L"\"";
        // add the params
        viewer = viewer + L" /patchfile:%1 /title:\"%title\"";
        if (!g_sGroupingUUID.IsEmpty())
        {
            viewer += L" /groupuuid:\"";
            viewer += g_sGroupingUUID;
            viewer += L"\"";
        }
    }
    if (viewer.Find(L"%1")>=0)
    {
        if (viewer.Find(L"\"%1\"") >= 0)
            viewer.Replace(L"%1", patchfile);
        else
            viewer.Replace(L"%1", L"\"" + patchfile + L"\"");
    }
    else
        viewer += L" \"" + patchfile + L"\"";
    if (viewer.Find(L"%title") >= 0)
    {
        viewer.Replace(L"%title", title);
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

    std::unique_ptr<TCHAR[]> buf(new TCHAR[len+1]);
    if (::ExpandEnvironmentStrings (s, buf.get(), len) == 0)
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
    CString normalizedFileName = L"\"" + fullFileName + L"\"";

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
        std::unique_ptr<TCHAR[]> cmdbuf(new TCHAR[buflen + 1]);
        if (FAILED(AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, verb, cmdbuf.get(), &buflen)))
        {
            documentClass = CRegString (extensionToUse + L"\\", L"", FALSE, HKEY_CLASSES_ROOT);

            CString key = documentClass + L"\\Shell\\" + verb + L"\\Command\\";
            application = CRegString (key, L"", FALSE, HKEY_CLASSES_ROOT);
        }
        else
        {
            application = cmdbuf.get();
        }

        // fallback to "open"

        if (application.IsEmpty())
        {
            buflen = 0;
            AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, L"open", NULL, &buflen);
            std::unique_ptr<TCHAR[]> cmdopenbuf (new TCHAR[buflen + 1]);
            if (FAILED(AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, L"open", cmdopenbuf.get(), &buflen)))
            {
                CString key = documentClass + L"\\Shell\\Open\\Command\\";
                application = CRegString (key, L"", FALSE, HKEY_CLASSES_ROOT);
            }
            else
            {
                application = cmdopenbuf.get();
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
        if (   (application.Find (L"%2") >= 0)
            || (application.Find (L"%*") >= 0))
        {
            // consumes extra parameters
            // -> probably a script execution
            // -> retry with "open" verb or ask user

            if (verb.CompareNoCase (L"open") == 0)
                application.Empty();
            else
                return GetAppForFile ( fileName
                                     , extension
                                     , L"open"
                                     , true);
        }
    }

    // exit here, if we were not successful

    if (application.IsEmpty())
        return application;

    // resolve parameters

    if (application.Find (L"%1") < 0)
        application += L" %1";

    if (application.Find(L"\"%1\"") >= 0)
        application.Replace(L"\"%1\"", L"%1");

    application.Replace (L"%1", normalizedFileName);

    return application;
}

BOOL CCommonAppUtils::StartTextViewer(CString file)
{
    CString viewer = GetAppForFile (file, L".txt", L"open", false);
    if (viewer.IsEmpty())
    {
        OPENASINFO oi = { 0 };
        oi.pcszFile = file;
        oi.oaifInFlags = OAIF_EXEC;
        return SHOpenWithDialog(NULL, &oi) == S_OK ? TRUE : FALSE;
    }
    return LaunchApplication (viewer, IDS_ERR_TEXTVIEWSTART, false)
        ? TRUE
        : FALSE;
}

bool CCommonAppUtils::LaunchApplication
    ( const CString& sCommandLine
    , UINT idErrMessageFormat
    , bool bWaitForStartup
    , bool bWaitForExit
    , HANDLE hWaitHandle)
{
    PROCESS_INFORMATION process;

    // make sure we get a writable copy of the command line

    size_t bufferLen = sCommandLine.GetLength()+1;
    std::unique_ptr<TCHAR[]> cleanCommandLine (new TCHAR[bufferLen]);
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
    {
        DWORD count = 1;
        HANDLE handles[2];
        handles[0] = process.hProcess;
        if (hWaitHandle)
        {
            count = 2;
            handles[1] = hWaitHandle;
        }
        WaitForMultipleObjects(count, handles, FALSE, INFINITE);
        if (hWaitHandle)
            CloseHandle(hWaitHandle);
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    return true;
}

bool CCommonAppUtils::RunTortoiseProc(const CString& sCommandLine)
{
    CString pathToExecutable = CPathUtils::GetAppDirectory()+L"TortoiseProc.exe";
    CString sCmd;
    sCmd.Format(L"\"%s\" %s", (LPCTSTR)pathToExecutable, (LPCTSTR)sCommandLine);
    if (AfxGetMainWnd()->GetSafeHwnd() && (sCommandLine.Find(L"/hwnd:")<0))
    {
        CString sCmdLine;
        sCmdLine.Format(L"%s /hwnd:%p", (LPCTSTR)sCommandLine, (void*)AfxGetMainWnd()->GetSafeHwnd());
        sCmd.Format(L"\"%s\" %s", (LPCTSTR)pathToExecutable, (LPCTSTR)sCmdLine);
    }
    if (!g_sGroupingUUID.IsEmpty())
    {
        sCmd += L" /groupuuid:\"";
        sCmd += g_sGroupingUUID;
        sCmd += L"\"";
    }

    return LaunchApplication(sCmd, NULL, false);
}


void CCommonAppUtils::ResizeAllListCtrlCols(CListCtrl * pListCtrl)
{
    int maxcol = ((CHeaderCtrl*)(pListCtrl->GetDlgItem(0)))->GetItemCount()-1;
    int nItemCount = pListCtrl->GetItemCount();
    TCHAR textbuf[MAX_PATH] = { 0 };
    CHeaderCtrl * pHdrCtrl = (CHeaderCtrl*)(pListCtrl->GetDlgItem(0));
    if (pHdrCtrl)
    {
        int imgWidth = 0;
        CImageList * pImgList = pListCtrl->GetImageList(LVSIL_SMALL);
        if ((pImgList)&&(pImgList->GetImageCount()))
        {
            IMAGEINFO imginfo;
            pImgList->GetImageInfo(0, &imginfo);
            imgWidth = (imginfo.rcImage.right - imginfo.rcImage.left) + 3;  // 3 pixels between icon and text
        }
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
                // add the image size
                if (col == 0)
                    linewidth += imgWidth;
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
            ::ReleaseDC(desktop, screen_dev);
        }
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
    if ((pszTargetfile == NULL) || (pszTargetfile[0] == 0) ||
        (pszLinkfile == NULL) || (pszLinkfile[0] == 0))
    {
        return E_INVALIDARG;
    }
    CComPtr<IShellLink> pShellLink;
    HRESULT hRes = pShellLink.CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER);
    if (FAILED(hRes))
        return hRes;

    hRes = pShellLink->SetPath(pszTargetfile);
    hRes = pShellLink->SetArguments(pszTargetargs);
    if (pszDescription[0] != 0)
    {
        hRes = pShellLink->SetDescription(pszDescription);
    }
    if (iShowmode > 0)
    {
        hRes = pShellLink->SetShowCmd(iShowmode);
    }
    if (pszCurdir[0] != 0)
    {
        hRes = pShellLink->SetWorkingDirectory(pszCurdir);
    }
    if (pszIconfile[0] != 0 && iIconindex >= 0)
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
    if (url.Left(2).Compare(L"^/") == 0)
    {
        // URL is relative to the repository root
        CString url1 = root + url.Mid(1);
        TCHAR buf[INTERNET_MAX_URL_LENGTH] = { 0 };
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
        int schemepos = root.Find(L"//");
        if (schemepos >= 0)
        {
            sHost = root.Left(root.Find('/', schemepos+3));
            CString url1 = sHost + url;
            TCHAR buf[INTERNET_MAX_URL_LENGTH] = { 0 };
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

bool CCommonAppUtils::FileOpenSave(CString& path, int * filterindex, UINT title, UINT filterId, bool bOpen, const CString& initialDir, HWND hwndOwner)
{
    HRESULT hr;
    // Create a new common save file dialog
    CComPtr<IFileDialog> pfd = NULL;

    hr = pfd.CoCreateInstance(bOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER);
    if (SUCCEEDED(hr))
    {
        // Set the dialog options
        DWORD dwOptions;
        if (SUCCEEDED(hr = pfd->GetOptions(&dwOptions)))
        {
            if (bOpen)
                hr = pfd->SetOptions(dwOptions | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
            else
            {
                hr = pfd->SetOptions(dwOptions | FOS_OVERWRITEPROMPT | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
                hr = pfd->SetFileName(CPathUtils::GetFileNameFromPath(path));
            }
        }

        // Set a title
        if (SUCCEEDED(hr) && title)
        {
            CString temp;
            temp.LoadString(title);
            CStringUtils::RemoveAccelerators(temp);
            pfd->SetTitle(temp);
        }
        if (filterId)
        {
            CSelectFileFilter fileFilter(filterId);

            hr = pfd->SetFileTypes(fileFilter.GetCount(), fileFilter);
        }

        // set the default folder
        CComPtr<IShellItem> psiFolder;
        if (!initialDir.IsEmpty())
        {
            hr = SHCreateItemFromParsingName(initialDir, NULL, IID_PPV_ARGS(&psiFolder));
            if (SUCCEEDED(hr))
                pfd->SetFolder(psiFolder);
            hr = S_OK;
        }

        // Show the save/open file dialog
        if (SUCCEEDED(hr) && SUCCEEDED(hr = pfd->Show(hwndOwner)))
        {
            // Get the selection from the user
            CComPtr<IShellItem> psiResult = NULL;
            hr = pfd->GetResult(&psiResult);
            if (SUCCEEDED(hr))
            {
                PWSTR pszPath = NULL;
                hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                if (SUCCEEDED(hr))
                {
                    path = CString(pszPath);
                    if (filterindex)
                    {
                        UINT fi = 0;
                        pfd->GetFileTypeIndex(&fi);
                        *filterindex = fi;
                    }
                    return true;
                }
            }
        }
    }
    else
    {
        OPENFILENAME ofn = {0};             // common dialog box structure
        TCHAR szFile[MAX_PATH] = {0};       // buffer for file name. Explorer can't handle paths longer than MAX_PATH.
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hwndOwner;
        wcscpy_s(szFile, (LPCTSTR)path);
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
        if (!initialDir.IsEmpty())
            ofn.lpstrInitialDir = initialDir;
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
        PreserveChdir preserveDir;
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
    }
    return false;
}

bool CCommonAppUtils::AddClipboardUrlToWindow( HWND hWnd )
{
    if (IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
        CClipboardHelper clipboard;
        clipboard.Open(hWnd);
        HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
        if (hglb)
        {
            LPCWSTR lpstr = (LPCWSTR)GlobalLock(hglb);
            CString sUrl = lpstr;
            GlobalUnlock(hglb);

            sUrl.Trim();
            CString sLowerCaseUrl = sUrl;
            sLowerCaseUrl.MakeLower();
            // check for illegal chars: they might be allowed in a regular url, e.g. '?',
            // but not in an url to an svn repository!
            if (sLowerCaseUrl.FindOneOf(L"\n\r?;=+$,<>#") >= 0)
                return false;

            if ((sLowerCaseUrl.Find(L"http://")==0) ||
                (sLowerCaseUrl.Find(L"https://")==0) ||
                (sLowerCaseUrl.Find(L"svn://")==0) ||
                (sLowerCaseUrl.Find(L"svn+ssh://")==0) ||
                (sLowerCaseUrl.Find(L"file://")==0))
            {
                ::SetWindowText(hWnd, (LPCTSTR)sUrl);
                return true;
            }
        }
    }
    return false;
}

void CCommonAppUtils::SetWindowTitle( HWND hWnd, const CString& urlorpath, const CString& dialogname )
{
#define MAX_PATH_LENGTH 80
    ASSERT(dialogname.GetLength() < MAX_PATH_LENGTH);
    WCHAR pathbuf[MAX_PATH] = {0};
    if (urlorpath.GetLength() >= MAX_PATH)
    {
        std::wstring str = (LPCTSTR)urlorpath;
        std::wregex rx(L"^(\\w+:|(?:\\\\|/+))((?:\\\\|/+)[^\\\\/]+(?:\\\\|/)[^\\\\/]+(?:\\\\|/)).*((?:\\\\|/)[^\\\\/]+(?:\\\\|/)[^\\\\/]+)$");
        std::wstring replacement = L"$1$2...$3";
        std::wstring str2 = std::regex_replace(str, rx, replacement);
        if (str2.size() >= MAX_PATH)
            str2 = str2.substr(0, MAX_PATH-2);
        PathCompactPathEx(pathbuf, str2.c_str(), MAX_PATH_LENGTH-dialogname.GetLength(), 0);
    }
    else
        PathCompactPathEx(pathbuf, urlorpath, MAX_PATH_LENGTH-dialogname.GetLength(), 0);
    CString title;
    switch (DWORD(CRegStdDWORD(L"Software\\TortoiseSVN\\DialogTitles", 0)))
    {
    case 0: // url/path - dialogname - appname
        title  = pathbuf;
        title += L" - " + dialogname + L" - " + CString(MAKEINTRESOURCE(IDS_APPNAME));
        break;
    case 1: // dialogname - url/path - appname
        title = dialogname + L" - " + pathbuf + L" - " + CString(MAKEINTRESOURCE(IDS_APPNAME));
        break;
    }
    SetWindowText(hWnd, title);
}

void CCommonAppUtils::MarkWindowAsUnpinnable( HWND hWnd )
{
    typedef HRESULT (WINAPI *SHGPSFW) (HWND hwnd,REFIID riid,void** ppv);

    CAutoLibrary hShell = AtlLoadSystemLibraryUsingFullPath(L"Shell32.dll");

    if (!hShell.IsValid())
        return;

    SHGPSFW pfnSHGPSFW = (SHGPSFW)::GetProcAddress(hShell, "SHGetPropertyStoreForWindow");
    if (pfnSHGPSFW == 0)
         return;

    IPropertyStore *pps = 0;
    HRESULT hr = pfnSHGPSFW(hWnd, IID_PPV_ARGS(&pps));
    if (SUCCEEDED(hr))
    {
        PROPVARIANT var;
        var.vt = VT_BOOL;
        var.boolVal = VARIANT_TRUE;
        hr = pps->SetValue(PKEY_AppUserModel_PreventPinning, var);
        pps->Release();
    }
}

HRESULT CCommonAppUtils::EnableAutoComplete(HWND hWndEdit, LPWSTR szCurrentWorkingDirectory, AUTOCOMPLETELISTOPTIONS acloOptions, AUTOCOMPLETEOPTIONS acoOptions, REFCLSID clsid)
{
    IAutoComplete *pac;
    HRESULT hr = CoCreateInstance(CLSID_AutoComplete,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pac));
    if (FAILED(hr))
    {
        return hr;
    }

    IUnknown *punkSource;
    hr = CoCreateInstance(clsid,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&punkSource));
    if (FAILED(hr))
    {
        pac->Release();
        return hr;
    }

    if ((acloOptions != ACLO_NONE) || (szCurrentWorkingDirectory != NULL))
    {
        IACList2 *pal2;
        hr = punkSource->QueryInterface(IID_PPV_ARGS(&pal2));
        if (SUCCEEDED(hr))
        {
            if (acloOptions != ACLO_NONE)
            {
                hr = pal2->SetOptions(acloOptions);
            }

            if (szCurrentWorkingDirectory != NULL)
            {
                ICurrentWorkingDirectory *pcwd;
                hr = pal2->QueryInterface(IID_PPV_ARGS(&pcwd));
                if (SUCCEEDED(hr))
                {
                    hr = pcwd->SetDirectory(szCurrentWorkingDirectory);
                    pcwd->Release();
                }
            }

            pal2->Release();
        }
    }

    hr = pac->Init(hWndEdit, punkSource, NULL, NULL);

    if (acoOptions != ACO_NONE)
    {
        IAutoComplete2 *pac2;
        hr = pac->QueryInterface(IID_PPV_ARGS(&pac2));
        if (SUCCEEDED(hr))
        {
            hr = pac2->SetOptions(acoOptions);
            pac2->Release();
        }
    }

    punkSource->Release();
    pac->Release();
    return hr;
}