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
#include "resource.h"
#include "..\\TortoiseShell\\resource.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "Utils.h"
#include "StringUtils.h"
#include "DirFileEnum.h"
#include "SVNConfig.h"
#include "SVNProperties.h"
#include "SVN.h"
#include "LogDlg.h"
#include "SVNProgressDlg.h"
#include ".\svnstatuslistctrl.h"

#define IDSVNLC_REVERT			1
#define IDSVNLC_COMPARE			2
#define IDSVNLC_OPEN			3
#define IDSVNLC_DELETE			4
#define IDSVNLC_IGNORE			5
#define IDSVNLC_GNUDIFF1		6
#define IDSVNLC_UPDATE			7
#define IDSVNLC_LOG				8
#define IDSVNLC_EDITCONFLICT	9
#define IDSVNLC_IGNOREMASK	   10

BEGIN_MESSAGE_MAP(CSVNStatusListCtrl, CListCtrl)
	ON_NOTIFY(HDN_ITEMCLICKA, 0, OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, OnHdnItemclick)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnLvnItemchanged)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


BOOL	CSVNStatusListCtrl::m_bAscending = FALSE;
int		CSVNStatusListCtrl::m_nSortedColumn = -1;

CSVNStatusListCtrl::CSVNStatusListCtrl() : CListCtrl()
{
	m_pStatLabel = NULL;
	m_pSelectButton = NULL;
}

CSVNStatusListCtrl::~CSVNStatusListCtrl()
{
	for (int i=0; i<m_templist.GetCount(); i++)
	{
		DeleteFile(m_templist.GetAt(i));
	}
	m_templist.RemoveAll();
	for (int i=0; i<m_arStatusArray.GetCount(); i++)
	{
		FileEntry * entry = m_arStatusArray.GetAt(i);
		delete entry;
	} 
	m_arStatusArray.RemoveAll();
}

CSVNStatusListCtrl::FileEntry * CSVNStatusListCtrl::GetListEntry(int index)
{
	if (index < 0)
		return NULL;
	if (index >= m_arListArray.GetCount())
		return NULL;
	if ((INT_PTR)m_arListArray.GetAt(index) >= m_arStatusArray.GetCount())
		return NULL;
	return m_arStatusArray.GetAt(m_arListArray.GetAt(index));
}

void CSVNStatusListCtrl::Init(DWORD dwColumns, bool bHasCheckboxes /* = TRUE */)
{
	m_dwColumns = dwColumns;
	// set the extended style of the listcontrol
	DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
	exStyle |= (bHasCheckboxes ? LVS_EX_CHECKBOXES : 0);
	SetExtendedStyle(exStyle);

	// clear all previously set header columns
	DeleteAllItems();
	int c = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		DeleteColumn(c--);

	// now set up the requested columns
	CString temp;
	int nCol = 0;
	// the relative path is always visible
	temp.LoadString(IDS_STATUSLIST_COLFILE);
	InsertColumn(nCol++, temp);
	if (dwColumns & SVNSLC_COLSTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLSTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLREMOTESTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLREMOTESTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLTEXTSTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLTEXTSTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLPROPSTATUS)
	{
		temp.LoadString(IDS_STATUSLIST_COLPROPSTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLREMOTETEXT)
	{
		temp.LoadString(IDS_STATUSLIST_COLREMOTETEXTSTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLREMOTEPROP)
	{
		temp.LoadString(IDS_STATUSLIST_COLREMOTEPROPSTATUS);
		InsertColumn(nCol++, temp);
	}
	if (dwColumns & SVNSLC_COLURL)
	{
		temp.LoadString(IDS_STATUSLIST_COLURL);
		InsertColumn(nCol++, temp);
	}

	SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	SetRedraw(true);
}

BOOL CSVNStatusListCtrl::GetStatus(CString sFilePath, bool bUpdate /* = FALSE */)
{
	BOOL bRet = TRUE;
	m_nTargetCount = 0;
	m_bHasExternalsFromDifferentRepos = FALSE;
	m_bHasExternals = FALSE;
	
	m_bBlock = TRUE;

	SVNConfig config;

	// first clear possible status data left from
	// previous GetStatus() calls
	for (int i=0; i<m_arStatusArray.GetCount(); i++)
	{
		FileEntry * entry = m_arStatusArray.GetAt(i);
		delete entry;
	}
	m_arStatusArray.RemoveAll();

	try
	{
		// Since svn_client_status() returns all files, even those in
		// folders included with svn:externals we need to check if all
		// files we get here belong to the same repository.
		// It is possible to commit changes in an external folder, as long
		// as the folder belongs to the same repository (but another path),
		// but it is not possible to commit all files if the externals are
		// from a different repository.
		//
		// To check if all files belong to the same repository, we compare the
		// UUID's - if they're identical then the files belong to the same
		// repository and can be committed. But if they're different, then
		// tell the user to commit all changes in the external folders
		// first and exit.
		CStringA sUUID;					// holds the repo UUID
		CStringArray arExtPaths;		// list of svn:external paths

		CString strLine = _T("");
		const TCHAR * strbuf = NULL;
		m_sURL.Empty();

		CStdioFile file(sFilePath, CFile::typeBinary | CFile::modeRead);
		// for every selected file/folder
		while (file.ReadString(strLine))
		{
			m_nTargetCount++;
			strLine.Replace('\\', '/');

			// remove trailing / characters since they mess up the filename list
			// However "/" and "c:/" will be left alone.
			if (strLine.GetLength() > 1 && strLine.Right(1) == _T("/")) 
			{
				strLine.Delete(strLine.GetLength()-1,1);
			}
			BOOL bIsFolder = PathIsDirectory(strLine);
			SVNStatus status;
			svn_wc_status_t * s;
			s = status.GetFirstFileStatus(strLine, &strbuf, bUpdate);

			// This one fixes a problem with externals: 
			// If a strLine is a file, svn:externals at its parent directory
			// will also be returned by GetXXXFileStatus. Hence, we skip all
			// status info until we found the one matching strLine.
			if (!bIsFolder)
			{
				if (strLine.CompareNoCase(strbuf)!=0)
				{
					while (s != 0)
					{
						CString temp = strbuf;
						temp.Replace('\\', '/');
						if (temp == strLine)
							break;
						s = status.GetNextFileStatus(&strbuf);
					}
					strLine = strLine.Left(strLine.ReverseFind('/'));
				}
			}

			if (s!=0)
			{
				CString temp;
				if ((s->entry)&&(s->entry->uuid))
				{
					if (sUUID.IsEmpty())
						sUUID = s->entry->uuid;
					else
					{
						if (sUUID.Compare(s->entry->uuid)!=0)
						{
							m_bHasExternalsFromDifferentRepos = TRUE;
							if (s->entry->kind == svn_node_dir)
								arExtPaths.Add(strLine);
						} 
					}
				}
				if ((SVNStatus::GetMoreImportant(s->text_status, s->prop_status) == svn_wc_status_unversioned)&&(PathIsDirectory(strbuf)))
				{
					// check if the unversioned folder is maybe versioned. This
					// could happen with nested layouts
					if (SVNStatus::GetAllStatus(temp) != svn_wc_status_unversioned)
						continue;	//ignore nested layouts
				}
				if (s->text_status == svn_wc_status_external)
					m_bHasExternals = TRUE;
				FileEntry * entry = new FileEntry();
				entry->path = strbuf;
				entry->basepath = strLine;
				entry->status = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
				entry->textstatus = s->text_status;
				entry->propstatus = s->prop_status;
				entry->remotestatus = SVNStatus::GetMoreImportant(s->repos_text_status, s->repos_prop_status);
				entry->remotetextstatus = s->repos_text_status;
				entry->remotepropstatus = s->repos_prop_status;
				entry->inunversionedfolder = FALSE;
				entry->checked = FALSE;
				entry->inexternal = m_bHasExternals;
				entry->direct = TRUE;
				if (s->entry)
				{
					if (s->entry->url)
					{
						CUtils::Unescape((char *)s->entry->url);
						entry->url = CUnicodeUtils::GetUnicode(s->entry->url);
					}
					if (m_sURL.IsEmpty())
						m_sURL = entry->url;
					else
						m_sURL.LoadString(IDS_STATUSLIST_MULTIPLETARGETS);
				}
				m_arStatusArray.Add(entry);
				if ((SVNStatus::GetMoreImportant(s->text_status, s->prop_status) == svn_wc_status_unversioned)&&(PathIsDirectory(strbuf)))
				{
					//we have an unversioned folder -> get all files in it recursively!
					CDirFileEnum filefinder(strbuf);
					CString filename;
					while (filefinder.NextFile(filename))
					{
						if (!config.MatchIgnorePattern(filename))
						{
							filename.Replace('\\', '/');
							FileEntry * entry = new FileEntry();
							entry->path = filename;									
							entry->basepath = strLine.Left(strLine.ReverseFind('/'));
							entry->status = svn_wc_status_unversioned;
							entry->textstatus = svn_wc_status_unversioned;
							entry->propstatus = svn_wc_status_unversioned;
							entry->remotestatus = svn_wc_status_unversioned;
							entry->remotetextstatus = svn_wc_status_unversioned;
							entry->remotepropstatus = svn_wc_status_unversioned;
							entry->inunversionedfolder = FALSE;
							entry->checked = FALSE;
							entry->inexternal = FALSE;
							entry->direct = FALSE;
							m_arStatusArray.Add(entry);
						}
					} // while (filefinder.NextFile(filename))
				}
				// for folders, get all statuses inside it too
				while (bIsFolder && ((s = status.GetNextFileStatus(&strbuf)) != NULL))
				{
					if ((SVNStatus::GetMoreImportant(s->text_status, s->prop_status) == svn_wc_status_unversioned) && (PathIsDirectory(strbuf)))
					{
						// check if the unversioned folder is maybe versioned. This
						// could happen with nested layouts
						if (SVNStatus::GetAllStatus(strbuf) != svn_wc_status_unversioned)
							continue;	//ignore nested layouts
					}

					if (SVNStatus::IsImportant(SVNStatus::GetMoreImportant(s->text_status, s->prop_status)))
					{
						if (s->entry)
						{
							if (s->entry->uuid)
							{
								if (sUUID.IsEmpty())
									sUUID = s->entry->uuid;
								else
								{
									if (sUUID.Compare(s->entry->uuid)!=0)
									{
										m_bHasExternalsFromDifferentRepos = TRUE;
										if (s->entry->kind == svn_node_dir)
											arExtPaths.Add(strbuf);
										continue;
									}
								}
							} 
							else
							{
								// added files don't have an UUID assigned yet, so check if they're
								// below an external folder
								BOOL bMatch = FALSE;
								for (int ix=0; ix<arExtPaths.GetCount(); ix++)
								{
									CString t = arExtPaths.GetAt(ix);
									if (t.CompareNoCase(temp.Left(t.GetLength()))==0)
									{
										bMatch = TRUE;
										break;
									}
								}
								if (bMatch)
									continue;
							}
						} // if (s->entry)
					} // if (SVNStatus::IsImportant(SVNStatus::GetMoreImportant(s->text_status, s->prop_status))) 

					if (s->text_status == svn_wc_status_external)
						m_bHasExternals = TRUE;

					FileEntry * entry = new FileEntry();
					entry->path = strbuf;
					entry->basepath = strLine;
					entry->status = SVNStatus::GetMoreImportant(s->text_status, s->prop_status);
					entry->textstatus = s->text_status;
					entry->propstatus = s->prop_status;
					entry->remotestatus = SVNStatus::GetMoreImportant(s->repos_text_status, s->repos_prop_status);
					entry->remotetextstatus = s->repos_text_status;
					entry->remotepropstatus = s->repos_prop_status;
					entry->inunversionedfolder = FALSE;
					entry->checked = FALSE;
					entry->inexternal = m_bHasExternals;
					entry->direct = FALSE;
					if (s->entry)
					{
						if (s->entry->url)
						{
							CUtils::Unescape((char *)s->entry->url);
							entry->url = CUnicodeUtils::GetUnicode(s->entry->url);
						}
					}
					m_arStatusArray.Add(entry);
					if ((entry->status == svn_wc_status_unversioned)&&(!config.MatchIgnorePattern(strbuf)))
					{
						if (PathIsDirectory(strbuf))
						{
							//we have an unversioned folder -> get all files in it recursively!
							CDirFileEnum filefinder(strbuf);
							CString filename;
							while (filefinder.NextFile(filename))
							{
								if (!config.MatchIgnorePattern(filename))
								{
									filename.Replace('\\', '/');
									FileEntry * entry = new FileEntry();
									entry->path = filename;									
									entry->basepath = strLine;
									entry->status = svn_wc_status_unversioned;
									entry->textstatus = svn_wc_status_unversioned;
									entry->propstatus = svn_wc_status_unversioned;
									entry->remotestatus = svn_wc_status_unversioned;
									entry->remotetextstatus = svn_wc_status_unversioned;
									entry->remotepropstatus = svn_wc_status_unversioned;
									entry->inunversionedfolder = FALSE;
									entry->checked = FALSE;
									entry->inexternal = FALSE;
									entry->direct = FALSE;
									m_arStatusArray.Add(entry);
								}
							} // while (filefinder.NextFile(filename))
						} // if (PathIsDirectory(temp))
					} // if ((stat == svn_wc_status_unversioned)&&(!config.MatchIgnorePattern(strbuf)))
				} // while (bIsFolder && ((s = status.GetNextFileStatus(&strbuf)) != NULL))
			} // if (s!=0) 
			else
			{
				bRet = FALSE;
				m_sLastError = status.GetLastErrorMsg();
			}
		} // while (file.ReadString(strLine)) 
		file.Close();
	}
	catch (CFileException* pE)
	{
		TRACE("CFileException in Commit!\n");
		pE->Delete();
	}
	Stat();
	m_bBlock = FALSE;
	return bRet;
}

void CSVNStatusListCtrl::Show(DWORD dwShow)
{
	AfxGetApp()->DoWaitCursor(1);
	m_dwShow = dwShow;
	SetRedraw(FALSE);
	DeleteAllItems();
	m_arListArray.RemoveAll();
	for (int i=0; i<m_arStatusArray.GetCount(); ++i)
	{
		FileEntry * entry = m_arStatusArray.GetAt(i);
		if (entry->inexternal && (!(dwShow & SVNSLC_SHOWINEXTERNALS)))
			continue;
		svn_wc_status_kind status = SVNStatus::GetMoreImportant(entry->status, entry->remotestatus);
		switch (status)
		{
		case 0:
		case svn_wc_status_none:
		case svn_wc_status_unversioned:
			if (dwShow & SVNSLC_SHOWUNVERSIONED)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_ignored:
			if ((entry->direct) && (dwShow & SVNSLC_SHOWDIRECTS))
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_incomplete:
			if (dwShow & SVNSLC_SHOWINCOMPLETE)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_normal:
			if (dwShow & SVNSLC_SHOWNORMAL)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_external:
			if (dwShow & SVNSLC_SHOWEXTERNAL)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_added:
			if (dwShow & SVNSLC_SHOWADDED)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_missing:
			if (dwShow & SVNSLC_SHOWMISSING)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_deleted:
			if (dwShow & SVNSLC_SHOWREMOVED)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_replaced:
			if (dwShow & SVNSLC_SHOWREPLACED)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_modified:
			if (dwShow & SVNSLC_SHOWMODIFIED)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_merged:
			if (dwShow & SVNSLC_SHOWMERGED)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_conflicted:
			if (dwShow & SVNSLC_SHOWCONFLICTED)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		case svn_wc_status_obstructed:
			if (dwShow & SVNSLC_SHOWOBSTRUCTED)
			{
				m_arListArray.Add(i);
				AddEntry(entry);
			}
			break;
		default:
			break;		// we should NEVER get here!
		}
	} // for (int i=0; i<m_arStatusArray.GetCount(); ++i)
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	SetRedraw(TRUE);
	GetStatistics();
	AfxGetApp()->DoWaitCursor(-1);
}

void CSVNStatusListCtrl::CheckAll(DWORD dwCheck)
{
	SelectAll(FALSE);
	AfxGetApp()->DoWaitCursor(1);
	SetRedraw(FALSE);
	m_bBlock = TRUE;
	for (int i=0; i<m_arListArray.GetCount(); ++i)
	{
		FileEntry * entry = GetListEntry(i);
		svn_wc_status_kind status = SVNStatus::GetMoreImportant(entry->status, entry->remotestatus);
		switch (status)
		{
		case 0:
		case svn_wc_status_none:
		case svn_wc_status_unversioned:
			if (dwCheck & SVNSLC_SHOWUNVERSIONED)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_ignored:
			break;
		case svn_wc_status_incomplete:
			if (dwCheck & SVNSLC_SHOWINCOMPLETE)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_normal:
			if (dwCheck & SVNSLC_SHOWNORMAL)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_external:
			if (dwCheck & SVNSLC_SHOWEXTERNAL)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_added:
			if (dwCheck & SVNSLC_SHOWADDED)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_missing:
			if (dwCheck & SVNSLC_SHOWMISSING)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_deleted:
			if (dwCheck & SVNSLC_SHOWREMOVED)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_replaced:
			if (dwCheck & SVNSLC_SHOWREPLACED)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_modified:
			if (dwCheck & SVNSLC_SHOWMODIFIED)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_merged:
			if (dwCheck & SVNSLC_SHOWMERGED)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_conflicted:
			if (dwCheck & SVNSLC_SHOWCONFLICTED)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		case svn_wc_status_obstructed:
			if (dwCheck & SVNSLC_SHOWOBSTRUCTED)
			{
				SetCheck(i);
				entry->checked = TRUE;
				m_nSelected++;
			}
			break;
		default:
			break;		// we should NEVER get here!
		}
	} // for (int i=0; i<m_arStatusArray.GetCount(); ++i)
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	SetRedraw(TRUE);
	GetStatistics();
	m_bBlock = FALSE;
	AfxGetApp()->DoWaitCursor(-1);
}

void CSVNStatusListCtrl::AddEntry(FileEntry * entry)
{
	CRegStdWORD langID = CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
	m_bBlock = TRUE;
	TCHAR buf[100];
	int index = GetItemCount();
	int nCol = 1;
	CString ponly;
	ponly.LoadString(IDS_STATUSLIST_PROPONLY);
	CString entryname = entry->path.Right(entry->path.GetLength() - entry->basepath.GetLength() - 1);
	if (entryname.IsEmpty())
		entryname = entry->path.Mid(entry->path.ReverseFind('/')+1);
	InsertItem(index, entryname);
	if (m_dwColumns & SVNSLC_COLSTATUS)
	{
		SVNStatus::GetStatusString(AfxGetResourceHandle(), entry->status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		if ((entry->status == entry->propstatus)&&
			(entry->status != svn_wc_status_normal)&&
			(entry->status != svn_wc_status_unversioned)&&
			(!SVNStatus::IsImportant(entry->textstatus)))
			_tcscat(buf, ponly);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLREMOTESTATUS)
	{
		SVNStatus::GetStatusString(AfxGetResourceHandle(), entry->remotestatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		if ((entry->remotestatus == entry->remotepropstatus)&&
			(entry->status != svn_wc_status_normal)&&
			(entry->status != svn_wc_status_unversioned)&&
			(!SVNStatus::IsImportant(entry->remotetextstatus)))
			_tcscat(buf, ponly);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLTEXTSTATUS)
	{
		SVNStatus::GetStatusString(AfxGetResourceHandle(), entry->textstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLPROPSTATUS)
	{
		SVNStatus::GetStatusString(AfxGetResourceHandle(), entry->propstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLREMOTETEXT)
	{
		SVNStatus::GetStatusString(AfxGetResourceHandle(), entry->remotetextstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLREMOTEPROP)
	{
		SVNStatus::GetStatusString(AfxGetResourceHandle(), entry->remotepropstatus, buf, sizeof(buf)/sizeof(TCHAR), (WORD)langID);
		SetItemText(index, nCol++, buf);
	}
	if (m_dwColumns & SVNSLC_COLURL)
	{
		SetItemText(index, nCol++, entry->url);
	}
	SetCheck(index, entry->checked);
	m_bBlock = FALSE;
}

void CSVNStatusListCtrl::Sort()
{
	qsort(m_arStatusArray.GetData(), m_arStatusArray.GetSize(), sizeof(FileEntry *), (GENERICCOMPAREFN)SortCompare);
	Show(m_dwShow);
}

int CSVNStatusListCtrl::SortCompare(const void * pElem1, const void * pElem2)
{
	FileEntry * entry1 = *((FileEntry**)pElem1);
	FileEntry * entry2 = *((FileEntry**)pElem2);
	int result = 0;

	switch (m_nSortedColumn)
	{
	case 7:
		{
			if (result == 0)
			{
				result = entry1->url.Compare(entry2->url);
			}
		}
	case 6:
		{
			if (result == 0)
			{
				result = entry1->remotepropstatus - entry2->remotepropstatus;
			}
		}
	case 5:
		{
			if (result == 0)
			{
				result = entry1->remotetextstatus - entry2->remotetextstatus;
			}
		}
	case 4:
		{
			if (result == 0)
			{
				result = entry1->propstatus - entry2->propstatus;
			}
		}
	case 3:
		{
			if (result == 0)
			{
				result = entry1->textstatus - entry2->textstatus;
			}
		}
	case 2:
		{
			if (result == 0)
			{
				result = entry1->remotestatus - entry2->remotestatus;
			}
		}
	case 1:
		{
			if (result == 0)
			{
				result = entry1->status - entry2->status;
			}
		}
	case 0:		//path column
		{
			if (result == 0)
			{
				result = entry1->path.Compare(entry2->path);
			}
		}
		break;
	default:
		break;
	} // switch (m_nSortedColumn)
	if (!m_bAscending)
		result = -result;
	return result;
}

void CSVNStatusListCtrl::OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;
	if (m_bBlock)
		return;
	m_bBlock = TRUE;
	if (m_nSortedColumn == phdr->iItem)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = TRUE;
	m_nSortedColumn = 0;
	// get the internal column from the visible columns
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLSTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLREMOTESTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLTEXTSTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLPROPSTATUS)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLREMOTETEXT)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLREMOTEPROP)
			m_nSortedColumn++;
	}
	if (m_nSortedColumn != phdr->iItem)
	{
		if (m_dwColumns & SVNSLC_COLURL)
			m_nSortedColumn++;
	}
	Sort();

#ifdef UNICODE
	CHeaderCtrl * pHeader = GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i=0; i<pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	pHeader->GetItem(m_nSortedColumn, &HeaderItem);
	HeaderItem.fmt |= (m_bAscending ? HDF_SORTDOWN : HDF_SORTUP);
	pHeader->SetItem(m_nSortedColumn, &HeaderItem);
#endif
	m_bBlock = FALSE;
}

void CSVNStatusListCtrl::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if ((pNMLV->uNewState==0)||(pNMLV->uNewState & LVIS_SELECTED))
		return;

	if (m_bBlock)
		return;

	FileEntry * entry = GetListEntry(pNMLV->iItem);
	if (entry == NULL)
		return;
	m_bBlock = TRUE;
	//was the item checked?
	if (GetCheck(pNMLV->iItem))
	{
		// if an unversioned item was checked, then we need to check if
		// the parent folders are unversioned too. If the parent folders actually
		// are unversioned, then check those too.
		if (entry->status == svn_wc_status_unversioned)
		{
			//we need to check the parent folder too
			CString folderpath = entry->path;
			for (int i=0; i<GetItemCount(); ++i)
			{
				if (!GetCheck(i))
				{
					FileEntry * e = GetListEntry(i);
					if (e == NULL)
						continue;
					CString t = e->path;
					if (CUtils::PathIsParent(t, folderpath))
					{
						SetCheck(i, TRUE);
						e->checked = TRUE;
						m_nSelected++;
					} 
				}
			}
		}
		if (entry->status == svn_wc_status_deleted)
		{
			// if a deleted folder gets checked, we have to check all
			// children of that folder too.
			if (PathIsDirectory(entry->path))
			{
				for (int j=0; j<GetItemCount(); ++j)
				{
					if (!GetCheck(j))
					{
						FileEntry * dd = GetListEntry(j);
						if (dd == NULL)
							continue;
						CString tt = dd->path;
						if (entry->path.CompareNoCase(tt.Left(entry->path.GetLength()))==0)
						{
							SetCheck(j, TRUE);
							dd->checked = TRUE;
							m_nSelected++;
						}
					}
				}
			}

			// if a deleted file or folder gets checked, we have to
			// check all parents of this item too.
			for (int i=0; i<GetItemCount(); ++i)
			{
				if (!GetCheck(i))
				{
					FileEntry * d = GetListEntry(i);
					if (d == NULL)
						continue;
					CString t = d->path;
					if (CUtils::PathIsParent(t, entry->path))
					{
						if (d->status == svn_wc_status_deleted)
						{
							SetCheck(i, TRUE);
							d->checked = TRUE;
							m_nSelected++;
							//now we need to check all children of this parent folder
							for (int j=0; j<GetItemCount(); j++)
							{
								if (!GetCheck(j))
								{
									FileEntry * dd = GetListEntry(j);
									if (dd == NULL)
										continue;
									CString tt = dd->path;
									if (t.CompareNoCase(tt.Left(t.GetLength()))==0)
									{
										SetCheck(j, TRUE);
										dd->checked = TRUE;
										m_nSelected++;
									}
								}
							}
						}
					}
				} // if (!GetCheck(i)) 
			} // for (int i=0; i<GetItemCount(); ++i)
		} // if (entry->status == svn_wc_status_deleted) 
		entry->checked = TRUE;
		m_nSelected++;
	} // if (GetCheck(pNMLV->iItem)) 
	else
	{
		//item was unchecked
		if (PathIsDirectory(entry->path))
		{
			//disable all files within an unselected folder
			for (int i=0; i<GetItemCount(); i++)
			{
				if (GetCheck(i))
				{
					FileEntry * d = GetListEntry(i);
					if (d == NULL)
						continue;
					CString t = d->path;
					if (CUtils::PathIsParent(entry->path, t))
					{
						SetCheck(i, FALSE);
						d->checked = FALSE;
						m_nSelected--;
					}
				}
			}
		}
		else if (entry->status == svn_wc_status_deleted)
		{
			//a "deleted" file was unchecked, so uncheck all parent folders
			//and all children of those parents
			for (int i=0; i<GetItemCount(); i++)
			{
				if (GetCheck(i))
				{
					FileEntry * d = GetListEntry(i);
					if (d == NULL)
						continue;
					CString t = d->path;
					if (CUtils::PathIsParent(t, entry->path))
					{
						if (d->status == svn_wc_status_deleted)
						{
							SetCheck(i, FALSE);
							d->checked = FALSE;
							m_nSelected--;
							//now we need to check all children of this parent folder
							t += _T("\\");
							for (int j=0; j<GetItemCount(); j++)
							{
								if (GetCheck(j))
								{
									FileEntry * dd = GetListEntry(j);
									if (dd == NULL)
										continue;
									CString tt = dd->path;
									if (t.CompareNoCase(tt.Left(t.GetLength()))==0)
									{
										SetCheck(j, FALSE);
										dd->checked = FALSE;
										m_nSelected--;
									}
								}
							}
						}
					}
				}
			} // for (int i=0; i<GetItemCount(); i++)
		} // else if (entry->status == svn_wc_status_deleted)
		entry->checked = FALSE;
		m_nSelected--;
	} // else -> from if (GetCheck(pNMLV->iItem)) 
	GetStatistics();
	m_bBlock = FALSE;
}

void CSVNStatusListCtrl::Stat()
{
	//now gather some statistics
	m_nUnversioned = 0;
	m_nNormal = 0;
	m_nModified = 0;
	m_nAdded = 0;
	m_nDeleted = 0;
	m_nConflicted = 0;
	m_nTotal = 0;
	m_nSelected = 0;
	for (int i=0; i<m_arStatusArray.GetCount(); ++i)
	{
		FileEntry * entry = m_arStatusArray.GetAt(i);
		if (entry)
		{
			switch (entry->status)
			{
			case svn_wc_status_normal:
				m_nNormal++;
				break;
			case svn_wc_status_added:
				m_nAdded++;
				break;
			case svn_wc_status_missing:
			case svn_wc_status_deleted:
				m_nDeleted++;
				break;
			case svn_wc_status_replaced:
			case svn_wc_status_modified:
			case svn_wc_status_merged:
				m_nModified++;
				break;
			case svn_wc_status_conflicted:
			case svn_wc_status_obstructed:
				m_nConflicted++;
				break;
			case svn_wc_status_ignored:
				m_nUnversioned++;
				break;
			default:
				{
					if (SVNStatus::IsImportant(entry->remotestatus))
						break;
					m_nUnversioned++;
					if (!entry->inunversionedfolder)
					{
						// check if the unversioned item is just
						// a file differing in case but still versioned
						for (int j=0; j<m_arStatusArray.GetCount(); ++j)
						{
							FileEntry * d = m_arStatusArray.GetAt(j);
							if ((d->status != svn_wc_status_unversioned)&&(entry->path.CompareNoCase(d->path)==0))
							{
								// adjust the case of the filename
								MoveFileEx(entry->path, d->path, MOVEFILE_REPLACE_EXISTING);
								DeleteItem(i);
								m_arStatusArray.RemoveAt(i);
								delete entry;
								i--;
								m_nUnversioned--;
								break;
							}
						}
					}
				}
				break;
			} // switch (entry->status) 
		} // if (entry) 
	} // for (int i=0; i<m_arStatusArray.GetCount(); ++i)
}

void CSVNStatusListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (pWnd == this)
	{
		int selIndex = GetSelectionMark();
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			GetItemRect(selIndex, &rect, LVIR_LABEL);
			ClientToScreen(&rect);
			point.x = rect.left + rect.Width()/2;
			point.y = rect.top + rect.Height()/2;
		}
		if (selIndex >= 0)
		{
			FileEntry * entry = GetListEntry(selIndex);
			if (entry == NULL)
				return;
			CString filepath = entry->path;
			svn_wc_status_kind wcStatus = entry->status;
			//entry is selected, now show the popup menu
			CMenu popup;
			if (popup.CreatePopupMenu())
			{
				CString temp;
				if ((wcStatus > svn_wc_status_normal)&&(wcStatus != svn_wc_status_added))
				{
					if (GetSelectedCount() == 1)
					{
						if (entry->remotestatus <= svn_wc_status_normal)
						{
							temp.LoadString(IDS_LOG_COMPAREWITHBASE);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COMPARE, temp);
						}
						popup.SetDefaultItem(IDSVNLC_COMPARE, FALSE);
						temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_GNUDIFF1, temp);
						temp.LoadString(IDS_MENUREVERT);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_REVERT, temp);
					}
				}
				if (entry->remotestatus > svn_wc_status_normal)
				{
					temp.LoadString(IDS_SVNACTION_UPDATE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_UPDATE, temp);
					if (GetSelectedCount() == 1)
					{
						temp.LoadString(IDS_LOG_POPUP_COMPARE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_COMPARE, temp);
						temp.LoadString(IDS_REPOBROWSE_SHOWLOG);
						popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_LOG, temp);
					}
				} // if (repoStatus > svn_wc_status_normal)
				if ((wcStatus > svn_wc_status_normal)&&(wcStatus != svn_wc_status_deleted) && (GetSelectedCount() == 1))
				{
					temp.LoadString(IDS_REPOBROWSE_OPEN);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_OPEN, temp);
				}
				if (wcStatus == svn_wc_status_unversioned)
				{
					temp.LoadString(IDS_REPOBROWSE_DELETE);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_DELETE, temp);
					if (GetSelectedCount() == 1)
					{
						CString filename = filepath.Mid(filepath.ReverseFind('/')+1);
						if (filename.ReverseFind('.')>=0)
						{
							CMenu submenu;
							if (submenu.CreateMenu())
							{
								submenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNORE, filename);
								filename = _T("*")+filename.Mid(filename.ReverseFind('.'));
								submenu.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNOREMASK, filename);
								temp.LoadString(IDS_MENUIGNORE);
								popup.InsertMenu(-1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)submenu.m_hMenu, temp);
							}
						}
						else
						{
							temp.LoadString(IDS_MENUIGNORE);
							popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_IGNORE, temp);
						}
					}
				}
				if (wcStatus == svn_wc_status_conflicted)
				{
					temp.LoadString(IDS_MENUCONFLICT);
					popup.AppendMenu(MF_STRING | MF_ENABLED, IDSVNLC_EDITCONFLICT, temp);
				}
				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
				m_bBlock = TRUE;
				AfxGetApp()->DoWaitCursor(1);
				switch (cmd)
				{
				case IDSVNLC_REVERT:
					{
						if (CMessageBox::Show(this->m_hWnd, IDS_PROC_WARNREVERT, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)==IDYES)
						{
							SVN svn;
							if (!svn.Revert(filepath, FALSE))
							{
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							}
							else
							{
								//since the entry got reverted we need to remove
								//it from the list too
								m_nTotal--;
								if (GetCheck(selIndex))
									m_nSelected--;
								RemoveListEntry(selIndex);
							}
						}  
					} 
					break;
				case IDSVNLC_COMPARE:
					{
						StartDiff(selIndex);
					}
					break;
				case IDSVNLC_GNUDIFF1:
					{
						CString tempfile = CUtils::GetTempFile();
						tempfile += _T(".diff");
						SVN svn;
						if (!svn.PegDiff(entry->path, SVNRev::REV_WC, SVNRev::REV_WC, SVNRev::REV_HEAD, TRUE, FALSE, TRUE, _T(""), tempfile))
						{
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							DeleteFile(tempfile);
							break;		//exit
						}
						m_templist.Add(tempfile);
						CUtils::StartDiffViewer(tempfile);
					}
					break;
				case IDSVNLC_UPDATE:
					{
						CString tempFile = BuildTargetFile();
						CSVNProgressDlg dlg;
						dlg.SetParams(Enum_Update, true, tempFile);
						dlg.DoModal();
					}
					break;
				case IDSVNLC_LOG:
					{
						CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
						long revend = reg;
						revend = -revend;
						CLogDlg dlg;
						dlg.SetParams(filepath, SVNRev::REV_HEAD, revend);
						dlg.DoModal();
					}
					break;
				case IDSVNLC_OPEN:
					{
						ShellExecute(this->m_hWnd, _T("open"), filepath, NULL, NULL, SW_SHOW);
					}
					break;
				case IDSVNLC_DELETE:
					{
						CString filelist;
						POSITION pos = GetFirstSelectedItemPosition();
						int index;
						while ((index = GetNextSelectedItem(pos)) >= 0)
						{
							filelist += GetListEntry(index)->path;
							filelist += _T("|");
						}
						filelist += _T("|");
						filelist.Replace('/','\\');
						TCHAR * buf = new TCHAR[filelist.GetLength()+2];
						_tcscpy(buf, filelist);
						for (int i=0; i<filelist.GetLength(); ++i)
							if (buf[i] == '|')
								buf[i] = 0;
						SHFILEOPSTRUCT fileop;
						fileop.hwnd = this->m_hWnd;
						fileop.wFunc = FO_DELETE;
						fileop.pFrom = buf;
						fileop.pTo = NULL;
						fileop.fFlags = FOF_ALLOWUNDO | FOF_NO_CONNECTED_ELEMENTS;
						fileop.lpszProgressTitle = _T("deleting file");
						SHFileOperation(&fileop);
						delete [] buf;

						if (! fileop.fAnyOperationsAborted)
						{
							POSITION pos = GetFirstSelectedItemPosition();
							int index;
							while ((index = GetNextSelectedItem(pos)) >= 0)
							{
								if (GetCheck(index))
									m_nSelected--;
								m_nTotal--;
								RemoveListEntry(index);
							}
						}
					}
					break;
				case IDSVNLC_IGNOREMASK:
					{
						filepath.Replace('\\', '/');
						CString name = _T("*")+filepath.Mid(filepath.ReverseFind('.'));
						CString parentfolder = filepath.Left(filepath.ReverseFind('/'));
						SVNProperties props(parentfolder);
						CStringA value;
						for (int i=0; i<props.GetCount(); i++)
						{
							CString propname(props.GetItemName(i).c_str());
							if (propname.CompareNoCase(_T("svn:ignore"))==0)
							{
								stdstring stemp;
								stdstring tmp = props.GetItemValue(i);
								//treat values as normal text even if they're not
								value = (char *)tmp.c_str();
							}
						}
						if (value.IsEmpty())
							value = name;
						else
						{
							value = value.Trim("\n\r");
							value += "\n";
							value += name;
							value.Remove('\r');
						}
						if (!props.Add(_T("svn:ignore"), value))
						{
							CString temp;
							temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, name);
							CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
						}
						for (int i=0; i<GetItemCount(); ++i)
						{
							FileEntry * entry = GetListEntry(i);
							if (entry == NULL)
								continue;								
							CString f = entry->path;
							if (CUtils::PathIsParent(parentfolder, f))
							{
								if (f.Mid(parentfolder.GetLength()).Find('/')<=0)
								{
									if (CStringUtils::WildCardMatch(name, f))
									{
										if (GetCheck(i))
											m_nSelected--;
										m_nTotal--;
										RemoveListEntry(i);
										i--;
									}
								}
							}
						}
					}
					break;
				case IDSVNLC_IGNORE:
					{
						filepath.Replace('\\', '/');
						CString name = filepath.Mid(filepath.ReverseFind('/')+1);
						CString parentfolder = filepath.Left(filepath.ReverseFind('/'));
						SVNProperties props(parentfolder);
						CStringA value;
						for (int i=0; i<props.GetCount(); i++)
						{
							CString propname(props.GetItemName(i).c_str());
							if (propname.CompareNoCase(_T("svn:ignore"))==0)
							{
								stdstring stemp;
								stdstring tmp = props.GetItemValue(i);
								//treat values as normal text even if they're not
								value = (char *)tmp.c_str();
							}
						}
						if (value.IsEmpty())
							value = name;
						else
						{
							value = value.Trim("\n\r");
							value += "\n";
							value += name;
							value.Remove('\r');
						}
						if (!props.Add(_T("svn:ignore"), value))
						{
							CString temp;
							temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, name);
							CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
						}
						if (GetCheck(selIndex))
							m_nSelected--;
						m_nTotal--;
						RemoveListEntry(selIndex);
					}
					break;
				case IDSVNLC_EDITCONFLICT:
					{
						filepath.Replace('/','\\');
						CString theirs;
						CString mine;
						CString base;
						CString merge = filepath;
						CString path = merge.Left(merge.ReverseFind('\\'));
						path = path + _T("\\");

						//we have the conflicted file (%merged)
						//now look for the other required files
						SVNStatus stat;
						stat.GetStatus(merge);
						if (stat.status->entry)
						{
							if (stat.status->entry->conflict_new)
							{
								theirs = CUnicodeUtils::GetUnicode(stat.status->entry->conflict_new);
								theirs = path + theirs;
							}
							if (stat.status->entry->conflict_old)
							{
								base = CUnicodeUtils::GetUnicode(stat.status->entry->conflict_old);
								base = path + base;
							}
							if (stat.status->entry->conflict_wrk)
							{
								mine = CUnicodeUtils::GetUnicode(stat.status->entry->conflict_wrk);
								mine = path + mine;
							}
						}
						CUtils::StartExtMerge(base, theirs, mine, merge);
					}
					break;
				default:
					m_bBlock = FALSE;
					break;
				} // switch (cmd)
				m_bBlock = FALSE;
				AfxGetApp()->DoWaitCursor(-1);
				GetStatistics();
			} // if (popup.CreatePopupMenu())
		} // if (selIndex >= 0)
	} // if (pWnd == this)
}

void CSVNStatusListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (m_bBlock)
		return;

	StartDiff(pNMLV->iItem);
}

void CSVNStatusListCtrl::StartDiff(int fileindex)
{
	if (fileindex < 0)
		return;
	FileEntry * entry = GetListEntry(fileindex);
	if (entry == NULL)
		return;
	if (entry->status == svn_wc_status_added)
		return;		//we don't compare an added file to itself
	if (entry->status == svn_wc_status_deleted)
		return;		//we don't compare a deleted file (nothing) with something
	if (entry->status == svn_wc_status_unversioned)
		return;		//we don't compare new files with nothing
	if (PathIsDirectory(entry->path))
		return;		//we also don't compare folders
	CString path1;
	CString path2;
	CString path3;

	if (entry->status > svn_wc_status_normal)
		 path2 = SVN::GetPristinePath(entry->path);

	if (entry->remotestatus > svn_wc_status_normal)
	{
		path3 = CUtils::GetTempFile();

		SVN svn;
		if (!svn.Cat(entry->path, SVNRev::REV_HEAD, path3))
		{
			CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		}
		m_templist.Add(path3);
	}

	if ((!CRegDWORD(_T("Software\\TortoiseSVN\\DontConvertBase"), TRUE))&&(SVN::GetTranslatedFile(path1, entry->path)))
	{
		m_templist.Add(path1);
	}
	else
	{
		path1 = entry->path;
	}

	CString name = CUtils::GetFileNameFromPath(entry->path);
	CString ext = CUtils::GetFileExtFromPath(entry->path);
	CString n1, n2, n3;
	n1.Format(IDS_DIFF_WCNAME, name);
	n2.Format(IDS_DIFF_BASENAME, name);
	n3.Format(IDS_DIFF_REMOTENAME, name);

	if (path2.IsEmpty())
		CUtils::StartDiffViewer(path1, path3, FALSE, n1, n3, ext);
	else if (path3.IsEmpty())
		CUtils::StartDiffViewer(path2, path1, FALSE, n2, n1, ext);
	else
		CUtils::StartExtMerge(path2, path3, path1, _T(""), n2, n3, n1);
}

CString CSVNStatusListCtrl::GetStatistics()
{
	CString sNormal, sAdded, sDeleted, sModified, sConflicted, sUnversioned;
	WORD langID = (WORD)(DWORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
	TCHAR buf[MAX_PATH];
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_normal, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sNormal = buf;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_added, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sAdded = buf;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_deleted, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sDeleted = buf;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_modified, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sModified = buf;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_conflicted, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sConflicted = buf;
	SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_unversioned, buf, sizeof(buf)/sizeof(TCHAR), langID);
	sUnversioned = buf;
	CString sToolTip;
	sToolTip.Format(_T("%s = %d\n%s = %d\n%s = %d\n%s = %d\n%s = %d\n%s = %d"),
		sNormal, m_nNormal,
		sUnversioned, m_nUnversioned,
		sModified, m_nModified,
		sAdded, m_nAdded,
		sDeleted, m_nDeleted,
		sConflicted, m_nConflicted
		);
	CString sStats;
	sStats.Format(IDS_LOGPROMPT_STATISTICSFORMAT, m_nSelected, GetItemCount());
	if (m_pStatLabel)
	{
		m_pStatLabel->SetWindowText(sStats);
	}

	if (m_pSelectButton)
	{
		if (m_nSelected == 0)
			m_pSelectButton->SetCheck(BST_UNCHECKED);
		else if (m_nSelected != GetItemCount())
			m_pSelectButton->SetCheck(BST_INDETERMINATE);
		else
			m_pSelectButton->SetCheck(BST_CHECKED);
	}
	return sToolTip;
}

void CSVNStatusListCtrl::SelectAll(BOOL bSelect)
{
	m_bBlock = TRUE;
	AfxGetApp()->DoWaitCursor(1);
	SetRedraw(FALSE);	
	for (int i=0; i<GetItemCount(); ++i)
	{
		FileEntry * entry = GetListEntry(i);
		if (entry == NULL)
			continue;
		entry->checked = bSelect;
		SetCheck(i, bSelect);
	}
	if (bSelect)
		m_nSelected = GetItemCount();
	else
		m_nSelected = 0;
	SetRedraw(TRUE);
	GetStatistics();
	AfxGetApp()->DoWaitCursor(-1);
	m_bBlock = FALSE;
}

void CSVNStatusListCtrl::OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	if (GetListEntry(pGetInfoTip->iItem))
		if (pGetInfoTip->cchTextMax > GetListEntry(pGetInfoTip->iItem)->path.GetLength())
			_tcsncpy(pGetInfoTip->pszText, GetListEntry(pGetInfoTip->iItem)->path, pGetInfoTip->cchTextMax);
	*pResult = 0;
}

void CSVNStatusListCtrl::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.
		// We'll cycle the colors through red, green, and light blue.

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;

		COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

		if (m_arListArray.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
		{
			FileEntry * entry = GetListEntry((int)pLVCD->nmcd.dwItemSpec);
			if (entry == NULL)
				return;

			switch (entry->status)
			{
			case svn_wc_status_added:
				crText = GetSysColor(COLOR_HIGHLIGHT);
				break;
			case svn_wc_status_missing:
			case svn_wc_status_deleted:
			case svn_wc_status_replaced:
				crText = RGB(100,0,0);
				break;
			case svn_wc_status_modified:
				crText = GetSysColor(COLOR_HIGHLIGHT);
				break;
			case svn_wc_status_merged:
				crText = RGB(0, 100, 0);
				break;
			case svn_wc_status_conflicted:
			case svn_wc_status_obstructed:
				crText = RGB(255, 0, 0);
				break;
			case svn_wc_status_none:
			case svn_wc_status_unversioned:
			case svn_wc_status_ignored:
			case svn_wc_status_incomplete:
			case svn_wc_status_normal:
			case svn_wc_status_external:
			default:
				crText = GetSysColor(COLOR_WINDOWTEXT);
				break;
			}
			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
		}
	}
}

CString CSVNStatusListCtrl::BuildTargetFile()
{
	CString tempFile = CUtils::GetTempFile();
	m_templist.Add(tempFile);
	HANDLE file = ::CreateFile (tempFile,
		GENERIC_WRITE, 
		FILE_SHARE_READ, 
		0, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_TEMPORARY,
		0);
	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		DWORD written = 0;
		::WriteFile (file, GetListEntry(index)->path, GetListEntry(index)->path.GetLength()*sizeof(TCHAR), &written, 0);
		::WriteFile (file, _T("\n"), 2, &written, 0);
	} // while ((index = m_FileListCtrl.GetNextSelectedItem(&pos)) >= 0)
	::CloseHandle(file);
	return tempFile;
}

BOOL CSVNStatusListCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd != this)
		return CListCtrl::OnSetCursor(pWnd, nHitTest, message);
	if (!m_bBlock)
	{
		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
		SetCursor(hCur);
		return CListCtrl::OnSetCursor(pWnd, nHitTest, message);
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
	SetCursor(hCur);
	return TRUE;
}

void CSVNStatusListCtrl::RemoveListEntry(int index)
{
	DeleteItem(index);
	FileEntry * entry = m_arStatusArray.GetAt(m_arListArray.GetAt(index));
	delete entry;
	m_arStatusArray.RemoveAt(m_arListArray.GetAt(index));
	m_arListArray.RemoveAt(index);
	for (int i=index; i<m_arListArray.GetCount(); ++i)
	{
		m_arListArray[i]--;
	}
}
