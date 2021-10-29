// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2014, 2017, 2020-2021 - TortoiseSVN

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
#include "RevisionGraph/RevisionGraphState.h"
#include "Future.h"
#include "Colors.h"
#include "SVNDiff.h"
#include "SVG.h"

using namespace Gdiplus;

enum
{
    REVGRAPH_PREVIEW_WIDTH  = 100,
    REVGRAPH_PREVIEW_HEIGHT = 200,

    // don't draw pre-views with more than that number of nodes

    REVGRAPH_PREVIEW_MAX_NODES = 10000
};

// don't try to draw nodes smaller than that:

#define REVGRAPH_MIN_NODE_HIGHT (0.5f)

// size of the node marker

constexpr int MARKER_SIZE = 11;

// radius of the rounded / slanted box corners  of the expand / collapse / split / join square gylphs

constexpr int CORNER_SIZE = 12;

// font sizes

constexpr int DEFAULT_ZOOM_FONT         = 9;  // default font size
constexpr int SMALL_ZOOM_FONT           = 11; // rel. larger font size for small zoom factors
constexpr int SMALL_ZOOM_FONT_THRESHOLD = 6;  // max. "small zoom" font size after scaling

// size of the expand / collapse / split / join square gylphs

constexpr int GLYPH_BITMAP_SIZE = 16;
constexpr int GLYPH_SIZE        = 12;

// glyph display delay definitions

constexpr int GLYPH_HOVER_EVENT = 10;  // timer ID for the glyph display delay
constexpr int GLYPH_HOVER_DELAY = 250; // delay until the glyphs are shown [ms]

// zoom control

const float MIN_ZOOM     = 0.01f;
const float MAX_ZOOM     = 2.0f;
const float DEFAULT_ZOOM = 1.0f;
const float ZOOM_STEP    = 0.9f;

// don't draw shadows below this zoom level

const float SHADOW_ZOOM_THRESHOLD = 0.2f;

/**
 * \ingroup TortoiseProc
 * node shapes for the revision graph
 */
enum NodeShape
{
    TSVNRectangle,
    TSVNRoundRect,
    TSVNOctangle,
    TSVNEllipse
};

#define MAXFONTS              4
#define MAX_TT_LENGTH         60000
#define MAX_TT_LENGTH_DEFAULT 1000

// forward declarations

class CRevisionGraphDlg;

// simplify usage of classes from other namespaces

using async::CFuture;
using async::IJob;

/**
 * \ingroup TortoiseProc
 * Window class showing a revision graph.
 *
 * The analyzation of the log data is done in the child class CRevisionGraph.
 * Here, we handle the window notifications.
 */
class CRevisionGraphWnd : public CWnd //, public CRevisionGraph
{
public:
    CRevisionGraphWnd(); // standard constructor
    ~CRevisionGraphWnd() override;
    enum
    {
        IDD                 = IDD_REVISIONGRAPH,
        WM_WORKERTHREADDONE = WM_APP + 1
    };

    CString m_sPath;
    SVNRev  m_pegRev;

    std::unique_ptr<CFuture<bool>> updateJob;
    CRevisionGraphState            m_state;

    void InitView();
    void Init(CWnd* pParent, LPRECT rect);
    void SaveGraphAs(CString sSavePath);

    bool FetchRevisionData(const CString& path, SVNRev pegRevision, CProgressDlg* progress, ITaskbarList3* pTaskbarList, HWND hWnd);
    bool AnalyzeRevisionData();
    bool IsUpdateJobRunning() const;

    bool GetShowOverview() const;
    void SetShowOverview(bool value);

    void GetSelected(const CVisibleGraphNode* node, bool head, CTSVNPath& path, SVNRev& rev, SVNRev& peg) const;
    void CompareRevs(bool bHead) const;
    void UnifiedDiffRevs(bool bHead) const;

    CRect GetGraphRect() const;
    CRect GetClientRect() const;
    CRect GetWindowRect() const;
    CRect GetViewRect() const;
    void  DoZoom(float nZoomFactor, bool updateScrollbars = true);
    bool  CancelMouseZoom();

    void SetDlgTitle(bool offline);

    void BuildPreview();
    void DeleteFonts();

protected:
    ULONGLONG m_ullTicks;
    CRect     m_overviewPosRect;
    CRect     m_overviewRect;

    bool m_bShowOverview;

    CRevisionGraphDlg* m_parent;

    const CVisibleGraphNode* m_selectedEntry1;
    const CVisibleGraphNode* m_selectedEntry2;
    LOGFONT                  m_lfBaseFont;
    CFont*                   m_apFonts[MAXFONTS];
    int                      m_nFontSize;
    CToolTipCtrl*            m_pDlgTip;
    char                     m_szTip[MAX_TT_LENGTH + 1];
    wchar_t                  m_wszTip[MAX_TT_LENGTH + 1];
    CString                  m_sTitle;

    float   m_fZoomFactor;
    CColors m_colors;
    bool    m_bTweakTrunkColors;
    bool    m_bTweakTagsColors;
    bool    m_bIsRubberBand;
    CPoint  m_ptRubberStart;
    CPoint  m_ptRubberEnd;

    CBitmap m_preview;
    int     m_previewWidth;
    int     m_previewHeight;
    float   m_previewZoom;

    index_t         m_hoverIndex;      // node the cursor currently hovers over
    DWORD           m_hoverGlyphs;     // the glyphs shown for \ref m_hoverIndex
    mutable index_t m_tooltipIndex;    // the node index we fetched the tooltip for
    bool            m_showHoverGlyphs; // if true, show the glyphs we currently hover over
                                       // (will be activated only after some delay)

    void            DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    virtual ULONG   GetGestureStatus(CPoint ptTouch) override;
    afx_msg void    OnPaint();
    afx_msg BOOL    OnEraseBkgnd(CDC* pDC);
    afx_msg void    OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void    OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void    OnSize(UINT nType, int cx, int cy);
    afx_msg INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTi) const override;
    afx_msg void    OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL    OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void    OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void    OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
    afx_msg void    OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void    OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnCaptureChanged(CWnd* pWnd);
    afx_msg LRESULT OnWorkerThreadDone(WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()
private:
    enum MarkerPosition
    {
        MpLeft  = 0,
        MpRight = 1,
    };

    enum GlyphType
    {
        NoGlyph       = -1,
        ExpandGlyph   = 0, // "+"
        CollapseGlyph = 1, // "-"
        SplitGlyph    = 2, // "x"
        JoinGlyph     = 3, // "o"
    };

    enum GlyphPosition
    {
        Above = 0,
        Right = 4,
        Below = 8,
    };

    class GraphicsDevice
    {
    public:
        GraphicsDevice()
            : pDC(nullptr)
            , graphics(nullptr)
            , pSvg(nullptr)
        {
        }
        ~GraphicsDevice() {}

    public:
        CDC*      pDC;
        Graphics* graphics;
        SVG*      pSvg;
    };

    class SvgGrouper
    {
    public:
        SvgGrouper(SVG* pSvg)
        {
            m_pSvg = pSvg;
            if (m_pSvg)
                m_pSvg->StartGroup();
        }
        ~SvgGrouper()
        {
            if (m_pSvg)
                m_pSvg->EndGroup();
        }

    private:
        SvgGrouper() = delete;

        SVG* m_pSvg;
    };

    bool        UpdateSelectedEntry(const CVisibleGraphNode* clickedentry);
    static void AppendMenu(CMenu& popup, UINT title, UINT command, UINT flags = MF_ENABLED);
    void        AddSVNOps(CMenu& popup) const;
    void        AddGraphOps(CMenu& popup, const CVisibleGraphNode* node);
    CString     GetSelectedURL() const;
    CString     GetWCURL() const;
    void        DoShowLog() const;
    void        DoCheckout() const;
    void        DoCheckForModification() const;
    void        DoMergeTo() const;
    void        DoUpdate() const;
    void        DoSwitch() const;
    void        DoSwitchToHead() const;
    void        DoBrowseRepo() const;
    void        ResetNodeFlags(DWORD flags);
    void        ToggleNodeFlag(const CVisibleGraphNode* node, DWORD flag);
    void        DoCopyUrl() const;

    void   SetScrollbar(int bar, int newPos, int clientMax, int graphMax);
    void   SetScrollbars(int nVert = -1, int nHorz = -1);
    CFont* GetFont(BOOL bItalic = FALSE, BOOL bBold = FALSE);

    CSize   UsableTooltipRect() const;
    CString DisplayableText(const CString& wholeText, const CSize& tooltipSize);
    CString TooltipText(index_t index) const;

    CPoint                                    GetLogCoordinates(CPoint point) const;
    index_t                                   GetHitNode(CPoint point, CSize border = CSize(0, 0)) const;
    DWORD                                     GetHoverGlyphs(CPoint point) const;
    const CRevisionGraphState::SVisibleGlyph* GetHitGlyph(CPoint point) const;

    void ClearVisibleGlyphs(const CRect& rect);

    using TCutRectangle = PointF[8];
    static void CutawayPoints(const RectF& rect, float cutLen, TCutRectangle& result);
    void        DrawRoundedRect(GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect) const;
    void        DrawOctangle(GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect) const;
    void        DrawShape(GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect, NodeShape shape) const;
    void        DrawShadow(GraphicsDevice& graphics, const RectF& rect,
                           Color shadowColor, NodeShape shape) const;
    void        DrawNode(GraphicsDevice& graphics, const RectF& rect,
                         Color contour, Color overlayColor,
                         const CVisibleGraphNode* node, NodeShape shape);
    RectF       TransformRectToScreen(const CRect& rect, const CSize& offset) const;
    RectF       GetNodeRect(const ILayoutNodeList::SNode& node, const CSize& offset) const;
    RectF       GetBranchCover(const ILayoutNodeList* nodeList, index_t nodeIndex, bool upward, const CSize& offset) const;

    void DrawSquare(GraphicsDevice& graphics, const PointF& leftTop,
                    const Color& lightColor, const Color& darkColor, const Color& penColor) const;
    void DrawGlyph(GraphicsDevice& graphics, Image* glyphs, const PointF& leftTop,
                   GlyphType glyph, GlyphPosition position) const;
    void DrawGlyphs(GraphicsDevice& graphics, Image* glyphs, const CVisibleGraphNode* node, const PointF& center,
                    GlyphType glyph1, GlyphType glyph2, GlyphPosition position, DWORD state1, DWORD state2, bool showAll);
    void DrawGlyphs(GraphicsDevice& graphics, Image* glyphs, const CVisibleGraphNode* node, const RectF& nodeRect,
                    DWORD state, DWORD allowed, bool upsideDown);
    void DrawMarker(GraphicsDevice& graphics, const RectF& noderect, MarkerPosition position, int relPosition, int colorIndex);
    void IndicateGlyphDirection(GraphicsDevice& graphics, const ILayoutNodeList* nodeList, const ILayoutNodeList::SNode& node, const RectF& nodeRect, DWORD glyphs, bool upsideDown, const CSize& offset) const;

    void DrawStripes(GraphicsDevice& graphics, const CSize& offset);

    void DrawShadows(GraphicsDevice& graphics, const CRect& logRect, const CSize& offset) const;
    void DrawNodes(GraphicsDevice& graphics, Image* glyphs, const CRect& logRect, const CSize& offset);
    void DrawConnections(GraphicsDevice& graphics, const CRect& logRect, const CSize& offset) const;
    void DrawTexts(GraphicsDevice& graphics, const CRect& logRect, const CSize& offset);
    void DrawCurrentNodeGlyphs(GraphicsDevice& graphics, Image* glyphs, const CSize& offset);
    void DrawGraph(GraphicsDevice& graphics, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw);

    static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
    void       DrawRubberBand();
};
