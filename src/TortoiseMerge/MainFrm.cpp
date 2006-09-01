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
#include "stdafx.h"
#include "TortoiseMerge.h"
#include "OpenDlg.h"
#include "Patch.h"
#include "ProgressDlg.h"
#include "Settings.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "MainFrm.h"
#include "LeftView.h"
#include "RightView.h"
#include "BottomView.h"
#include ".\mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif						 


// CMainFrame
const UINT CMainFrame::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);

IMPLEMENT_DYNAMIC(CMainFrame, CNewFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CNewFrameWnd)
	ON_WM_CREATE()
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CNewFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CNewFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CNewFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CNewFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_VIEW_WHITESPACES, OnViewWhitespaces)
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
	ON_COMMAND(ID_VIEW_ONEWAYDIFF, OnViewOnewaydiff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ONEWAYDIFF, OnUpdateViewOnewaydiff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WHITESPACES, OnUpdateViewWhitespaces)
	ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
	ON_WM_CLOSE()
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage) 
	ON_COMMAND(ID_EDIT_FINDNEXT, OnEditFindnext)
	ON_COMMAND(ID_FILE_RELOAD, OnFileReload)
	ON_COMMAND(ID_VIEW_LINEDOWN, OnViewLinedown)
	ON_COMMAND(ID_VIEW_LINEUP, OnViewLineup)
	ON_UPDATE_COMMAND_UI(ID_MERGE_MARKASRESOLVED, OnUpdateMergeMarkasresolved)
	ON_COMMAND(ID_MERGE_MARKASRESOLVED, OnMergeMarkasresolved)
	ON_UPDATE_COMMAND_UI(ID_MERGE_NEXTCONFLICT, OnUpdateMergeNextconflict)
	ON_UPDATE_COMMAND_UI(ID_MERGE_PREVIOUSCONFLICT, OnUpdateMergePreviousconflict)
	ON_WM_MOVE()
	ON_WM_MOVING()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_VIEW_SWITCHLEFT, OnViewSwitchleft)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SWITCHLEFT, OnUpdateViewSwitchleft)
	ON_COMMAND(ID_VIEW_LINELEFT, &CMainFrame::OnViewLineleft)
	ON_COMMAND(ID_VIEW_LINERIGHT, &CMainFrame::OnViewLineright)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWFILELIST, &CMainFrame::OnUpdateViewShowfilelist)
	ON_COMMAND(ID_VIEW_SHOWFILELIST, &CMainFrame::OnViewShowfilelist)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_LEFTVIEW,
	ID_INDICATOR_RIGHTVIEW,
	ID_INDICATOR_BOTTOMVIEW,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_pFindDialog = NULL;
	m_nSearchIndex = 0;
	m_bInitSplitter = FALSE;
	m_bOneWay = (0 != ((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\OnePane"))));
	m_bReversedPatch = FALSE;
	m_bHasConflicts = false;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CNewFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_WRAPABLE| CBRS_SIZE_DYNAMIC) ||
		 !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	m_wndToolBar.SetWindowText(_T("Main"));

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	} 

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	if (!m_wndLocatorBar.Create(this, IDD_DIFFLOCATOR, 
		CBRS_ALIGN_LEFT, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	if (!m_wndLineDiffBar.Create(this, IDD_LINEDIFF, 
		CBRS_ALIGN_BOTTOM, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	//if (!m_wndReBar.Create(this) ||
	//	!m_wndReBar.AddBar(&m_wndToolBar))
	//{
	//	TRACE0("Failed to create rebar\n");
	//	return -1;      // fail to create
	//}
	m_wndLocatorBar.m_pMainFrm = this;
	m_wndLineDiffBar.m_pMainFrm = this;
	m_DefaultNewMenu.SetMenuDrawMode(CNewMenu::STYLE_XP);
	m_DefaultNewMenu.LoadToolBar(IDR_MAINFRAME);
	m_DefaultNewMenu.SetXpBlending();
	m_DefaultNewMenu.SetSelectDisableMode(FALSE);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	CNewMenu::SetMenuDrawMode(CNewMenu::STYLE_XP);
	if( !CNewFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CNewFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CNewFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers


BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
	CRect cr; 
	GetClientRect( &cr);


	// split into three panes:
	//        -------------
	//        |     |     |
	//        |     |     |
	//        |------------
	//        |           |
	//        |           |
	//        |------------

	// create a splitter with 2 rows, 1 column 
	if (!m_wndSplitter.CreateStatic(this, 2, 1))
	{ 
		TRACE0("Failed to CreateStaticSplitter\n"); 
		return FALSE; 
	}

	// add the second splitter pane - which is a nested splitter with 2 columns 
	if (!m_wndSplitter2.CreateStatic( 
		&m_wndSplitter, // our parent window is the first splitter 
		1, 2, // the new splitter is 1 row, 2 columns
		WS_CHILD | WS_VISIBLE | WS_BORDER, // style, WS_BORDER is needed 
		m_wndSplitter.IdFromRowCol(0, 0) 
		// new splitter is in the first column, 2nd column of first splitter 
		)) 
	{ 
		TRACE0("Failed to create nested splitter\n"); 
		return FALSE; 
	}
	// add the first splitter pane - the default view in row 0 
	if (!m_wndSplitter.CreateView(1, 0, 
		RUNTIME_CLASS(CBottomView), CSize(cr.Width(), cr.Height()), pContext)) 
	{ 
		TRACE0("Failed to create first pane\n"); 
		return FALSE; 
	}
	m_pwndBottomView = (CBottomView *)m_wndSplitter.GetPane(1,0);
	m_pwndBottomView->m_pwndLocator = &m_wndLocatorBar;
	m_pwndBottomView->m_pwndLineDiffBar = &m_wndLineDiffBar;
	m_pwndBottomView->m_pwndStatusBar = &m_wndStatusBar;
	m_pwndBottomView->m_pMainFrame = this;

	// now create the two views inside the nested splitter 

	if (!m_wndSplitter2.CreateView(0, 0, 
		RUNTIME_CLASS(CLeftView), CSize(cr.Width()/2, cr.Height()/2), pContext)) 
	{ 
		TRACE0("Failed to create second pane\n"); 
		return FALSE; 
	}
	m_pwndLeftView = (CLeftView *)m_wndSplitter2.GetPane(0,0);
	m_pwndLeftView->m_pwndLocator = &m_wndLocatorBar;
	m_pwndLeftView->m_pwndLineDiffBar = &m_wndLineDiffBar;
	m_pwndLeftView->m_pwndStatusBar = &m_wndStatusBar;
	m_pwndLeftView->m_pMainFrame = this;

	if (!m_wndSplitter2.CreateView(0, 1, 
		RUNTIME_CLASS(CRightView), CSize(cr.Width()/2, cr.Height()/2), pContext)) 
	{ 
		TRACE0("Failed to create third pane\n"); 
		return FALSE; 
	}
	m_pwndRightView = (CRightView *)m_wndSplitter2.GetPane(0,1);
	m_pwndRightView->m_pwndLocator = &m_wndLocatorBar;
	m_pwndRightView->m_pwndLineDiffBar = &m_wndLineDiffBar;
	m_pwndRightView->m_pwndStatusBar = &m_wndStatusBar;
	m_pwndRightView->m_pMainFrame = this;
	m_bInitSplitter = TRUE;

	m_dlgFilePatches.Create(IDD_FILEPATCHES, this);
	UpdateLayout();
	return TRUE;
}

// Callback function
BOOL CMainFrame::PatchFile(CString sFilePath, CString sVersion, BOOL bAutoPatch)
{
	//first, do a "dry run" of patching...
	if (!m_Patch.PatchFile(sFilePath))
	{
		//patching not successful, so retrieve the
		//base file from version control and try
		//again...
		CString sTemp;
		CProgressDlg progDlg;
		sTemp.Format(IDS_GETVERSIONOFFILE, sVersion);
		progDlg.SetLine(1, sTemp, true);
		progDlg.SetLine(2, sFilePath, true);
		sTemp.LoadString(IDS_GETVERSIONOFFILETITLE);
		progDlg.SetTitle(sTemp);
		progDlg.SetShowProgressBar(false);
		progDlg.SetAnimation(IDR_DOWNLOAD);
		progDlg.SetTime(FALSE);
		progDlg.ShowModeless(this);
		CString sBaseFile = m_TempFiles.GetTempFilePath();
		if (!CAppUtils::GetVersionedFile(sFilePath, sVersion, sBaseFile, &progDlg, this->m_hWnd))
		{
			progDlg.Stop();
			CString sErrMsg;
			sErrMsg.Format(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, sVersion, sFilePath);
			MessageBox(sErrMsg, NULL, MB_ICONERROR);
			return FALSE;
		} // if (!CUtils::GetVersionedFile(sFilePath, sVersion, sBaseFile, this->m_hWnd)) 
		progDlg.Stop();
		CString sTempFile = m_TempFiles.GetTempFilePath();
		if (!m_Patch.PatchFile(sFilePath, sTempFile, sBaseFile))
		{
			MessageBox(m_Patch.GetErrorMessage(), NULL, MB_ICONERROR);
			return FALSE;
		} // if (!m_Patch.PatchFile(sFilePath, sTempFile, sBaseFile)) 
		CString temp;
		temp.Format(_T("%s Revision %s"), CPathUtils::GetFileNameFromPath(sFilePath), sVersion);
		this->m_Data.m_baseFile.SetFileName(sBaseFile);
		this->m_Data.m_baseFile.SetDescriptiveName(temp);
		temp.Format(_T("%s %s"), CPathUtils::GetFileNameFromPath(sFilePath), m_Data.m_sPatchPatched);
		this->m_Data.m_theirFile.SetFileName(sTempFile);
		this->m_Data.m_theirFile.SetDescriptiveName(temp);
		this->m_Data.m_yourFile.SetFileName(sFilePath);
		this->m_Data.m_yourFile.SetDescriptiveName(CPathUtils::GetFileNameFromPath(sFilePath));
		this->m_Data.m_mergedFile.SetFileName(sFilePath);
		this->m_Data.m_mergedFile.SetDescriptiveName(CPathUtils::GetFileNameFromPath(sFilePath));
		TRACE(_T("comparing %s and %s\nagainst the base file %s\n"), (LPCTSTR)sTempFile, (LPCTSTR)sFilePath, (LPCTSTR)sBaseFile);
	} // if (!m_Patch.PatchFile(sFilePath)) 
	else
	{
		//"dry run" was successful, so save the patched file somewhere...
		CString sTempFile = m_TempFiles.GetTempFilePath();
		if (!m_Patch.PatchFile(sFilePath, sTempFile))
		{
			MessageBox(m_Patch.GetErrorMessage(), NULL, MB_ICONERROR);
			return FALSE;
		} // if (!m_Patch.PatchFile(sFilePath, sTempFile)) 
		if (m_bReversedPatch)
		{
			this->m_Data.m_baseFile.SetFileName(sTempFile);
			CString temp;
			temp.Format(_T("%s %s"), CPathUtils::GetFileNameFromPath(sFilePath), m_Data.m_sPatchPatched);
			this->m_Data.m_baseFile.SetDescriptiveName(temp);
			this->m_Data.m_yourFile.SetFileName(sFilePath);
			temp.Format(_T("%s %s"), CPathUtils::GetFileNameFromPath(sFilePath), m_Data.m_sPatchOriginal);
			this->m_Data.m_yourFile.SetDescriptiveName(temp);
			this->m_Data.m_theirFile.SetOutOfUse();
			this->m_Data.m_mergedFile.SetOutOfUse();
		}
		else
		{
			if (!PathFileExists(sFilePath))
			{
				this->m_Data.m_baseFile.SetFileName(m_TempFiles.GetTempFilePath());
				this->m_Data.m_baseFile.CreateEmptyFile();
			}
			else
			{
				this->m_Data.m_baseFile.SetFileName(sFilePath);
			}
			CString sDescription;
			sDescription.Format(_T("%s %s"), (LPCTSTR)CPathUtils::GetFileNameFromPath(sFilePath), (LPCTSTR)m_Data.m_sPatchOriginal);
			this->m_Data.m_baseFile.SetDescriptiveName(sDescription);
			this->m_Data.m_yourFile.SetFileName(sTempFile);
			CString temp;
			temp.Format(_T("%s %s"), (LPCTSTR)CPathUtils::GetFileNameFromPath(sFilePath), (LPCTSTR)m_Data.m_sPatchPatched);
			this->m_Data.m_yourFile.SetDescriptiveName(temp);
			this->m_Data.m_theirFile.SetOutOfUse();
			this->m_Data.m_mergedFile.SetFileName(sFilePath);
		}
		TRACE(_T("comparing %s\nwith the patched result %s\n"), (LPCTSTR)sFilePath, (LPCTSTR)sTempFile);
	}
	LoadViews();
	if (bAutoPatch)
	{
		OnFileSave();
	}
	return TRUE;
}

// Callback function
BOOL CMainFrame::DiffFiles(CString sURL1, CString sRev1, CString sURL2, CString sRev2)
{
	CString tempfile1 = m_TempFiles.GetTempFilePath();
	CString tempfile2 = m_TempFiles.GetTempFilePath();
	
	ASSERT(tempfile1.Compare(tempfile2));
	
	CString sTemp;
	CProgressDlg progDlg;
	sTemp.Format(IDS_GETVERSIONOFFILE, sRev1);
	progDlg.SetLine(1, sTemp, true);
	progDlg.SetLine(2, sURL1, true);
	sTemp.LoadString(IDS_GETVERSIONOFFILETITLE);
	progDlg.SetTitle(sTemp);
	progDlg.SetShowProgressBar(true);
	progDlg.SetAnimation(IDR_DOWNLOAD);
	progDlg.SetTime(FALSE);
	progDlg.SetProgress(1,100);
	progDlg.ShowModeless(this);
	if (!CAppUtils::GetVersionedFile(sURL1, sRev1, tempfile1, &progDlg, this->m_hWnd))
	{
		progDlg.Stop();
		CString sErrMsg;
		sErrMsg.Format(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, sRev1, sURL1);
		MessageBox(sErrMsg, NULL, MB_ICONERROR);
		return FALSE;
	}
	sTemp.Format(IDS_GETVERSIONOFFILE, sRev2);
	progDlg.SetLine(1, sTemp, true);
	progDlg.SetLine(2, sURL2, true);
	progDlg.SetProgress(50, 100);
	if (!CAppUtils::GetVersionedFile(sURL2, sRev2, tempfile2, &progDlg, this->m_hWnd))
	{
		progDlg.Stop();
		CString sErrMsg;
		sErrMsg.Format(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, sRev2, sURL2);
		MessageBox(sErrMsg, NULL, MB_ICONERROR);
		return FALSE;
	}
	progDlg.SetProgress(100,100);
	progDlg.Stop();
	CString temp;
	temp.Format(_T("%s Revision %s"), CPathUtils::GetFileNameFromPath(sURL1), sRev1);
	this->m_Data.m_baseFile.SetFileName(tempfile1);
	this->m_Data.m_baseFile.SetDescriptiveName(temp);
	temp.Format(_T("%s Revision %s"), CPathUtils::GetFileNameFromPath(sURL2), sRev2);
	this->m_Data.m_yourFile.SetFileName(tempfile2);
	this->m_Data.m_yourFile.SetDescriptiveName(temp);

	LoadViews();

	return TRUE;
}

void CMainFrame::OnFileOpen()
{
	COpenDlg dlg;
	if (dlg.DoModal()!=IDOK)
	{
		return;
	}
	m_dlgFilePatches.ShowWindow(SW_HIDE);
	m_dlgFilePatches.Init(NULL, NULL, CString(), NULL);
	TRACE(_T("got the files:\n   %s\n   %s\n   %s\n   %s\n   %s\n"), (LPCTSTR)dlg.m_sBaseFile, (LPCTSTR)dlg.m_sTheirFile, (LPCTSTR)dlg.m_sYourFile, 
		(LPCTSTR)dlg.m_sUnifiedDiffFile, (LPCTSTR)dlg.m_sPatchDirectory);
	this->m_Data.m_baseFile.SetFileName(dlg.m_sBaseFile);
	this->m_Data.m_theirFile.SetFileName(dlg.m_sTheirFile);
	this->m_Data.m_yourFile.SetFileName(dlg.m_sYourFile);
	this->m_Data.m_sDiffFile = dlg.m_sUnifiedDiffFile;
	this->m_Data.m_sPatchPath = dlg.m_sPatchDirectory;
	this->m_Data.m_mergedFile.SetOutOfUse();
	g_crasher.AddFile((LPCSTR)(LPCTSTR)dlg.m_sBaseFile, (LPCSTR)(LPCTSTR)_T("Basefile"));
	g_crasher.AddFile((LPCSTR)(LPCTSTR)dlg.m_sTheirFile, (LPCSTR)(LPCTSTR)_T("Theirfile"));
	g_crasher.AddFile((LPCSTR)(LPCTSTR)dlg.m_sYourFile, (LPCSTR)(LPCTSTR)_T("Yourfile"));
	g_crasher.AddFile((LPCSTR)(LPCTSTR)dlg.m_sUnifiedDiffFile, (LPCSTR)(LPCTSTR)_T("Difffile"));
	
	if (!m_Data.IsBaseFileInUse() && m_Data.IsTheirFileInUse() && m_Data.IsYourFileInUse())
	{
		// a diff between two files means "Yours" against "Base", not "Theirs" against "Yours"
		m_Data.m_baseFile.TransferDetailsFrom(m_Data.m_theirFile);
	}
	if (m_Data.IsBaseFileInUse() && m_Data.IsTheirFileInUse() && !m_Data.IsYourFileInUse())
	{
		// a diff between two files means "Yours" against "Base", not "Theirs" against "Base"
		m_Data.m_yourFile.TransferDetailsFrom(m_Data.m_theirFile);
	}

	LoadViews();
}

BOOL CMainFrame::LoadViews(BOOL bReload)
{
	m_Data.SetBlame(m_bBlame);
	m_bHasConflicts = false;
	if (bReload)
	{
		if (!this->m_Data.Load())
		{
			::MessageBox(NULL, m_Data.GetError(), _T("TortoiseMerge"), MB_ICONERROR);
			m_Data.m_mergedFile.SetOutOfUse();
			return FALSE;
		}
	}
	BOOL bGoFirstDiff = (0 != ((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\FirstDiffOnLoad"))));
	if (!m_Data.IsBaseFileInUse())
	{
		if (m_Data.IsYourFileInUse() && m_Data.IsTheirFileInUse())
		{
			m_Data.m_baseFile.TransferDetailsFrom(m_Data.m_theirFile);
		}
		else if ((!m_Data.m_sDiffFile.IsEmpty())&&(!m_Patch.OpenUnifiedDiffFile(m_Data.m_sDiffFile)))
		{
			m_pwndLeftView->m_sWindowName = _T("");
			m_pwndLeftView->m_sFullFilePath = _T("");
			m_pwndRightView->m_sWindowName = _T("");
			m_pwndRightView->m_sFullFilePath = _T("");
			m_pwndBottomView->m_sWindowName = _T("");
			m_pwndBottomView->m_sFullFilePath = _T("");
			MessageBox(m_Patch.GetErrorMessage(), NULL, MB_ICONERROR);
			return FALSE;
		}
		if (m_Patch.GetNumberOfFiles() > 0)
		{
			CString betterpatchpath = m_Patch.CheckPatchPath(m_Data.m_sPatchPath);
			if (betterpatchpath.CompareNoCase(m_Data.m_sPatchPath)!=0)
			{
				CString msg;
				msg.Format(IDS_WARNBETTERPATCHPATHFOUND, (LPCTSTR)m_Data.m_sPatchPath, (LPCTSTR)betterpatchpath);
				if (CMessageBox::Show(m_hWnd, msg, _T("TortoiseMerge"), MB_ICONQUESTION | MB_YESNO)==IDYES)
					m_Data.m_sPatchPath = betterpatchpath;
			}
			m_dlgFilePatches.Init(&m_Patch, this, m_Data.m_sPatchPath, this);
			m_dlgFilePatches.ShowWindow(SW_SHOW);
			m_pwndLeftView->m_sWindowName = _T("");
			m_pwndLeftView->m_sFullFilePath = _T("");
			m_pwndRightView->m_sWindowName = _T("");
			m_pwndRightView->m_sFullFilePath = _T("");
			m_pwndBottomView->m_sWindowName = _T("");
			m_pwndBottomView->m_sFullFilePath = _T("");
			if (!m_wndSplitter.IsRowHidden(1))
				m_wndSplitter.HideRow(1);
			m_pwndLeftView->SetHidden(FALSE);
			m_pwndRightView->SetHidden(FALSE);
			m_pwndBottomView->SetHidden(TRUE);
		}
	} // if (m_Data.!IsBaseFileInUse()) 
	if (m_Data.IsBaseFileInUse() && !m_Data.IsYourFileInUse() && m_Data.IsTheirFileInUse())
	{
		m_Data.m_yourFile.TransferDetailsFrom(m_Data.m_theirFile);
	}
	if (m_Data.IsBaseFileInUse() && m_Data.IsYourFileInUse() && !m_Data.IsTheirFileInUse())
	{
		//diff between YOUR and BASE
		if (m_bOneWay)
		{
			if (!m_wndSplitter2.IsColumnHidden(1))
				m_wndSplitter2.HideColumn(1);
			if (bReload)
			{
				m_pwndLeftView->m_arDiffLines = &m_Data.m_arDiffYourBaseBoth;
				m_pwndLeftView->m_arLineStates = &m_Data.m_arStateYourBaseBoth;
				m_pwndLeftView->m_arLineLines = &m_Data.m_arLinesYourBaseBoth;
			}
			m_pwndLeftView->m_sWindowName = m_Data.m_baseFile.GetWindowName() + _T(" - ") + m_Data.m_yourFile.GetWindowName();
			m_pwndLeftView->m_sFullFilePath = m_pwndLeftView->m_sWindowName;
			if (!m_wndSplitter.IsRowHidden(1))
				m_wndSplitter.HideRow(1);
			m_pwndLeftView->SetHidden(FALSE);
			m_pwndRightView->SetHidden(TRUE);
			m_pwndBottomView->SetHidden(TRUE);
			::SetWindowPos(m_pwndLeftView->m_hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		} // if (m_bOneWay)
		else
		{
			if (m_wndSplitter2.IsColumnHidden(1))
				m_wndSplitter2.ShowColumn();
			if (bReload)
			{
				m_pwndLeftView->m_arDiffLines = &m_Data.m_arDiffYourBaseLeft;
				m_pwndLeftView->m_arLineStates = &m_Data.m_arStateYourBaseLeft;
				m_pwndLeftView->m_arLineLines = &m_Data.m_arLinesYourBaseLeft;
			}
			m_pwndLeftView->m_sWindowName = m_Data.m_baseFile.GetWindowName();
			m_pwndLeftView->m_sFullFilePath = m_Data.m_baseFile.GetFilename();
			if (bReload)
			{
				m_pwndRightView->m_arDiffLines = &m_Data.m_arDiffYourBaseRight;
				m_pwndRightView->m_arLineStates = &m_Data.m_arStateYourBaseRight;
				m_pwndRightView->m_arLineLines = &m_Data.m_arLinesYourBaseRight;
			}
			m_pwndRightView->m_sWindowName = m_Data.m_yourFile.GetWindowName();
			m_pwndRightView->m_sFullFilePath = m_Data.m_yourFile.GetFilename();
			if (!m_wndSplitter.IsRowHidden(1))
				m_wndSplitter.HideRow(1);
			m_pwndLeftView->SetHidden(FALSE);
			m_pwndRightView->SetHidden(FALSE);
			m_pwndBottomView->SetHidden(TRUE);
		}
	} // if (!m_Data.!IsBaseFileInUse() && !m_Data.!IsYourFileInUse() && m_Data.!IsTheirFileInUse())
	else if (m_Data.IsBaseFileInUse() && m_Data.IsYourFileInUse() && m_Data.IsTheirFileInUse())
	{
		//diff between THEIR, YOUR and BASE
		if (bReload)
		{
			m_pwndLeftView->m_arDiffLines = &m_Data.m_arDiffTheirBaseBoth;
			m_pwndLeftView->m_arLineStates = &m_Data.m_arStateTheirBaseBoth;
			m_pwndLeftView->m_arLineLines = &m_Data.m_arLinesTheirBaseBoth;
		}
		m_pwndLeftView->m_sWindowName.LoadString(IDS_VIEWTITLE_THEIRS);
		m_pwndLeftView->m_sWindowName += _T(" - ") + m_Data.m_theirFile.GetWindowName();
		m_pwndLeftView->m_sFullFilePath = m_Data.m_theirFile.GetFilename();
		if (bReload)
		{
			m_pwndRightView->m_arDiffLines = &m_Data.m_arDiffYourBaseBoth;
			m_pwndRightView->m_arLineStates = &m_Data.m_arStateYourBaseBoth;
			m_pwndRightView->m_arLineLines = &m_Data.m_arLinesYourBaseBoth;
		}
		m_pwndRightView->m_sWindowName.LoadString(IDS_VIEWTITLE_MINE);
		m_pwndRightView->m_sWindowName += _T(" - ") + m_Data.m_yourFile.GetWindowName();
		m_pwndRightView->m_sFullFilePath = m_Data.m_yourFile.GetFilename();
		if (bReload)
		{
			m_pwndBottomView->m_arDiffLines = &m_Data.m_arDiff3;
			m_pwndBottomView->m_arLineStates = &m_Data.m_arStateDiff3;
			m_pwndBottomView->m_arLineLines = &m_Data.m_arLinesDiff3;
		}
		m_pwndBottomView->m_sWindowName.LoadString(IDS_VIEWTITLE_MERGED);
		m_pwndBottomView->m_sWindowName += _T(" - ") + m_Data.m_mergedFile.GetWindowName();
		m_pwndBottomView->m_sFullFilePath = m_Data.m_mergedFile.GetFilename();
		if (m_wndSplitter2.IsColumnHidden(1))
			m_wndSplitter2.ShowColumn();
		if (m_wndSplitter.IsRowHidden(1))
			m_wndSplitter.ShowRow();
		m_pwndLeftView->SetHidden(FALSE);
		m_pwndRightView->SetHidden(FALSE);
		m_pwndBottomView->SetHidden(FALSE);
	}
	if (!m_Data.m_mergedFile.InUse())
	{
		m_Data.m_mergedFile.SetFileName(m_Data.m_yourFile.GetFilename());
	}
	m_pwndLeftView->DocumentUpdated();
	m_pwndRightView->DocumentUpdated();
	m_pwndBottomView->DocumentUpdated();
	m_wndLocatorBar.DocumentUpdated();
	m_wndLineDiffBar.DocumentUpdated();
	UpdateLayout();
	SetActiveView(m_pwndLeftView);
	if (bGoFirstDiff)
		m_pwndLeftView->GoToFirstDifference();
	CheckResolved();
	return TRUE;
}

void CMainFrame::UpdateLayout()
{
	if (m_bInitSplitter)
	{
		CRect cr;
		GetWindowRect(&cr);
		m_wndSplitter.SetRowInfo(0, cr.Height()/2, 0);
		m_wndSplitter.SetRowInfo(1, cr.Height()/2, 0);
		m_wndSplitter.SetColumnInfo(0, cr.Width() / 2, 50);
		m_wndSplitter2.SetRowInfo(0, cr.Height()/2, 0);
		m_wndSplitter2.SetColumnInfo(0, cr.Width() / 2, 50);
		m_wndSplitter2.SetColumnInfo(1, cr.Width() / 2, 50);

		m_wndSplitter.RecalcLayout();
	} // if (m_bInitSplitter) 
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    if (m_bInitSplitter && nType != SIZE_MINIMIZED)
    {
		UpdateLayout();
    } // if (m_bInitSplitter && nType != SIZE_MINIMIZED) 
    CNewFrameWnd::OnSize(nType, cx, cy);
}

void CMainFrame::OnViewWhitespaces()
{
	CRegDWORD regViewWhitespaces = CRegDWORD(_T("Software\\TortoiseMerge\\ViewWhitespaces"), 1);
	BOOL bViewWhitespaces = regViewWhitespaces;
	if (m_pwndLeftView)
		bViewWhitespaces = m_pwndLeftView->m_bViewWhitespace;

	bViewWhitespaces = !bViewWhitespaces;
	regViewWhitespaces = bViewWhitespaces;
	if (m_pwndLeftView)
	{
		m_pwndLeftView->m_bViewWhitespace = bViewWhitespaces;
		m_pwndLeftView->Invalidate();
	} // if (m_pwndLeftView)
	if (m_pwndRightView)
	{
		m_pwndRightView->m_bViewWhitespace = bViewWhitespaces;
		m_pwndRightView->Invalidate();
	} // if (m_pwndRightView)
	if (m_pwndBottomView)
	{
		m_pwndBottomView->m_bViewWhitespace = bViewWhitespaces;
		m_pwndBottomView->Invalidate();
	} // if (m_pwndBottomView) 
}

void CMainFrame::OnUpdateViewWhitespaces(CCmdUI *pCmdUI)
{
	if (m_pwndLeftView)
		pCmdUI->SetCheck(m_pwndLeftView->m_bViewWhitespace);
}

void CMainFrame::OnViewOnewaydiff()
{
	m_bOneWay = !m_bOneWay;
	LoadViews(TRUE);
}

int CMainFrame::CheckResolved()
{
	//only in three way diffs can be conflicts!
	m_bHasConflicts = true;
	if (this->m_pwndBottomView->IsWindowVisible())
	{
		if (this->m_pwndBottomView->m_arLineStates)
		{
			for (int i=0; i<this->m_pwndBottomView->m_arLineStates->GetCount(); i++)
			{
				if (CDiffData::DIFFSTATE_CONFLICTED == (CDiffData::DiffStates)this->m_pwndBottomView->m_arLineStates->GetAt(i))
					return i;
			}
		}
	}
	m_bHasConflicts = false;
	return -1;
}

void CMainFrame::SaveFile(const CString& sFilePath)
{
	CStdCStringArray * arText = NULL;
	CStdDWORDArray * arStates = NULL;
	CFileTextLines * pOriginFile = &m_Data.m_arBaseFile;
	if ((m_pwndBottomView)&&(m_pwndBottomView->IsWindowVisible()))
	{
		arText = m_pwndBottomView->m_arDiffLines;
		arStates = m_pwndBottomView->m_arLineStates;
		m_pwndBottomView->SetModified(FALSE);
		if ((m_pwndRightView)&&(m_pwndRightView->IsWindowVisible()))
			m_pwndRightView->SetModified(FALSE);
		Invalidate();
	} // if (m_pwndBottomView) 
	else if ((m_pwndRightView)&&(m_pwndRightView->IsWindowVisible()))
	{
		arText = m_pwndRightView->m_arDiffLines;
		if (m_Data.IsYourFileInUse())
			pOriginFile = &m_Data.m_arYourFile;
		else if (m_Data.IsTheirFileInUse())
			pOriginFile = &m_Data.m_arTheirFile;
		arStates = m_pwndRightView->m_arLineStates;
		m_pwndRightView->SetModified(FALSE);
		if ((m_pwndBottomView)&&(m_pwndBottomView->IsWindowVisible()))
			m_pwndBottomView->SetModified(FALSE);
		Invalidate();
	} 
	else
	{
		// nothing to save!
		return;
	}
	if ((arText)&&(arStates)&&(pOriginFile))
	{
		CFileTextLines file;
		pOriginFile->CopySettings(&file);
		for (int i=0; i<arText->GetCount(); i++)
		{
			//only copy non-removed lines
			CDiffData::DiffStates state = (CDiffData::DiffStates)arStates->GetAt(i);
			switch (state)
			{
			case CDiffData::DIFFSTATE_ADDED:
			case CDiffData::DIFFSTATE_CONFLICTADDED:
			case CDiffData::DIFFSTATE_IDENTICALADDED:
			case CDiffData::DIFFSTATE_NORMAL:
			case CDiffData::DIFFSTATE_THEIRSADDED:
			case CDiffData::DIFFSTATE_UNKNOWN:
			case CDiffData::DIFFSTATE_YOURSADDED:
				file.Add(arText->GetAt(i));
				break;
			case CDiffData::DIFFSTATE_CONFLICTED:
				{
					int first = i;
					int last = i;
					do 
					{
						last++;
					} while( last<arStates->GetCount() && ((CDiffData::DiffStates)arStates->GetAt(last))==CDiffData::DIFFSTATE_CONFLICTED);
					file.Add(_T("<<<<<<< .mine"));
					for (int j=first; j<last; j++)
					{
						file.Add(m_pwndRightView->m_arDiffLines->GetAt(j));
					}
					file.Add(_T("======="));
					for (int j=first; j<last; j++)
					{
						file.Add(m_pwndLeftView->m_arDiffLines->GetAt(j));
					}
					file.Add(_T(">>>>>>> .theirs"));
					i = last-1;
				}
				break;
			case CDiffData::DIFFSTATE_EMPTY:
			case CDiffData::DIFFSTATE_CONFLICTEMPTY:
			case CDiffData::DIFFSTATE_IDENTICALREMOVED:
			case CDiffData::DIFFSTATE_REMOVED:
			case CDiffData::DIFFSTATE_THEIRSREMOVED:
			case CDiffData::DIFFSTATE_YOURSREMOVED:
				break;
			default:
				break;
			} // switch (state) 
		} // for (int i=0; i<arText->GetCount(); i++) 
		if (!file.Save(sFilePath, false))
		{
			CMessageBox::Show(m_hWnd, file.GetErrorString(), _T("TortoiseMerge"), MB_ICONERROR);
			return;
		}
		m_dlgFilePatches.SetFileStatusAsPatched(sFilePath);
	} // if ((arText)&&(arStates)&&(pOriginFile)) 
}

void CMainFrame::OnFileSave()
{
	FileSave();
}

bool CMainFrame::FileSave()
{
	if ((m_bReadOnly)||(!this->m_Data.m_mergedFile.InUse()))
		return FileSaveAs();
	int nConflictLine = CheckResolved();
	if (nConflictLine >= 0)
	{
		CString sTemp;
		sTemp.Format(IDS_ERR_MAINFRAME_FILEHASCONFLICTS, nConflictLine+1);
		if (MessageBox(sTemp, 0, MB_ICONERROR | MB_YESNO)!=IDYES)
		{
			if (m_pwndBottomView)
				m_pwndBottomView->GoToLine(nConflictLine);
			return false;
		}
	}
	if (((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\Backup"))) != 0)
	{
		DeleteFile(m_Data.m_mergedFile.GetFilename() + _T(".bak"));
		MoveFile(m_Data.m_mergedFile.GetFilename(), m_Data.m_mergedFile.GetFilename() + _T(".bak"));
	}
	SaveFile(this->m_Data.m_mergedFile.GetFilename());
	return true;
}

void CMainFrame::OnFileSaveAs()
{
	FileSaveAs();
}

bool CMainFrame::FileSaveAs()
{
	int nConflictLine = CheckResolved();
	if (nConflictLine >= 0)
	{
		CString sTemp;
		sTemp.Format(IDS_ERR_MAINFRAME_FILEHASCONFLICTS, nConflictLine+1);
		if (MessageBox(sTemp, 0, MB_ICONERROR | MB_YESNO)!=IDYES)
		{
			if (m_pwndBottomView)
				m_pwndBottomView->GoToLine(nConflictLine);
			return false;
		}
	}
	OPENFILENAME ofn;		// common dialog box structure
	TCHAR szFile[MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
	ofn.hwndOwner = this->m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
	CString temp;
	temp.LoadString(IDS_SAVEASTITLE);
	if (!temp.IsEmpty())
		ofn.lpstrTitle = temp;
	ofn.Flags = OFN_OVERWRITEPROMPT;
	CString sFilter;
	sFilter.LoadString(IDS_COMMONFILEFILTER);
	TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
	_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
	// Replace '|' delimeters with '\0's
	TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
	while (ptr != pszFilters)
	{
		if (*ptr == '|')
			*ptr = '\0';
		ptr--;
	} // while (ptr != pszFilters) 
	ofn.lpstrFilter = pszFilters;
	ofn.nFilterIndex = 1;

	// Display the Open dialog box. 
	CString sFile;
	if (GetSaveFileName(&ofn)==TRUE)
	{
		sFile = CString(ofn.lpstrFile);
		SaveFile(sFile);
		delete [] pszFilters;
		return true;
	} // if (GetSaveFileName(&ofn)==TRUE)
	delete [] pszFilters;
	return false;
}

void CMainFrame::OnUpdateFileSave(CCmdUI *pCmdUI)
{
	BOOL bEnable = FALSE;
	if ((!m_bReadOnly)&&(this->m_Data.m_mergedFile.InUse()))
	{
		if (m_pwndBottomView)
		{
			if ((m_pwndBottomView->IsWindowVisible())&&(m_pwndBottomView->m_arDiffLines))
			{
				bEnable = TRUE;
			} 
		}
		if (m_pwndRightView)
		{
			if ((m_pwndRightView->IsWindowVisible())&&(m_pwndRightView->m_arDiffLines))
			{
				if (m_pwndRightView->IsModified() || (m_Data.m_yourFile.GetWindowName().Right(9).Compare(_T(": patched"))==0))
					bEnable = TRUE;
			} 
		}
	}
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnUpdateFileSaveAs(CCmdUI *pCmdUI)
{
	BOOL bEnable = FALSE;
	if (m_pwndBottomView)
	{
		if ((m_pwndBottomView->IsWindowVisible())&&(m_pwndBottomView->m_arDiffLines))
		{
			bEnable = TRUE;
		}
	}
	if (m_pwndRightView)
	{
		if ((m_pwndRightView->IsWindowVisible())&&(m_pwndRightView->m_arDiffLines))
		{
			bEnable = TRUE;
		}
	} 
	pCmdUI->Enable(bEnable);
}


void CMainFrame::OnUpdateViewOnewaydiff(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!m_bOneWay);
	BOOL bEnable = TRUE;
	if (m_pwndBottomView)
	{
		if (m_pwndBottomView->IsWindowVisible())
			bEnable = FALSE;
	}
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnViewOptions()
{
	CString sTemp;
	sTemp.LoadString(IDS_SETTINGSTITLE);
	CSettings dlg(sTemp);
	dlg.DoModal();
	if (dlg.IsReloadNeeded())
	{
		if (((m_pwndBottomView)&&(m_pwndBottomView->IsModified())) ||
			((m_pwndRightView)&&(m_pwndRightView->IsModified())))
		{
			CString sTemp;
			sTemp.LoadString(IDS_WARNMODIFIEDLOOSECHANGESOPTIONS);
			if (MessageBox(sTemp, 0, MB_YESNO | MB_ICONQUESTION)==IDNO)
			{
				return;
			}
		}
		m_Data.LoadRegistry();
		LoadViews();
		return;
	} // if (dlg.IsReloadNeeded())
	m_Data.LoadRegistry();
	if (m_pwndBottomView)
		m_pwndBottomView->Invalidate();
	if (m_pwndLeftView)
		m_pwndLeftView->Invalidate();
	if (m_pwndRightView)
		m_pwndRightView->Invalidate();
}

void CMainFrame::OnClose()
{
	if ((m_pFindDialog)&&(!m_pFindDialog->IsTerminating()))
	{
		m_pFindDialog->SendMessage(WM_CLOSE);
		return;
	}
	int ret = IDNO;
	if (((m_pwndBottomView)&&(m_pwndBottomView->IsModified())) ||
		((m_pwndRightView)&&(m_pwndRightView->IsModified())))
	{
		CString sTemp;
		sTemp.LoadString(IDS_ASKFORSAVE);
		ret = MessageBox(sTemp, 0, MB_YESNOCANCEL | MB_ICONQUESTION);
		if (ret == IDYES)
		{
			if (!FileSave())
				return;
		}
	}
	if ((ret == IDNO)||(ret == IDYES))
	{
		WINDOWPLACEMENT    wp;

		// before it is destroyed, save the position of the window
		wp.length = sizeof wp;

		if ( GetWindowPlacement(&wp) )
		{

			if ( IsIconic() )
				// never restore to Iconic state
				wp.showCmd = SW_SHOW ;

			if ((wp.flags & WPF_RESTORETOMAXIMIZED) != 0)
				// if maximized and maybe iconic restore maximized state
				wp.showCmd = SW_SHOWMAXIMIZED ;

			// and write it to the .INI file
			WriteWindowPlacement(&wp);
		}
		__super::OnClose();
	}
}

void CMainFrame::OnEditFind()
{
	if (m_pFindDialog)
	{
		return;
	}
	else
	{
		m_pFindDialog = new CFindDlg();
		m_pFindDialog->Create(this);
	}
}

LRESULT CMainFrame::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_pFindDialog != NULL);

    if (m_pFindDialog->IsTerminating())
    {
	    // invalidate the handle identifying the dialog box.
        m_pFindDialog = NULL;
        return 0;
    }

    if(m_pFindDialog->FindNext())
    {
        //read data from dialog
        m_sFindText = m_pFindDialog->GetFindString();
        m_bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
		m_bLimitToDiff = m_pFindDialog->LimitToDiffs();
		m_bWholeWord = m_pFindDialog->WholeWord();
	
		OnEditFindnext();
    } // if(m_pFindDialog->FindNext()) 

    return 0;
}

bool CharIsDelimiter(const CString& ch)
{
	CString delimiters(_T(" .,:;=+-*/\\\n\t()[]<>@"));
	return delimiters.Find(ch) >= 0;
}

bool CMainFrame::StringFound(const CString& str)const
{
	int nSubStringStartIdx = str.Find(m_sFindText);
	bool bStringFound = (nSubStringStartIdx >= 0);
	if (bStringFound && m_bWholeWord)
	{
		if (nSubStringStartIdx)
			bStringFound = CharIsDelimiter(str.Mid(nSubStringStartIdx-1,1));
		
		if (bStringFound)
		{
			int nEndIndex = nSubStringStartIdx + m_sFindText.GetLength();
			if (str.GetLength() > nEndIndex)
				bStringFound = CharIsDelimiter(str.Mid(nEndIndex, 1));
		}
	}
	return bStringFound;
}

void CMainFrame::OnEditFindnext()
{
	if (m_sFindText.IsEmpty())
		return;

	if ((m_pwndLeftView)&&(m_pwndLeftView->m_arDiffLines))
	{
		bool bFound = FALSE;

		CString left;
		CString right;
		CString bottom;
		CDiffData::DiffStates leftstate = CDiffData::DIFFSTATE_NORMAL;
		CDiffData::DiffStates rightstate = CDiffData::DIFFSTATE_NORMAL;
		CDiffData::DiffStates bottomstate = CDiffData::DIFFSTATE_NORMAL;
		int i = 0;
		const int idxLimits[2][2]={{m_nSearchIndex, m_pwndLeftView->m_arDiffLines->GetCount()},
								   {0, m_nSearchIndex}};
		for (int j=0; j != 2 && !bFound; ++j)
		{
			for (i=idxLimits[j][0]; i != idxLimits[j][1]; i++)
			{
				left = m_pwndLeftView->m_arDiffLines->GetAt(i);
				leftstate = (CDiffData::DiffStates)m_pwndLeftView->m_arLineStates->GetAt(i);
				if ((!m_bOneWay)&&(m_pwndRightView->m_arDiffLines))
				{
					right = m_pwndRightView->m_arDiffLines->GetAt(i);
					rightstate = (CDiffData::DiffStates)m_pwndRightView->m_arLineStates->GetAt(i);
				}
				if ((m_pwndBottomView)&&(m_pwndBottomView->m_arDiffLines))
				{
					bottom = m_pwndBottomView->m_arDiffLines->GetAt(i);
					bottomstate = (CDiffData::DiffStates)m_pwndBottomView->m_arLineStates->GetAt(i);
				}

				if (!m_bMatchCase)
				{
					left = left.MakeLower();
					right = right.MakeLower();
					bottom = bottom.MakeLower();
					m_sFindText = m_sFindText.MakeLower();
				}
				if (StringFound(left))
				{
					if ((!m_bLimitToDiff)||(leftstate != CDiffData::DIFFSTATE_NORMAL))
					{
						bFound = TRUE;
						break;
					}
				} 
				else if (StringFound(right))
				{
					if ((!m_bLimitToDiff)||(rightstate != CDiffData::DIFFSTATE_NORMAL))
					{
						bFound = TRUE;
						break;
					}
				} 
				else if (StringFound(bottom))
				{
					if ((!m_bLimitToDiff)||(bottomstate != CDiffData::DIFFSTATE_NORMAL))
					{
						bFound = TRUE;
						break;
					}
				} 
			}
		}
		if (bFound)
		{
			m_nSearchIndex = i;
			m_pwndLeftView->GoToLine(m_nSearchIndex);
			if (StringFound(left))
			{
				m_pwndLeftView->SetFocus();
				m_pwndLeftView->SelectLines(m_nSearchIndex);
			}
			else if (StringFound(right))
			{
				m_pwndRightView->SetFocus();
				m_pwndRightView->SelectLines(m_nSearchIndex);
			}
			else if (StringFound(bottom))
			{
				m_pwndBottomView->SetFocus();
				m_pwndBottomView->SelectLines(m_nSearchIndex);
			}
			m_nSearchIndex++;
			if (m_nSearchIndex >= m_pwndLeftView->m_arDiffLines->GetCount())
				m_nSearchIndex = 0;
		} // if (bFound) 
		else
		{
			m_nSearchIndex = 0;
		}
	} // if ((m_pwndLeftView)&&(m_pwndLeftView->m_arDiffLines)) 
}

void CMainFrame::OnViewLinedown()
{
	if (m_pwndLeftView)
		m_pwndLeftView->ScrollToLine(m_pwndLeftView->m_nTopLine+1);
	if (m_pwndRightView)
		m_pwndRightView->ScrollToLine(m_pwndRightView->m_nTopLine+1);
	if (m_pwndBottomView)
		m_pwndBottomView->ScrollToLine(m_pwndBottomView->m_nTopLine+1);
	m_wndLocatorBar.Invalidate();
}

void CMainFrame::OnViewLineup()
{
	if (m_pwndLeftView)
		m_pwndLeftView->ScrollToLine(m_pwndLeftView->m_nTopLine-1);
	if (m_pwndRightView)
		m_pwndRightView->ScrollToLine(m_pwndRightView->m_nTopLine-1);
	if (m_pwndBottomView)
		m_pwndBottomView->ScrollToLine(m_pwndBottomView->m_nTopLine-1);
	m_wndLocatorBar.Invalidate();
}

void CMainFrame::OnViewLineleft()
{
	if (m_pwndLeftView)
		m_pwndLeftView->ScrollSide(-1);
	if (m_pwndRightView)
		m_pwndRightView->ScrollSide(-1);
	if (m_pwndBottomView)
		m_pwndBottomView->ScrollSide(-1);
}

void CMainFrame::OnViewLineright()
{
	if (m_pwndLeftView)
		m_pwndLeftView->ScrollSide(1);
	if (m_pwndRightView)
		m_pwndRightView->ScrollSide(1);
	if (m_pwndBottomView)
		m_pwndBottomView->ScrollSide(1);
}

void CMainFrame::OnFileReload()
{
	if (((m_pwndBottomView)&&(m_pwndBottomView->IsModified())) ||
		((m_pwndRightView)&&(m_pwndRightView->IsModified())))
	{
		CString sTemp;
		sTemp.LoadString(IDS_WARNMODIFIEDLOOSECHANGES);
		if (MessageBox(sTemp, 0, MB_YESNO | MB_ICONQUESTION)==IDNO)
		{
			return;
		}
	} // ified())))
	m_Data.LoadRegistry();
	LoadViews();
}

void CMainFrame::ActivateFrame(int nCmdShow)
{
	// nCmdShow is the normal show mode this frame should be in
	// translate default nCmdShow (-1)
	if (nCmdShow == -1)
	{
		if (!IsWindowVisible())
			nCmdShow = SW_SHOWNORMAL;
		else if (IsIconic())
			nCmdShow = SW_RESTORE;
	}

	// bring to top before showing
	BringToTop(nCmdShow);

	if (nCmdShow != -1)
	{
		// show the window as specified
		WINDOWPLACEMENT wp;

		if ( !ReadWindowPlacement(&wp) )
		{
			ShowWindow(nCmdShow);
		}
		else
		{
			if ( nCmdShow != SW_SHOWNORMAL )  
				wp.showCmd = nCmdShow;

			SetWindowPlacement(&wp);
		}

		// and finally, bring to top after showing
		BringToTop(nCmdShow);
	}
	return;
}

BOOL CMainFrame::ReadWindowPlacement(WINDOWPLACEMENT * pwp)
{
	CRegString placement = CRegString(_T("Software\\TortoiseMerge\\WindowPos"));
	CString sPlacement = placement;
	if (sPlacement.IsEmpty())
		return FALSE;
	int nRead = _stscanf_s(sPlacement, _T("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d"),
				&pwp->flags, &pwp->showCmd,
				&pwp->ptMinPosition.x, &pwp->ptMinPosition.y,
				&pwp->ptMaxPosition.x, &pwp->ptMaxPosition.y,
				&pwp->rcNormalPosition.left,  &pwp->rcNormalPosition.top,
				&pwp->rcNormalPosition.right, &pwp->rcNormalPosition.bottom);
	if ( nRead != 10 )  
		return FALSE;
	pwp->length = sizeof(WINDOWPLACEMENT);

	return TRUE;
}

void CMainFrame::WriteWindowPlacement(WINDOWPLACEMENT * pwp)
{
	CRegString placement = CRegString(_T("Software\\TortoiseMerge\\WindowPos"));
	TCHAR szBuffer[sizeof("-32767")*8 + sizeof("65535")*2];
	CString s;

	_stprintf_s(szBuffer, sizeof("-32767")*8 + sizeof("65535")*2, _T("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d"),
			pwp->flags, pwp->showCmd,
			pwp->ptMinPosition.x, pwp->ptMinPosition.y,
			pwp->ptMaxPosition.x, pwp->ptMaxPosition.y,
			pwp->rcNormalPosition.left, pwp->rcNormalPosition.top,
			pwp->rcNormalPosition.right, pwp->rcNormalPosition.bottom);
	placement = szBuffer;
}

void CMainFrame::OnUpdateMergeMarkasresolved(CCmdUI *pCmdUI)
{
	if (pCmdUI == NULL)
		return;
	BOOL bEnable = FALSE;
	if ((!m_bReadOnly)&&(this->m_Data.m_mergedFile.InUse()))
	{
		if (m_pwndBottomView)
		{
			if ((m_pwndBottomView->IsWindowVisible())&&(m_pwndBottomView->m_arDiffLines))
			{
				bEnable = TRUE;
			} 
		}
	}
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnMergeMarkasresolved()
{
	int nConflictLine = CheckResolved();
	if (nConflictLine >= 0)
	{
		CString sTemp;
		sTemp.Format(IDS_ERR_MAINFRAME_FILEHASCONFLICTS, nConflictLine+1);
		if (MessageBox(sTemp, 0, MB_ICONERROR | MB_YESNO)!=IDYES)
		{
			if (m_pwndBottomView)
				m_pwndBottomView->GoToLine(nConflictLine);
			return;
		}
	}
	// now check if the file has already been saved and if not, save it.
	if (this->m_Data.m_mergedFile.InUse())
	{
		if (m_pwndBottomView)
		{
			if ((m_pwndBottomView->IsWindowVisible())&&(m_pwndBottomView->m_arDiffLines))
			{
				FileSave();
			} 
		}
	}	
	MarkAsResolved();
}

BOOL CMainFrame::MarkAsResolved()
{
	if (m_bReadOnly)
		return FALSE;
	if ((m_pwndBottomView)&&(m_pwndBottomView->IsWindowVisible()))
	{
		TCHAR buf[MAX_PATH*3];
		GetModuleFileName(NULL, buf, MAX_PATH);
		TCHAR * end = _tcsrchr(buf, '\\');
		end++;
		(*end) = 0;
		_tcscat_s(buf, MAX_PATH*3, _T("TortoiseProc.exe /command:resolve /path:\""));
		_tcscat_s(buf, MAX_PATH*3, this->m_Data.m_mergedFile.GetFilename());
		_tcscat_s(buf, MAX_PATH*3, _T("\" /closeonend:1 /noquestion /notempfile"));
		STARTUPINFO startup;
		PROCESS_INFORMATION process;
		memset(&startup, 0, sizeof(startup));
		startup.cb = sizeof(startup);
		memset(&process, 0, sizeof(process));
		if (CreateProcess(NULL, buf, NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
		{
			LPVOID lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
				);
			MessageBox((LPCTSTR)lpMsgBuf, _T("TortoiseMerge"), MB_OK | MB_ICONINFORMATION);
			LocalFree( lpMsgBuf );
			return FALSE;
		}
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
	}
	else
		return FALSE;
	return TRUE;
}

void CMainFrame::OnUpdateMergeNextconflict(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bHasConflicts);
}

void CMainFrame::OnUpdateMergePreviousconflict(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bHasConflicts);
}

void CMainFrame::OnMoving(UINT fwSide, LPRECT pRect)
{
	// if the pathfilelist dialog is attached to the mainframe,
	// move it along with the mainframe
	if (::IsWindow(m_dlgFilePatches.m_hWnd))
	{
		RECT patchrect;
		m_dlgFilePatches.GetWindowRect(&patchrect);
		if (::IsWindow(this->m_hWnd))
		{
			RECT thisrect;
			GetWindowRect(&thisrect);
			if (patchrect.right == thisrect.left)
			{
				m_dlgFilePatches.SetWindowPos(NULL, patchrect.left - (thisrect.left - pRect->left), patchrect.top - (thisrect.top - pRect->top), 
					0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
			}
		}
	}
	__super::OnMoving(fwSide, pRect);
}

void CMainFrame::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	BOOL bShow = FALSE;
	if ((m_pwndBottomView)&&(m_pwndBottomView->HasSelection()))
		bShow = TRUE;
	if ((m_pwndRightView)&&(m_pwndRightView->HasSelection()))
		bShow = TRUE;
	if ((m_pwndLeftView)&&(m_pwndLeftView->HasSelection()))
		bShow = TRUE;
	pCmdUI->Enable(bShow);
}

void CMainFrame::OnViewSwitchleft()
{
	CWorkingFile file = m_Data.m_baseFile;
	m_Data.m_baseFile = m_Data.m_yourFile;
	m_Data.m_yourFile = file;
	if (m_Data.m_mergedFile.GetFilename().CompareNoCase(m_Data.m_yourFile.GetFilename())==0)
	{
		m_Data.m_mergedFile = m_Data.m_baseFile;
	}
	else if (m_Data.m_mergedFile.GetFilename().CompareNoCase(m_Data.m_baseFile.GetFilename())==0)
	{
		m_Data.m_mergedFile = m_Data.m_yourFile;
	}
	LoadViews();
}

void CMainFrame::OnUpdateViewSwitchleft(CCmdUI *pCmdUI)
{
	BOOL bEnable = TRUE;
	if (m_pwndBottomView)
	{
		if (m_pwndBottomView->IsWindowVisible())
			bEnable = FALSE;
	}
	pCmdUI->Enable(bEnable);
}


void CMainFrame::OnUpdateViewShowfilelist(CCmdUI *pCmdUI)
{
	if (m_dlgFilePatches.HasFiles())
	{
		pCmdUI->Enable(true);
	}
	else
		pCmdUI->Enable(false);
	pCmdUI->SetCheck(m_dlgFilePatches.IsWindowVisible());
}

void CMainFrame::OnViewShowfilelist()
{
	m_dlgFilePatches.ShowWindow(m_dlgFilePatches.IsWindowVisible() ? SW_HIDE : SW_SHOW);
}
