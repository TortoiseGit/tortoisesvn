#include "stdafx.h"
#include "MessageBox.h"
#include "SmartHandle.h"
#include "XMessageBox.h"


UINT TSVNMessageBox( HWND hWnd, LPCTSTR lpMessage, LPCTSTR lpCaption, UINT uType, LPCTSTR sHelpPath /*= NULL*/ )
{
    XMSGBOXPARAMS xmsg;
    if (sHelpPath)
        _tcscpy_s(xmsg.szHelpPath, sHelpPath);

    return XMessageBox(hWnd, lpMessage, lpCaption, uType, &xmsg);
}

UINT TSVNMessageBox( HWND hWnd, UINT nMessage, UINT nCaption, UINT uType, LPCTSTR sHelpPath /*= NULL*/ )
{
    return TSVNMessageBox(hWnd, (LPCTSTR)CString(MAKEINTRESOURCE(nMessage)), (LPCTSTR)CString(MAKEINTRESOURCE(nCaption)), uType, sHelpPath);
}

UINT TSVNMessageBox( HWND hWnd, LPCTSTR lpMessage, LPCTSTR lpCaption, int nDef, LPCTSTR lpButton1, LPCTSTR lpButton2, LPCTSTR lpButton3 /*= NULL*/ )
{
    CString sButtonTexts;
    if (lpButton1)
        sButtonTexts = lpButton1;
    if (lpButton2)
    {
        sButtonTexts += L"\n";
        sButtonTexts += lpButton2;
    }
    if (lpButton3)
    {
        sButtonTexts += L"\n";
        sButtonTexts += lpButton3;
    }
    XMSGBOXPARAMS xmsg;
    _tcscpy_s(xmsg.szCustomButtons, (LPCTSTR)sButtonTexts);

    return XMessageBox(hWnd, lpMessage, lpCaption, nDef, &xmsg);
}

