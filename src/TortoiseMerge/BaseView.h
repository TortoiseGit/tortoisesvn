

#pragma once

#include "DiffData.h"
#include "LocatorBar.h"



/**
 * \ingroup TortoiseMerge
 *
 * View class providing the basic functionality for
 * showing diffs. Has three parent classes which inherit
 * from this base class: CLeftView, CRightView and CBottomView.
 */
class CBaseView : public CView
{
public:
	CBaseView();
	virtual ~CBaseView();

public:
	/**
	 * Indicates that the underlying document has been updated. Reloads all
	 * data and redraws the view.
	 */
	virtual void	DocumentUpdated();
	/**
	 * Returns the number of lines visible on the view.
	 */
	int				GetScreenLines();
	/**
	 * Scrolls the view to the given line.
	 * \param nNewTopLine The new top line to scroll the view to
	 * \param bTrackScrollBar If TRUE, then the scrollbars are affected too.
	 */
	void			ScrollToLine(int nNewTopLine, BOOL bTrackScrollBar = TRUE);

	CStringArray *	m_arDiffLines;		///< Array of Strings containing all lines of the text file
	CDWordArray	*	m_arLineStates;		///< Array of Strings containing a diff state for each text line
	CString			m_sWindowName;		///< The name of the view which is shown as a window title to the user

	BOOL			m_bViewWhitespace;	///< If TRUE, then SPACE and TAB are shown as special characters
	int				m_nTopLine;			///< The topmost text line in the view

	static CLocatorBar * m_pwndLocator;	///< Pointer to the locator bar on the left
	static CStatusBar * m_pwndStatusBar;///< Pointer to the status bar

protected:
	virtual BOOL	PreCreateWindow(CREATESTRUCT& cs);
	virtual void	OnDraw(CDC * pDC);
	afx_msg void	OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void	OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnDestroy();
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL	OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void	OnKillFocus(CWnd* pNewWnd);
	afx_msg void	OnSetFocus(CWnd* pOldWnd);
	afx_msg void	OnContextMenu(CWnd* pWnd, CPoint point);

	DECLARE_MESSAGE_MAP()

protected:
	void			DrawSingleLine(CDC *pdc, const CRect &rc, int nLineIndex);
	void			DrawMargin(CDC *pdc, const CRect &rect, int nLineIndex);
	void			ExpandChars(LPCTSTR pszChars, int nOffset, int nCount, CString &line);

	BOOL			IsLineRemoved(int nLineIndex);

	void			RecalcVertScrollBar(BOOL bPositionOnly = FALSE);
	void			RecalcHorzScrollBar(BOOL bPositionOnly = FALSE);

	void			ScrollToChar(int nNewOffsetChar, BOOL bTrackScrollBar = TRUE);

	void			OnDoMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void			OnDoHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar, CBaseView * master);
	void			OnDoVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar, CBaseView * master);

	int				GetLineActualLength(int index);
	int				GetLineCount();
	void			CalcLineCharDim();
	int				GetLineHeight();
	int				GetCharWidth();
	int				GetMaxLineLength();
	int				GetLineLength(int index);
	int				GetScreenChars();
	LPCTSTR			GetLineChars(int index);
	CFont *			GetFont(BOOL bItalic = FALSE, BOOL bBold = FALSE, BOOL bStrikeOut = FALSE);
	int				GetLineFromPoint(CPoint point);

	virtual BOOL	IsStateSelectable(CDiffData::DiffStates state);
	virtual	void	OnContextMenu(CPoint point, int nLine);
	/**
	 * Updates the status bar pane. Call this if the document changed.
	 */
	void			UpdateStatusBar();
protected:
	UINT			m_nStatusBarID;		///< The ID of the status bar pane used by this view. Must be set by the parent class.

	int				m_nLineHeight;
	int				m_nCharWidth;
	int				m_nMaxLineLength;
	int				m_nScreenLines;
	int				m_nScreenChars;
	int				m_nOffsetChar;

	int				m_nSelBlockStart;
	int				m_nSelBlockEnd;

	HICON			m_hAddedIcon;
	HICON			m_hRemovedIcon;
	HICON			m_hConflictedIcon;

	LOGFONT			m_lfBaseFont;
	CFont *			m_apFonts[8];

	CBitmap *		m_pCacheBitmap;
	CDC *			m_pDC;

	// These three pointers lead to the three parent
	// classes CLeftView, CRightView and CBottomView
	// and are used for the communication between
	// the views (e.g. synchronized scrolling, ...)
	// To find out which parent class this object
	// is made of just compare e.g. (m_pwndLeft==this).
	static CBaseView * m_pwndLeft;		///< Pointer to the left view. Must be set by the CLeftView parent class.
	static CBaseView * m_pwndRight;		///< Pointer to the right view. Must be set by the CRightView parent class.
	static CBaseView * m_pwndBottom;	///< Pointer to the bottom view. Must be set by the CBottomView parent class.

};

