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
#include "stdafx.h"
#include "DragDropTreeCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDragDropTreeCtrl::CDragDropTreeCtrl()
{
    m_hDragItem = NULL;
    m_pImageList = NULL;
    m_bDragging = FALSE;
    m_nDelayInterval = 500;     // Default delay interval = 500 milliseconds
    m_nScrollInterval = 200;    // Default scroll interval = 200 milliseconds
    m_nScrollMargin = 10;       // Default scroll margin = 10 pixels
}

CDragDropTreeCtrl::~CDragDropTreeCtrl()
{
    // Delete the image list created by CreateDragImage.
    if (m_pImageList != NULL)
        delete m_pImageList;
}

BEGIN_MESSAGE_MAP(CDragDropTreeCtrl, CTreeCtrl)
    ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDragDropTreeCtrl message handlers

BOOL CDragDropTreeCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
    // Make sure the control's TVS_DISABLEDRAGDROP flag is not set.
    // If you subclass an existing tree view control rather than create
    // a CDragDropTreeCtrl outright, it's YOUR responsibility to see that
    // this flag isn't set.
    cs.style &= ~TVS_DISABLEDRAGDROP;
    return CTreeCtrl::PreCreateWindow(cs);
}

void CDragDropTreeCtrl::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

    HTREEITEM hItem = pNMTreeView->itemNew.hItem;

    // Create a drag image. If the assertion fails, you probably forgot
    // to assign an image list to the control with SetImageList. Create-
    // DragImage will not work if the control hasn't been assigned an
    // image list!
    m_pImageList = CreateDragImage(hItem);
    ASSERT(m_pImageList != NULL);

    if (m_pImageList != NULL)
    {
        // Compute the coordinates of the "hot spot"--the location of the
        // cursor relative to the upper left corner of the item rectangle.
        CRect rect;
        GetItemRect(hItem, rect, TRUE);
        CPoint point(pNMTreeView->ptDrag.x, pNMTreeView->ptDrag.y);
        CPoint hotSpot = point;
        hotSpot.x -= rect.left;
        hotSpot.y -= rect.top;

        // Convert the client coordinates in "point" to coordinates relative
        // to the upper left corner of the control window.
        CPoint client(0, 0);
        ClientToScreen(&client);
        GetWindowRect(rect);
        point.x += client.x - rect.left;
        point.y += client.y - rect.top;

        // Capture the mouse and begin dragging.
        SetCapture();
        m_pImageList->BeginDrag(0, hotSpot);
        m_pImageList->DragEnter(this, point);
        m_hDragItem = hItem;
        m_bDragging = TRUE;
    }
}

void CDragDropTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
    CTreeCtrl::OnMouseMove(nFlags, point);

    if (m_bDragging && m_pImageList != NULL)
    {
        // Stop the scroll timer if it's running.
        KillTimer(1);

        // Erase the old drag image and draw a new one.
        m_pImageList->DragMove(point);

        // Highlight the drop target if the cursor is over an item.
        HTREEITEM hItem = HighlightDropTarget(point);

        // Modify the cursor to provide visual feedback to the user.
        // Note: It's important to do this AFTER the call to DragMove.
        ::SetCursor(hItem == NULL ?
                    AfxGetApp()->LoadStandardCursor(IDC_NO) :
                    (HCURSOR) ::GetClassLongPtr(m_hWnd, GCLP_HCURSOR));

        // Set a timer if the cursor is at the top or bottom of the window,
        // or if it's over a collapsed item.
        CRect rect;
        GetClientRect(rect);
        int cy = rect.Height();

        if ((point.y >= 0 && point.y <= m_nScrollMargin) ||
            (point.y >= cy - m_nScrollMargin && point.y <= cy) ||
            (hItem != NULL && ItemHasChildren(hItem) &&
            !IsItemExpanded(hItem)))

            SetTimer(1, m_nDelayInterval, NULL);
    }
}

void CDragDropTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
    CTreeCtrl::OnLButtonUp(nFlags, point);

    if (m_bDragging && m_pImageList != NULL)
    {
        // Stop the scroll timer if it's running.
        KillTimer(1);

        // Terminate the dragging operation and release the mouse.
        m_pImageList->DragLeave(this);
        m_pImageList->EndDrag();
        ::ReleaseCapture();
        m_bDragging = FALSE;
        SelectDropTarget(NULL);

        // Delete the image list created by CreateDragImage.
        delete m_pImageList;
        m_pImageList = NULL;

        // Get the HTREEITEM of the drop target and exit now if it's NULL.
        UINT nFlags;
        HTREEITEM hItem = HitTest(point, &nFlags);
        if (hItem == NULL)
            return;

        if (hItem == m_hDragItem)
            return;
        else if (hItem == GetParentItem(m_hDragItem))
            return;
        else if (IsChildOf(hItem, m_hDragItem))
            return;

        // Move the dragged item and its subitems (if any) to the drop point.
        MoveTree(hItem, m_hDragItem);

        // Mark the item as having children
        TVITEM tvItem;
        tvItem.mask = TVIF_CHILDREN | TVIF_HANDLE;
        tvItem.cChildren = 1;
        tvItem.hItem = hItem;
        SetItem(&tvItem);

        m_hDragItem = NULL;
    }
}

void CDragDropTreeCtrl::OnTimer(UINT_PTR nIDEvent)
{
    CTreeCtrl::OnTimer(nIDEvent);

    // Reset the timer.
    SetTimer(1, m_nScrollInterval, NULL);

    // Get the current cursor position and window height.
    DWORD dwPos = ::GetMessagePos();
    CPoint point(LOWORD(dwPos), HIWORD(dwPos));
    ScreenToClient(&point);

    CRect rect;
    GetClientRect(rect);
    int cy = rect.Height();

    // Scroll the window if the cursor is near the top or bottom.
    if (point.y >= 0 && point.y <= m_nScrollMargin)
    {
        HTREEITEM hFirstVisible = GetFirstVisibleItem();
        m_pImageList->DragShowNolock(FALSE);
        SendMessage(WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), NULL);
        m_pImageList->DragShowNolock(TRUE);

        // Kill the timer if the window did not scroll, or redraw the
        // drop target highlight if the window did scroll.
        if (GetFirstVisibleItem() == hFirstVisible)
            KillTimer(1);
        else
        {
            HighlightDropTarget(point);
            return;
        }
    }
    else if (point.y >= cy - m_nScrollMargin && point.y <= cy)
    {
        HTREEITEM hFirstVisible = GetFirstVisibleItem();
        m_pImageList->DragShowNolock(FALSE);
        SendMessage(WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
        m_pImageList->DragShowNolock(TRUE);

        // Kill the timer if the window did not scroll, or redraw the
        // drop target highlight if the window did scroll.
        if (GetFirstVisibleItem() == hFirstVisible)
            KillTimer(1);
        else
        {
            HighlightDropTarget(point);
            return;
        }
    }

    // If the cursor is hovering over a collapsed item, expand the tree.
    UINT nFlags;
    HTREEITEM hItem = HitTest(point, &nFlags);

    if (hItem != NULL && ItemHasChildren(hItem) && !IsItemExpanded(hItem))
    {
        m_pImageList->DragShowNolock(FALSE);
        Expand(hItem, TVE_EXPAND);
        m_pImageList->DragShowNolock(TRUE);
        KillTimer(1);
        return;
    }
}

BOOL CDragDropTreeCtrl::IsChildOf(HTREEITEM hItem1, HTREEITEM hItem2)
{
    HTREEITEM hParent = hItem1;
    while ((hParent = GetParentItem(hParent)) != NULL)
    {
        if (hParent == hItem2)
            return TRUE;
    }
    return FALSE;
}

void CDragDropTreeCtrl::MoveTree(HTREEITEM hDest, HTREEITEM hSrc)
{
    CopyTree(hDest, hSrc);
    DeleteItem(hSrc);
}

void CDragDropTreeCtrl::CopyTree(HTREEITEM hDest, HTREEITEM hSrc)
{
    // Get the attributes of item to be copied.
    TVITEMEX tvItem = { 0 };
    tvItem.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_EXPANDEDIMAGE | TVIF_HANDLE | TVIF_IMAGE | TVIF_INTEGRAL | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_STATEEX | TVIF_TEXT;
    tvItem.hItem = hSrc;
    GetItem((TVITEM*)&tvItem);


    // Create an exact copy of the item at the destination.
    TVINSERTSTRUCT tvInsertItem = { 0 };
    tvInsertItem.itemex = tvItem;
    tvInsertItem.hInsertAfter = TVI_SORT;
    tvInsertItem.hParent = hDest;
    HTREEITEM hNewItem = InsertItem(&tvInsertItem);

    // If the item has subitems, copy them, too.
    if (ItemHasChildren(hSrc))
        CopyChildren(hNewItem, hSrc);

    // Select the newly added item.
    SelectItem(hNewItem);
}

void CDragDropTreeCtrl::CopyChildren(HTREEITEM hDest, HTREEITEM hSrc)
{
    // Get the first subitem.
    HTREEITEM hItem = GetChildItem(hSrc);
    if (hItem == NULL)
        return; // item has no children

    // Create a copy of it at the destination.
    TVITEMEX tvItem = { 0 };
    tvItem.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_EXPANDEDIMAGE | TVIF_HANDLE | TVIF_IMAGE | TVIF_INTEGRAL | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_STATEEX | TVIF_TEXT;
    tvItem.hItem = hSrc;
    GetItem((TVITEM*)&tvItem);

    TVINSERTSTRUCT tvInsertItem = { 0 };
    tvInsertItem.itemex = tvItem;
    tvInsertItem.hInsertAfter = TVI_SORT;
    tvInsertItem.hParent = hDest;
    HTREEITEM hNewItem = InsertItem(&tvInsertItem);

    // If the subitem has subitems, copy them, too.
    if (ItemHasChildren(hItem))
        CopyChildren(hNewItem, hItem);

    // Do the same for other subitems of hSrc.
    while ((hItem = GetNextSiblingItem(hItem)) != NULL)
    {
        tvItem.mask = TVIF_CHILDREN | TVIF_DI_SETITEM | TVIF_EXPANDEDIMAGE | TVIF_HANDLE | TVIF_IMAGE | TVIF_INTEGRAL | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_STATEEX | TVIF_TEXT;
        tvItem.hItem = hSrc;
        GetItem((TVITEM*)&tvItem);
        tvInsertItem.itemex = tvItem;
        tvInsertItem.hInsertAfter = TVI_SORT;
        tvInsertItem.hParent = hDest;
        hNewItem = InsertItem(&tvInsertItem);
        if (ItemHasChildren(hItem))
            CopyChildren(hNewItem, hItem);
    }
}

BOOL CDragDropTreeCtrl::IsItemExpanded(HTREEITEM hItem)
{
    return GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED;
}

HTREEITEM CDragDropTreeCtrl::HighlightDropTarget(CPoint point)
{
    // Find out which item (if any) the cursor is over.
    UINT nFlags;
    HTREEITEM hItem = HitTest(point, &nFlags);

    // Highlight the item, or unhighlight all items if the cursor isn't
    // over an item.
    m_pImageList->DragShowNolock(FALSE);
    SelectDropTarget(hItem);
    m_pImageList->DragShowNolock(TRUE);

    // Return the handle of the highlighted item.
    return hItem;
}
