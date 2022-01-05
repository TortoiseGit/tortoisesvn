// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2010-2018, 2020-2022 - TortoiseSVN

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
#include "DPIAware.h"
#include "LoadIconEx.h"
#include "IconMenu.h"
#include <oleacc.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <WinInet.h>
#include <regex>
#include <propkey.h>

#include "UnicodeUtils.h"

extern CString sOrigCwd;
extern CString g_sGroupingUuid;

BOOL           CCommonAppUtils::StartUnifiedDiffViewer(const CString& patchfile, const CString& title, BOOL bWait)
{
    CRegString v      = CRegString(L"Software\\TortoiseSVN\\DiffViewer");
    CString    viewer = v;
    if (viewer.IsEmpty() || (viewer.Left(1).Compare(L"#") == 0))
    {
        // use TortoiseUDiff
        viewer = CPathUtils::GetAppDirectory();
        viewer += L"TortoiseUDiff.exe";
        // enquote the path to TortoiseUDiff
        viewer = L"\"" + viewer + L"\"";
        // add the params
        viewer = viewer + L" /patchfile:%1 /title:\"%title\"";
        if (!g_sGroupingUuid.IsEmpty())
        {
            viewer += L" /groupuuid:\"";
            viewer += g_sGroupingUuid;
            viewer += L"\"";
        }
    }
    if (viewer.Find(L"%1") >= 0)
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

    if (!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
    {
        return FALSE;
    }
    return TRUE;
}

CString CCommonAppUtils::ExpandEnvironmentStrings(const CString& s)
{
    DWORD len = ::ExpandEnvironmentStrings(s, nullptr, 0);
    if (len == 0)
        return s;

    auto buf = std::make_unique<wchar_t[]>(len + 1LL);
    if (::ExpandEnvironmentStrings(s, buf.get(), len) == 0)
        return s;

    return buf.get();
}

CString CCommonAppUtils::GetAppForFile(const CString& fileName,
                                       const CString& extension,
                                       const CString& verb,
                                       bool           applySecurityHeuristics)
{
    CString application;

    // normalize file path

    CString fullFileName       = ExpandEnvironmentStrings(fileName);
    CString normalizedFileName = L"\"" + fullFileName + L"\"";

    // registry lookup

    CString extensionToUse     = extension.IsEmpty()
                                     ? CPathUtils::GetFileExtFromPath(fullFileName)
                                     : extension;

    if (!extensionToUse.IsEmpty())
    {
        // lookup by verb

        CString documentClass;
        DWORD   bufLen = 0;
        AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, verb, nullptr, &bufLen);
        auto cmdBuf = std::make_unique<wchar_t[]>(bufLen + 1LL);
        if (FAILED(AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, verb, cmdBuf.get(), &bufLen)))
        {
            documentClass = CRegString(extensionToUse + L"\\", L"", FALSE, HKEY_CLASSES_ROOT);

            CString key   = documentClass + L"\\Shell\\" + verb + L"\\Command\\";
            application   = CRegString(key, L"", FALSE, HKEY_CLASSES_ROOT);
        }
        else
        {
            application = cmdBuf.get();
        }

        // fallback to "open"

        if (application.IsEmpty())
        {
            bufLen = 0;
            AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, L"open", nullptr, &bufLen);
            auto cmdOpenBuf = std::make_unique<wchar_t[]>(bufLen + 1LL);
            if (FAILED(AssocQueryString(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCSTR_COMMAND, extensionToUse, L"open", cmdOpenBuf.get(), &bufLen)))
            {
                CString key = documentClass + L"\\Shell\\Open\\Command\\";
                application = CRegString(key, L"", FALSE, HKEY_CLASSES_ROOT);
            }
            else
            {
                application = cmdOpenBuf.get();
            }
        }
    }

    // normalize application path

    application = ExpandEnvironmentStrings(application);

    // security heuristics:
    // some scripting languages (e.g. python) will execute the document
    // instead of open it. Try to identify these cases.

    if (applySecurityHeuristics)
    {
        if ((application.Find(L"%2") >= 0) || (application.Find(L"%*") >= 0))
        {
            // consumes extra parameters
            // -> probably a script execution
            // -> retry with "open" verb or ask user

            if (verb.CompareNoCase(L"open") == 0)
                application.Empty();
            else
                return GetAppForFile(fileName, extension, L"open", true);
        }
    }

    // exit here, if we were not successful

    if (application.IsEmpty())
        return application;

    // resolve parameters

    if (application.Find(L"%1") < 0)
        application += L" %1";

    if (application.Find(L"\"%1\"") >= 0)
        application.Replace(L"\"%1\"", L"%1");

    application.Replace(L"%1", normalizedFileName);

    return application;
}

BOOL CCommonAppUtils::StartTextViewer(CString file)
{
    CString viewer = GetAppForFile(file, L".txt", L"open", false);
    if (viewer.IsEmpty())
    {
        OPENASINFO oi  = {nullptr};
        oi.pcszFile    = file;
        oi.oaifInFlags = OAIF_EXEC;
        return SHOpenWithDialog(nullptr, &oi) == S_OK ? TRUE : FALSE;
    }
    return LaunchApplication(viewer, IDS_ERR_TEXTVIEWSTART, false)
               ? TRUE
               : FALSE;
}

bool CCommonAppUtils::LaunchApplication(const CString& sCommandLine, UINT idErrMessageFormat, bool bWaitForStartup, bool bWaitForExit, HANDLE hWaitHandle)
{
    PROCESS_INFORMATION process;

    if (!CCreateProcessHelper::CreateProcess(nullptr, sCommandLine, sOrigCwd, &process))
    {
        if (idErrMessageFormat != 0)
        {
            CFormatMessageWrapper errorDetails;
            CString               msg;
            msg.Format(idErrMessageFormat, static_cast<LPCWSTR>(errorDetails));
            CString title;
            title.LoadString(IDS_APPNAME);
            MessageBox(nullptr, msg, title, MB_OK | MB_ICONINFORMATION);
        }
        return false;
    }
    AllowSetForegroundWindow(process.dwProcessId);

    if (bWaitForStartup)
        WaitForInputIdle(process.hProcess, 10000);

    if (bWaitForExit)
    {
        DWORD  count = 1;
        HANDLE handles[2];
        handles[0] = process.hProcess;
        if (hWaitHandle)
        {
            count      = 2;
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
    CString pathToExecutable = CPathUtils::GetAppDirectory() + L"TortoiseProc.exe";
    CString sCmd;
    sCmd.Format(L"\"%s\" %s", static_cast<LPCWSTR>(pathToExecutable), static_cast<LPCWSTR>(sCommandLine));
    if (AfxGetMainWnd()->GetSafeHwnd() && (sCommandLine.Find(L"/hwnd:") < 0))
        sCmd.AppendFormat(L" /hwnd:%p", static_cast<void*>(AfxGetMainWnd()->GetSafeHwnd()));
    if (!g_sGroupingUuid.IsEmpty())
    {
        sCmd += L" /groupuuid:\"";
        sCmd += g_sGroupingUuid;
        sCmd += L"\"";
    }

    return LaunchApplication(sCmd, NULL, false);
}

void CCommonAppUtils::ResizeAllListCtrlCols(CListCtrl* pListCtrl)
{
    CHeaderCtrl* pHdrCtrl          = pListCtrl->GetHeaderCtrl();
    int          nItemCount        = pListCtrl->GetItemCount();
    wchar_t      textBuf[MAX_PATH] = {0};
    if (pHdrCtrl)
    {
        int         maxCol   = pHdrCtrl->GetItemCount() - 1;
        int         imgWidth = 0;
        CImageList* pImgList = pListCtrl->GetImageList(LVSIL_SMALL);
        if ((pImgList) && (pImgList->GetImageCount()))
        {
            IMAGEINFO imgInfo;
            pImgList->GetImageInfo(0, &imgInfo);
            imgWidth = (imgInfo.rcImage.right - imgInfo.rcImage.left) + 3; // 3 pixels between icon and text
        }
        for (int col = 0; col <= maxCol; col++)
        {
            HDITEM hdi     = {0};
            hdi.mask       = HDI_TEXT;
            hdi.pszText    = textBuf;
            hdi.cchTextMax = _countof(textBuf);
            pHdrCtrl->GetItem(col, &hdi);
            int cx = pListCtrl->GetStringWidth(hdi.pszText) + 20; // 20 pixels for col separator and margin

            for (int index = 0; index < nItemCount; ++index)
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

bool CCommonAppUtils::SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID)
{
    auto size = CDPIAware::Instance().Scale(hListCtrl, 128);
    return SetListCtrlBackgroundImage(hListCtrl, nID, size, size);
}

bool CCommonAppUtils::SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width /* = 128 */, int height /* = 128 */)
{
    // create a bitmap from the icon
    CAutoIcon hIcon = ::LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(nID), width, height);
    if (!hIcon)
        return false;

    IconBitmapUtils iutils;
    auto            bmp = iutils.IconToBitmapPARGB32(hIcon, width, height);

    LVBKIMAGE       lv;
    lv.ulFlags        = LVBKIF_TYPE_WATERMARK | LVBKIF_FLAG_ALPHABLEND;
    lv.hbm            = bmp;
    lv.xOffsetPercent = 100;
    lv.yOffsetPercent = 100;
    ListView_SetBkImage(hListCtrl, &lv);
    return true;
}

HRESULT CCommonAppUtils::CreateShortCut(LPCWSTR pszTargetfile, LPCWSTR pszTargetargs,
                                        LPCWSTR pszLinkfile, LPCWSTR pszDescription,
                                        int iShowmode, LPCWSTR pszCurdir,
                                        LPCWSTR pszIconfile, int iIconIndex)
{
    if ((pszTargetfile == nullptr) || (pszTargetfile[0] == 0) ||
        (pszLinkfile == nullptr) || (pszLinkfile[0] == 0))
    {
        return E_INVALIDARG;
    }
    CComPtr<IShellLink> pShellLink;
    HRESULT             hRes = pShellLink.CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
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
    if (pszIconfile[0] != 0 && iIconIndex >= 0)
    {
        hRes = pShellLink->SetIconLocation(pszIconfile, iIconIndex);
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

HRESULT CCommonAppUtils::CreateShortcutToURL(LPCWSTR pszURL, LPCWSTR pszLinkFile)
{
    CComPtr<IUniformResourceLocator> pURL;
    // Create an IUniformResourceLocator object
    HRESULT                          hRes = pURL.CoCreateInstance(CLSID_InternetShortcut, nullptr, CLSCTX_INPROC_SERVER);
    if (FAILED(hRes))
        return hRes;

    hRes = pURL->SetURL(pszURL, 0);
    if (FAILED(hRes))
        return hRes;

    CComPtr<IPersistFile> pPf;
    hRes = pURL.QueryInterface(&pPf);
    if (SUCCEEDED(hRes))
    {
        // Save the shortcut via the IPersistFile::Save member function.
        hRes = pPf->Save(pszLinkFile, TRUE);
    }
    return hRes;
}

bool CCommonAppUtils::SetAccProperty(HWND hWnd, const MSAAPROPID& propId, const CString& text)
{
    ATL::CComPtr<IAccPropServices> pAccPropSvc;
    HRESULT                        hr = pAccPropSvc.CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_SERVER);

    if (hr == S_OK && pAccPropSvc)
    {
        pAccPropSvc->SetHwndPropStr(hWnd, static_cast<DWORD>(OBJID_CLIENT), CHILDID_SELF, propId, text);
        return true;
    }
    return false;
}

bool CCommonAppUtils::SetAccProperty(HWND hWnd, const MSAAPROPID& propId, long value)
{
    ATL::CComPtr<IAccPropServices> pAccPropSvc;
    HRESULT                        hr = pAccPropSvc.CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_SERVER);

    if (hr == S_OK && pAccPropSvc)
    {
        VARIANT var;
        var.vt     = VT_I4;
        var.intVal = value;
        pAccPropSvc->SetHwndProp(hWnd, static_cast<DWORD>(OBJID_CLIENT), CHILDID_SELF, propId, var);
        return true;
    }
    return false;
}

wchar_t CCommonAppUtils::FindAcceleratorKey(CWnd* pWnd, UINT id)
{
    CString controlText;
    pWnd->GetDlgItem(id)->GetWindowText(controlText);
    int ampersandPos = controlText.Find('&');
    if (ampersandPos >= 0)
    {
        return controlText[ampersandPos + 1];
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
        CString url1                         = root + url.Mid(1);
        wchar_t buf[INTERNET_MAX_URL_LENGTH] = {0};
        DWORD   len                          = url.GetLength();
        if (UrlCanonicalize(static_cast<LPCWSTR>(url1), buf, &len, 0) == S_OK)
            return CString(buf, len);
        return url1;
    }
    else if (url[0] == '/')
    {
        // find the server's hostname
        int schemePos = root.Find(L"//");
        if (schemePos >= 0)
        {
            CString sHost                        = root.Left(root.Find('/', schemePos + 3));
            CString url1                         = sHost + url;
            wchar_t buf[INTERNET_MAX_URL_LENGTH] = {0};
            DWORD   len                          = url.GetLength();
            if (UrlCanonicalize(static_cast<LPCWSTR>(url), buf, &len, 0) == S_OK)
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

bool CCommonAppUtils::FileOpenSave(CString& path, int* filterindex, UINT title, UINT filterId, bool bOpen, const CString& initialDir, HWND hwndOwner)
{
    // Create a new common save file dialog
    CComPtr<IFileDialog> pfd = nullptr;

    HRESULT              hr  = pfd.CoCreateInstance(bOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER);
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
            hr = SHCreateItemFromParsingName(initialDir, nullptr, IID_PPV_ARGS(&psiFolder));
            if (SUCCEEDED(hr))
                pfd->SetFolder(psiFolder);
            hr = S_OK;
        }

        // Show the save/open file dialog
        if (SUCCEEDED(hr) && SUCCEEDED(hr = pfd->Show(hwndOwner)))
        {
            // Get the selection from the user
            CComPtr<IShellItem> psiResult = nullptr;
            hr                            = pfd->GetResult(&psiResult);
            if (SUCCEEDED(hr))
            {
                PWSTR pszPath = nullptr;
                hr            = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
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

    return false;
}

bool CCommonAppUtils::AddClipboardUrlToWindow(HWND hWnd)
{
    if (IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
        CClipboardHelper clipboard;
        clipboard.Open(hWnd);
        HGLOBAL hGlb = GetClipboardData(CF_UNICODETEXT);
        if (hGlb)
        {
            LPCWSTR lpStr = static_cast<LPCWSTR>(GlobalLock(hGlb));
            CString sUrl  = lpStr;
            GlobalUnlock(hGlb);

            sUrl.Trim();
            CString sLowerCaseUrl = sUrl;
            sLowerCaseUrl.MakeLower();
            // check for illegal chars: they might be allowed in a regular url, e.g. '?',
            // but not in an url to an svn repository!
            if (sLowerCaseUrl.FindOneOf(L"\n\r?;=+$,<>#") >= 0)
                return false;

            if ((sLowerCaseUrl.Find(L"http://") == 0) ||
                (sLowerCaseUrl.Find(L"https://") == 0) ||
                (sLowerCaseUrl.Find(L"svn://") == 0) ||
                (sLowerCaseUrl.Find(L"svn+ssh://") == 0) ||
                (sLowerCaseUrl.Find(L"file://") == 0))
            {
                ::SetWindowText(hWnd, static_cast<LPCWSTR>(sUrl));
                return true;
            }
        }
    }
    return false;
}

CString CCommonAppUtils::FormatWindowTitle(const CString& urlOrPath, const CString& dialogName)
{
#define MAX_PATH_LENGTH 80
    ASSERT(dialogName.GetLength() < MAX_PATH_LENGTH);
    WCHAR pathbuf[MAX_PATH] = {0};
    if (urlOrPath.GetLength() >= MAX_PATH)
    {
        std::wstring str = static_cast<LPCWSTR>(urlOrPath);
        std::wregex  rx(L"^(\\w+:|(?:\\\\|/+))((?:\\\\|/+)[^\\\\/]+(?:\\\\|/)[^\\\\/]+(?:\\\\|/)).*((?:\\\\|/)[^\\\\/]+(?:\\\\|/)[^\\\\/]+)$");
        std::wstring replacement = L"$1$2...$3";
        std::wstring str2        = std::regex_replace(str, rx, replacement);
        if (str2.size() >= MAX_PATH)
            str2 = str2.substr(0, MAX_PATH - 2);
        PathCompactPathEx(pathbuf, str2.c_str(), MAX_PATH_LENGTH - dialogName.GetLength(), 0);
    }
    else
        PathCompactPathEx(pathbuf, urlOrPath, MAX_PATH_LENGTH - dialogName.GetLength(), 0);
    CString title;
    switch (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\DialogTitles", 0)))
    {
        case 0: // url/path - dialogName - appname
            title = pathbuf;
            title += L" - " + dialogName + L" - " + CString(MAKEINTRESOURCE(IDS_APPNAME));
            break;
        case 1: // dialogName - url/path - appname
            title = dialogName + L" - " + pathbuf + L" - " + CString(MAKEINTRESOURCE(IDS_APPNAME));
            break;
    }

    return title;
}

void CCommonAppUtils::SetWindowTitle(HWND hWnd, const CString& urlOrPath, const CString& dialogName)
{
    CString title(FormatWindowTitle(urlOrPath, dialogName));
    SetWindowText(hWnd, title);
}

void CCommonAppUtils::MarkWindowAsUnpinnable(HWND hWnd)
{
    using SHGPSFW       = HRESULT(WINAPI*)(HWND hwnd, REFIID riid, void** ppv);

    CAutoLibrary hShell = AtlLoadSystemLibraryUsingFullPath(L"Shell32.dll");

    if (!hShell.IsValid())
        return;

    // ReSharper disable once CppInconsistentNaming
    SHGPSFW pfnSHGPSFW = reinterpret_cast<SHGPSFW>(::GetProcAddress(hShell, "SHGetPropertyStoreForWindow"));
    if (pfnSHGPSFW == nullptr)
        return;

    IPropertyStore* pps = nullptr;
    HRESULT         hr  = pfnSHGPSFW(hWnd, IID_PPV_ARGS(&pps));
    if (SUCCEEDED(hr))
    {
        PROPVARIANT var;
        var.vt      = VT_BOOL;
        var.boolVal = VARIANT_TRUE;
        pps->SetValue(PKEY_AppUserModel_PreventPinning, var);
        pps->Release();
    }
}

HRESULT CCommonAppUtils::EnableAutoComplete(HWND hWndEdit, LPWSTR szCurrentWorkingDirectory, AUTOCOMPLETELISTOPTIONS acloOptions, AUTOCOMPLETEOPTIONS acoOptions, REFCLSID clsid)
{
    CComPtr<IAutoComplete> pac;
    HRESULT                hr = pac.CoCreateInstance(CLSID_AutoComplete,
                                      nullptr,
                                      CLSCTX_INPROC_SERVER);
    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<IUnknown> punkSource;
    hr = punkSource.CoCreateInstance(clsid,
                                     nullptr,
                                     CLSCTX_INPROC_SERVER);
    if (FAILED(hr))
    {
        return hr;
    }

    if ((acloOptions != ACLO_NONE) || (szCurrentWorkingDirectory != nullptr))
    {
        CComPtr<IACList2> pal2;
        hr = punkSource.QueryInterface(&pal2);
        if (SUCCEEDED(hr))
        {
            if (acloOptions != ACLO_NONE)
            {
                hr = pal2->SetOptions(acloOptions);
            }

            if (szCurrentWorkingDirectory != nullptr)
            {
                CComPtr<ICurrentWorkingDirectory> pcwd;
                hr = pal2.QueryInterface(&pcwd);
                if (SUCCEEDED(hr))
                {
                    hr = pcwd->SetDirectory(szCurrentWorkingDirectory);
                }
            }
        }
    }

    hr = pac->Init(hWndEdit, punkSource, nullptr, nullptr);

    if (acoOptions != ACO_NONE)
    {
        CComPtr<IAutoComplete2> pac2;
        hr = pac.QueryInterface(&pac2);
        if (SUCCEEDED(hr))
        {
            hr = pac2->SetOptions(acoOptions);
        }
    }

    return hr;
}

HICON CCommonAppUtils::LoadIconEx(UINT resourceId, UINT cx, UINT cy)
{
    return ::LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(resourceId),
                        cx, cy);
}

const char* CCommonAppUtils::GetResourceData(const wchar_t* resName, int id, DWORD& resLen)
{
    resLen         = 0;
    auto hResource = FindResource(nullptr, MAKEINTRESOURCE(id), resName);
    if (!hResource)
        return nullptr;
    auto hResourceLoaded = LoadResource(nullptr, hResource);
    if (!hResourceLoaded)
        return nullptr;
    auto lpResLock = static_cast<const char*>(LockResource(hResourceLoaded));
    resLen         = SizeofResource(nullptr, hResource);
    return lpResLock;
}

bool CCommonAppUtils::StartHtmlHelp(DWORD_PTR id)
{
    static std::map<DWORD_PTR, std::wstring> idMap;

#if defined(IDR_HELPCONTEXT) && defined(IDR_HELPALIAS) && defined(IDS_APPNAME) && defined(IDS_HELPURLPART)
    if (idMap.empty())
    {
        std::map<std::string, DWORD_PTR> contextMap;
        DWORD                            resSize = 0;
        const char*                      resData = GetResourceData(L"help", IDR_HELPCONTEXT, resSize);
        if (resData != nullptr)
        {
            auto                     resString = std::string(resData, resSize);
            std::vector<std::string> lines;
            stringtok(lines, resString, true, "\r\n");
            for (const auto& line : lines)
            {
                if (line.starts_with("//"))
                    continue;
                if (line.empty())
                    continue;
                std::vector<std::string> lineParts;
                stringtok(lineParts, line, true, " ");
                if (lineParts.size() == 3)
                {
                    contextMap[lineParts[1]] = std::stoi(lineParts[2], nullptr, 0);
                }
            }
        }
        std::map<std::string, std::string> aliasMap;
        resSize = 0;
        resData = GetResourceData(L"help", IDR_HELPALIAS, resSize);
        if (resData != nullptr)
        {
            auto                     resString = std::string(resData, resSize);
            std::vector<std::string> lines;
            stringtok(lines, resString, true, "\r\n");
            for (const auto& line : lines)
            {
                if (line.empty())
                    continue;
                std::vector<std::string> lineParts;
                stringtok(lineParts, line, true, "=");
                if (lineParts.size() == 2)
                {
                    aliasMap[lineParts[0]] = lineParts[1];
                }
            }
        }
        for (const auto& [textId, link] : aliasMap)
        {
            if (contextMap.contains(textId))
            {
                auto numId   = contextMap.find(textId)->second;
                idMap[numId] = CUnicodeUtils::StdGetUnicode(link);
            }
        }
    }

    std::wstring baseUrl = L"https://tortoisesvn.net/docs/release/";
    CString      appName(MAKEINTRESOURCE(IDS_APPNAME));
    CString      langPart(MAKEINTRESOURCE(IDS_HELPURLPART));
    if (langPart.IsEmpty())
        langPart = L"en";

    CString page = L"index.html";
    if (idMap.contains(id))
    {
        page = idMap[id].c_str();
        page.Replace(L"@", L"%40");
    }

    baseUrl += appName;
    baseUrl += L"_";
    baseUrl += langPart;
    baseUrl += L"/";
    baseUrl += page;

    ShellExecute(nullptr, L"open", baseUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return true;
#else
    CWinApp* pApp = AfxGetApp();
    ASSERT_VALID(pApp);

    CString helpFile{pApp->m_pszHelpFilePath};

    if (helpFile.IsEmpty() || !PathFileExists(helpFile))
        return false;

    wchar_t windir[MAX_PATH] = {0};
    if (!GetWindowsDirectory(windir, _countof(windir)))
        return false;

   	CString mapID;
    if (id)
        mapID.Format(L" -mapid %Iu", id);

    CString cmd;
    cmd.Format(L"%s\\HH.exe%s \"%s\"", windir, static_cast<LPCWSTR>(mapID), static_cast<LPCWSTR>(helpFile));

    return CCreateProcessHelper::CreateProcessDetached(nullptr, cmd);
#endif
}
