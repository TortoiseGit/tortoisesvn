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
		delete (MyGraphSeries *)m_graphDataArray.GetAt(j);
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
	temp.LoadString(IDS_STATGRAPH_COMMITSBYAUTHOR);
	sel = m_cGraphType.AddString(temp);
	m_cGraphType.SetItemData(sel, 1);
	m_cGraphType.SetCurSel(sel);
	temp.LoadString(IDS_STATGRAPH_COMMITSBYDATE);
	sel = m_cGraphType.AddString(temp);
	m_cGraphType.SetItemData(sel, 2);

	AddAnchor(IDC_GRAPHTYPELABEL, TOP_LEFT);
	AddAnchor(IDC_GRAPH, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_GRAPHCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);

	ShowCommitsByAuthor();

	return TRUE;
}

void CStatGraphDlg::ShowCommitsByAuthor()
{
	m_graph.Clear();
	m_graphData.Clear();
	m_graph.AddSeries(m_graphData);

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
		m_graphData.SetData(nGroup, iter->second);
		iter++;
	}

	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::ShowCommitsByDate()
{
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
	for (int i=m_parDates->GetCount()-1; i>=0; --i)
	{
		stdstring author = stdstring(m_parAuthors->GetAt(i));
		if (authorcommits.find(author) != authorcommits.end())
		{
			authorcommits[author] += 1;
		}
		else
			authorcommits[author] = 1;
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
		}
	}


	// Paint the graph now that we're through.
	m_graph.Invalidate();
}

void CStatGraphDlg::OnCbnSelchangeGraphcombo()
{
	switch (m_cGraphType.GetItemData(m_cGraphType.GetCurSel()))
	{
	case 1:
		ShowCommitsByAuthor();
		break;
	case 2:
		ShowCommitsByDate();
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