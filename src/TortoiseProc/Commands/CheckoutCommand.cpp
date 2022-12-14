// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2012, 2014, 2020-2022 - TortoiseSVN

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
#include "CheckoutCommand.h"

#include "CheckoutDlg.h"
#include "SVNProgressDlg.h"
#include "StringUtils.h"
#include "TortoiseProc.h"

bool CheckoutCommand::Execute()
{
    bool bRet = false;
    // Get the directory supplied in the command line. If there isn't
    // one then we should use first the default checkout path
    // specified in the settings dialog, and fall back to the current
    // working directory instead if no such path was specified.
    CTSVNPath  checkoutDirectory;
    CRegString regDefCheckoutPath(L"Software\\TortoiseSVN\\DefaultCheckoutPath");
    if (cmdLinePath.IsEmpty())
    {
        if (CString(regDefCheckoutPath).IsEmpty())
        {
            checkoutDirectory.SetFromWin(sOrigCwd, true);
            DWORD len    = ::GetTempPath(0, nullptr);
            auto  szPath = std::make_unique<wchar_t[]>(len);
            ::GetTempPath(len, szPath.get());
            if (_wcsnicmp(checkoutDirectory.GetWinPath(), szPath.get(), len - 2 /* \\ and \0 */) == 0)
            {
                // if the current directory is set to a temp directory,
                // we don't use that but leave it empty instead.
                checkoutDirectory.Reset();
            }
        }
        else
        {
            checkoutDirectory.SetFromWin(CString(regDefCheckoutPath));
        }
    }
    else
    {
        checkoutDirectory = cmdLinePath;
    }

    CCheckoutDlg dlg;
    dlg.m_urLs.LoadFromAsteriskSeparatedString(parser.GetVal(L"url"));
    if (dlg.m_urLs.GetCount() == 0)
    {
        SVN svn;
        if (svn.IsRepository(cmdLinePath))
        {
            CString url;
            // The path points to a local repository.
            // Add 'file:///' so the repository browser recognizes
            // it as an URL to the local repository.
            if (cmdLinePath.GetWinPathString().GetAt(0) == '\\') // starts with '\' means an UNC path
            {
                CString p = cmdLinePath.GetWinPathString();
                p.TrimLeft('\\');
                url = L"file://" + p;
            }
            else
                url = L"file:///" + cmdLinePath.GetWinPathString();
            url.Replace('\\', '/');
            dlg.m_urLs.AddPath(CTSVNPath(url));
            checkoutDirectory.AppendRawString(L"wc");
        }
    }
    dlg.m_strCheckoutDirectory = checkoutDirectory.GetWinPathString();
    // if there is no url specified on the command line, check if there's one
    // specified in the settings dialog to use as the default and use that
    CRegString regDefCheckoutUrl(L"Software\\TortoiseSVN\\DefaultCheckoutUrl");
    if (!CString(regDefCheckoutUrl).IsEmpty())
    {
        // if the URL specified is a child of the default URL, we also
        // adjust the default checkout path
        // e.g.
        // Url specified on command line: http://server.com/repos/project/trunk/folder
        // Url specified as default     : http://server.com/repos/project/trunk
        // checkout path specified      : c:\work\project
        // -->
        // checkout path adjusted       : c:\work\project\folder
        CTSVNPath clUrl  = dlg.m_urLs.GetCommonDirectory();
        CTSVNPath defUrl = CTSVNPath(CString(regDefCheckoutUrl));
        if (defUrl.IsAncestorOf(clUrl))
        {
            // the default url is the parent of the specified url
            if (CTSVNPath::CheckChild(CTSVNPath(CString(regDefCheckoutPath)), CTSVNPath(dlg.m_strCheckoutDirectory)))
            {
                dlg.m_strCheckoutDirectory = CString(regDefCheckoutPath) + clUrl.GetWinPathString().Mid(defUrl.GetWinPathString().GetLength());
                dlg.m_strCheckoutDirectory.Replace(L"\\\\", L"\\");
            }
        }
        if (dlg.m_urLs.GetCount() == 0)
            dlg.m_urLs.AddPath(defUrl);
    }

    for (int i = 0; i < dlg.m_urLs.GetCount(); ++i)
    {
        CString pathString = dlg.m_urLs[i].GetWinPathString();
        if (pathString.Left(5).Compare(L"tsvn:") == 0)
        {
            pathString = pathString.Mid(5);
            if (pathString.Find('?') >= 0)
            {
                dlg.m_revision = SVNRev(pathString.Mid(pathString.Find('?') + 1));
                pathString   = pathString.Left(pathString.Find('?'));
            }
        }

        dlg.m_urLs[i].SetFromWin(pathString);
    }
    if (parser.HasKey(L"revision"))
    {
        SVNRev rev   = SVNRev(parser.GetVal(L"revision"));
        dlg.m_revision = rev;
    }
    dlg.m_blockPathAdjustments = parser.HasKey(L"blockpathadjustments");
    if (dlg.DoModal() == IDOK)
    {
        checkoutDirectory.SetFromWin(dlg.m_strCheckoutDirectory, true);

        CSVNProgressDlg progDlg;
        theApp.m_pMainWnd = &progDlg;

        bool useStandardCheckout = dlg.m_standardCheckout || ((dlg.m_urLs.GetCount() > 1) && dlg.m_bIndependentWCs);

        progDlg.SetCommand(useStandardCheckout
                               ? !dlg.m_checkoutDepths.empty()
                                     ? CSVNProgressDlg::SVNProgress_SparseCheckout
                                     : CSVNProgressDlg::SVNProgress_Checkout
                           : dlg.m_parentExists && (dlg.m_urLs.GetCount() == 1)
                               ? CSVNProgressDlg::SVNProgress_Update
                               : CSVNProgressDlg::SVNProgress_SingleFileCheckout);

        if (!dlg.m_checkoutDepths.empty())
            progDlg.SetPathDepths(dlg.m_checkoutDepths);
        progDlg.SetAutoClose(parser);
        DWORD options = dlg.m_bStorePristines ? ProgOptStorePristines : ProgOptNone;
        if (dlg.m_bNoExternals)
            options |= ProgOptIgnoreExternals;
        progDlg.SetOptions(options);
        progDlg.SetPathList(CTSVNPathList(checkoutDirectory));
        progDlg.SetUrl(dlg.m_urLs.CreateAsteriskSeparatedString());
        progDlg.SetRevision(dlg.m_revision);
        progDlg.SetDepth(dlg.m_depth);
        progDlg.DoModal();
        bRet = !progDlg.DidErrorsOccur();
        if (parser.HasVal(L"outfile"))
        {
            CString sText;
            sText = checkoutDirectory.GetWinPathString();
            sText += L"\n";
            sText += dlg.m_urLs.CreateAsteriskSeparatedString();
            sText += L"\n";
            sText += dlg.m_revision.ToString();
            CStringUtils::WriteStringToTextFile(parser.GetVal(L"outfile"), static_cast<LPCWSTR>(sText), true);
        }
    }
    return bRet;
}
