// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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

#include "stdafx.h"
#include "TortoiseProc.h"
#include "StatGraphDlg.h"
#include ".\statgraphdlg.h"


// CStatGraphDlg dialog

IMPLEMENT_DYNAMIC(CStatGraphDlg, CResizableDialog)
CStatGraphDlg::CStatGraphDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CStatGraphDlg::IDD, pParent)
{
}

CStatGraphDlg::~CStatGraphDlg()
{
	for (int j=0; j<m_graphDataArray.GetCount(); ++j)
		delete ((MyGraphSeries *)m_graphDataArray.GetAt(j));
	m_graphDataArray.RemoveAll();
}

void CStatGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GRAPH, m_graph);
	DDX_Control(pDX, IDC_GRAPHCOMBO, m_cGraphType);
}


BEGIN_MESSAGE_MAP(CStatGraphDlg, CResizableDialog)
	ON_CBN_SELCHANGE(IDC_GRAPHCOMBO, OnCbnSelchangeGraphcombo)
END_MESSAGE_MAP()


// CStatGraphDlg message handlers

BOOL CStatGraphDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

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

	AddAnchor(IDOK, BOTTOM_RIGHT);
	EnableSaveRestore(_T("StatGraphDlg"));
	ShowStats();

	return TRUE;
}

void CStatGraphDlg::ShowLabels(BOOL bShow)
{
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
}

void CStatGraphDlg::ShowCommitsByAuthor()
{
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
	m_graph.SetGraphType(MyGraph::Bar);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHORX);
	m_graph.SetYAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHORY);
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
	while (iter != authorcommits.end()) 
	{
		nGroup = m_graph.AppendGroup(iter->first.c_str());
		graphData->SetData(nGroup, iter->second);
		iter++;
	}

	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::ShowCommitsByDate()
{
	ShowLabels(FALSE);
	m_graph.Clear();

	for (int j=0; j<m_graphDataArray.GetCount(); ++j)
		delete ((MyGraphSeries *)m_graphDataArray.GetAt(j));
	m_graphDataArray.RemoveAll();

	//Set up the graph.
	CString temp;
	m_graph.SetGraphType(MyGraph::Line);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATEY);
	m_graph.SetYAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATEX);
	m_graph.SetXAxisLabel(temp);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATE);
	m_graph.SetGraphTitle(temp);

	//first find the number of authors available
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

	int week = 0;

	std::map<stdstring, LONG> authorcommits;

	if (m_parDates->GetCount()>0)
		week = GetWeek(CTime((__time64_t)m_parDates->GetAt(m_parDates->GetCount()-1)));
	BOOL weekover = FALSE;
	for (int i=m_parDates->GetCount()-1; i>=0; --i)
	{
		weekover = FALSE;
		CTime time((__time64_t)m_parDates->GetAt(i));
		if (week != GetWeek(time))
		{
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
			temp.Format(_T("%d"), week);
			graphData->SetLabel(temp);
			m_graph.AddSeries(*graphData);
			m_graphDataArray.Add(graphData);
			week = GetWeek(time);
			authorcommits.clear();
			weekover = TRUE;
		}
		stdstring author = stdstring(m_parAuthors->GetAt(i));
		if (authorcommits.find(author) != authorcommits.end())
		{
			authorcommits[author] += 1;
		}
		else
			authorcommits[author] = 1;
	}
	if (!weekover)
	{
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
		temp.Format(_T("%d"), week);
		graphData->SetLabel(temp);
		m_graph.AddSeries(*graphData);
		m_graphDataArray.Add(graphData);
		authorcommits.clear();
	}


	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::ShowStats()
{
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
	for (int i=m_parDates->GetCount()-1; i>=0; --i)
	{
		CTime time((__time64_t)m_parDates->GetAt(i));
		commits++;
		filechanges += m_parFileChanges->GetAt(i);
		weekover = FALSE;
		if (nCurrentWeek != GetWeek(time))
		{	
			std::map<stdstring, LONG>::iterator iter;
			iter = authorcommits.begin();
			while (iter != authorcommits.end()) 
			{
				if (AuthorCommits.find(iter->first) == AuthorCommits.end())
					AuthorCommits[iter->first] = 0;
				if (AuthorCommitsMin.find(iter->first) == AuthorCommitsMin.end())
					AuthorCommitsMin[iter->first] = 0;
				if (AuthorCommitsMax.find(iter->first) == AuthorCommitsMax.end())
					AuthorCommitsMax[iter->first] = 0;

				AuthorCommits[iter->first] += iter->second;
				if ((AuthorCommitsMin[iter->first] == 0)||(AuthorCommitsMin[iter->first] > iter->second))
					AuthorCommitsMin[iter->first] = iter->second;
				if (AuthorCommitsMax[iter->first] < iter->second)
					AuthorCommitsMax[iter->first] = iter->second;
				iter++;
			}
			authorcommits.clear();

			nWeeks++;
			nCurrentWeek = GetWeek(time);
			if ((nCommitsMin == 0)||(nCommitsMin > commits))
				nCommitsMin = commits;
			if (nCommitsMax < commits)
				nCommitsMax = commits;
			commits = 0;
			if ((nFileChangesMin == 0)||(nFileChangesMin > filechanges))
				nFileChangesMin = filechanges;
			if (nFileChangesMax < filechanges)
				nFileChangesMax = filechanges;
			nFileChanges += filechanges;
			filechanges = 0;
			weekover = TRUE;
		}
		stdstring author = stdstring(m_parAuthors->GetAt(i));
		if (authorcommits.find(author) != authorcommits.end())
		{
			authorcommits[author] += 1;
		}
		else
		{
			authorcommits[author] = 1;
		}
	} // for (int i=m_parDates->GetCount()-1; i>=0; --i)
	if (!weekover)
	{
		std::map<stdstring, LONG>::iterator iter;
		iter = authorcommits.begin();
		while (iter != authorcommits.end()) 
		{
			if (AuthorCommits.find(iter->first) == AuthorCommits.end())
				AuthorCommits[iter->first] = 0;
			if (AuthorCommitsMin.find(iter->first) == AuthorCommitsMin.end())
				AuthorCommitsMin[iter->first] = 0;
			if (AuthorCommitsMax.find(iter->first) == AuthorCommitsMax.end())
				AuthorCommitsMax[iter->first] = 0;

			AuthorCommits[iter->first] += iter->second;
			if ((AuthorCommitsMin[iter->first] == 0)||(AuthorCommitsMin[iter->first] > iter->second))
				AuthorCommitsMin[iter->first] = iter->second;
			if (AuthorCommitsMax[iter->first] < iter->second)
				AuthorCommitsMax[iter->first] = iter->second;
			iter++;
		}
		authorcommits.clear();

		nWeeks++;

		if ((nCommitsMin == 0)||(nCommitsMin > commits))
			nCommitsMin = commits;
		if (nCommitsMax < commits)
			nCommitsMax = commits;
		commits = 0;
		if ((nFileChangesMin == 0)||(nFileChangesMin > filechanges))
			nFileChangesMin = filechanges;
		if (nFileChangesMax < filechanges)
			nFileChangesMax = filechanges;
		nFileChanges += filechanges;
		filechanges = 0;
	} // if (!weekover)

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
	CTime pTmpTime(time.GetYear(), 1, 1, 0,0,0);

	// Get the first Sunday (start of year);
	while(pTmpTime.GetDayOfWeek() != 1)
	{
		pTmpTime += 86400;
	}

	CTimeSpan tmSpan = (time - pTmpTime);

	int iResult = int(tmSpan.GetDays() / 7) + 1;

	return iResult;
}