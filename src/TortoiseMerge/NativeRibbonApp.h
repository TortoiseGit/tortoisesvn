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
#pragma once

class CNativeRibbonDynamicItemInfo
{
public:
    CNativeRibbonDynamicItemInfo(UINT cmdId, const CString &text, UINT imageId)
        : m_cmdId(cmdId)
        , m_text(text)
        , m_imageId(imageId)
    {
    }

    const CString &GetLabel() const { return m_text; }
    UINT           GetCommandId() const { return m_cmdId; }
    UINT           GetImageId() const { return m_imageId; }

private:
    UINT    m_cmdId;
    CString m_text;
    UINT    m_imageId;
};

class CNativeRibbonApp
    : public IUIApplication
    , public IUICommandHandler
{
public:
    CNativeRibbonApp(CFrameWnd *pFrame, IUIFramework *pFramework);
    virtual ~CNativeRibbonApp();

    void SetSettingsFileName(const CString &file)
    {
        m_settingsFileName = file;
    }

    void UpdateCmdUI(BOOL bDisableIfNoHandler);
    int  GetRibbonHeight() const;
    void SetItems(UINT cmdId, const std::list<CNativeRibbonDynamicItemInfo> &items) const;

protected:
    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
    ULONG STDMETHODCALLTYPE   AddRef() override;
    ULONG STDMETHODCALLTYPE   Release() override;

    // IUIApplication
    HRESULT STDMETHODCALLTYPE OnViewChanged(UINT32      viewId,
                                            UI_VIEWTYPE typeID,
                                            IUnknown *  view,
                                            UI_VIEWVERB verb,
                                            INT32       uReasonCode) override;

    HRESULT STDMETHODCALLTYPE OnCreateUICommand(UINT32              commandId,
                                                UI_COMMANDTYPE      typeID,
                                                IUICommandHandler **commandHandler) override;

    HRESULT STDMETHODCALLTYPE OnDestroyUICommand(UINT32             commandId,
                                                 UI_COMMANDTYPE     typeID,
                                                 IUICommandHandler *commandHandler) override;

    // IUICommandHandler
    HRESULT STDMETHODCALLTYPE Execute(UINT32                commandId,
                                      UI_EXECUTIONVERB      verb,
                                      const PROPERTYKEY *   key,
                                      const PROPVARIANT *   currentValue,
                                      IUISimplePropertySet *commandExecutionProperties) override;

    HRESULT STDMETHODCALLTYPE UpdateProperty(UINT32             commandId,
                                             REFPROPERTYKEY     key,
                                             const PROPVARIANT *currentValue,
                                             PROPVARIANT *      newValue) override;

    static HRESULT         SaveRibbonViewSettings(IUIRibbon *pRibbonView, const CString &fileName);
    static HRESULT         LoadRibbonViewSettings(IUIRibbon *pRibbonView, const CString &fileName);
    CComPtr<IUICollection> GetUICommandItemsSource(UINT commandId) const;
    void                   SetUICommandItemsSource(UINT commandId, IUICollection *pItems) const;
    static UINT            GetCommandIdProperty(IUISimplePropertySet *propertySet);

private:
    CFrameWnd *           m_pFrame;
    CComPtr<IUIFramework> m_pFramework;
    std::list<UINT32>     m_commandIds;
    std::list<UINT32>     m_collectionCommandIds;
    ULONG                 m_cRefCount;
    CString               m_settingsFileName;
};
