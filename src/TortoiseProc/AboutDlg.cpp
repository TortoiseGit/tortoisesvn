// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014, 2016-2018, 2020-2021 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "AboutDlg.h"
#include "svn_version.h"
#include "../version.h"
#include "../../ext/serf/serf.h"
#include "../../ext/sqlite/sqlite3.h"
#include "../../ext/openssl/include/openssl/opensslv.h"
#include "CreateProcessHelper.h"
#include "AppUtils.h"
#include "../Utils/DPIAware.h"

IMPLEMENT_DYNAMIC(CAboutDlg, CStandAloneDialog)
CAboutDlg::CAboutDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CAboutDlg::IDD, pParent)
{
}

CAboutDlg::~CAboutDlg()
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_WEBLINK, m_cWebLink);
    DDX_Control(pDX, IDC_SUPPORTLINK, m_cSupportLink);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CStandAloneDialog)
    ON_WM_TIMER()
    ON_WM_MOUSEMOVE()
    ON_BN_CLICKED(IDC_UPDATE, OnBnClickedUpdate)
    ON_WM_CLOSE()
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_VERSIONBOX);
    m_aeroControls.SubclassControl(this, IDC_CREDITS1);
    m_aeroControls.SubclassControl(this, IDC_CREDITS2);
    m_aeroControls.SubclassControl(this, IDOK);
    m_aeroControls.SubclassControl(this, IDC_UPDATE);

    // set the version string
    CString              temp;
    const svn_version_t* svnver = svn_client_version();

    CString svnvertag = CString(svnver->tag);
    if (svnvertag.IsEmpty())
    {
        svnvertag = L"-release";
    }
    CString sIpv6;

#if APR_HAVE_IPV6
    sIpv6 = L"\r\nipv6 enabled";
#endif

    temp.Format(IDS_ABOUTVERSION, TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD, TEXT(TSVN_PLATFORM), TEXT(TSVN_VERDATE), static_cast<LPCWSTR>(sIpv6),
                svnver->major, svnver->minor, svnver->patch, static_cast<LPCWSTR>(svnvertag),
                APR_MAJOR_VERSION, APR_MINOR_VERSION, APR_PATCH_VERSION,
                APU_MAJOR_VERSION, APU_MINOR_VERSION, APU_PATCH_VERSION,
                SERF_MAJOR_VERSION, SERF_MINOR_VERSION, SERF_PATCH_VERSION,
                _T(OPENSSL_VERSION_TEXT),
                _T(ZLIB_VERSION),
                _T(SQLITE_VERSION));
    SetDlgItemText(IDC_VERSIONABOUT, temp);
    this->SetWindowText(L"TortoiseSVN");

    CPictureHolder tmpPic;
    tmpPic.CreateFromBitmap(IDB_LOGOFLIPPED);
    m_renderSrc.Create32BitFromPicture(&tmpPic, CDPIAware::Instance().Scale(GetSafeHwnd(), 468), CDPIAware::Instance().Scale(GetSafeHwnd(), 64));
    m_renderDest.Create32BitFromPicture(&tmpPic, CDPIAware::Instance().Scale(GetSafeHwnd(), 468), CDPIAware::Instance().Scale(GetSafeHwnd(), 64));

    m_waterEffect.Create(CDPIAware::Instance().Scale(GetSafeHwnd(), 468), CDPIAware::Instance().Scale(GetSafeHwnd(), 64));
    SetTimer(ID_EFFECTTIMER, 40, nullptr);
    SetTimer(ID_DROPTIMER, 1500, nullptr);

    m_cWebLink.SetURL(L"https://tortoisesvn.net");
    m_cSupportLink.SetURL(L"https://tortoisesvn.net/donate.html");

    CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    GetDlgItem(IDOK)->SetFocus();
    return FALSE;
}

void CAboutDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == ID_EFFECTTIMER)
    {
        m_waterEffect.Render(static_cast<DWORD*>(m_renderSrc.GetDIBits()), static_cast<DWORD*>(m_renderDest.GetDIBits()));
        CClientDC dc(this);
        CPoint    ptOrigin(CDPIAware::Instance().Scale(GetSafeHwnd(), 15), CDPIAware::Instance().Scale(GetSafeHwnd(), 20));
        m_renderDest.Draw(&dc, ptOrigin);
    }
    if (nIDEvent == ID_DROPTIMER)
    {
        CRect r;
        r.left   = CDPIAware::Instance().Scale(GetSafeHwnd(), 15);
        r.top    = CDPIAware::Instance().Scale(GetSafeHwnd(), 20);
        r.right  = r.left + m_renderSrc.GetWidth();
        r.bottom = r.top + m_renderSrc.GetHeight();
        m_waterEffect.Blob(RANDOM(r.left, r.right), RANDOM(r.top, r.bottom), 5, 800, m_waterEffect.m_iHpage);
    }
    CStandAloneDialog::OnTimer(nIDEvent);
}

void CAboutDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    auto  dpix15 = CDPIAware::Instance().Scale(GetSafeHwnd(), 15);
    auto  dpiy20 = CDPIAware::Instance().Scale(GetSafeHwnd(), 20);
    CRect r;
    r.left   = dpix15;
    r.top    = dpiy20;
    r.right  = r.left + m_renderSrc.GetWidth();
    r.bottom = r.top + m_renderSrc.GetHeight();

    if (r.PtInRect(point) == TRUE)
    {
        // dibs are drawn upside down...
        point.y -= dpiy20;
        point.y = CDPIAware::Instance().Scale(GetSafeHwnd(), 64) - point.y;

        if (nFlags & MK_LBUTTON)
            m_waterEffect.Blob(point.x - dpix15, point.y, CDPIAware::Instance().Scale(GetSafeHwnd(), 10), 1600, m_waterEffect.m_iHpage);
        else
            m_waterEffect.Blob(point.x - dpix15, point.y, CDPIAware::Instance().Scale(GetSafeHwnd(), 5), 50, m_waterEffect.m_iHpage);
    }

    CStandAloneDialog::OnMouseMove(nFlags, point);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CAboutDlg::OnBnClickedUpdate()
{
    wchar_t com[MAX_PATH + 100] = {0};
    GetModuleFileName(nullptr, com, MAX_PATH);

    CCreateProcessHelper::CreateProcessDetached(com, L" /command:updatecheck /visible");
}

void CAboutDlg::OnClose()
{
    KillTimer(ID_EFFECTTIMER);
    KillTimer(ID_DROPTIMER);

    CStandAloneDialog::OnClose();
}
