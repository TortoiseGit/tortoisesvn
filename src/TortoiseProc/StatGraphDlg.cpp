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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "StatGraphDlg.h"
#include "gdiplus.h"
#include "StringUtils.h"
#include "PathUtils.h"
#include "MemDC.h"
#include "MessageBox.h"
#include "Registry.h"

#include <cmath>
#include <locale>
#include <list>
#include <utility>

using namespace Gdiplus;

// BinaryPredicate for comparing authors based on their commit count
class MoreCommitsThan : public std::binary_function<stdstring, stdstring, bool> {
public:
	MoreCommitsThan(std::map<stdstring, LONG> * author_commits) : m_authorCommits(author_commits) {}

	bool operator()(const stdstring& lhs, const stdstring& rhs) {
		return (*m_authorCommits)[lhs] > (*m_authorCommits)[rhs];
	}

private:
	std::map<stdstring, LONG> * m_authorCommits;
};


IMPLEMENT_DYNAMIC(CStatGraphDlg, CResizableStandAloneDialog)
CStatGraphDlg::CStatGraphDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CStatGraphDlg::IDD, pParent)
	, m_bStacked(FALSE)
	, m_GraphType(MyGraph::Bar)
	, m_bAuthorsCaseSensitive(TRUE)
	, m_bSortByCommitCount(TRUE)
	, m_nWeeks(-1)
{
	m_parDates = NULL;
	m_parFileChanges = NULL;
	m_parAuthors = NULL;
	m_pToolTip = NULL;
}

CStatGraphDlg::~CStatGraphDlg()
{
	ClearGraph();
	DestroyIcon(m_hGraphBarIcon);
	DestroyIcon(m_hGraphBarStackedIcon);
	DestroyIcon(m_hGraphLineIcon);
	DestroyIcon(m_hGraphLineStackedIcon);
	DestroyIcon(m_hGraphPieIcon);
	delete m_pToolTip;
}

void CStatGraphDlg::OnOK() {
	StoreCurrentGraphType();
	__super::OnOK();
}

void CStatGraphDlg::OnCancel() {
	StoreCurrentGraphType();
	__super::OnCancel();
}

void CStatGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GRAPH, m_graph);
	DDX_Control(pDX, IDC_GRAPHCOMBO, m_cGraphType);
	DDX_Control(pDX, IDC_SKIPPER, m_Skipper);
	DDX_Check(pDX, IDC_AUTHORSCASESENSITIVE, m_bAuthorsCaseSensitive);
	DDX_Check(pDX, IDC_SORTBYCOMMITCOUNT, m_bSortByCommitCount);
	DDX_Control(pDX, IDC_GRAPHBARBUTTON, m_btnGraphBar);
	DDX_Control(pDX, IDC_GRAPHBARSTACKEDBUTTON, m_btnGraphBarStacked);
	DDX_Control(pDX, IDC_GRAPHLINEBUTTON, m_btnGraphLine);
	DDX_Control(pDX, IDC_GRAPHLINESTACKEDBUTTON, m_btnGraphLineStacked);
	DDX_Control(pDX, IDC_GRAPHPIEBUTTON, m_btnGraphPie);
}


BEGIN_MESSAGE_MAP(CStatGraphDlg, CResizableStandAloneDialog)
	ON_CBN_SELCHANGE(IDC_GRAPHCOMBO, OnCbnSelchangeGraphcombo)
	ON_WM_HSCROLL()
	ON_NOTIFY(TTN_NEEDTEXT, NULL, OnNeedText)
	ON_BN_CLICKED(IDC_AUTHORSCASESENSITIVE, &CStatGraphDlg::AuthorsCaseSensitiveChanged)
	ON_BN_CLICKED(IDC_SORTBYCOMMITCOUNT, &CStatGraphDlg::SortModeChanged)
	ON_BN_CLICKED(IDC_GRAPHBARBUTTON, &CStatGraphDlg::OnBnClickedGraphbarbutton)
	ON_BN_CLICKED(IDC_GRAPHBARSTACKEDBUTTON, &CStatGraphDlg::OnBnClickedGraphbarstackedbutton)
	ON_BN_CLICKED(IDC_GRAPHLINEBUTTON, &CStatGraphDlg::OnBnClickedGraphlinebutton)
	ON_BN_CLICKED(IDC_GRAPHLINESTACKEDBUTTON, &CStatGraphDlg::OnBnClickedGraphlinestackedbutton)
	ON_BN_CLICKED(IDC_GRAPHPIEBUTTON, &CStatGraphDlg::OnBnClickedGraphpiebutton)
	ON_COMMAND(ID_FILE_SAVESTATGRAPHAS, &CStatGraphDlg::OnFileSavestatgraphas)
END_MESSAGE_MAP()

BOOL CStatGraphDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_pToolTip = new CToolTipCtrl;
	if (m_pToolTip->Create(this))
	{
		m_pToolTip->AddTool(&m_btnGraphPie, IDS_STATGRAPH_PIEBUTTON_TT);
		m_pToolTip->AddTool(&m_btnGraphLineStacked, IDS_STATGRAPH_LINESTACKEDBUTTON_TT);
		m_pToolTip->AddTool(&m_btnGraphLine, IDS_STATGRAPH_LINEBUTTON_TT);
		m_pToolTip->AddTool(&m_btnGraphBarStacked, IDS_STATGRAPH_BARSTACKEDBUTTON_TT);
		m_pToolTip->AddTool(&m_btnGraphBar, IDS_STATGRAPH_BARBUTTON_TT);

		m_pToolTip->Activate(TRUE);
	}
	
	CString temp;
	int sel = 0;
	temp.LoadString(IDS_STATGRAPH_STATS);
	sel = m_cGraphType.AddString(temp);
	m_cGraphType.SetItemData(sel, 1);
	m_cGraphType.SetCurSel(sel);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATE);
	sel = m_cGraphType.AddString(temp);
	m_cGraphType.SetItemData(sel, 2);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHOR);
	sel = m_cGraphType.AddString(temp);
	m_cGraphType.SetItemData(sel, 3);

	// set the dialog title to "Statistics - path/to/whatever/we/show/the/statistics/for"
	CString sTitle;
	GetWindowText(sTitle);
	if(m_path.IsDirectory())
		SetWindowText(sTitle + _T(" - ") + m_path.GetWinPathString());
	else
		SetWindowText(sTitle + _T(" - ") + m_path.GetFilename());

	m_hGraphBarIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_GRAPHBAR), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hGraphBarStackedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_GRAPHBARSTACKED), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hGraphLineIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_GRAPHLINE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hGraphLineStackedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_GRAPHLINESTACKED), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hGraphPieIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_GRAPHPIE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	m_btnGraphBar.SetIcon(m_hGraphBarIcon);
	m_btnGraphBar.SetButtonStyle(WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON);
	m_btnGraphBarStacked.SetIcon(m_hGraphBarStackedIcon);
	m_btnGraphBarStacked.SetButtonStyle(WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON);
	m_btnGraphLine.SetIcon(m_hGraphLineIcon);
	m_btnGraphLine.SetButtonStyle(WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON);
	m_btnGraphLineStacked.SetIcon(m_hGraphLineStackedIcon);
	m_btnGraphLineStacked.SetButtonStyle(WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON);
	m_btnGraphPie.SetIcon(m_hGraphPieIcon);
	m_btnGraphPie.SetButtonStyle(WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON);

	AddAnchor(IDC_GRAPHTYPELABEL, TOP_LEFT);
	AddAnchor(IDC_GRAPH, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHCOMBO, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_NUMWEEK, TOP_LEFT);
	AddAnchor(IDC_NUMWEEKVALUE, TOP_RIGHT);
	AddAnchor(IDC_NUMAUTHOR, TOP_LEFT);
	AddAnchor(IDC_NUMAUTHORVALUE, TOP_RIGHT);
	AddAnchor(IDC_NUMCOMMITS, TOP_LEFT);
	AddAnchor(IDC_NUMCOMMITSVALUE, TOP_RIGHT);
	AddAnchor(IDC_NUMFILECHANGES, TOP_LEFT);
	AddAnchor(IDC_NUMFILECHANGESVALUE, TOP_RIGHT);

	AddAnchor(IDC_AVG, TOP_RIGHT);
	AddAnchor(IDC_MIN, TOP_RIGHT);
	AddAnchor(IDC_MAX, TOP_RIGHT);
	AddAnchor(IDC_COMMITSEACHWEEK, TOP_LEFT);
	AddAnchor(IDC_MOSTACTIVEAUTHOR, TOP_LEFT);
	AddAnchor(IDC_LEASTACTIVEAUTHOR, TOP_LEFT);
	AddAnchor(IDC_MOSTACTIVEAUTHORNAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LEASTACTIVEAUTHORNAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FILECHANGESEACHWEEK, TOP_LEFT);
	AddAnchor(IDC_COMMITSEACHWEEKAVG, TOP_RIGHT);
	AddAnchor(IDC_COMMITSEACHWEEKMIN, TOP_RIGHT);
	AddAnchor(IDC_COMMITSEACHWEEKMAX, TOP_RIGHT);
	AddAnchor(IDC_MOSTACTIVEAUTHORAVG, TOP_RIGHT);
	AddAnchor(IDC_MOSTACTIVEAUTHORMIN, TOP_RIGHT);
	AddAnchor(IDC_MOSTACTIVEAUTHORMAX, TOP_RIGHT);
	AddAnchor(IDC_LEASTACTIVEAUTHORAVG, TOP_RIGHT);
	AddAnchor(IDC_LEASTACTIVEAUTHORMIN, TOP_RIGHT);
	AddAnchor(IDC_LEASTACTIVEAUTHORMAX, TOP_RIGHT);
	AddAnchor(IDC_FILECHANGESEACHWEEKAVG, TOP_RIGHT);
	AddAnchor(IDC_FILECHANGESEACHWEEKMIN, TOP_RIGHT);
	AddAnchor(IDC_FILECHANGESEACHWEEKMAX, TOP_RIGHT);

	AddAnchor(IDC_GRAPHBARBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHBARSTACKEDBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHLINEBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHLINESTACKEDBUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHPIEBUTTON, BOTTOM_RIGHT);

	AddAnchor(IDC_AUTHORSCASESENSITIVE, BOTTOM_LEFT);
	AddAnchor(IDC_SORTBYCOMMITCOUNT, BOTTOM_LEFT);
	AddAnchor(IDC_SKIPPER, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SKIPPERLABEL, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	EnableSaveRestore(_T("StatGraphDlg"));

	// gather statistics data, only needs to be updated when the checkbox with 
	// the case sensitivity of author names is changed
	GatherData();

	// set the min/max values on the skipper
	int max_authors_count = max(1, min(m_authorNames.size(),100) );
	// TODO : limit the max count based on the resolution, for now we use 100
	m_Skipper.SetRange(1, max_authors_count );
	m_Skipper.SetPos( min(max_authors_count, 10) );
	m_Skipper.SetPageSize(5);

	// we use a stats page encoding here, 0 stands for the statistics dialog
	CRegDWORD lastStatsPage = CRegDWORD(_T("Software\\TortoiseSVN\\LastViewedStatsPage"), 0);

	// open last viewed statistics page as first page
	int graphtype = lastStatsPage / 10;
	graphtype = max(1, min(3, graphtype));
	m_cGraphType.SetCurSel(graphtype-1);

	OnCbnSelchangeGraphcombo();

	int statspage = lastStatsPage % 10;
	switch (statspage) {
		case 1 : 
			m_GraphType = MyGraph::Bar;
			m_bStacked = true;
			break;
		case 2 : 
			m_GraphType = MyGraph::Bar;
			m_bStacked = false;
			break;
		case 3 : 
			m_GraphType = MyGraph::Line;
			m_bStacked = true;
			break;
		case 4 : 
			m_GraphType = MyGraph::Line;
			m_bStacked = false;
			break;
		case 5 : 
			m_GraphType = MyGraph::PieChart;
			break;

		default : return TRUE;
	}
	RedrawGraph();

	return TRUE;
}

void CStatGraphDlg::ShowLabels(BOOL bShow)
{
	if ((m_parAuthors==NULL)||(m_parDates==NULL)||(m_parFileChanges==NULL))
		return;
	int nCmdShow = SW_SHOW;
	if (!bShow)
		nCmdShow = SW_HIDE;
	GetDlgItem(IDC_GRAPH)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_NUMWEEK)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMWEEKVALUE)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMAUTHOR)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMAUTHORVALUE)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMCOMMITS)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMCOMMITSVALUE)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMFILECHANGES)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_NUMFILECHANGESVALUE)->ShowWindow(nCmdShow);

	GetDlgItem(IDC_AVG)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MIN)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MAX)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_COMMITSEACHWEEK)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MOSTACTIVEAUTHOR)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_LEASTACTIVEAUTHOR)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MOSTACTIVEAUTHORNAME)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_LEASTACTIVEAUTHORNAME)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_FILECHANGESEACHWEEK)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_COMMITSEACHWEEKAVG)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_COMMITSEACHWEEKMIN)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_COMMITSEACHWEEKMAX)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MOSTACTIVEAUTHORAVG)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MOSTACTIVEAUTHORMIN)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_MOSTACTIVEAUTHORMAX)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_LEASTACTIVEAUTHORAVG)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_LEASTACTIVEAUTHORMIN)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_LEASTACTIVEAUTHORMAX)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_FILECHANGESEACHWEEKAVG)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_FILECHANGESEACHWEEKMIN)->ShowWindow(nCmdShow);
	GetDlgItem(IDC_FILECHANGESEACHWEEKMAX)->ShowWindow(nCmdShow);

	GetDlgItem(IDC_SORTBYCOMMITCOUNT)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_SKIPPER)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_SKIPPERLABEL)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphBar.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphBarStacked.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphLine.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphLineStacked.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphPie.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
}

void CStatGraphDlg::UpdateWeekCount()
{
	// Sanity check
	if (!m_parDates)
	{
		return;
	}
	// Already updated? No need to do it again.
	if (m_nWeeks >= 0)
		return;

	// Determine first and last date in dates array
	__time64_t min_date = (__time64_t)m_parDates->GetAt(0);
	__time64_t max_date = min_date;
	unsigned int count = m_parDates->GetCount();
	for (unsigned int i=0; i<count; ++i) 
	{
		__time64_t d = (__time64_t)m_parDates->GetAt(i);
		if (d < min_date)		min_date = d;
		else if (d > max_date)	max_date = d;
	}

	// Here we must round the min_data to the first day of the week
	// to align the interval min_data ... max_date with the week start
	TCHAR loc[2];
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, loc, sizeof(loc));
	int iFirstDayOfWeek = int(loc[0]-'0');
	CTime start_time(min_date);
	int iDayOfWeek = (start_time.GetDayOfWeek()+iFirstDayOfWeek)%7;
	start_time -= CTimeSpan(iDayOfWeek,0,0,0);
	// Store start date of the interval in the member variable m_minDate
	m_minDate = start_time.GetTime();
	
	// How many weeks does the time period cover?

	// Get time difference between start and end date
	double secs = _difftime64(max_date, m_minDate);
	// ... a week has 604800 seconds
	m_nWeeks = (int)ceil(secs / 604800.0);
}

int CStatGraphDlg::GetWeek(const CTime& time)
{
	int iWeekOfYear = 0;

	int iYear = time.GetYear();
	int iFirstDayOfWeek = 0;
	int iFirstWeekOfYear = 0;
	TCHAR loc[2];
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, loc, sizeof(loc));
	iFirstDayOfWeek = int(loc[0]-'0');
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTWEEKOFYEAR, loc, sizeof(loc));
	iFirstWeekOfYear = int(loc[0]-'0');
	CTime dDateFirstJanuary(iYear,1,1,12,1,1);
	int iDayOfWeek = (dDateFirstJanuary.GetDayOfWeek()+5+iFirstDayOfWeek)%7;

	// Select mode
	// 0 Week containing 1/1 is the first week of that year.
	// 1 First full week following 1/1 is the first week of that year.
	// 2 First week containing at least four days is the first week of that year.
	switch (iFirstWeekOfYear)
	{
	case 0:
		{
			// Week containing 1/1 is the first week of that year.

			// check if this week reaches into the next year
			dDateFirstJanuary = CTime(iYear+1,1,1,0,0,0);

			// Get start of week
			iDayOfWeek = (time.GetDayOfWeek()+5+iFirstDayOfWeek)%7;
			CTime dStartOfWeek = time-CTimeSpan(iDayOfWeek,0,0,0);

			// If this week spans over to 1/1 this is week 1
			if (dStartOfWeek+CTimeSpan(6,0,0,0)>=dDateFirstJanuary)
			{
				// we are in the last week of the year that spans over 1/1
				iWeekOfYear = 1;
			}
			else
			{
				// Get week day of 1/1
				dDateFirstJanuary = CTime(iYear,1,1,0,0,0);
				iDayOfWeek = (dDateFirstJanuary.GetDayOfWeek()+5+iFirstDayOfWeek)%7;
				// Just count from 1/1
				iWeekOfYear = (int)(((time-dDateFirstJanuary).GetDays()+iDayOfWeek) / 7) + 1;
			}
		}
		break;
	case 1:
		{
			// First full week following 1/1 is the first week of that year.

			// If the 1.1 is the start of the week everything is ok
			// else we need the next week is the correct result
			iWeekOfYear =
				(int)(((time-dDateFirstJanuary).GetDays()+iDayOfWeek) / 7) +
				(iDayOfWeek==0 ? 1:0);

			// If we are in week 0 we are in the first not full week
			// calculate from the last year
			if (iWeekOfYear==0)
			{
				// Special case: we are in the week of 1.1 but 1.1. is not on the
				// start of week. Calculate based on the last year
				dDateFirstJanuary = CTime(iYear-1,1,1,0,0,0);
				iDayOfWeek =
					(dDateFirstJanuary.GetDayOfWeek()+5+iFirstDayOfWeek)%7;
				// and we correct this in the same we we done this before but
				// the result is now 52 or 53 and not 0
				iWeekOfYear =
					(int)(((time-dDateFirstJanuary).GetDays()+iDayOfWeek) / 7) +
					(iDayOfWeek<=3 ? 1:0);
			}
		}
		break;
	case 2:
		{
			// First week containing at least four days is the first week of that year.

			// Each year can start with any day of the week. But our
			// weeks always start with Monday. So we add the day of week
			// before calculation of the final week of year.
			// Rule: is the 1.1 a Mo,Tu,We,Th than the week starts on the 1.1 with
			// week==1, else a week later, so we add one for all those days if
			// day is less <=3 Mo,Tu,We,Th. Otherwise 1.1 is in the last week of the
			// previous year
			iWeekOfYear =
				(int)(((time-dDateFirstJanuary).GetDays()+iDayOfWeek) / 7) +
				(iDayOfWeek<=3 ? 1:0);

			// special cases
			if (iWeekOfYear==0)
			{
				// special case week 0. We got a day before the 1.1, 2.1 or 3.1, were the
				// 1.1. is not a Mo, Tu, We, Th. So the week 1 does not start with the 1.1.
				// So we calculate the week according to the 1.1 of the year before

				dDateFirstJanuary = CTime(iYear-1,1,1,0,0,0);
				iDayOfWeek =
					(dDateFirstJanuary.GetDayOfWeek()+5+iFirstDayOfWeek)%7;
				// and we correct this in the same we we done this before but the result
				// is now 52 or 53 and not 0
				iWeekOfYear =
					(int)(((time-dDateFirstJanuary).GetDays()+iDayOfWeek) / 7) +
					(iDayOfWeek<=3 ? 1:0);
			}
			else if (iWeekOfYear==53)
			{
				// special case week 53. Either we got the correct week 53 or we just got the
				// week 1 of the next year. So is the 1.1.(year+1) also a Mo, Tu, We, Th than
				// we already have the week 1, otherwise week 53 is correct

				dDateFirstJanuary = CTime(iYear+1,1,1,0,0,0);
				iDayOfWeek =
					(dDateFirstJanuary.GetDayOfWeek()+5+iFirstDayOfWeek)%7;
				// 1.1. in week 1 or week 53?
				iWeekOfYear = iDayOfWeek<=3 ? 1:53;
			}
		}
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	// return result
	return iWeekOfYear;
}

void CStatGraphDlg::GatherData() {
	// Sanity check
	if ((m_parAuthors==NULL)||(m_parDates==NULL)||(m_parFileChanges==NULL))
		return;
	m_nTotalCommits = m_parAuthors->GetCount();
	m_nTotalFileChanges = 0;

	// Update m_nWeeks and m_minDate
	UpdateWeekCount();

	// Now create a mapping that holds the information per week.
	m_commitsPerWeekAndAuthor.clear();
	m_filechangesPerWeekAndAuthor.clear();
	m_commitsPerAuthor.clear();

	LONG timeIntervalLength = 604800; // we define the interval to be one week

	// Now loop over all weeks and gather the info
	for (LONG i=0; i<m_nTotalCommits; ++i) 
	{
		// Find the interval number
		__time64_t commitDate = (__time64_t)m_parDates->GetAt(i);
		double secsSinceMinDate = _difftime64(commitDate, m_minDate);
		int interval = (int)floor(secsSinceMinDate/timeIntervalLength);
		// Find the authors name
		CString sAuth = m_parAuthors->GetAt(i);
		if (!m_bAuthorsCaseSensitive)
			sAuth = sAuth.MakeLower();
		stdstring author = stdstring(sAuth);
		// Increase total commit count for this author
		m_commitsPerAuthor[author]++;
		// Increase the commit count for this author in this week
		m_commitsPerWeekAndAuthor[interval][author]++;
		// Increase the filechange count for this author in this week
		int fileChanges = m_parFileChanges->GetAt(i);
		m_filechangesPerWeekAndAuthor[interval][author] += fileChanges;
		m_nTotalFileChanges += fileChanges;
	}

	// Find first and last interval number.
	if (!m_commitsPerWeekAndAuthor.empty()) 
	{
		IntervalDataMap::iterator interval_it = m_commitsPerWeekAndAuthor.begin();
		m_firstInterval = interval_it->first;
		interval_it = m_commitsPerWeekAndAuthor.end();
		--interval_it;
		m_lastInterval = interval_it->first;
		// Sanity check - if m_lastInterval is too large it could freeze TSVN and take up all memory!!!
		assert(m_lastInterval >= 0 && m_lastInterval < 10000); 
	}
	else
	{
		m_firstInterval = 0;
		m_lastInterval = -1;
	}

	// Get a list of authors names
	m_authorNames.clear();
	for (std::map<stdstring, LONG>::iterator it = m_commitsPerAuthor.begin(); 
		it != m_commitsPerAuthor.end(); ++it) 
	{
		m_authorNames.push_back(it->first);
	}

	// Sort the list of authors based on commit count
	m_authorNames.sort( MoreCommitsThan(&m_commitsPerAuthor) );

	// All done, now the statistics pages can retrieve the data and 
	// extract the information to be shown.
}

void CStatGraphDlg::FilterSkippedAuthors(std::list<stdstring>& included_authors, 
										 std::list<stdstring>& skipped_authors)
{
	included_authors.clear();
	skipped_authors.clear();

	unsigned int included_authors_count = m_Skipper.GetPos();
	// if we only leave out one author, still include him with his name
	if (included_authors_count + 1 == m_authorNames.size()) 
		++included_authors_count;

	// add the included authors first
	std::list<stdstring>::iterator author_it = m_authorNames.begin();
	while (included_authors_count > 0 && author_it != m_authorNames.end()) {
		// Add him/her to the included list
		included_authors.push_back(*author_it);
		++author_it;
		--included_authors_count;
	}

	// If we haven't reached the end yet, copy all remaining authors into the
	// skipped author list.
	std::copy(author_it, m_authorNames.end(), std::back_inserter(skipped_authors) );

	// Sort authors alphabetically if user wants that.
	if (!m_bSortByCommitCount) {
		included_authors.sort();
	}
}

void CStatGraphDlg::ShowCommitsByAuthor()
{
	if ((m_parAuthors==NULL)||(m_parDates==NULL)||(m_parFileChanges==NULL))
		return;
	ShowLabels(FALSE);
	ClearGraph();

	// This function relies on a previous call of GatherData(). 
	// This can be detected by checking the weekcount. 
	// If the weekcount is equal to -1, it hasn't been called before.
	if (m_nWeeks == -1)
		GatherData();
	// If week count is still -1, something bad has happened, probably invalid data!
	if (m_nWeeks == -1)
		return;

	// We need at least one author
	if (m_authorNames.empty()) return;

	// Add a single series to the chart
	MyGraphSeries * graphData = new MyGraphSeries();
	m_graph.AddSeries(*graphData);
	m_graphDataArray.Add(graphData);

	// Set up the graph.
	CString temp;
	UpdateData();
	m_graph.SetGraphType(m_GraphType, m_bStacked);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHORY);
	m_graph.SetYAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHORX);
	m_graph.SetXAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHOR);
	m_graph.SetGraphTitle(temp);

	// Find out which authors are to be shown and which are to be skipped.
	std::list<stdstring> authors;
	std::list<stdstring> others;
	FilterSkippedAuthors(authors, others);

	// Loop over all authors in the authors list and 
	// add them to the graph.

	for (std::list<stdstring>::iterator it = authors.begin(); it != authors.end(); ++it)
	{
		int group = m_graph.AppendGroup(it->c_str());
		graphData->SetData(group, m_commitsPerAuthor[*it]);
	}

	// If we have other authors, count them and their commits.
	int nOthers = others.size();
	if (nOthers != 0)
	{
		int nCommits = 0;
		for (std::list<stdstring>::iterator it = others.begin(); it != others.end(); ++it)
		{
			nCommits += m_commitsPerAuthor[*it];
		}
		CString sOthers(MAKEINTRESOURCE(IDS_STATGRAPH_OTHERGROUP));
		CString temp;
		temp.Format(_T(" (%ld)"), nOthers);
		sOthers += temp;
		int group = m_graph.AppendGroup(sOthers);
		graphData->SetData(group, nCommits);
	}

	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::ShowCommitsByDate()
{
	if ((m_parAuthors==NULL)||(m_parDates==NULL)||(m_parFileChanges==NULL))
		return;
	ShowLabels(FALSE);
	ClearGraph();

	// This function relies on a previous call of GatherData(). 
	// This can be detected by checking the weekcount. 
	// If the weekcount is equal to -1, it hasn't been called before.
	if (m_nWeeks == -1)
		GatherData();
	// If week count is still -1, something bad has happened, probably invalid data!
	if (m_nWeeks == -1)
		return;

	// We need at least one author
	if (m_authorNames.empty()) return;

	// Set up the graph.
	CString temp;
	UpdateData();
	m_graph.SetGraphType(m_GraphType, m_bStacked);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATEY);
	m_graph.SetYAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATE);
	m_graph.SetGraphTitle(temp);

	m_graph.SetXAxisLabel(GetUnitString());

	// Find out which authors are to be shown and which are to be skipped.
	std::list<stdstring> authors;
	std::list<stdstring> others;
	FilterSkippedAuthors(authors, others);

	// Add a graph series for each author.
	AuthorDataMap authorGraphMap;
	for (std::list<stdstring>::iterator it = authors.begin(); it != authors.end(); ++it)
		authorGraphMap[*it] = m_graph.AppendGroup(it->c_str());
	// If we have skipped authors, add a graph series for all those.
	CString sOthers(MAKEINTRESOURCE(IDS_STATGRAPH_OTHERGROUP));
	stdstring othersName;
	if (!others.empty()) {
		temp.Format(_T(" (%ld)"), others.size());
		sOthers += temp;
		othersName = wide_string((LPCWSTR)sOthers);
		authorGraphMap[othersName] = m_graph.AppendGroup(wide_string((LPCWSTR)sOthers).c_str());
	}

	int lastunit = 0;
	CTime lasttime;

	// Mapping to collect commit counts in each interval
	AuthorDataMap	commitCount;

	if (m_lastInterval != -1)
	{
		lasttime = CTime(m_minDate) + CTimeSpan(7*m_firstInterval,0,0,0);
		lastunit = GetUnit(lasttime);
	}
	// Loop over all intervals/weeks and collect filtered data.
	// Sum up data in each interval until the time unit changes.
	for (int i=m_firstInterval; i<=m_lastInterval; ++i)
	{
		// Get the time corresponding to the current interval
		CTime t = CTime(m_minDate) + CTimeSpan(7*i,0,0,0);
		int unit = GetUnit(t);

		// Check if new interval has started, so that we can add 
		// the previously collected data to the chart!
		if (unit != lastunit)
		{
			// Create a new data series for this unit/interval.
			MyGraphSeries * graphData = new MyGraphSeries();
			// Loop over all created graphs and set the corresponding data.
			for (AuthorDataMap::const_iterator it = authorGraphMap.begin(); it != authorGraphMap.end(); ++it) 
			{
				graphData->SetData(it->second, commitCount[it->first]);
			}
			temp = GetUnitLabel(lastunit, lasttime);
			graphData->SetLabel(temp);
			m_graph.AddSeries(*graphData);
			m_graphDataArray.Add(graphData);

			lastunit = unit;
			// Reset commit count mapping.
			commitCount.clear();
		}
		lasttime = t;
		// Collect data for authors listed by name.
		for (std::list<stdstring>::iterator it = authors.begin(); it != authors.end(); ++it)
		{
			// Do we have some data for the current author in the current interval?
			AuthorDataMap::const_iterator data_it = m_commitsPerWeekAndAuthor[i].find(*it);
			if (data_it == m_commitsPerWeekAndAuthor[i].end()) continue;
			commitCount[*it] += data_it->second;
		}
		// Collect data for all skipped authors.
		for (std::list<stdstring>::iterator it = others.begin(); it != others.end(); ++it)
		{
			// Do we have some data for the author in the current interval?
			AuthorDataMap::const_iterator data_it = m_commitsPerWeekAndAuthor[i].find(*it);
			if (data_it == m_commitsPerWeekAndAuthor[i].end()) continue;
			commitCount[othersName] += data_it->second;
		}
	}
	// All commit intervals done, add the data collected for the last unit
	// also to the chart.
	MyGraphSeries * graphData = new MyGraphSeries();
	for (AuthorDataMap::const_iterator it = authorGraphMap.begin(); it != authorGraphMap.end(); ++it) 
	{
		graphData->SetData(it->second, commitCount[it->first]);
	}
	temp = GetUnitLabel(lastunit, lasttime);
	graphData->SetLabel(temp);
	m_graph.AddSeries(*graphData);
	m_graphDataArray.Add(graphData);

	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::ShowStats()
{
	if ((m_parAuthors==NULL)||(m_parDates==NULL)||(m_parFileChanges==NULL))
		return;
	ShowLabels(TRUE);

	// This function relies on a previous call of GatherData(). 
	// This can be detected by checking the weekcount. 
	// If the weekcount is equal to -1, it hasn't been called before.
	if (m_nWeeks == -1)
		GatherData();
	// If week count is still -1, something bad has happened, probably invalid data!
	if (m_nWeeks == -1)
		return;

	// Now we can use the gathered data to update the stats dialog.
	unsigned int nAuthors = m_authorNames.size();

	// Find most and least active author names.
	stdstring mostActiveAuthor;
	stdstring leastActiveAuthor;
	if (nAuthors > 0) 
	{
		mostActiveAuthor = m_authorNames.front();
		leastActiveAuthor = m_authorNames.back();
	}

	// Obtain the statistics for the table.
	long nCommitsMin = -1;
	long nCommitsMax = -1;
	long nFileChangesMin = -1;
	long nFileChangesMax = -1;

	long nMostActiveMaxCommits = -1;
	long nMostActiveMinCommits = -1;
	long nLeastActiveMaxCommits = -1;
	long nLeastActiveMinCommits = -1;

	// Loop over all intervals and find min and max values for commit count and file changes.
	// Also store the stats for the most and least active authors.
	for (int i=m_firstInterval; i<=m_lastInterval; ++i) 
	{
		// Loop over all commits in this interval and count the number of commits by all authors.
		int commitCount = 0;
		AuthorDataMap::iterator commit_endit = m_commitsPerWeekAndAuthor[i].end();
		for (AuthorDataMap::iterator commit_it = m_commitsPerWeekAndAuthor[i].begin();
			commit_it != commit_endit; ++commit_it)
		{
			commitCount += commit_it->second;
		}
		if (nCommitsMin == -1 || commitCount < nCommitsMin)
			nCommitsMin = commitCount;
		if (nCommitsMax == -1 || commitCount > nCommitsMax)
			nCommitsMax = commitCount;

		// Loop over all commits in this interval and count the number of file changes by all authors.
		int fileChangeCount = 0;
		AuthorDataMap::iterator filechange_endit = m_filechangesPerWeekAndAuthor[i].end();
		for (AuthorDataMap::iterator filechange_it = m_filechangesPerWeekAndAuthor[i].begin();
			filechange_it != filechange_endit; ++filechange_it)
		{
			fileChangeCount += filechange_it->second;
		}
		if (nFileChangesMin == -1 || fileChangeCount < nFileChangesMin)
			nFileChangesMin = fileChangeCount;
		if (nFileChangesMax == -1 || fileChangeCount > nFileChangesMax)
			nFileChangesMax = fileChangeCount;

		// also get min/max data for most and least active authors
		if (nAuthors > 0) {
			// check if author is present in this interval
			AuthorDataMap::iterator author_it = m_commitsPerWeekAndAuthor[i].find(mostActiveAuthor);
			long authorCommits;
			if (author_it == m_commitsPerWeekAndAuthor[i].end())
				authorCommits = 0;
			else
				authorCommits = author_it->second;
			if (nMostActiveMaxCommits == -1 || authorCommits > nMostActiveMaxCommits)
				nMostActiveMaxCommits = authorCommits;
			if (nMostActiveMinCommits == -1 || authorCommits < nMostActiveMinCommits)
				nMostActiveMinCommits = authorCommits;

			author_it = m_commitsPerWeekAndAuthor[i].find(leastActiveAuthor);
			if (author_it == m_commitsPerWeekAndAuthor[i].end())
				authorCommits = 0;
			else
				authorCommits = author_it->second;
			if (nLeastActiveMaxCommits == -1 || authorCommits > nLeastActiveMaxCommits)
				nLeastActiveMaxCommits = authorCommits;
			if (nLeastActiveMinCommits == -1 || authorCommits < nLeastActiveMinCommits)
				nLeastActiveMinCommits = authorCommits;
		}
	}
	if (nMostActiveMaxCommits == -1)	nMostActiveMaxCommits = 0;
	if (nMostActiveMinCommits == -1)	nMostActiveMinCommits = 0;
	if (nLeastActiveMaxCommits == -1)	nLeastActiveMaxCommits = 0;
	if (nLeastActiveMinCommits == -1)	nLeastActiveMinCommits = 0;

	int nWeeks = m_nWeeks;
	if (nWeeks == 0)
		nWeeks = 1;
	// We have now all data we want and we can fill in the labels...
	CString number;
	number.Format(_T("%ld"), nWeeks);
	SetDlgItemText(IDC_NUMWEEKVALUE, number);
	number.Format(_T("%ld"), nAuthors);
	SetDlgItemText(IDC_NUMAUTHORVALUE, number);
	number.Format(_T("%ld"), m_nTotalCommits);
	SetDlgItemText(IDC_NUMCOMMITSVALUE, number);
	number.Format(_T("%ld"), m_nTotalFileChanges);
	SetDlgItemText(IDC_NUMFILECHANGESVALUE, number);

	number.Format(_T("%ld"), m_parAuthors->GetCount() / nWeeks);
	SetDlgItemText(IDC_COMMITSEACHWEEKAVG, number);
	number.Format(_T("%ld"), nCommitsMax);
	SetDlgItemText(IDC_COMMITSEACHWEEKMAX, number);
	number.Format(_T("%ld"), nCommitsMin);
	SetDlgItemText(IDC_COMMITSEACHWEEKMIN, number);

	number.Format(_T("%ld"), m_nTotalFileChanges / nWeeks);
	SetDlgItemText(IDC_FILECHANGESEACHWEEKAVG, number);
	number.Format(_T("%ld"), nFileChangesMax);
	SetDlgItemText(IDC_FILECHANGESEACHWEEKMAX, number);
	number.Format(_T("%ld"), nFileChangesMin);
	SetDlgItemText(IDC_FILECHANGESEACHWEEKMIN, number);

	if (nAuthors == 0)
	{
		SetDlgItemText(IDC_MOSTACTIVEAUTHORNAME, _T(""));
		SetDlgItemText(IDC_MOSTACTIVEAUTHORAVG, _T("0"));
		SetDlgItemText(IDC_MOSTACTIVEAUTHORMAX, _T("0"));
		SetDlgItemText(IDC_MOSTACTIVEAUTHORMIN, _T("0"));
		SetDlgItemText(IDC_LEASTACTIVEAUTHORNAME, _T(""));
		SetDlgItemText(IDC_LEASTACTIVEAUTHORAVG, _T("0"));
		SetDlgItemText(IDC_LEASTACTIVEAUTHORMAX, _T("0"));
		SetDlgItemText(IDC_LEASTACTIVEAUTHORMIN, _T("0"));
	}
	else
	{
		SetDlgItemText(IDC_MOSTACTIVEAUTHORNAME, mostActiveAuthor.c_str());
		number.Format(_T("%ld"), m_commitsPerAuthor[mostActiveAuthor] / nWeeks);
		SetDlgItemText(IDC_MOSTACTIVEAUTHORAVG, number);
		number.Format(_T("%ld"), nMostActiveMaxCommits);
		SetDlgItemText(IDC_MOSTACTIVEAUTHORMAX, number);
		number.Format(_T("%ld"), nMostActiveMinCommits);
		SetDlgItemText(IDC_MOSTACTIVEAUTHORMIN, number);

		SetDlgItemText(IDC_LEASTACTIVEAUTHORNAME, leastActiveAuthor.c_str());
		number.Format(_T("%ld"), m_commitsPerAuthor[leastActiveAuthor] / nWeeks);
		SetDlgItemText(IDC_LEASTACTIVEAUTHORAVG, number);
		number.Format(_T("%ld"), nLeastActiveMaxCommits);
		SetDlgItemText(IDC_LEASTACTIVEAUTHORMAX, number);
		number.Format(_T("%ld"), nLeastActiveMinCommits);
		SetDlgItemText(IDC_LEASTACTIVEAUTHORMIN, number);
	}
}

void CStatGraphDlg::OnCbnSelchangeGraphcombo()
{
	UpdateData();
	DWORD_PTR graphtype = m_cGraphType.GetItemData(m_cGraphType.GetCurSel());
	switch (graphtype)
	{
	case 1:
		// labels
		// intended fall through
	case 2:
		// by date
		m_btnGraphLine.EnableWindow(TRUE);
		m_btnGraphLineStacked.EnableWindow(TRUE);
		if (m_nWeeks > 10)
		{
			m_btnGraphPie.EnableWindow(FALSE);
		}
		m_GraphType = MyGraph::Line;
		m_bStacked = false;
		break;
	case 3:
		// by author
		m_btnGraphLine.EnableWindow(FALSE);
		m_btnGraphLineStacked.EnableWindow(FALSE);
		m_btnGraphPie.EnableWindow(TRUE);
		m_GraphType = MyGraph::Bar;
		m_bStacked = false;
		break;
	}
	RedrawGraph();
}


int CStatGraphDlg::GetUnitCount()
{
	if (m_nWeeks < 15)
		return m_nWeeks;
	if (m_nWeeks < 80)
		return (m_nWeeks/4)+1;
	if (m_nWeeks < 320)
		return (m_nWeeks/13)+1; // quarters
	return (m_nWeeks/52)+1;
}

int CStatGraphDlg::GetUnit(const CTime& time)
{
	if (m_nWeeks < 15)
		return GetWeek(time);
	if (m_nWeeks < 80)
		return time.GetMonth();
	if (m_nWeeks < 320)
		return ((time.GetMonth()-1)/3)+1; // quarters
	return time.GetYear();
}

CStatGraphDlg::UnitType CStatGraphDlg::GetUnitType()
{
	if (m_nWeeks < 15)
		return Weeks;
	if (m_nWeeks < 80)
		return Months;
	if (m_nWeeks < 320)
		return Quarters;
	return Years;
}

CString CStatGraphDlg::GetUnitString()
{
	if (m_nWeeks < 15)
		return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXWEEK));
	if (m_nWeeks < 80)
		return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXMONTH));
	if (m_nWeeks < 320)
		return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXQUARTER));
	return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXYEAR));
}

CString CStatGraphDlg::GetUnitLabel(int unit, CTime &lasttime)
{
	// TODO : FS#398, do this properly with locales
	CString temp;
	switch (GetUnitType())
	{
	case Weeks:
		if ((unit == 1)&&(lasttime.GetMonth() == 12))
			// in some locales, the last week of a year can actually be
			// the first week-of-the-year of the next year.
			temp.Format(_T("%d/%.2d"), unit, (lasttime.GetYear()+1)%100);
		else
			temp.Format(_T("%d/%.2d"), unit, lasttime.GetYear()%100);
		break;
	case Months:
		temp.Format(_T("%d/%.2d"), unit, lasttime.GetYear()%100);
		break;
	case Quarters:
		temp.Format(IDS_STATGRAPH_QUARTERLABEL, unit, lasttime.GetYear()%100);
		break;
	case Years:
		temp.Format(_T("%d"), unit);
		break;
	}
	return temp;
}

void CStatGraphDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (nSBCode == TB_THUMBTRACK)
		return CDialog::OnHScroll(nSBCode, nPos, pScrollBar);

	switch (m_cGraphType.GetItemData(m_cGraphType.GetCurSel()))
	{
	case 1:
		ShowStats();
		break;
	case 2:
		ShowCommitsByDate();
		break;
	case 3:
		ShowCommitsByAuthor();
		break;
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CStatGraphDlg::OnNeedText(NMHDR *pnmh, LRESULT * /*pResult*/)
{
	TOOLTIPTEXT* pttt = (TOOLTIPTEXT*) pnmh;
	if (pttt->hdr.idFrom == (UINT) m_Skipper.GetSafeHwnd())
	{
		unsigned int included_authors_count = m_Skipper.GetPos();
		// if we only leave out one author, still include him with his name
		if (included_authors_count + 1 == m_authorNames.size()) 
			++included_authors_count;

		// find the minimum number of commits that the shown authors have
		int min_commits = 0;
		included_authors_count = min(included_authors_count, m_authorNames.size());
		std::list<stdstring>::iterator author_it = m_authorNames.begin();
		advance(author_it, included_authors_count);
		if (author_it != m_authorNames.begin())
			min_commits = m_commitsPerAuthor[ *(--author_it) ];

		CString string;
		int percentage = int(min_commits*100.0/m_nTotalCommits);
		string.Format(IDS_STATGRAPH_AUTHORSLIDER_TT, m_Skipper.GetPos(), min_commits, percentage);
		::lstrcpy(pttt->szText, (LPCTSTR) string);
	}
}

void CStatGraphDlg::AuthorsCaseSensitiveChanged() 
{
	UpdateData();	// update checkbox state
	GatherData();	// first regenerate the statistics data
	RedrawGraph();	// then update the current statistics page
}

void CStatGraphDlg::SortModeChanged() 
{
	UpdateData();	// update checkbox state
	RedrawGraph();	// then update the current statistics page
}

void CStatGraphDlg::ClearGraph() 
{
	m_graph.Clear();
	for (int j=0; j<m_graphDataArray.GetCount(); ++j)
		delete ((MyGraphSeries *)m_graphDataArray.GetAt(j));
	m_graphDataArray.RemoveAll();
}

void CStatGraphDlg::RedrawGraph()
{
	EnableDisableMenu();
	m_btnGraphBar.SetState(BST_UNCHECKED);
	m_btnGraphBarStacked.SetState(BST_UNCHECKED);
	m_btnGraphLine.SetState(BST_UNCHECKED);
	m_btnGraphLineStacked.SetState(BST_UNCHECKED);
	m_btnGraphPie.SetState(BST_UNCHECKED);

	if ((m_GraphType == MyGraph::Bar)&&(m_bStacked))
	{
		m_btnGraphBarStacked.SetState(BST_CHECKED);
	}
	if ((m_GraphType == MyGraph::Bar)&&(!m_bStacked))
	{
		m_btnGraphBar.SetState(BST_CHECKED);
	}
	if ((m_GraphType == MyGraph::Line)&&(m_bStacked))
	{
		m_btnGraphLineStacked.SetState(BST_CHECKED);
	}
	if ((m_GraphType == MyGraph::Line)&&(!m_bStacked))
	{
		m_btnGraphLine.SetState(BST_CHECKED);
	}
	if (m_GraphType == MyGraph::PieChart)
	{
		m_btnGraphPie.SetState(BST_CHECKED);
	}


	UpdateData();
	switch (m_cGraphType.GetItemData(m_cGraphType.GetCurSel()))
	{
	case 1:
		ShowStats();
		break;
	case 2:
		ShowCommitsByDate();
		break;
	case 3:
		ShowCommitsByAuthor();
		break;
	}
}
void CStatGraphDlg::OnBnClickedGraphbarbutton()
{
	m_GraphType = MyGraph::Bar;
	m_bStacked = false;
	RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphbarstackedbutton()
{
	m_GraphType = MyGraph::Bar;
	m_bStacked = true;
	RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphlinebutton()
{
	m_GraphType = MyGraph::Line;
	m_bStacked = false;
	RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphlinestackedbutton()
{
	m_GraphType = MyGraph::Line;
	m_bStacked = true;
	RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphpiebutton()
{
	m_GraphType = MyGraph::PieChart;
	m_bStacked = false;
	RedrawGraph();
}

BOOL CStatGraphDlg::PreTranslateMessage(MSG* pMsg)
{
	if (NULL != m_pToolTip)
		m_pToolTip->RelayEvent(pMsg);

	return CStandAloneDialogTmpl<CResizableDialog>::PreTranslateMessage(pMsg);
}

void CStatGraphDlg::EnableDisableMenu()
{
	UINT nEnable = MF_BYCOMMAND;
	if (m_cGraphType.GetItemData(m_cGraphType.GetCurSel()) == 1)
		nEnable |= (MF_DISABLED | MF_GRAYED);
	else
		nEnable |= MF_ENABLED;
	GetMenu()->EnableMenuItem(ID_FILE_SAVESTATGRAPHAS, nEnable);
}

void CStatGraphDlg::OnFileSavestatgraphas()
{
	CString temp;
	// ask for the filename to save the picture
	OPENFILENAME ofn = {0};				// common dialog box structure
	TCHAR szFile[MAX_PATH] = {0};		// buffer for file name
	// Initialize OPENFILENAME
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = this->m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	temp.LoadString(IDS_REVGRAPH_SAVEPIC);
	CStringUtils::RemoveAccelerators(temp);
	if (temp.IsEmpty())
		ofn.lpstrTitle = NULL;
	else
		ofn.lpstrTitle = temp;
	ofn.Flags = OFN_OVERWRITEPROMPT;

	CString sFilter;
	sFilter.LoadString(IDS_PICTUREFILEFILTER);
	TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
	_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
	// Replace '|' delimiters with '\0's
	TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
	while (ptr != pszFilters)
	{
		if (*ptr == '|')
			*ptr = '\0';
		ptr--;
	}
	ofn.lpstrFilter = pszFilters;
	ofn.nFilterIndex = 1;
	// Display the Open dialog box. 
	CString tempfile;
	if (GetSaveFileName(&ofn)==TRUE)
	{
		tempfile = CString(ofn.lpstrFile);
		// if the user doesn't specify a file extension, default to
		// wmf and add that extension to the filename. But only if the
		// user chose the 'pictures' filter. The filename isn't changed
		// if the 'All files' filter was chosen.
		CString extension;
		int dotPos = tempfile.ReverseFind('.');
		int slashPos = tempfile.ReverseFind('\\');
		if (dotPos > slashPos)
			extension = tempfile.Mid(dotPos);
		if ((ofn.nFilterIndex == 1)&&(extension.IsEmpty()))
		{
			extension = _T(".wmf");
			tempfile += extension;
		}
		SaveGraph(tempfile);
	}
	delete [] pszFilters;
}

void CStatGraphDlg::SaveGraph(CString sFilename)
{
	CString extension = CPathUtils::GetFileExtFromPath(sFilename);
	if (extension.CompareNoCase(_T(".wmf"))==0)
	{
		// save the graph as an enhanced metafile
		CMyMetaFileDC wmfDC;
		wmfDC.CreateEnhanced(NULL, sFilename, NULL, _T("TortoiseSVN\0Statistics\0\0"));
		wmfDC.SetAttribDC(GetDC()->GetSafeHdc());
		RedrawGraph();
		m_graph.DrawGraph(wmfDC);
		HENHMETAFILE hemf = wmfDC.CloseEnhanced();
		DeleteEnhMetaFile(hemf);
	}
	else
	{
		// to save the graph as a pixel picture (e.g. gif, png, jpeg, ...)
		// the user needs to have GDI+ installed. So check if GDI+ is 
		// available before we start using it.
		TCHAR gdifindbuf[MAX_PATH];
		_tcscpy_s(gdifindbuf, MAX_PATH, _T("gdiplus.dll"));
		if (PathFindOnPath(gdifindbuf, NULL))
		{
			ATLTRACE("gdi plus found!");
		}
		else
		{
			ATLTRACE("gdi plus not found!");
			CMessageBox::Show(m_hWnd, IDS_ERR_GDIPLUS_MISSING, IDS_APPNAME, MB_ICONERROR);
			return;
		}

		// save the graph as a pixel picture instead of a vector picture
		// create dc to paint on
		try
		{
			CWindowDC ddc(this);
			CDC dc;
			if (!dc.CreateCompatibleDC(&ddc))
			{
				LPVOID lpMsgBuf;
				if (!FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL ))
				{
					return;
				}
				MessageBox( (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
				LocalFree( lpMsgBuf );
				return;
			}
			CRect rect;
			GetDlgItem(IDC_GRAPH)->GetClientRect(&rect);
			HBITMAP hbm = ::CreateCompatibleBitmap(ddc.m_hDC, rect.Width(), rect.Height());
			if (hbm==0)
			{
				LPVOID lpMsgBuf;
				if (!FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL ))
				{
					return;
				}
				MessageBox( (LPCTSTR)lpMsgBuf, _T("Error"), MB_OK | MB_ICONINFORMATION );
				LocalFree( lpMsgBuf );
				return;
			}
			HBITMAP oldbm = (HBITMAP)dc.SelectObject(hbm);
			// paint the whole graph
			RedrawGraph();
			m_graph.DrawGraph(dc);
			// now use GDI+ to save the picture
			CLSID   encoderClsid;
			GdiplusStartupInput gdiplusStartupInput;
			ULONG_PTR           gdiplusToken;
			CString sErrormessage;
			if (GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL )==Ok)
			{   
				{
					Bitmap bitmap(hbm, NULL);
					if (bitmap.GetLastStatus()==Ok)
					{
						// Get the CLSID of the encoder.
						int ret = 0;
						if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(_T(".png"))==0)
							ret = GetEncoderClsid(L"image/png", &encoderClsid);
						else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(_T(".jpg"))==0)
							ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
						else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(_T(".jpeg"))==0)
							ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
						else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(_T(".bmp"))==0)
							ret = GetEncoderClsid(L"image/bmp", &encoderClsid);
						else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(_T(".gif"))==0)
							ret = GetEncoderClsid(L"image/gif", &encoderClsid);
						else
						{
							sFilename += _T(".jpg");
							ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
						}
						if (ret >= 0)
						{
							CStringW tfile = CStringW(sFilename);
							bitmap.Save(tfile, &encoderClsid, NULL);
						}
						else
						{
							sErrormessage.Format(IDS_REVGRAPH_ERR_NOENCODER, CPathUtils::GetFileExtFromPath(sFilename));
						}
					}
					else
					{
						sErrormessage.LoadString(IDS_REVGRAPH_ERR_NOBITMAP);
					}
				}
				GdiplusShutdown(gdiplusToken);
			}
			else
			{
				sErrormessage.LoadString(IDS_REVGRAPH_ERR_GDIINIT);
			}
			dc.SelectObject(oldbm);
			dc.DeleteDC();
			if (!sErrormessage.IsEmpty())
			{
				CMessageBox::Show(m_hWnd, sErrormessage, _T("TortoiseSVN"), MB_ICONERROR);
			}
		}
		catch (CException * pE)
		{
			TCHAR szErrorMsg[2048];
			pE->GetErrorMessage(szErrorMsg, 2048);
			CMessageBox::Show(m_hWnd, szErrorMsg, _T("TortoiseSVN"), MB_ICONERROR);
		}
	}
}

int CStatGraphDlg::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	if (GetImageEncodersSize(&num, &size)!=Ok)
		return -1;
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	if (GetImageEncoders(num, size, pImageCodecInfo)==Ok)
	{
		for (UINT j = 0; j < num; ++j)
		{
			if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				return j;  // Success
			}
		}
	}
	free (pImageCodecInfo);
	return -1;  // Failure
}

void CStatGraphDlg::StoreCurrentGraphType()
{
	UpdateData();
	DWORD_PTR graphtype = m_cGraphType.GetItemData(m_cGraphType.GetCurSel());
	// encode the current chart type
	int statspage = graphtype*10;
	if ((m_GraphType == MyGraph::Bar)&&(m_bStacked))
	{
		statspage += 1;
	}
	if ((m_GraphType == MyGraph::Bar)&&(!m_bStacked))
	{
		statspage += 2;
	}
	if ((m_GraphType == MyGraph::Line)&&(m_bStacked))
	{
		statspage += 3;
	}
	if ((m_GraphType == MyGraph::Line)&&(!m_bStacked))
	{
		statspage += 4;
	}
	if (m_GraphType == MyGraph::PieChart)
	{
		statspage += 5;
	}
	
	// store current chart type in registry
	CRegDWORD lastStatsPage = CRegDWORD(_T("Software\\TortoiseSVN\\LastViewedStatsPage"), 0);
	lastStatsPage = statspage;
}
