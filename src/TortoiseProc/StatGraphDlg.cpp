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
#include "UnicodeUtils.h"


// CStatGraphDlg dialog

IMPLEMENT_DYNAMIC(CStatGraphDlg, CResizableStandAloneDialog)
CStatGraphDlg::CStatGraphDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CStatGraphDlg::IDD, pParent)
	, m_bStacked(FALSE)
{
	m_parDates = NULL;
	m_parFileChanges = NULL;
	m_parAuthors = NULL;
}

CStatGraphDlg::~CStatGraphDlg()
{
	for (int j=0; j<m_graphDataArray.GetCount(); ++j)
		delete ((MyGraphSeries *)m_graphDataArray.GetAt(j));
	m_graphDataArray.RemoveAll();
}

void CStatGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GRAPH, m_graph);
	DDX_Control(pDX, IDC_GRAPHCOMBO, m_cGraphType);
	DDX_Control(pDX, IDC_SKIPPER, m_Skipper);
	DDX_Check(pDX, IDC_STACKED, m_bStacked);
}


BEGIN_MESSAGE_MAP(CStatGraphDlg, CResizableStandAloneDialog)
	ON_CBN_SELCHANGE(IDC_GRAPHCOMBO, OnCbnSelchangeGraphcombo)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_STACKED, &CStatGraphDlg::OnBnClickedStacked)
	ON_NOTIFY(TTN_NEEDTEXT, NULL, OnNeedText) 
END_MESSAGE_MAP()


// CStatGraphDlg message handlers

BOOL CStatGraphDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

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
	AddAnchor(IDC_STACKED, BOTTOM_LEFT);
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
	GetDlgItem(IDC_STACKED)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
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
	m_graph.SetGraphType(MyGraph::Bar, !!m_bStacked);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHORY);
	m_graph.SetYAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHORX);
	m_graph.SetXAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHOR);
	m_graph.SetGraphTitle(temp);

	std::map<stdstring, LONG> authorcommits;
	for (int i=0; i<m_parAuthors->GetCount(); ++i)
	{
		stdstring author = stdstring(m_parAuthors->GetAt(i));
		if (authorcommits.find(author) != authorcommits.end())
		{
			authorcommits[author] += 1;
		}
		else
			authorcommits[author] = 1;
	}

	int nGroup(-1);

	std::map<stdstring, LONG>::iterator iter;
	iter = authorcommits.begin();
	CString sOthers(MAKEINTRESOURCE(IDS_STATGRAPH_OTHERGROUP));
	int nOthers = 0;
	while (iter != authorcommits.end()) 
	{
		if (iter->second < (m_parAuthors->GetCount() * m_Skipper.GetPos() / 200))
		{
			nOthers += iter->second;
		}
		else
		{
			nGroup = m_graph.AppendGroup(iter->first.c_str());
			graphData->SetData(nGroup, iter->second);
		}
		iter++;
	}
	if (nOthers)
	{
		nGroup = m_graph.AppendGroup(sOthers);
		graphData->SetData(nGroup, nOthers);
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

	//Set up the graph.
	CString temp;
	UpdateData();
	m_graph.SetGraphType(MyGraph::Line, !!m_bStacked);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATEY);
	m_graph.SetYAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATEX);
	m_graph.SetXAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATE);
	m_graph.SetGraphTitle(temp);


	std::map<stdstring, LONG> author_commits;
	for (int i=0; i<m_parAuthors->GetCount(); ++i)
	{
		stdstring author = stdstring(m_parAuthors->GetAt(i));
		if (author_commits.find(author) != author_commits.end())
		{
			author_commits[author] += 1;
		}
		else
			author_commits[author] = 1;
	}

	//first find the number of authors available
	size_t numAuthors = 0;

	std::map<stdstring, LONG> authors;
	for (int i=0; i<m_parAuthors->GetCount(); ++i)
	{
		stdstring author = stdstring(m_parAuthors->GetAt(i));
		if (author_commits[author] < (m_parAuthors->GetCount() * m_Skipper.GetPos() / 200))
			authors[wide_string((LPCWSTR)sOthers)] = -11;
		else
			authors[author] = -11;
	}
	numAuthors = authors.size();
	
	std::map<stdstring, LONG>::iterator iter;
	iter = authors.begin();
	while (iter != authors.end()) 
	{
		authors[iter->first] = m_graph.AppendGroup(iter->first.c_str());
		iter++;
	}

	int week = 0;
	int numweeks = 0;
	// how many weeks do we cover here?
	for (int weekindex = 0; weekindex<m_parDates->GetCount(); ++weekindex)
	{
		if (week != GetWeek(CTime((__time64_t)m_parDates->GetAt(weekindex))))
		{
			week = GetWeek(CTime((__time64_t)m_parDates->GetAt(weekindex)));
			numweeks++;
		}
	}

	week = 0;
	int weekcount = 0;

	std::map<stdstring, LONG> authorcommits;

	if (m_parDates->GetCount()>0)
	{
		week = GetWeek(CTime((__time64_t)m_parDates->GetAt(m_parDates->GetCount()-1)));
	}
	CTime lasttime((__time64_t)m_parDates->GetAt(m_parDates->GetCount()-1));
	for (int i=m_parDates->GetCount()-1; i>=0; --i)
	{
		CTime time((__time64_t)m_parDates->GetAt(i));
		int timeweek = GetWeek(time);
		if (week != timeweek)
		{
			weekcount++;
			std::map<stdstring, LONG>::iterator iter;
			MyGraphSeries * graphData = new MyGraphSeries();
			iter = authors.begin();
			while (iter != authors.end()) 
			{
				if (authorcommits.find(iter->first) != authorcommits.end())
					graphData->SetData(iter->second, authorcommits[iter->first]);
				else
					graphData->SetData(iter->second, 0);
				iter++;
			}
			// If there are too many weeks, only show labels for some of them
			if ((numweeks < 10)||((numweeks%10) == (weekcount % (numweeks/10))))
			{
				temp.Format(_T("%d"), week);
				graphData->SetLabel(temp);
			}
			m_graph.AddSeries(*graphData);
			m_graphDataArray.Add(graphData);
			
			CTimeSpan oneweek = CTimeSpan(7,0,0,0);
			while (abs(timeweek - week) > 1)
			{
				lasttime += oneweek;
				week = GetWeek(lasttime);
				if (week == timeweek)
					break;		//year lap
				std::map<stdstring, LONG>::iterator iter;
				MyGraphSeries * graphData = new MyGraphSeries();
				iter = authors.begin();
				while (iter != authors.end()) 
				{
					graphData->SetData(iter->second, 0);
					iter++;
				}
				temp.Format(_T("%d"), week);
				graphData->SetLabel(temp);
				m_graph.AddSeries(*graphData);
				m_graphDataArray.Add(graphData);
			}
			week = timeweek;
			authorcommits.clear();
		}
		lasttime = time;
		stdstring author = stdstring(m_parAuthors->GetAt(i));
		if (authorcommits.find(author) != authorcommits.end())
		{
			if (authors.find(author) != authors.end())
				authorcommits[author] += 1;
			else
				authorcommits[stdstring((LPCTSTR)sOthers)] += 1;
		}
		else
		{
			if (authors.find(author) != authors.end())
				authorcommits[author] = 1;
			else if (authorcommits.find(stdstring((LPCTSTR)sOthers)) == authorcommits.end())
				authorcommits[stdstring((LPCTSTR)sOthers)] = 1;
			else
				authorcommits[stdstring((LPCTSTR)sOthers)] += 1;
		}
	}

	MyGraphSeries * graphData = new MyGraphSeries();
	iter = authors.begin();
	while (iter != authors.end()) 
	{
		if (authorcommits.find(iter->first) != authorcommits.end())
		{
			graphData->SetData(iter->second, authorcommits[iter->first]);
		}
		else
			graphData->SetData(iter->second, 0);
		iter++;
	}

	temp.Format(_T("%d"), week);
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
		stdstring author = stdstring(m_parAuthors->GetAt(i));
		if (authors.find(author) == authors.end())
		{
			authors[author] = m_graph.AppendGroup(author.c_str());
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
		stdstring author = stdstring(m_parAuthors->GetAt(i));
		if (authorcommits.find(author) != authorcommits.end())
		{
			authorcommits[author] += 1;
		}
		else
		{
			authorcommits[author] = 1;
		}
		if (nCurrentWeek != GetWeek(time))
		{	
			std::map<stdstring, LONG>::iterator iter;
			iter = authors.begin();
			while (iter != authors.end()) 
			{
				if (AuthorCommits.find(iter->first) == AuthorCommits.end())
					AuthorCommits[iter->first] = 0;
				if (AuthorCommitsMin.find(iter->first) == AuthorCommitsMin.end())
					AuthorCommitsMin[iter->first] = -1;
				if (AuthorCommitsMax.find(iter->first) == AuthorCommitsMax.end())
					AuthorCommitsMax[iter->first] = 0;
				if (authorcommits.find(iter->first) == authorcommits.end())
					authorcommits[iter->first] = 0;

				long ac = authorcommits[iter->first];
				AuthorCommits[iter->first] += ac;
				if ((AuthorCommitsMin[iter->first] == -1)||(AuthorCommitsMin[iter->first] > ac))
					AuthorCommitsMin[iter->first] = ac;
				if (AuthorCommitsMax[iter->first] < ac)
					AuthorCommitsMax[iter->first] = ac;
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
			if (AuthorCommits.find(iter->first) == AuthorCommits.end())
				AuthorCommits[iter->first] = 0;
			if (AuthorCommitsMin.find(iter->first) == AuthorCommitsMin.end())
				AuthorCommitsMin[iter->first] = -1;
			if (AuthorCommitsMax.find(iter->first) == AuthorCommitsMax.end())
				AuthorCommitsMax[iter->first] = 0;
			if (authorcommits.find(iter->first) == authorcommits.end())
				authorcommits[iter->first] = 0;

			long ac = authorcommits[iter->first];
			AuthorCommits[iter->first] += ac;
			if ((AuthorCommitsMin[iter->first] == -1)||(AuthorCommitsMin[iter->first] > ac))
				AuthorCommitsMin[iter->first] = ac;
			if (AuthorCommitsMax[iter->first] < ac)
				AuthorCommitsMax[iter->first] = ac;
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

	std::map<stdstring, LONG>::iterator iter;
	iter = AuthorCommits.begin();
	stdstring mostauthor;
	stdstring leastauthor;

	if (iter != AuthorCommits.end())
	{
		mostauthor = iter->first;
		leastauthor = iter->first;
	}
	while (iter != AuthorCommits.end()) 
	{
		if (AuthorCommits[mostauthor] < iter->second)
			mostauthor = iter->first;

		if (AuthorCommits[leastauthor] > iter->second)
			leastauthor = iter->first;

		iter++;
	}
	GetDlgItem(IDC_MOSTACTIVEAUTHORNAME)->SetWindowText(mostauthor.c_str());
	number.Format(_T("%ld"), AuthorCommits[mostauthor] / nWeeks);
	GetDlgItem(IDC_MOSTACTIVEAUTHORAVG)->SetWindowText(number);
	number.Format(_T("%ld"), AuthorCommitsMax[mostauthor]);
	GetDlgItem(IDC_MOSTACTIVEAUTHORMAX)->SetWindowText(number);
	number.Format(_T("%ld"), AuthorCommitsMin[mostauthor]);
	GetDlgItem(IDC_MOSTACTIVEAUTHORMIN)->SetWindowText(number);

	GetDlgItem(IDC_LEASTACTIVEAUTHORNAME)->SetWindowText(leastauthor.c_str());
	number.Format(_T("%ld"), AuthorCommits[leastauthor] / nWeeks);
	GetDlgItem(IDC_LEASTACTIVEAUTHORAVG)->SetWindowText(number);
	number.Format(_T("%ld"), AuthorCommitsMax[leastauthor]);
	GetDlgItem(IDC_LEASTACTIVEAUTHORMAX)->SetWindowText(number);
	number.Format(_T("%ld"), AuthorCommitsMin[leastauthor]);
	GetDlgItem(IDC_LEASTACTIVEAUTHORMIN)->SetWindowText(number);
}

void CStatGraphDlg::OnCbnSelchangeGraphcombo()
{
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

void CStatGraphDlg::OnBnClickedStacked()
{
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

void CStatGraphDlg::OnNeedText(NMHDR *pnmh, LRESULT * /*pResult*/)
{
	TOOLTIPTEXT* pttt = (TOOLTIPTEXT*) pnmh;
	if (pttt->hdr.idFrom == (UINT) m_Skipper.GetSafeHwnd())
	{
		CString string;
		string.Format(_T("%d %%"), m_Skipper.GetPos());
		::lstrcpy(pttt->szText, (LPCTSTR) string);
	}
}