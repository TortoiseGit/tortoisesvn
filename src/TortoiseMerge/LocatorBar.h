#pragma once

class CMainFrame;

/**
 * \ingroup TortoiseMerge
 *
 * A Toolbar showing the differences in the views. The Toolbar
 * is best attached to the left of the mainframe. The Toolbar
 * also scrolls the views to the location the user clicks
 * on the bar.
 */
class CLocatorBar : public CDialogBar
{
	DECLARE_DYNAMIC(CLocatorBar)

public:
	CLocatorBar();
	virtual ~CLocatorBar();

	void			DocumentUpdated();

protected:
	afx_msg void	OnPaint();
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);

	CBitmap *		m_pCacheBitmap;

	int				m_nLines;
	CDWordArray		m_arLeft;
	CDWordArray		m_arRight;
	CDWordArray		m_arBottom;

	DECLARE_MESSAGE_MAP()
public:
	CMainFrame *	m_pMainFrm;
};


