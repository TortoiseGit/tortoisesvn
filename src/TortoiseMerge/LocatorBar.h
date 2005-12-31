// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
	afx_msg void	OnMouseMove(UINT nFlags, CPoint point);
	LRESULT			OnMouseLeave(WPARAM, LPARAM);

	CBitmap *		m_pCacheBitmap;

	int				m_nLines;
	CPoint			m_MousePos;
	BOOL			m_bMouseWithin;
	BOOL			m_bUseMagnifier;
	CDWordArray		m_arLeft;
	CDWordArray		m_arRight;
	CDWordArray		m_arBottom;

	DECLARE_MESSAGE_MAP()
public:
	CMainFrame *	m_pMainFrm;
};


