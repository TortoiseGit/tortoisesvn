// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2015, 2021 - TortoiseSVN

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
#include <vector>
#include <wrl\implements.h>
#include <windows.ui.notifications.h>

typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ::IInspectable *>                                           DesktopToastActivatedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastDismissedEventArgs *> DesktopToastDismissedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastFailedEventArgs *>    DesktopToastFailedEventHandler;

enum ToastNotificationAction
{
    Activate = 1,              // notification activated
    Dismiss_ApplicationHidden, // The application hid the toast using ToastNotifier.hide()
    Dismiss_UserCanceled,      // The user dismissed this toast
    Dismiss_TimedOut,          // The toast has timed out
    Dismiss_NotActivated,      // Toast not activated
    Failed,                    // The toast encountered an error
};

class ToastNotifications
{
public:
    ToastNotifications() {}
    ~ToastNotifications() {}

    HRESULT ShowToast(HWND hMainWnd, LPCWSTR appID, LPCWSTR iconpath, const std::vector<std::wstring> &lines) const;

private:
    static HRESULT SetNodeValueString(_In_ HSTRING inputString, _In_ ABI::Windows::Data::Xml::Dom::IXmlNode *node, _In_ ABI::Windows::Data::Xml::Dom::IXmlDocument *xml);
};

class ToastEventHandler : public Microsoft::WRL::Implements<DesktopToastActivatedEventHandler, DesktopToastDismissedEventHandler, DesktopToastFailedEventHandler>
{
public:
    ToastEventHandler(_In_ HWND hMainWnd);
    virtual ~ToastEventHandler();

    // DesktopToastActivatedEventHandler
    HRESULT STDMETHODCALLTYPE Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ IInspectable *args) override;

    // DesktopToastDismissedEventHandler
    HRESULT STDMETHODCALLTYPE Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastDismissedEventArgs *e) override;

    // DesktopToastFailedEventHandler
    HRESULT STDMETHODCALLTYPE Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastFailedEventArgs *e) override;

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG l = InterlockedDecrement(&m_ref);
        if (l == 0)
            delete this;
        return l;
    }

    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void **ppv) override
    {
        if (IsEqualIID(riid, IID_IUnknown))
            *ppv = static_cast<IUnknown *>(static_cast<DesktopToastActivatedEventHandler *>(this));
        else if (IsEqualIID(riid, __uuidof(DesktopToastActivatedEventHandler)))
            *ppv = static_cast<DesktopToastActivatedEventHandler *>(this);
        else if (IsEqualIID(riid, __uuidof(DesktopToastDismissedEventHandler)))
            *ppv = static_cast<DesktopToastDismissedEventHandler *>(this);
        else if (IsEqualIID(riid, __uuidof(DesktopToastFailedEventHandler)))
            *ppv = static_cast<DesktopToastFailedEventHandler *>(this);
        else
            *ppv = nullptr;

        if (*ppv)
        {
            static_cast<IUnknown *>(*ppv)->AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

private:
    ULONG m_ref;
    HWND  m_hMainWnd;
};

static UINT WM_TOASTNOTIFICATION = RegisterWindowMessage(L"TOASTNOTIFICATION_MSG");
