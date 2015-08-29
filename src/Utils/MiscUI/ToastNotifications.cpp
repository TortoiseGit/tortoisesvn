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
#include "stdafx.h"
#include "ToastNotifications.h"

#include "StringReferenceWrapper.h"


ToastEventHandler::ToastEventHandler(_In_ HWND hMainWnd)
    : _ref(1)
    , m_hMainWnd(hMainWnd)
{

}

ToastEventHandler::~ToastEventHandler()
{

}

// DesktopToastActivatedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification* /* sender */, _In_ IInspectable* /* args */)
{
    SetForegroundWindow(m_hMainWnd);
    LRESULT result = SendMessage(m_hMainWnd, WM_TOASTNOTIFICATION, ToastNotificationAction::Activate, NULL);
    return result ? S_OK : E_FAIL;
}

// DesktopToastDismissedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification* /* sender */, _In_ ABI::Windows::UI::Notifications::IToastDismissedEventArgs* e)
{
    ABI::Windows::UI::Notifications::ToastDismissalReason tdr;
    HRESULT hr = e->get_Reason(&tdr);

    if (SUCCEEDED(hr))
    {
        ToastNotificationAction action;
        switch (tdr)
        {
            case ABI::Windows::UI::Notifications::ToastDismissalReason_ApplicationHidden:
            action = ToastNotificationAction::Dismiss_ApplicationHidden;
            break;

            case ABI::Windows::UI::Notifications::ToastDismissalReason_UserCanceled:
            action = ToastNotificationAction::Dismiss_UserCanceled;
            break;

            case ABI::Windows::UI::Notifications::ToastDismissalReason_TimedOut:
            action = ToastNotificationAction::Dismiss_TimedOut;
            break;

            default:
            action = ToastNotificationAction::Dismiss_NotActivated;
            break;
        }

        LRESULT result = SendMessage(m_hMainWnd, WM_TOASTNOTIFICATION, action, NULL);
        hr = result ? S_OK : E_FAIL;
    }
    return hr;
}

// DesktopToastFailedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification* /* sender */, _In_ ABI::Windows::UI::Notifications::IToastFailedEventArgs* /* e */)
{
    LRESULT result = SendMessage(m_hMainWnd, WM_TOASTNOTIFICATION, ToastNotificationAction::Failed, NULL);
    return result ? S_OK : E_FAIL;
}

HRESULT ToastNotifications::ShowToast(HWND hMainWnd, LPCWSTR appID, LPCWSTR iconpath, const std::vector<std::wstring>& lines)
{
    typedef HRESULT(FAR STDAPICALLTYPE *f_roGetActivationFactory)(_In_ HSTRING activatableClassId, _In_ REFIID iid, _COM_Outptr_ void ** factory);
    f_roGetActivationFactory roGetActivationFactory = 0;
    auto hLib = LoadLibrary(L"COMBASE.DLL");
    if (hLib == NULL)
        return E_FAIL;
    roGetActivationFactory = (f_roGetActivationFactory)GetProcAddress(hLib, "RoGetActivationFactory");
    if (roGetActivationFactory == nullptr)
    {
        FreeLibrary(hLib);
        return E_FAIL;
    }

    Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotificationManagerStatics> toastStatics;
    HRESULT hr = roGetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), IID_INS_ARGS(&toastStatics));

    if (SUCCEEDED(hr))
    {
        Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument> toastXml;

        // Retrieve the template XML
        hr = toastStatics->GetTemplateContent(ABI::Windows::UI::Notifications::ToastTemplateType_ToastImageAndText04, &toastXml);
        if (SUCCEEDED(hr))
        {
            Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeList> nodeList;
            hr = toastXml->GetElementsByTagName(StringReferenceWrapper(L"image").Get(), &nodeList);
            if (SUCCEEDED(hr))
            {
                Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> imageNode;
                hr = nodeList->Item(0, &imageNode);
                if (SUCCEEDED(hr))
                {
                    Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap> attributes;

                    hr = imageNode->get_Attributes(&attributes);
                    if (SUCCEEDED(hr))
                    {
                        Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> srcAttribute;

                        hr = attributes->GetNamedItem(StringReferenceWrapper(L"src").Get(), &srcAttribute);
                        if (SUCCEEDED(hr))
                        {
                            hr = SetNodeValueString(StringReferenceWrapper(iconpath).Get(), srcAttribute.Get(), toastXml.Get());
                        }
                    }
                }
            }



            if (SUCCEEDED(hr))
            {
                hr = !lines.empty() ? S_OK : E_INVALIDARG;
                if (SUCCEEDED(hr))
                {
                    Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeList> nodeList2;
                    hr = toastXml->GetElementsByTagName(StringReferenceWrapper(L"text").Get(), &nodeList2);
                    if (SUCCEEDED(hr))
                    {
                        UINT32 nodeListLength;
                        hr = nodeList2->get_Length(&nodeListLength);
                        if (SUCCEEDED(hr))
                        {
                            hr = lines.size() <= nodeListLength ? S_OK : E_INVALIDARG;
                            if (SUCCEEDED(hr))
                            {
                                for (UINT32 i = 0; i < lines.size(); i++)
                                {
                                    Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> textNode;
                                    hr = nodeList2->Item(i, &textNode);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = SetNodeValueString(StringReferenceWrapper(lines[i].c_str()).Get(), textNode.Get(), toastXml.Get());
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }


        if (SUCCEEDED(hr))
        {
            //hr = CreateToast(toastStatics.Get(), toastXml.Get());
            Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotifier> notifier;
            hr = toastStatics->CreateToastNotifierWithId(StringReferenceWrapper(appID).Get(), &notifier);
            if (SUCCEEDED(hr))
            {
                Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotificationFactory> factory;
                hr = roGetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), IID_INS_ARGS(&factory));
                if (SUCCEEDED(hr))
                {
                    Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> toast;
                    hr = factory->CreateToastNotification(toastXml.Get(), &toast);
                    if (SUCCEEDED(hr))
                    {
                        // Register the event handlers
                        EventRegistrationToken activatedToken, dismissedToken, failedToken;
                        Microsoft::WRL::ComPtr<ToastEventHandler> eventHandler(new ToastEventHandler(hMainWnd));

                        hr = toast->add_Activated(eventHandler.Get(), &activatedToken);
                        if (SUCCEEDED(hr))
                        {
                            hr = toast->add_Dismissed(eventHandler.Get(), &dismissedToken);
                            if (SUCCEEDED(hr))
                            {
                                hr = toast->add_Failed(eventHandler.Get(), &failedToken);
                                if (SUCCEEDED(hr))
                                {
                                    hr = notifier->Show(toast.Get());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    FreeLibrary(hLib);
    return hr;
}

HRESULT ToastNotifications::SetNodeValueString(HSTRING inputString, ABI::Windows::Data::Xml::Dom::IXmlNode * node, ABI::Windows::Data::Xml::Dom::IXmlDocument * xml)
{
    Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlText> inputText;

    HRESULT hr = xml->CreateTextNode(inputString, &inputText);
    if (SUCCEEDED(hr))
    {
        Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> inputTextNode;

        hr = inputText.As(&inputTextNode);
        if (SUCCEEDED(hr))
        {
            Microsoft::WRL::ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> pAppendedChild;
            hr = node->AppendChild(inputTextNode.Get(), &pAppendedChild);
        }
    }

    return hr;
}
