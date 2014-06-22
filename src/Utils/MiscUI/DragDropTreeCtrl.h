// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2014 - TortoiseSVN

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


class CDragDropTreeCtrl : public CTreeCtrl
{
public:
    CDragDropTreeCtrl();
    virtual ~CDragDropTreeCtrl();

    void SetDroppedMessage(UINT msg) { m_WMOnDropped = msg; }
protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    HTREEITEM HighlightDropTarget(CPoint point);
    int m_nDelayInterval;
    BOOL IsItemExpanded(HTREEITEM hItem);
    int m_nScrollMargin;
    int m_nScrollInterval;
    void CopyChildren(HTREEITEM hDest, HTREEITEM hSrc);
    void CopyTree(HTREEITEM hDest, HTREEITEM hSrc);
    void MoveTree(HTREEITEM hDest, HTREEITEM hSrc);
    BOOL IsChildOf(HTREEITEM hItem1, HTREEITEM hItem2);
    BOOL m_bDragging;
    CImageList* m_pImageList;
    HTREEITEM m_hDragItem;
    UINT m_WMOnDropped;

    afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    DECLARE_MESSAGE_MAP()
};

