// Copyright (C) 2017 - TortoiseSVN

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
#include "NativeRibbonApp.h"

CNativeRibbonApp::CNativeRibbonApp(CFrameWnd *pFrame, IUIFramework *pFramework)
    : m_pFrame(pFrame)
    , m_pFramework(pFramework)
    , m_cRefCount(0)
{
}

CNativeRibbonApp::~CNativeRibbonApp()
{
    ASSERT(m_cRefCount == 0);
}

STDMETHODIMP CNativeRibbonApp::QueryInterface(REFIID riid, void **ppvObject)
{
    if (riid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = (IUICommandHandler*)this;
        return S_OK;
    }
    else if (riid == __uuidof(IUIApplication))
    {
        AddRef();
        *ppvObject = (IUIApplication*)this;
        return S_OK;
    }
    else if (riid == __uuidof(IUICommandHandler))
    {
        AddRef();
        *ppvObject = (IUICommandHandler*)this;
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CNativeRibbonApp::AddRef(void)
{
    return InterlockedIncrement(&m_cRefCount);
}

STDMETHODIMP_(ULONG) CNativeRibbonApp::Release(void)
{
    return InterlockedDecrement(&m_cRefCount);
}

STDMETHODIMP CNativeRibbonApp::OnViewChanged(
    UINT32 viewId,
    UI_VIEWTYPE typeID,
    IUnknown *view,
    UI_VIEWVERB verb,
    INT32 uReasonCode)
{
    UNREFERENCED_PARAMETER(viewId);
    UNREFERENCED_PARAMETER(view);
    UNREFERENCED_PARAMETER(verb);
    UNREFERENCED_PARAMETER(uReasonCode);

    if (typeID == UI_VIEWTYPE_RIBBON)
    {
        if (verb == UI_VIEWVERB_CREATE)
        {
            if (!m_SettingsFileName.IsEmpty())
            {
                CComQIPtr<IUIRibbon> ribbonView(view);
                if (ribbonView)
                {
                    LoadRibbonViewSettings(ribbonView, m_SettingsFileName);
                }
            }

            m_pFrame->RecalcLayout();
        }
        else if (verb == UI_VIEWVERB_DESTROY)
        {
            CComQIPtr<IUIRibbon> ribbonView(view);
            if (ribbonView)
            {
                SaveRibbonViewSettings(ribbonView, m_SettingsFileName);
            }
        }
        else if (verb == UI_VIEWVERB_SIZE)
        {
            m_pFrame->RecalcLayout();
        }
    }

    return S_OK;
}

STDMETHODIMP CNativeRibbonApp::OnCreateUICommand(UINT32 commandId,
    UI_COMMANDTYPE typeID,
    IUICommandHandler **commandHandler)
{
    UNREFERENCED_PARAMETER(typeID);

    m_commandIds.push_back(commandId);

    return QueryInterface(IID_PPV_ARGS(commandHandler));
}

STDMETHODIMP CNativeRibbonApp::OnDestroyUICommand(UINT32 commandId,
    UI_COMMANDTYPE typeID,
    IUICommandHandler *commandHandler)
{
    UNREFERENCED_PARAMETER(typeID);
    UNREFERENCED_PARAMETER(commandHandler);

    m_commandIds.remove(commandId);

    return S_OK;
}

STDMETHODIMP CNativeRibbonApp::Execute(UINT32 commandId,
    UI_EXECUTIONVERB verb,
    const PROPERTYKEY *key,
    const PROPVARIANT *currentValue,
    IUISimplePropertySet *commandExecutionProperties)
{
    UNREFERENCED_PARAMETER(key);
    UNREFERENCED_PARAMETER(currentValue);
    if (verb == UI_EXECUTIONVERB_EXECUTE)
    {
        if (commandExecutionProperties)
        {
            PROPVARIANT val = { 0 };
            if (commandExecutionProperties->GetValue(UI_PKEY_CommandId, &val) == S_OK)
            {
                UIPropertyToUInt32(UI_PKEY_CommandId, val, &commandId);
            }
        }

        m_pFrame->PostMessage(WM_COMMAND, commandId);
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

class CRibbonCmdUI : public CCmdUI
{
public:
    CString m_Text;
    BOOL m_bOn;
    int m_nCheck;
    int m_bCheckChanged;

    CRibbonCmdUI(int commandId)
        : m_bOn(FALSE)
        , m_nCheck(0)
        , m_bCheckChanged(FALSE)
    {
        m_nID = commandId;
    }

    virtual void Enable(BOOL bOn)
    {
        m_bOn = bOn;
        m_bEnableChanged = TRUE;
    }

    virtual void SetCheck(int nCheck)
    {
        m_nCheck = nCheck;
        m_bCheckChanged = TRUE;
    }

    virtual void SetText(LPCTSTR lpszText)
    {
        m_Text = lpszText;
    }
};

STDMETHODIMP CNativeRibbonApp::UpdateProperty(
    UINT32 commandId,
    REFPROPERTYKEY key,
    const PROPVARIANT *currentValue,
    PROPVARIANT *newValue)
{
    UNREFERENCED_PARAMETER(currentValue);

    if (key == UI_PKEY_TooltipTitle)
    {
        CString str;
        if (!str.LoadString(commandId))
            return S_FALSE;

        int nIndex = str.Find(L'\n');
        if (nIndex <= 0)
            return S_FALSE;

        str = str.Mid(nIndex + 1);

        CString strLabel;

        if (m_pFrame != NULL && (CKeyboardManager::FindDefaultAccelerator(commandId, strLabel, m_pFrame, TRUE) ||
            CKeyboardManager::FindDefaultAccelerator(commandId, strLabel, m_pFrame->GetActiveFrame(), FALSE)))
        {
            str += _T(" (");
            str += strLabel;
            str += _T(')');
        }

        return UIInitPropertyFromString(UI_PKEY_TooltipTitle, str, newValue);
    }
    else if (key == UI_PKEY_TooltipDescription)
    {
        CString str;
        if (!str.LoadString(commandId))
            return S_FALSE;

        int nIndex = str.Find(L'\n');
        if (nIndex <= 0)
            return S_FALSE;

        str = str.Left(nIndex);

        return UIInitPropertyFromString(UI_PKEY_TooltipDescription, str, newValue);
    }
    else if (key == UI_PKEY_Enabled)
    {
        CRibbonCmdUI ui(commandId);
        ui.DoUpdate(m_pFrame, TRUE);

        return UIInitPropertyFromBoolean(UI_PKEY_Enabled, ui.m_bOn, newValue);
    }
    else if (key == UI_PKEY_BooleanValue)
    {
        CRibbonCmdUI ui(commandId);
        ui.DoUpdate(m_pFrame, TRUE);

        return UIInitPropertyFromBoolean(UI_PKEY_BooleanValue, ui.m_nCheck, newValue);
    }
    else
    {
        return E_NOTIMPL;
    }
}

HRESULT CNativeRibbonApp::SaveRibbonViewSettings(IUIRibbon * pRibbonView, const CString & fileName)
{
    HRESULT hr;
    CComPtr<IStream> stream;

    hr = SHCreateStreamOnFileEx(fileName, STGM_WRITE | STGM_CREATE, FILE_ATTRIBUTE_NORMAL, TRUE, nullptr, &stream);
    if (FAILED(hr))
        return hr;

    hr = pRibbonView->SaveSettingsToStream(stream);
    if (FAILED(hr))
    {
        stream->Revert();
        return hr;
    }

    hr = stream->Commit(STGC_DEFAULT);

    return hr;
}

HRESULT CNativeRibbonApp::LoadRibbonViewSettings(IUIRibbon * pRibbonView, const CString & fileName)
{
    HRESULT hr;
    CComPtr<IStream> stream;

    hr = SHCreateStreamOnFileEx(fileName, STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &stream);
    if (FAILED(hr))
        return hr;

    hr = pRibbonView->LoadSettingsFromStream(stream);

    return hr;
}

void CNativeRibbonApp::UpdateCmdUI(BOOL bDisableIfNoHandler)
{
    for (auto it = m_commandIds.begin(); it != m_commandIds.end(); ++it)
    {
        CRibbonCmdUI ui(*it);
        ui.DoUpdate(m_pFrame, bDisableIfNoHandler);
        if (ui.m_bEnableChanged)
        {
            PROPVARIANT val = { 0 };
            UIInitPropertyFromBoolean(UI_PKEY_Enabled, ui.m_bOn, &val);
            m_pFramework->SetUICommandProperty(*it, UI_PKEY_Enabled, val);
        }

        if (ui.m_bCheckChanged)
        {
            PROPVARIANT val = { 0 };
            UIInitPropertyFromBoolean(UI_PKEY_BooleanValue, ui.m_nCheck, &val);
            m_pFramework->SetUICommandProperty(*it, UI_PKEY_BooleanValue, val);
        }
    }
}

int CNativeRibbonApp::GetRibbonHeight()
{
    CComPtr<IUIRibbon> pRibbon;

    if (SUCCEEDED(m_pFramework->GetView(0, IID_PPV_ARGS(&pRibbon))))
    {
        UINT32 cy = 0;
        pRibbon->GetHeight(&cy);
        return (int)cy;
    }
    else
    {
        return 0;
    }
}
