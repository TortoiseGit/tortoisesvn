// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010, 2012-2013 - TortoiseSVN

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
#pragma once
#include "SVN.h"
#include "ResizablePageEx.h"
#include "AppUtils.h"

#define WM_TSVN_MAXREVFOUND         (WM_APP + 1)

/**
 * base class for the merge wizard property pages
 */
class CMergeWizardBasePage : public CResizablePageEx, public SVN
{
public:
    CMergeWizardBasePage()
        : CResizablePageEx()
        , m_bCancelled(false)
        , m_bThreadRunning(false)
        , m_pThread(NULL) {;}
    explicit CMergeWizardBasePage(UINT nIDTemplate, UINT nIDCaption = 0)
        : CResizablePageEx(nIDTemplate, nIDCaption, 0)
        , m_bCancelled(false)
        , m_bThreadRunning(false)
        , m_pThread(NULL) {;}
    explicit CMergeWizardBasePage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0)
        : CResizablePageEx(lpszTemplateName, nIDCaption, 0)
        , m_bCancelled(false)
        , m_bThreadRunning(false)
        , m_pThread(NULL) {;}

    virtual ~CMergeWizardBasePage() {;}
    virtual bool    OkToCancel() { return true; }

protected:
    virtual void    SetButtonTexts();
    void            AdjustControlSize(UINT nID);
    void            StartWCCheckThread(const CTSVNPath& path);
    void            StopWCCheckThread();
    void            ShowEditBalloon(UINT nIdControl, UINT nIdText, UINT nIdTitle, int nIcon = TTI_WARNING);
    void            ShowComboBalloon(CComboBoxEx * pCombo, UINT nIdText, UINT nIdTitle, int nIcon = TTI_WARNING);

    static UINT     FindRevThreadEntry(LPVOID pVoid);
    UINT            FindRevThread();
    virtual BOOL    Cancel() override {return m_bCancelled;}
private:
    CTSVNPath       m_path;
    bool            m_bCancelled;
    CWinThread *    m_pThread;
    volatile LONG   m_bThreadRunning;
};