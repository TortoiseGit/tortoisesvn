// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#pragma once
#include "StandAloneDlg.h"
#include "RevisionGraph.h"
#include "ProgressDlg.h"

enum NodeShape
{
	TSVNRectangle,
	TSVNRoundRect,
	TSVNOctangle
};
#define STARTPOINT_PT		(CPoint(5, 5))
#define SHADOW_OFFSET_PT	(CPoint(4, 4))
#define ROUND_RECT			(12)

#define RGB_DEF_SEL				RGB(160, 160, 160)
#define RGB_DEF_SHADOW			RGB(128, 128, 128)
#define RGB_DEF_HEADER			RGB(255, 0, 0)
#define RGB_DEF_TAG				RGB(0, 0, 0)
#define RGB_DEF_BRANCH			RGB(0, 0, 255)
#define RGB_DEF_NODE			RGB(0, 0, 255)

#define NODE_RECT_WIDTH			200
#define NODE_SPACE_LEFT			12
#define NODE_SPACE_RIGHT		100
#define NODE_SPACE_LINE			20
#define NODE_RECT_HEIGTH		60
#define NODE_SPACE_TOP			20
#define NODE_SPACE_BOTTOM		20

#define MAXFONTS				4
#define	MAX_TT_LENGTH			10000

class CRevisionGraphDlg : public CResizableStandAloneDialog, public CRevisionGraph //CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRevisionGraphDlg)
public:
	CRevisionGraphDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRevisionGraphDlg();
	enum { IDD = IDD_REVISIONGRAPH };


	CString			m_sPath;
	CProgressDlg	m_Progress;
	BOOL			m_bThreadRunning;

	void			InitView();
#ifdef DEBUG
	void			FillTestData();
#endif
protected:
	BOOL			m_bNoGraph;
	DWORD			m_dwTicks;
	CRect			m_ViewRect;
	CPtrArray		m_arConnections;
	CArray<CRect, CRect> m_arNodeList;
	CDWordArray		m_arNodeRevList;
	LONG			m_lSelectedRev;
	LOGFONT			m_lfBaseFont;
	CFont *			m_apFonts[MAXFONTS];
	int				m_nFontSize;
	CToolTipCtrl *	m_pDlgTip;
	char			m_szTip[MAX_TT_LENGTH+1];
	wchar_t			m_wszTip[MAX_TT_LENGTH+1];

	int				m_node_rect_width;
	int				m_node_space_left;
	int				m_node_space_right;
	int				m_node_space_line;
	int				m_node_rect_heigth;
	int				m_node_space_top;
	int				m_node_space_bottom;
	CPoint			m_RoundRectPt;
	int				m_nZoomFactor;
	
	HACCEL			m_hAccel;

	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	ProgressCallback(CString text, CString text2, DWORD done, DWORD total);
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void	OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg int		OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL	OnToolTipNotify(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnFileSavegraphas();
	afx_msg BOOL	OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void	OnViewZoomin();
	afx_msg void	OnViewZoomout();
	afx_msg void	OnMenuexit();
	afx_msg void	OnMenuhelp();

	DECLARE_MESSAGE_MAP()
private:
	INT_PTR			GetIndexOfRevision(LONG rev) const;
	void			SetScrollbars(int nVert = 0, int nHorz = 0);
	CRect *			GetViewSize();
	CFont*			GetFont(BOOL bItalic = FALSE, BOOL bBold = FALSE);

	void			DrawOctangle(CDC * pDC, const CRect& rect);
	void			DrawNode(CDC * pDC, const CRect& rect,
							COLORREF contour, CRevisionEntry *rentry,
							NodeShape shape, BOOL isSel, int penStyle = PS_SOLID);
	void			DrawGraph(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos);
	void			BuildConnections();
	void			DrawConnections(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos);
	int				GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	void			DoZoom(int nZoomFactor);
	static UINT		WorkerThread(LPVOID pVoid);


};
