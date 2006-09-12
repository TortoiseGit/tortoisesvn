// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "StatGraphDlg.h"
#include "gdiplus.h"
#include "UnicodeUtils.h"
#include "StringUtils.h"
#include "PathUtils.h"
#include "MemDC.h"
#include "MessageBox.h"
#include <math.h>
#include <locale>

using namespace Gdiplus;

IMPLEMENT_DYNAMIC(CStatGraphDlg, CResizableStandAloneDialog)
CStatGraphDlg::CStatGraphDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CStatGraphDlg::IDD, pParent)
	, m_bStacked(FALSE)
	, m_GraphType(MyGraph::Bar)
	, m_bIgnoreAuthorCase(FALSE)
	, m_weekcount(-1)
{
	m_parDates = NULL;
	m_parFileChanges = NULL;
	m_parAuthors = NULL;
	m_pToolTip = NULL;
}

CStatGraphDlg::~CStatGraphDlg()
{
	for (int j=0; j<m_graphDataArray.GetCount(); ++j)
		delete ((MyGraphSeries *)m_graphDataArray.GetAt(j));
	m_graphDataArray.RemoveAll();
	DestroyIcon(m_hGraphBarIcon);
	DestroyIcon(m_hGraphBarStackedIcon);
	DestroyIcon(m_hGraphLineIcon);
	DestroyIcon(m_hGraphLineStackedIcon);
	DestroyIcon(m_hGraphPieIcon);
	delete m_pToolTip;
}

void CStatGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GRAPH, m_graph);
	DDX_Control(pDX, IDC_GRAPHCOMBO, m_cGraphType);
	DDX_Control(pDX, IDC_SKIPPER, m_Skipper);
	DDX_Check(pDX, IDC_IGNORECASE, m_bIgnoreAuthorCase);
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
	ON_BN_CLICKED(IDC_IGNORECASE, &CStatGraphDlg::RedrawGraph)
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

	m_Skipper.SetRange(0, 100);
	m_Skipper.SetPos(10);
	m_Skipper.SetPageSize(5);

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

	AddAnchor(IDC_IGNORECASE, BOTTOM_LEFT);
	AddAnchor(IDC_SKIPPER, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SKIPPERLABEL, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	EnableSaveRestore(_T("StatGraphDlg"));
	ShowStats();

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

	GetDlgItem(IDC_SKIPPER)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_SKIPPERLABEL)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphBar.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphBarStacked.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphLine.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphLineStacked.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
	m_btnGraphPie.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
}

void CStatGraphDlg::ShowCommitsByAuthor()
{
	if ((m_parAuthors==NULL)||(m_parDates==NULL)||(m_parFileChanges==NULL))
		return;
	ShowLabels(FALSE);
	m_graph.Clear();
	for (int j=0; j<m_graphDataArray.GetCount(); ++j)
		delete ((MyGraphSeries *)m_graphDataArray.GetAt(j));
	m_graphDataArray.RemoveAll();
	MyGraphSeries * graphData = new MyGraphSeries();
	m_graph.AddSeries(*graphData);
	m_graphDataArray.Add(graphData);

	//Set up the graph.
	CString temp;
	UpdateData();
	m_graph.SetGraphType(m_GraphType, m_bStacked);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHORY);
	m_graph.SetYAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHORX);
	m_graph.SetXAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHOR);
	m_graph.SetGraphTitle(temp);

	std::map<stdstring, LONG> authorcommits;
	LONG nTotalCommits = 0;
	for (int i=0; i<m_parAuthors->GetCount(); ++i)
	{
		CString sAuth = m_parAuthors->GetAt(i);
		if (m_bIgnoreAuthorCase)
			sAuth = sAuth.MakeLower();
		stdstring author = stdstring(sAuth);
		std::map<stdstring, LONG>::iterator it = authorcommits.lower_bound(author);
		if (it == authorcommits.end() || it->first != author)
			it = authorcommits.insert(it, std::make_pair(author, 0));
		it->second++;
		nTotalCommits++;
	}

	int nGroup(-1);

	// since maps are sorted by key, create a new map
	// with the number of commits as the key, and the author as the value
	std::multimap<LONG, stdstring> authorcommitssorted;
	for (std::map<stdstring, LONG>::iterator it = authorcommits.begin(); it != authorcommits.end(); ++it)
	{
		authorcommitssorted.insert(std::pair<LONG, stdstring>(it->second, it->first));
	}

	std::multimap<LONG, stdstring>::reverse_iterator iter;
	iter = authorcommitssorted.rbegin();
	CString sOthers(MAKEINTRESOURCE(IDS_STATGRAPH_OTHERGROUP));
	int nOthers = 0;
	int nOthersCount = 0;
	int nLastOthersValue = 0;
	stdstring sLastOthersAuthor;
	while (iter != authorcommitssorted.rend())
	{
		if (iter->first < (nTotalCommits * m_Skipper.GetPos() / 200))
		{
			nOthers += iter->first;
			nOthersCount++;
			sLastOthersAuthor = iter->second;
			nLastOthersValue = iter->first;
		}
		else
		{
			nGroup = m_graph.AppendGroup(iter->second.c_str());
			graphData->SetData(nGroup, iter->first);
		}
		iter++;
	}
	if (nOthers)
	{
		if (nOthers == 1)
		{
			// since there's only one author in the "others" group,
			// just show that authors info instead of "others"
			nGroup = m_graph.AppendGroup(sLastOthersAuthor.c_str());
			graphData->SetData(nGroup, nLastOthersValue);
		}
		else
		{
			CString temp;
			temp.Format(_T(" (%ld)"), nOthersCount);
			sOthers += temp;
			nGroup = m_graph.AppendGroup(sOthers);
			graphData->SetData(nGroup, nOthers);
		}
	}

	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::ShowCommitsByDate()
{
	if ((m_parAuthors==NULL)||(m_parDates==NULL)||(m_parFileChanges==NULL))
		return;
	CString sOthers(MAKEINTRESOURCE(IDS_STATGRAPH_OTHERGROUP));
	ShowLabels(FALSE);
	m_graph.Clear();

	for (int j=0; j<m_graphDataArray.GetCount(); ++j)
		delete ((MyGraphSeries *)m_graphDataArray.GetAt(j));
	m_graphDataArray.RemoveAll();

	InitUnits();

	// Set up the graph.
	CString temp;
	UpdateData();
	m_graph.SetGraphType(m_GraphType, m_bStacked);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATEY);
	m_graph.SetYAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATE);
	m_graph.SetGraphTitle(temp);

	// Find how many commits each author has done
	std::map<stdstring, LONG> author_commits;
	LONG nTotalCommits = 0;
	for (int i=0; i<m_parAuthors->GetCount(); ++i)
	{
		CString sAuth = m_parAuthors->GetAt(i);
		if (m_bIgnoreAuthorCase)
			sAuth = sAuth.MakeLower();
		stdstring author = stdstring(sAuth);
		std::map<stdstring, LONG>::iterator it = author_commits.lower_bound(author);
		if (it == author_commits.end() || it->first != author)
			it = author_commits.insert(it, std::make_pair(author, 0));
		it->second++;
		nTotalCommits++;
	}

	// Append each author to the graph
	std::map<stdstring, LONG> authors;
	std::map<stdstring, LONG>::iterator iter;
	int nOthersCount = 0;
	stdstring sLastOthersAuthor;
	iter = author_commits.begin();
	while (iter != author_commits.end())
	{
		if (author_commits[iter->first] < (nTotalCommits * m_Skipper.GetPos() / 200))
		{
			nOthersCount++;
			sLastOthersAuthor = iter->first;
		}
		else
			authors[iter->first] = m_graph.AppendGroup(iter->first.c_str());
		iter++;
	}
	if (nOthersCount)
	{
		if (nOthersCount == 1)
		{
			authors[sLastOthersAuthor] = m_graph.AppendGroup(sLastOthersAuthor.c_str());
		}
		else
		{
			temp.Format(_T(" (%ld)"), nOthersCount);
			sOthers += temp;
			authors[wide_string((LPCWSTR)sOthers)] = m_graph.AppendGroup(wide_string((LPCWSTR)sOthers).c_str());
		}
	}

	m_graph.SetXAxisLabel(GetUnitString());

	int unit = 0;
	int unitcount = 0;

	std::map<stdstring, LONG> authorcommits;

	if (m_parDates->GetCount()>0)
	{
		unit = GetUnit(CTime((__time64_t)m_parDates->GetAt(m_parDates->GetCount()-1)));
	}
	CTime lasttime((__time64_t)m_parDates->GetAt(m_parDates->GetCount()-1));
	for (int i=m_parDates->GetCount()-1; i>=0; --i)
	{
		CTime time((__time64_t)m_parDates->GetAt(i));
		int timeunit = GetUnit(time);
		if (unit != timeunit)
		{
			unitcount++;
			std::map<stdstring, LONG>::iterator iter;
			MyGraphSeries * graphData = new MyGraphSeries();
			iter = authors.begin();
			while (iter != authors.end())
			{
				std::map<stdstring, LONG>::iterator ac_it = authorcommits.lower_bound(iter->first);
				if (ac_it != authorcommits.end() && ac_it->first == iter->first)
					graphData->SetData(iter->second, ac_it->second);
				else
					graphData->SetData(iter->second, 0);
				iter++;
			}
			switch(GetUnitType())
			{
			case Weeks:
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
			graphData->SetLabel(temp);
			m_graph.AddSeries(*graphData);
			m_graphDataArray.Add(graphData);

			CTimeSpan oneday = CTimeSpan(1,0,0,0);
			while (abs(timeunit - unit) > 1)
			{
				while (unit == GetUnit(lasttime))
					lasttime += oneday;
				unit = GetUnit(lasttime);
				if (unit == timeunit)
					break;		//year lap
				std::map<stdstring, LONG>::iterator iter;
				MyGraphSeries * graphData = new MyGraphSeries();
				iter = authors.begin();
				while (iter != authors.end())
				{
					graphData->SetData(iter->second, 0);
					iter++;
				}
				switch(GetUnitType())
				{
				case Weeks:
				case Months:
					temp.Format(_T("%d/%.2d"), unit, time.GetYear()%100);
					break;
				case Quarters:
					temp.Format(IDS_STATGRAPH_QUARTERLABEL, unit, time.GetYear()%100);
					break;
				case Years:
					temp.Format(_T("%d"), unit);
					break;
				}
				graphData->SetLabel(temp);
				m_graph.AddSeries(*graphData);
				m_graphDataArray.Add(graphData);
			}
			unit = timeunit;
			authorcommits.clear();
		}
		lasttime = time;
		CString sAuth = m_parAuthors->GetAt(i);
		if (m_bIgnoreAuthorCase)
			sAuth = sAuth.MakeLower();
		stdstring author = stdstring(sAuth);
		if (authors.find(author) == authors.end())
			author = stdstring((LPCTSTR)sOthers);
		std::map<stdstring, LONG>::iterator ac_it = authorcommits.lower_bound(author);
		if (ac_it == authorcommits.end() || ac_it->first != author)
			ac_it = authorcommits.insert(ac_it, std::make_pair(author, 0));
		ac_it->second++;
	}

	MyGraphSeries * graphData = new MyGraphSeries();
	iter = authors.begin();
	while (iter != authors.end())
	{
		int val = 0;
		std::map<stdstring, LONG>::const_iterator it = authorcommits.lower_bound(iter->first);
		if (it != authorcommits.end() && it->first == iter->first)
			val = it->second;
		graphData->SetData(iter->second, val);
		iter++;
	}

	switch(GetUnitType())
	{
	case Weeks:
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
	graphData->SetLabel(temp);
	m_graph.AddSeries(*graphData);
	m_graphDataArray.Add(graphData);
	authorcommits.clear();


	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::ShowStats()
{
	if ((m_parAuthors==NULL)||(m_parDates==NULL)||(m_parFileChanges==NULL))
		return;
	ShowLabels(TRUE);
	int nWeeks = 0;
	int nCurrentWeek = 0;
	long nCommitsMin = 0;
	long nCommitsMax = 0;
	long nFileChanges = 0;
	long nFileChangesMin = 0;
	long nFileChangesMax = 0;

	int numAuthors = 0;
	std::map<stdstring, LONG> authors;
	for (int i=0; i<m_parAuthors->GetCount(); ++i)
	{
		CString sAuth = m_parAuthors->GetAt(i);
		if (m_bIgnoreAuthorCase)
			sAuth = sAuth.MakeLower();
		stdstring author = stdstring(sAuth);
		std::map<stdstring, LONG>::iterator it = authors.lower_bound(author);
		if (it == authors.end() || it->first != author)
		{
			authors.insert(it, std::make_pair(author, m_graph.AppendGroup(author.c_str())));
			numAuthors++;
		}
	}
	std::map<stdstring, LONG> authorcommits;
	std::map<stdstring, LONG> AuthorCommits;
	std::map<stdstring, LONG> AuthorCommitsMin;
	std::map<stdstring, LONG> AuthorCommitsMax;

	int commits = 0;
	int filechanges = 0;
	BOOL weekover = FALSE;
	if (m_parDates->GetCount()>0)
	{
		nCurrentWeek = GetWeek(CTime((__time64_t)m_parDates->GetAt(m_parDates->GetCount()-1)));
	}
	for (int i=m_parDates->GetCount()-1; i>=0; --i)
	{
		CTime time((__time64_t)m_parDates->GetAt(i));
		commits++;
		filechanges += m_parFileChanges->GetAt(i);
		weekover = FALSE;
		CString sAuth = m_parAuthors->GetAt(i);
		if (m_bIgnoreAuthorCase)
			sAuth = sAuth.MakeLower();
		stdstring author = stdstring(sAuth);
		std::map<stdstring, LONG>::iterator it = authorcommits.lower_bound(author);
		if (it == authorcommits.end() || it->first != author)
			it = authorcommits.insert(it, std::make_pair(author, 0));
		it->second++;
		if (nCurrentWeek != GetWeek(time))
		{
			std::map<stdstring, LONG>::iterator iter;
			iter = authors.begin();
			while (iter != authors.end())
			{
				std::map<stdstring, LONG>::iterator AC_it = AuthorCommits.lower_bound(iter->first);
				if (AC_it == AuthorCommits.end() || AC_it->first != iter->first)
					AC_it = AuthorCommits.insert(AC_it, std::make_pair(iter->first, 0));

				std::map<stdstring, LONG>::iterator ACMIN_it = AuthorCommitsMin.lower_bound(iter->first);
				if (ACMIN_it == AuthorCommitsMin.end() || ACMIN_it->first != iter->first)
					ACMIN_it = AuthorCommitsMin.insert(ACMIN_it, std::make_pair(iter->first, -1));

				std::map<stdstring, LONG>::iterator ACMAX_it = AuthorCommitsMax.lower_bound(iter->first);
				if (ACMAX_it == AuthorCommitsMax.end() || ACMAX_it->first != iter->first)
					ACMAX_it = AuthorCommitsMax.insert(ACMAX_it, std::make_pair(iter->first, 0));

				std::map<stdstring, LONG>::iterator ac_it = authorcommits.lower_bound(iter->first);
				if (ac_it == authorcommits.end() || ac_it->first != iter->first)
					ac_it = authorcommits.insert(ac_it, std::make_pair(iter->first, 0));

				AC_it->second += ac_it->second;
				if (ACMIN_it->second == -1 || ACMIN_it->second > ac_it->second)
					ACMIN_it->second = ac_it->second;
				if (ACMAX_it->second < ac_it->second)
					ACMAX_it->second = ac_it->second;
				iter++;
			}
			authorcommits.clear();

			nWeeks++;
			nCurrentWeek = GetWeek(time);
			if ((nCommitsMin == -1)||(nCommitsMin > commits))
				nCommitsMin = commits;
			if (nCommitsMax < commits)
				nCommitsMax = commits;
			commits = 0;
			if ((nFileChangesMin == -1)||(nFileChangesMin > filechanges))
				nFileChangesMin = filechanges;
			if (nFileChangesMax < filechanges)
				nFileChangesMax = filechanges;
			nFileChanges += filechanges;
			filechanges = 0;
			weekover = TRUE;
		}
	} // for (int i=m_parDates->GetCount()-1; i>=0; --i)
	if (!weekover)
	{
		nWeeks++;
		std::map<stdstring, LONG>::iterator iter;
		iter = authors.begin();
		while (iter != authors.end())
		{

			std::map<stdstring, LONG>::iterator AC_it = AuthorCommits.lower_bound(iter->first);
			if (AC_it == AuthorCommits.end() || AC_it->first != iter->first)
				AC_it = AuthorCommits.insert(AC_it, std::make_pair(iter->first, 0));

			std::map<stdstring, LONG>::iterator ACMIN_it = AuthorCommitsMin.lower_bound(iter->first);
			if (ACMIN_it == AuthorCommitsMin.end() || ACMIN_it->first != iter->first)
				ACMIN_it = AuthorCommitsMin.insert(ACMIN_it, std::make_pair(iter->first, -1));

			std::map<stdstring, LONG>::iterator ACMAX_it = AuthorCommitsMax.lower_bound(iter->first);
			if (ACMAX_it == AuthorCommitsMax.end() || ACMAX_it->first != iter->first)
				ACMAX_it = AuthorCommitsMax.insert(ACMAX_it, std::make_pair(iter->first, 0));

			std::map<stdstring, LONG>::iterator ac_it = authorcommits.lower_bound(iter->first);
			if (ac_it == authorcommits.end() || ac_it->first != iter->first)
				ac_it = authorcommits.insert(ac_it, std::make_pair(iter->first, 0));

			AC_it->second += ac_it->second;
			if (ACMIN_it->second == -1 || ACMIN_it->second > ac_it->second)
				ACMIN_it->second = ac_it->second;
			if (ACMAX_it->second < ac_it->second)
				ACMAX_it->second = ac_it->second;
			iter++;
		}
		authorcommits.clear();

		if ((nCommitsMin == -1)||(nCommitsMin > commits))
			nCommitsMin = commits;
		if (nCommitsMax < commits)
			nCommitsMax = commits;
		commits = 0;
		if ((nFileChangesMin == -1)||(nFileChangesMin > filechanges))
			nFileChangesMin = filechanges;
		if (nFileChangesMax < filechanges)
			nFileChangesMax = filechanges;
		nFileChanges += filechanges;
		filechanges = 0;
	} // if (!weekover)

	if (nWeeks==0)
		nWeeks = 1;
	// we have now all data we want
	// so fill in the labels...
	CString number;
	number.Format(_T("%ld"), nWeeks);
	GetDlgItem(IDC_NUMWEEKVALUE)->SetWindowText(number);
	number.Format(_T("%ld"), numAuthors);
	GetDlgItem(IDC_NUMAUTHORVALUE)->SetWindowText(number);
	number.Format(_T("%ld"), m_parDates->GetCount());
	GetDlgItem(IDC_NUMCOMMITSVALUE)->SetWindowText(number);
	number.Format(_T("%ld"), nFileChanges);
	GetDlgItem(IDC_NUMFILECHANGESVALUE)->SetWindowText(number);

	number.Format(_T("%ld"), m_parAuthors->GetCount() / nWeeks);
	GetDlgItem(IDC_COMMITSEACHWEEKAVG)->SetWindowText(number);
	number.Format(_T("%ld"), nCommitsMax);
	GetDlgItem(IDC_COMMITSEACHWEEKMAX)->SetWindowText(number);
	number.Format(_T("%ld"), nCommitsMin);
	GetDlgItem(IDC_COMMITSEACHWEEKMIN)->SetWindowText(number);

	number.Format(_T("%ld"), nFileChanges / nWeeks);
	GetDlgItem(IDC_FILECHANGESEACHWEEKAVG)->SetWindowText(number);
	number.Format(_T("%ld"), nFileChangesMax);
	GetDlgItem(IDC_FILECHANGESEACHWEEKMAX)->SetWindowText(number);
	number.Format(_T("%ld"), nFileChangesMin);
	GetDlgItem(IDC_FILECHANGESEACHWEEKMIN)->SetWindowText(number);

	if (AuthorCommits.empty())
	{
		GetDlgItem(IDC_MOSTACTIVEAUTHORNAME)->SetWindowText(_T(""));
		GetDlgItem(IDC_MOSTACTIVEAUTHORAVG)->SetWindowText(_T("0"));
		GetDlgItem(IDC_MOSTACTIVEAUTHORMAX)->SetWindowText(_T("0"));
		GetDlgItem(IDC_MOSTACTIVEAUTHORMIN)->SetWindowText(_T("0"));
		GetDlgItem(IDC_LEASTACTIVEAUTHORNAME)->SetWindowText(_T(""));
		GetDlgItem(IDC_LEASTACTIVEAUTHORAVG)->SetWindowText(_T("0"));
		GetDlgItem(IDC_LEASTACTIVEAUTHORMAX)->SetWindowText(_T("0"));
		GetDlgItem(IDC_LEASTACTIVEAUTHORMIN)->SetWindowText(_T("0"));
	}
	else // AuthorCommits isn't empty
	{
		std::map<stdstring, LONG>::const_iterator iter, most_it, least_it;
		iter = most_it = least_it = AuthorCommits.begin();
		while (iter != AuthorCommits.end())
		{
			if (most_it->second < iter->second)
				most_it = iter;
			if (least_it->second > iter->second)
				least_it = iter;
			iter++;
		}
		// assuming AuthorCommitsMax and AuthorCommitsMin aren't empty too
		GetDlgItem(IDC_MOSTACTIVEAUTHORNAME)->SetWindowText(most_it->first.c_str());
		number.Format(_T("%ld"), most_it->second / nWeeks);
		GetDlgItem(IDC_MOSTACTIVEAUTHORAVG)->SetWindowText(number);
		number.Format(_T("%ld"), AuthorCommitsMax[most_it->first]);
		GetDlgItem(IDC_MOSTACTIVEAUTHORMAX)->SetWindowText(number);
		number.Format(_T("%ld"), AuthorCommitsMin[most_it->first]);
		GetDlgItem(IDC_MOSTACTIVEAUTHORMIN)->SetWindowText(number);

		GetDlgItem(IDC_LEASTACTIVEAUTHORNAME)->SetWindowText(least_it->first.c_str());
		number.Format(_T("%ld"), least_it->second / nWeeks);
		GetDlgItem(IDC_LEASTACTIVEAUTHORAVG)->SetWindowText(number);
		number.Format(_T("%ld"), AuthorCommitsMax[least_it->first]);
		GetDlgItem(IDC_LEASTACTIVEAUTHORMAX)->SetWindowText(number);
		number.Format(_T("%ld"), AuthorCommitsMin[least_it->first]);
		GetDlgItem(IDC_LEASTACTIVEAUTHORMIN)->SetWindowText(number);
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
		if (GetWeeksCount() > 10)
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
	CTime dDateFirstJanuary(iYear,1,1,0,0,0);
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
			// weeks always start with monday. So we add the day of week
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
				// week 1 of the next year. So ist the 1.1.(year+1) also a Mo, Tu, We, Th than
				// we alrady have the week 1, otherwise week 53 is correct

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

int	CStatGraphDlg::GetWeeksCount()
{
	// Sanity check
	if (!m_parDates)
	{
		return 0;
	}
	if (m_weekcount >= 0)
		return m_weekcount;

	// How many weeks does the time period cover?
	__time64_t date1 = (__time64_t)m_parDates->GetAt(0);
	__time64_t date2 = (__time64_t)m_parDates->GetAt(m_parDates->GetCount()-1);

	if (date1 > date2)
	{
		__time64_t date = date1;
		date1 = date2;
		date2 = date;
	}
	double secs = _difftime64(date2, date1);
	// a week has 604800 seconds
	m_weekcount = (int)ceil(secs / 604800.0);
	return m_weekcount;
}

void CStatGraphDlg::InitUnits()
{
	GetWeeksCount();
}

int CStatGraphDlg::GetUnitCount()
{
	if (m_weekcount < 15)
		return m_weekcount;
	if (m_weekcount < 80)
		return (m_weekcount/4)+1;
	if (m_weekcount < 320)
		return (m_weekcount/13)+1; // quarters
	return (m_weekcount/52)+1;
}

int CStatGraphDlg::GetUnit(const CTime& time)
{
	if (m_weekcount < 15)
		return GetWeek(time);
	if (m_weekcount < 80)
		return time.GetMonth();
	if (m_weekcount < 320)
		return ((time.GetMonth()-1)/3)+1; // quarters
	return time.GetYear();
}

CStatGraphDlg::UnitType CStatGraphDlg::GetUnitType()
{
	if (m_weekcount < 15)
		return Weeks;
	if (m_weekcount < 80)
		return Months;
	if (m_weekcount < 320)
		return Quarters;
	return Years;
}

CString CStatGraphDlg::GetUnitString()
{
	if (m_weekcount < 15)
		return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXWEEK));
	if (m_weekcount < 80)
		return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXMONTH));
	if (m_weekcount < 320)
		return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXQUARTER));
	return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXYEAR));
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
		CString string;
		string.Format(_T("%d %%"), m_Skipper.GetPos()/2);
		::lstrcpy(pttt->szText, (LPCTSTR) string);
	}
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
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
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
