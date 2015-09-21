// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSVN

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
#include <wrl\client.h>
#include <wrl\implements.h>
#include <windows.ui.notifications.h>

typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ::IInspectable *> DesktopToastActivatedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastDismissedEventArgs *> DesktopToastDismissedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastFailedEventArgs *> DesktopToastFailedEventHandler;

enum ToastNotificationAction
{
    Activate = 1,                       // notification activated
    Dismiss_ApplicationHidden,          // The application hid the toast using ToastNotifier.hide()
    Dismiss_UserCanceled,               // The user dismissed this toast
    Dismiss_TimedOut,                   // The toast has timed out
    Dismiss_NotActivated,               // Toast not activated
    Failed,                             // The toast encountered an error
};

class ToastNotifications
{
public:
    ToastNotifications() {}
    ~ToastNotifications() {}

    HRESULT ShowToast(HWND hMainWnd, LPCWSTR appID, LPCWSTR iconpath, const std::vector<std::wstring>& lines);

private:
    HRESULT SetNodeValueString(_In_ HSTRING inputString, _In_ ABI::Windows::Data::Xml::Dom::IXmlNode *node, _In_ ABI::Windows::Data::Xml::Dom::IXmlDocument *xml);
};

class ToastEventHandler : public Microsoft::WRL::Implements<DesktopToastActivatedEventHandler, DesktopToastDismissedEventHandler, DesktopToastFailedEventHandler>
{
public:
    ToastEventHandler(_In_ HWND hMainWnd);
    ~ToastEventHandler();

    // DesktopToastActivatedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ IInspectable* args);

    // DesktopToastDismissedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastDismissedEventArgs *e);

    // DesktopToastFailedEventHandler
    IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastFailedEventArgs *e);

    // IUnknown
    IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_ref); }

    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG l = InterlockedDecrement(&_ref);
        if (l == 0) delete this;
        return l;
    }

    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void **ppv)
    {
        if (IsEqualIID(riid, IID_IUnknown))
            *ppv = static_cast<IUnknown*>(static_cast<DesktopToastActivatedEventHandler*>(this));
        else if (IsEqualIID(riid, __uuidof(DesktopToastActivatedEventHandler)))
            *ppv = static_cast<DesktopToastActivatedEventHandler*>(this);
        else if (IsEqualIID(riid, __uuidof(DesktopToastDismissedEventHandler)))
            *ppv = static_cast<DesktopToastDismissedEventHandler*>(this);
        else if (IsEqualIID(riid, __uuidof(DesktopToastFailedEventHandler)))
            *ppv = static_cast<DesktopToastFailedEventHandler*>(this);
        else *ppv = nullptr;

        if (*ppv)
        {
            reinterpret_cast<IUnknown*>(*ppv)->AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

private:
    ULONG _ref;
    HWND m_hMainWnd;
};

static UINT WM_TOASTNOTIFICATION = RegisterWindowMessage(L"TOASTNOTIFICATION_MSG");
