// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "stdafx.h"
#include "ProgressDlg.h"
#include <atlconv.h>

extern "C" const GUID CLSID_ProgressDialog = {0xf8383852, 0xfcd3, 0x11d1, 0xa6, 0xb9, 0x0, 0x60, 0x97, 0xdf, 0x5b, 0xd4};
extern "C" const GUID IID_IProgressDialog = {0xebbc7c04, 0x315e, 0x11d2, 0xb6, 0x2f, 0x0, 0x60, 0x97, 0xdf, 0x5b, 0xd4};

CProgressDlg::CProgressDlg() :
		    m_bValid(false),			//not valid by default
            m_isVisible(false),
		    m_dwDlgFlags(PROGDLG_NORMAL), 
		    m_pIDlg(NULL)
{
	HRESULT hr;

    hr = CoCreateInstance (CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER,
                            IID_IProgressDialog, (void**)&m_pIDlg);

    if (SUCCEEDED(hr))
        m_bValid = true;				//instance successfully created
}

CProgressDlg::~CProgressDlg()
{
    if (m_bValid)
    {
	    if (m_isVisible)			//still visible, so stop first before destroying
	        m_pIDlg->StopProgressDialog();

    	m_pIDlg->Release();
    }
}

void CProgressDlg::SetTitle(LPCTSTR szTitle)
{
    USES_CONVERSION;
    m_pIDlg->SetTitle(T2COLE(szTitle));
}

void CProgressDlg::SetLine(DWORD dwLine, LPCTSTR szText, bool bCompactPath /* = false */)
{
    USES_CONVERSION;
    m_pIDlg->SetLine(dwLine, T2COLE(szText), bCompactPath, NULL);
}

void CProgressDlg::SetCancelMsg(LPCTSTR szMessage)
{
    USES_CONVERSION;
    m_pIDlg->SetCancelMsg(T2COLE(szMessage), NULL);
}

void CProgressDlg::SetAnimation(HINSTANCE hinst, UINT uRsrcID)
{
    ASSERT(m_bValid);
    m_pIDlg->SetAnimation(hinst, uRsrcID);
}

void CProgressDlg::SetAnimation(UINT uRsrcID)
{
    ASSERT(m_bValid);
    m_pIDlg->SetAnimation(AfxGetResourceHandle(), uRsrcID);
}

void CProgressDlg::SetTime(bool bTime /* = true */)
{
    m_dwDlgFlags &= ~(PROGDLG_NOTIME | PROGDLG_AUTOTIME);

    if (bTime)
        m_dwDlgFlags |= PROGDLG_AUTOTIME;
    else
        m_dwDlgFlags |= PROGDLG_NOTIME;
}

void CProgressDlg::SetShowProgressBar(bool bShow /* = true */)
{
    if (bShow)
        m_dwDlgFlags &= ~PROGDLG_NOPROGRESSBAR;
    else
        m_dwDlgFlags |= PROGDLG_NOPROGRESSBAR;
}

HRESULT CProgressDlg::ShowModal (CWnd* pwndParent)
{
	HRESULT hr;
    ASSERT_VALID(pwndParent);

    hr = m_pIDlg->StartProgressDialog(pwndParent->GetSafeHwnd(),
										NULL,
										m_dwDlgFlags | PROGDLG_MODAL,
										NULL);

    if (SUCCEEDED(hr))
	{
        m_isVisible = true;
    }
    return hr;
}

HRESULT CProgressDlg::ShowModeless(CWnd* pwndParent)
{
	HRESULT hr;

	if (pwndParent == NULL)
		hr = m_pIDlg->StartProgressDialog(NULL, NULL, m_dwDlgFlags, NULL);
	else
		hr = m_pIDlg->StartProgressDialog(pwndParent->GetSafeHwnd(), NULL, m_dwDlgFlags, NULL);

    if (SUCCEEDED(hr))
    {
        m_isVisible = true;
    }
    return hr;
}

void CProgressDlg::SetProgress(DWORD dwProgress, DWORD dwMax)
{
    m_pIDlg->SetProgress(dwProgress, dwMax);
}


void CProgressDlg::SetProgress(ULONGLONG u64Progress, ULONGLONG u64ProgressMax)
{
    m_pIDlg->SetProgress64(u64Progress, u64ProgressMax);
}


bool CProgressDlg::HasUserCancelled()
{
    return (0 != m_pIDlg->HasUserCancelled());
}

void CProgressDlg::Stop()
{
    if (m_isVisible)
    {
        m_pIDlg->StopProgressDialog();
        m_isVisible = false;
    }
}

void CProgressDlg::ResetTimer()
{
    m_pIDlg->Timer(PDTIMER_RESET, NULL);
}
