// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2005 - Stefan Kueng

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
class CLineDiffBar : public CDialogBar
{
	DECLARE_DYNAMIC(CLineDiffBar)

public:
	CLineDiffBar();
	virtual ~CLineDiffBar();
	void			ShowLines(int nLineIndex);
	void			DocumentUpdated();

protected:
	afx_msg void	OnPaint();
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);

	virtual CSize	CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	//virtual CSize	CalcDynamicLayout(int nLength, DWORD nMode);

	void DrawInlineDiff(CDC &dc, const CRect *upperrect, const CRect *lowerrect, int line);

	CBitmap *		m_pCacheBitmap;

	int				m_nLineIndex;
	int				m_nLineHeight;
	int				m_nIgnoreWS;

	int				m_bBinaryDiff;

	DECLARE_MESSAGE_MAP()
public:
	CMainFrame *	m_pMainFrm;

private:
	const TCHAR *Search(const TCHAR *haystack, size_t haystacklen, 
						const TCHAR *needle, size_t needlelen, 
						size_t *max);
	void InLineDiff(CDWordArray & result, CString & base, CString & your);
	static COLORREF m_BinDiffColors[];
};


