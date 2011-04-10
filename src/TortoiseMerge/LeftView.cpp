// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2011 - TortoiseSVN

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
#include "StdAfx.h"
#include "Resource.h"
#include "AppUtils.h"

#include "leftview.h"
#include "BottomView.h"

IMPLEMENT_DYNCREATE(CLeftView, CBaseView)

CLeftView::CLeftView(void)
{
    m_pwndLeft = this;
    m_nStatusBarID = ID_INDICATOR_LEFTVIEW;
}

CLeftView::~CLeftView(void)
{
}

void CLeftView::OnContextMenu(CPoint point, int /*nLine*/, DiffStates state)
{
    if (!this->IsWindowVisible())
        return;

    CMenu popup;
    if (!popup.CreatePopupMenu())
        return;

#define ID_USEBLOCK 1
#define ID_USEFILE 2
#define ID_USETHEIRANDYOURBLOCK 3
#define ID_USEYOURANDTHEIRBLOCK 4
#define ID_USEBOTHTHISFIRST 5
#define ID_USEBOTHTHISLAST 6

    const UINT uFlags = GetMenuFlags( state );

    CString temp;

    if (m_pwndBottom->IsWindowVisible())
    {
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISBLOCK);
        popup.AppendMenu(uFlags, ID_USEBLOCK, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISFILE);
        popup.AppendMenu(MF_STRING | MF_ENABLED, ID_USEFILE, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
        popup.AppendMenu(uFlags, ID_USEYOURANDTHEIRBLOCK, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
        popup.AppendMenu(uFlags, ID_USETHEIRANDYOURBLOCK, temp);
    }
    else
    {
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISBLOCK);
        popup.AppendMenu(uFlags, ID_USEBLOCK, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISFILE);
        popup.AppendMenu(MF_STRING | MF_ENABLED, ID_USEFILE, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISFIRST);
        popup.AppendMenu(uFlags, ID_USEBOTHTHISFIRST, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISLAST);
        popup.AppendMenu(uFlags, ID_USEBOTHTHISLAST, temp);
    }

    AddCutCopyAndPaste(popup);

    CompensateForKeyboard(point);

    int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
    ResetUndoStep();
    switch (cmd)
    {
    case ID_EDIT_COPY:
        OnEditCopy();
        break;
    case ID_EDIT_CUT:
        OnEditCopy();
        RemoveSelectedText();
        break;
    case ID_EDIT_PASTE:
        PasteText();
        break;
    case ID_USEFILE:
        UseFile();
        return;
    case ID_USEBLOCK:
        UseBlock();
        return;
    case ID_USEYOURANDTHEIRBLOCK:
        UseYourAndTheirBlock();
        return;
    case ID_USETHEIRANDYOURBLOCK:
        UseTheirAndYourBlock();
        return;
    case ID_USEBOTHTHISLAST:
        UseBothRightFirst();
        return;
    case ID_USEBOTHTHISFIRST:
        UseBothLeftFirst();
        return;
    default:
        return;
    } // switch (cmd)
    SaveUndoStep();
    return;
}

void CLeftView::UseFile()
{
    if (m_pwndBottom->IsWindowVisible())
    {
        m_pwndBottom->UseLeftFile();
    }
    else
    {
        m_pwndRight->UseLeftFile();
    }
}

void CLeftView::UseBlock()
{
    if (m_pwndBottom->IsWindowVisible())
    {
        m_pwndBottom->UseLeftBlock();
    }
    else
    {
        m_pwndRight->UseLeftBlock();
    }
}
