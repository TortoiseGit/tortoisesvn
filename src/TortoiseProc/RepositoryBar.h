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

#include "SVNUrl.h"
#include "HistoryCombo.h"

class CRepositoryTree;


/**
 * \ingroup TortoiseProc
 * Provides a CReBarCtrl which can be used as a "control center" for the
 * repository browser. To associate the repository bar with any repository
 * tree control, call AssocTree() once after both objects are created. As
 * long as they are associated, the bar and the tree form a "team" of
 * controls that work together.
 *
 * \par requirements
 * win98 or later, win95 with IE4
 * win2K or later, NT4 with IE4
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date MAR-2004
 *
 * \author Thomas Epting
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CRepositoryBar : public CReBarCtrl
{
	DECLARE_DYNAMIC(CRepositoryBar)

public:
	CRepositoryBar();
	virtual ~CRepositoryBar();

public:
	/**
	 * Creates the repository bar. Set \a in_dialog to TRUE when the bar
	 * is placed inside of a dialog. Otherwise it is assumed that the
	 * bar is placed in a frame window.
	 */
	bool Create(CWnd* parent, UINT id, bool in_dialog = true);

	/**
	 * Associates a repository tree with this bar. If another tree was
	 * previously associated, this connection is terminated.
	 */
	void AssocTree(CRepositoryTree *repo_tree);

	/**
	 * Show the given \a svn_url in the URL combo and the revision button.
	 */
	void ShowUrl(const SVNUrl& svn_url);

	/**
	 * Show the given \a svn_url in the URL combo and the revision button,
	 * and select it in the associated repository tree. If no \a svn_url
	 * is given, the current values are used (which effectively refreshes
	 * the tree).
	 */
	void GotoUrl(const SVNUrl& svn_url = SVNUrl());

	/**
	 * Returns the current URL.
	 */
	SVNUrl GetCurrentUrl() const;

	/**
	 * Saves the URL history of the HistoryCombo.
	 */
	void SaveHistory();
	
	void SetRevision(SVNRev rev);

protected:
	afx_msg void OnCbnSelEndOK();
	afx_msg void OnBnClicked();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()

private:
	CRepositoryTree	*m_pRepositoryTree;
	SVNUrl m_SvnUrl;

	class CRepositoryCombo : public CHistoryCombo {
		CRepositoryBar *m_bar;
	public:
		CRepositoryCombo(CRepositoryBar *bar) : m_bar(bar) { }
		virtual bool OnReturnKeyPressed();
	} m_cbxUrl;

	CButton m_btnRevision;
};


/**
 * \ingroup TortoiseProc
 * Implements a visual container for a CRepositoryBar which may be added to a
 * dialog. A CRepositoryBarCnr is not needed if the CRepositoryBar is attached
 * to a frame window.
 *
 * \par requirements
 * win98 or later, win95 with IE4
 * win2K or later, NT4 with IE4
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date MAR-2004
 *
 * \author Thomas Epting
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CRepositoryBarCnr : public CStatic
{
	DECLARE_DYNAMIC(CRepositoryBarCnr)

public:
	CRepositoryBarCnr(CRepositoryBar *repository_bar);
	~CRepositoryBarCnr();

	// Generated message map functions
protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	virtual void DrawItem(LPDRAWITEMSTRUCT);

	DECLARE_MESSAGE_MAP()

private:
	CRepositoryBar *m_pbarRepository;

};


