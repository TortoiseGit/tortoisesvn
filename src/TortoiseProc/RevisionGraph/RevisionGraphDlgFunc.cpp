// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2013-2015, 2018, 2020-2021 - TortoiseSVN

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
#include "RevisionGraphDlg.h"
#include "SVN.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "RevisionGraphWnd.h"
#include "CachedLogInfo.h"
#include "RevisionGraph/IRevisionGraphLayout.h"
#include "RevisionGraph/FullGraphBuilder.h"
#include "RevisionGraph/FullGraphFinalizer.h"
#include "RevisionGraph/VisibleGraphBuilder.h"
#include "RevisionGraph/StandardLayout.h"
#include "RevisionGraph/ShowWC.h"
#include "RevisionGraph/ShowWCModification.h"
#include "DPIAware.h"
#include "AppUtils.h"

#pragma warning(push)
#pragma warning(disable : 4458) // declaration of 'xxx' hides class member
#include <gdiplus.h>

#pragma warning(pop)

#ifdef _DEBUG
// ReSharper disable once CppInconsistentNaming
#    define new DEBUG_NEW
#    undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

void CRevisionGraphWnd::InitView()
{
    m_bIsRubberBand = false;

    SetScrollbars();
}

void CRevisionGraphWnd::BuildPreview()
{
    m_preview.DeleteObject();
    if (!m_bShowOverview)
        return;

    // is there a point in drawing this at all?

    int nodeCount = m_state.GetNodeCount();
    if ((nodeCount > REVGRAPH_PREVIEW_MAX_NODES) || (nodeCount == 0))
        return;

    float origZoom = m_fZoomFactor;

    CRect clientRect = GetClientRect();
    CSize preViewSize(max(CDPIAware::Instance().Scale(GetSafeHwnd(), REVGRAPH_PREVIEW_WIDTH), clientRect.Width() / 4), max(CDPIAware::Instance().Scale(GetSafeHwnd(), REVGRAPH_PREVIEW_HEIGHT), clientRect.Height() / 4));

    // zoom the graph so that it is completely visible in the window
    CRect graphRect = GetGraphRect();
    float horzFact  = static_cast<float>(graphRect.Width()) / static_cast<float>(preViewSize.cx);
    float vertFact  = static_cast<float>(graphRect.Height()) / static_cast<float>(preViewSize.cy);
    m_previewZoom   = min(DEFAULT_ZOOM, 1.0f / (max(horzFact, vertFact)));

    // make sure the preview window has a minimal size

    m_previewWidth  = min(static_cast<int>(max(graphRect.Width() * m_previewZoom, 30.0f)), static_cast<int>(preViewSize.cx));
    m_previewHeight = min(static_cast<int>(max(graphRect.Height() * m_previewZoom, 30.0f)), static_cast<int>(preViewSize.cy));

    CClientDC ddc(this);
    CDC       dc;
    if (!dc.CreateCompatibleDC(&ddc))
        return;

    m_preview.CreateCompatibleBitmap(&ddc, m_previewWidth, m_previewHeight);
    HBITMAP oldBm = static_cast<HBITMAP>(dc.SelectObject(m_preview));

    // paint the whole graph
    DoZoom(m_previewZoom, false);
    CRect          rect(0, 0, m_previewWidth, m_previewHeight);
    GraphicsDevice dev;
    dev.pDC = &dc;
    DrawGraph(dev, rect, 0, 0, true);

    // now we have a bitmap the size of the preview window
    dc.SelectObject(oldBm);
    dc.DeleteDC();

    DoZoom(origZoom, false);
}

void CRevisionGraphWnd::SetScrollbar(int bar, int newPos, int clientMax, int graphMax)
{
    SCROLLINFO scrollInfo = {sizeof(SCROLLINFO), SIF_ALL};
    GetScrollInfo(bar, &scrollInfo);

    clientMax     = max(1, clientMax);
    int oldHeight = scrollInfo.nMax <= 0 ? clientMax : scrollInfo.nMax;
    int newHeight = static_cast<int>(graphMax * m_fZoomFactor);
    int maxPos    = max(0, newHeight - clientMax);
    int pos       = min(maxPos, newPos >= 0
                                    ? newPos
                                    : scrollInfo.nPos * newHeight / oldHeight);

    scrollInfo.nPos      = pos;
    scrollInfo.nMin      = 0;
    scrollInfo.nMax      = newHeight;
    scrollInfo.nPage     = clientMax;
    scrollInfo.nTrackPos = pos;

    SetScrollInfo(bar, &scrollInfo);
}

void CRevisionGraphWnd::SetScrollbars(int nVert, int nHorz)
{
    CRect        clientRect = GetClientRect();
    const CRect& pRect      = GetGraphRect();

    SetScrollbar(SB_VERT, nVert, clientRect.Height(), pRect.Height());
    SetScrollbar(SB_HORZ, nHorz, clientRect.Width(), pRect.Width());
}

CRect CRevisionGraphWnd::GetGraphRect() const
{
    return m_state.GetGraphRect();
}

CRect CRevisionGraphWnd::GetClientRect() const
{
    CRect clientRect;
    CWnd::GetClientRect(&clientRect);
    return clientRect;
}

CRect CRevisionGraphWnd::GetWindowRect() const
{
    CRect windowRect;
    CWnd::GetWindowRect(&windowRect);
    return windowRect;
}

CRect CRevisionGraphWnd::GetViewRect() const
{
    CRect result;
    result.UnionRect(GetClientRect(), GetGraphRect());
    return result;
}

int CRevisionGraphWnd::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num  = 0; // number of image encoders
    UINT size = 0; // size of the image encoder array in bytes

    if (GetImageEncodersSize(&num, &size) != Ok)
        return -1;
    if (size == 0)
        return -1; // Failure

    ImageCodecInfo* pImageCodecInfo = static_cast<ImageCodecInfo*>(malloc(size));
    if (pImageCodecInfo == nullptr)
        return -1; // Failure

    if (GetImageEncoders(num, size, pImageCodecInfo) == Ok)
    {
        for (UINT j = 0; j < num; ++j)
        {
            if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
            {
                *pClsid = pImageCodecInfo[j].Clsid;
                free(pImageCodecInfo);
                return j; // Success
            }
        }
    }
    free(pImageCodecInfo);
    return -1; // Failure
}

bool CRevisionGraphWnd::FetchRevisionData(const CString& path, SVNRev pegRevision, CProgressDlg* progress, ITaskbarList3* pTaskbarList, HWND hWnd)
{
    // (re-)fetch the data
    SVN svn;
    if (svn.GetRepositoryRoot(CTSVNPath(path)) == svn.GetURLFromPath(CTSVNPath(path)))
    {
        m_state.SetLastErrorMessage(CString(MAKEINTRESOURCE(IDS_REVGRAPH_ERR_NOGRAPHFORROOT)));
        return false;
    }

    auto newFullHistory = std::make_unique<CFullHistory>();

    bool showWCRev          = m_state.GetOptions()->GetOption<CShowWC>()->IsSelected();
    bool showWCModification = m_state.GetOptions()->GetOption<CShowWCModification>()->IsSelected();
    bool result             = newFullHistory->FetchRevisionData(path,
                                                    pegRevision,
                                                    showWCRev,
                                                    showWCModification,
                                                    progress,
                                                    pTaskbarList,
                                                    hWnd);

    m_state.SetLastErrorMessage(newFullHistory->GetLastErrorMessage());

    if (result)
    {
        auto newFullGraph = std::make_unique<CFullGraph>();

        CFullGraphBuilder builder(*newFullHistory, *newFullGraph);
        builder.Run();

        CFullGraphFinalizer finalizer(*newFullHistory, *newFullGraph);
        finalizer.Run();

        m_state.SetQueryResult(newFullHistory, newFullGraph, showWCRev || showWCModification);
    }

    return result;
}

bool CRevisionGraphWnd::AnalyzeRevisionData()
{
    CSyncPointer<const CFullGraph> fullGraph(m_state.GetFullGraph());
    if ((fullGraph.get() != nullptr) && (fullGraph->GetNodeCount() > 0))
    {
        // filter graph

        CSyncPointer<CAllRevisionGraphOptions> options(m_state.GetOptions());
        options->Prepare();

        auto                 visibleGraph = std::make_unique<CVisibleGraph>();
        CVisibleGraphBuilder builder(*fullGraph, *visibleGraph, options->GetCopyFilterOptions());
        builder.Run();
        options->GetModificationOptions().Apply(visibleGraph.get());

        index_t index = 0;
        for (size_t i = 0, count = visibleGraph->GetRootCount(); i < count; ++i)
            index = visibleGraph->GetRoot(i)->InitIndex(index);

        // layout nodes

        auto newLayout = std::make_unique<CStandardLayout>(m_state.GetFullHistory()->GetCache(), visibleGraph.get(), m_state.GetFullHistory()->GetWCInfo());
        options->GetLayoutOptions().Apply(newLayout.get(), GetSafeHwnd());
        newLayout->Finalize();

        // switch state

        m_state.SetAnalysisResult(visibleGraph, newLayout);
    }

    return m_state.GetNodes().get() != nullptr;
}

bool CRevisionGraphWnd::IsUpdateJobRunning() const
{
    return (updateJob.get() != nullptr) && !updateJob->IsDone();
}

bool CRevisionGraphWnd::GetShowOverview() const
{
    return m_bShowOverview;
}

void CRevisionGraphWnd::SetShowOverview(bool value)
{
    m_bShowOverview = value;
    if (m_bShowOverview)
        BuildPreview();
}

void CRevisionGraphWnd::GetSelected(const CVisibleGraphNode* node, bool head, CTSVNPath& path, SVNRev& rev, SVNRev& peg) const
{
    CString repoRoot = m_state.GetRepositoryRoot();

    // get path and revision

    path.SetFromSVN(repoRoot + CUnicodeUtils::GetUnicode(node->GetPath().GetPath().c_str()));
    rev = head ? SVNRev::REV_HEAD : node->GetRevision();

    // handle 'modified WC' node

    if (node->GetClassification().Is(CNodeClassification::IS_MODIFIED_WC))
    {
        path.SetFromUnknown(m_sPath);
        rev = SVNRev::REV_WC;

        // don't set peg, if we aren't the first node
        // (i.e. would not be valid for node1)

        if (node == m_selectedEntry1)
            peg = SVNRev::REV_WC;
    }
    else
    {
        // set head, if still necessary

        if (head && !peg.IsValid())
            peg = node->GetRevision();
    }
}

void CRevisionGraphWnd::CompareRevs(bool bHead) const
{
    ASSERT(m_selectedEntry1 != NULL);
    ASSERT(m_selectedEntry2 != NULL);

    CSyncPointer<SVN> svn(m_state.GetSVN());

    CTSVNPath url1;
    CTSVNPath url2;
    SVNRev    rev1;
    SVNRev    rev2;
    SVNRev    peg;

    GetSelected(m_selectedEntry1, bHead, url1, rev1, peg);
    GetSelected(m_selectedEntry2, bHead, url2, rev2, peg);

    bool alternativeTool = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
    if (m_state.PromptShown())
    {
        SVNDiff diff(svn.get(), this->m_hWnd);
        diff.SetAlternativeTool(alternativeTool);
        diff.ShowCompare(url1, rev1, url2, rev2, peg, false, true, L"");
    }
    else
    {
        CAppUtils::StartShowCompare(m_hWnd, url1, rev1,
                                    url2, rev2, peg, SVNRev(), false, true, L"", alternativeTool);
    }
}

void CRevisionGraphWnd::UnifiedDiffRevs(bool bHead) const
{
    ASSERT(m_selectedEntry1 != NULL);
    ASSERT(m_selectedEntry2 != NULL);

    CSyncPointer<SVN> svn(m_state.GetSVN());

    CTSVNPath url1;
    CTSVNPath url2;
    SVNRev    rev1;
    SVNRev    rev2;
    SVNRev    peg;

    GetSelected(m_selectedEntry1, bHead, url1, rev1, peg);
    GetSelected(m_selectedEntry2, bHead, url2, rev2, peg);

    bool alternativeTool = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
    if (m_state.PromptShown())
    {
        SVNDiff diff(svn.get(), this->m_hWnd);
        diff.SetAlternativeTool(alternativeTool);
        diff.ShowUnifiedDiff(url1, rev1, url2, rev2, peg, true, L"", false, false, false);
    }
    else
    {
        CAppUtils::StartShowUnifiedDiff(m_hWnd, url1, rev1,
                                        url2, rev2, peg,
                                        SVNRev(), true, L"", alternativeTool, false, false, false);
    }
}

void CRevisionGraphWnd::DoZoom(float fZoomFactor, bool updateScrollbars)
{
    float oldzoom = m_fZoomFactor;
    m_fZoomFactor = fZoomFactor;

    m_nFontSize = max(1, static_cast<int>(DEFAULT_ZOOM_FONT * fZoomFactor));
    if (m_nFontSize < SMALL_ZOOM_FONT_THRESHOLD)
        m_nFontSize = min(static_cast<int>(SMALL_ZOOM_FONT_THRESHOLD), static_cast<int>(SMALL_ZOOM_FONT * fZoomFactor));

    for (int i = 0; i < MAXFONTS; i++)
    {
        if (m_apFonts[i] != nullptr)
        {
            m_apFonts[i]->DeleteObject();
            delete m_apFonts[i];
        }
        m_apFonts[i] = nullptr;
    }

    if (updateScrollbars)
    {
        SCROLLINFO si1 = {sizeof(SCROLLINFO), SIF_ALL};
        GetScrollInfo(SB_VERT, &si1);
        SCROLLINFO si2 = {sizeof(SCROLLINFO), SIF_ALL};
        GetScrollInfo(SB_HORZ, &si2);

        InitView();

        si1.nPos = static_cast<int>(static_cast<float>(si1.nPos) * m_fZoomFactor / oldzoom);
        si2.nPos = static_cast<int>(static_cast<float>(si2.nPos) * m_fZoomFactor / oldzoom);
        SetScrollPos(SB_VERT, si1.nPos);
        SetScrollPos(SB_HORZ, si2.nPos);
    }

    Invalidate(FALSE);
}
