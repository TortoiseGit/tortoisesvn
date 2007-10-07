// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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

#include "StandAloneDlg.h"
#include "MyGraph.h"
#include "XPImageButton.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"

#include <map>
#include <list>

/**
 * \ingroup TortoiseProc
 * Helper class for drawing and then saving the drawing to a metafile (wmf)
 */
class CMyMetaFileDC : public CMetaFileDC
{
public:
	HGDIOBJ CMyMetaFileDC::SelectObject(HGDIOBJ hObject) 
	{
		return (hObject != NULL) ? ::SelectObject(m_hDC, hObject) : NULL; 
	}
};

/**
 * \ingroup TortoiseProc
 * Helper dialog showing statistics gathered from the log messages shown in the
 * log dialog.
 */
class CStatGraphDlg : public CResizableStandAloneDialog//CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CStatGraphDlg)

public:
	CStatGraphDlg(CWnd* pParent = NULL);
	virtual ~CStatGraphDlg();

	enum { IDD = IDD_STATGRAPH };

	// Data passed from the caller of the dialog.
	CDWordArray	*	m_parDates;
	CDWordArray	*	m_parFileChanges;
	CStringArray *	m_parAuthors;
	CTSVNPath		m_path;

protected:
	
	// ** protected data types **

	/// The types of units used in the various graphs.
	enum UnitType
	{
		Weeks,
		Months,
		Quarters,
		Years
	};

	typedef std::map<int, std::map<stdstring, LONG> >	WeeklyDataMap;
	typedef std::map<stdstring, LONG>					AuthorDataMap;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnCbnSelchangeGraphcombo();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedStacked();
	afx_msg void OnNeedText(NMHDR *pnmh, LRESULT *pResult);
	afx_msg void OnBnClickedGraphbarbutton();
	afx_msg void OnBnClickedGraphbarstackedbutton();
	afx_msg void OnBnClickedGraphlinebutton();
	afx_msg void OnBnClickedGraphlinestackedbutton();
	afx_msg void OnBnClickedGraphpiebutton();
	afx_msg void OnFileSavestatgraphas();

	DECLARE_MESSAGE_MAP()

	CPtrArray		m_graphDataArray;
	MyGraph			m_graph;
	CComboBox		m_cGraphType;
	CSliderCtrl		m_Skipper;
	BOOL			m_bIgnoreAuthorCase;

	CXPImageButton	m_btnGraphBar;
	CXPImageButton	m_btnGraphBarStacked;
	CXPImageButton	m_btnGraphLine;
	CXPImageButton	m_btnGraphLineStacked;
	CXPImageButton	m_btnGraphPie;

	HICON			m_hGraphBarIcon;
	HICON			m_hGraphBarStackedIcon;
	HICON			m_hGraphLineIcon;
	HICON			m_hGraphLineStackedIcon;
	HICON			m_hGraphPieIcon;

	MyGraph::GraphType	m_GraphType;
	bool				m_bStacked;

	/// Updates the variables m_weekCount and m_minDate and returns the number 
	/// of weeks in the revision interval.
	int			GetWeeksCount();
	int			m_weekCount;	///< Number of weeks in the revision interval.
	__time64_t	m_minDate;		///< The starting date/time for the revision interval.

	/// Returns the week-of-the-year for the given time.
	int			GetWeek(const CTime& time);

	/// Parses the data given to the dialog and generates mappings with statistical data. 
	void					GatherData();

	LONG					m_nTotalCommits;
	LONG					m_nTotalFileChanges;
	WeeklyDataMap			m_commitsPerWeekAndAuthor;
	WeeklyDataMap			m_filechangesPerWeekAndAuthor;
	AuthorDataMap			m_commitsPerAuthor;
	std::list<stdstring>	m_authorNames;

	/// Shows the graph with commit counts per author and date.
	void		ShowCommitsByDate();
	/// Shows the graph with commit counts per author.
	void		ShowCommitsByAuthor();
	/// Shows the initial statistics page.
	void		ShowStats();

	void		ShowLabels(BOOL bShow);
	/// Recalculates statistical data because the number and names of authors 
	/// can have changed, also calls RedrawGraph().
	void		IgnoreCaseChanged();
	/// Updates the currently shown statistics page.
	void		RedrawGraph();

	void					InitUnits();
	int						GetUnitCount();
	int						GetUnit(const CTime& time);
	CStatGraphDlg::UnitType	GetUnitType();
	CString					GetUnitString();
	CString					GetUnitLabel(int unit, CTime &lasttime);

	void		EnableDisableMenu();

	void		SaveGraph(CString sFilename);
	int			GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

	CToolTipCtrl* m_pToolTip;
};
