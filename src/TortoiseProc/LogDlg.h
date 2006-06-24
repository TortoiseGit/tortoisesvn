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
#pragma once

#include "svn.h"
#include "ProjectProperties.h"
#include "StandAloneDlg.h"
#include "TSVNPath.h"
#include "registry.h"
#include "SplitterControl.h"
#include "Colors.h"
#include "MenuButton.h"
#include "afxwin.h"
#include "afxdtctl.h"

#define ID_COMPARE		1
#define ID_SAVEAS		2
#define ID_COMPARETWO	3
#define ID_UPDATE		4
#define ID_COPY			5
#define ID_REVERTREV	6
#define ID_GNUDIFF1		7
#define ID_GNUDIFF2		8
#define ID_FINDENTRY	9
#define ID_REVERT	   10
#define	ID_REFRESH	   11
#define ID_OPEN		   12
#define ID_REPOBROWSE  13
#define ID_DELETE	   14
#define ID_IGNORE	   15
#define	ID_LOG		   16
#define ID_POPPROPS	   17
#define ID_EDITAUTHOR  18
#define ID_EDITLOG     19

#define ID_DIFF			20
#define ID_EDITCONFLICT	21
#define ID_OPENWITH		22
#define ID_COPYCLIPBOARD 23
#define ID_CHECKOUT		24
#define ID_CONFLICTUSETHEIRS 25
#define ID_CONFLICTUSEMINE 26
#define ID_REVERTTOREV	27
#define ID_EXPLORE		28
#define ID_BLAMECOMPARE 29
#define ID_BLAMETWO     30
#define ID_BLAMEDIFF    31

#define LOGFILTER_ALL      1
#define LOGFILTER_MESSAGES 2
#define LOGFILTER_PATHS    3
#define LOGFILTER_AUTHORS  4
#define LOGFILTER_REVS	   5


#define LOGFILTER_TIMER		101

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

/**
 * \ingroup TortoiseProc
 * Shows log messages of a single file or folder in a listbox. 
 */
class CLogDlg : public CResizableStandAloneDialog, public SVN //CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CLogDlg)

public:
	CLogDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLogDlg();

// Dialog Data
	enum { IDD = IDD_LOGMESSAGE };

protected:
	//implement the virtual methods from SVN base class
	virtual BOOL Log(svn_revnum_t rev, const CString& author, const CString& date, const CString& message, LogChangedPathArray * cpaths, apr_time_t time, int filechanges, BOOL copies, DWORD actions);
	virtual BOOL Cancel();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedGetall();
	afx_msg void OnNMDblclkLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedStatbutton();
	afx_msg void OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetdispinfoLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnStnClickedFiltericon();
	afx_msg void OnBnClickedFiltercancel();
	afx_msg void OnEnChangeSearchedit();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDtnDatetimechangeDateto(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDtnDatetimechangeDatefrom(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnColumnclickLogmsg(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedNexthundred();
	afx_msg void OnBnClickedHidepaths();
	afx_msg void OnBnClickedCheckStoponcopy();
	afx_msg void OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void	FillLogMessageCtrl(bool bShow = true);
	void	DoDiffFromLog(int selIndex, svn_revnum_t rev1, svn_revnum_t rev2, bool blame);

	DECLARE_MESSAGE_MAP()
public:
	void SetParams(const CTSVNPath& path, SVNRev pegrev, SVNRev startrev, SVNRev endrev, int limit, 
				BOOL bStrict = CRegDWORD(_T("Software\\TortoiseSVN\\LastLogStrict"), FALSE), BOOL bSaveStrict = TRUE);
	void SetProjectPropertiesPath(const CTSVNPath& path) {m_ProjectProperties.ReadProps(path);}
	bool IsThreadRunning() {return !!m_bThreadRunning;}
	void SetDialogTitle(const CString& sTitle) {m_sTitle = sTitle;}
	void SetSelect(bool bSelect) {m_bSelect = bSelect;}
	void ContinuousSelection(bool bCont = true) {m_bSelectionMustBeContinuous = bCont;}

private:
	static UINT LogThreadEntry(LPVOID pVoid);
	UINT LogThread();
	void Refresh();
	BOOL DiffPossible(LogChangedPath * changedpath, svn_revnum_t rev);
	void EditAuthor(int index);
	void EditLogMessage(int index);
	void DoSizeV1(int delta);
	void DoSizeV2(int delta);
	void SetSplitterRange();
	void SetFilterCueText();
	BOOL IsEntryInDateRange(int i);
	void CopySelectionToClipBoard();
	CTSVNPathList GetChangedPathsFromSelectedRevisions(bool bRelativePaths = false, bool bUseFilter = true);
    void SortShownListArray();
    void SetSortArrow(CListCtrl * control, int nColumn, bool bAscending);
	bool IsSelectionContinuous();
	void EnableOKButton();
	void GetAll(bool bForceAll = false);

	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	static int __cdecl	SortCompare(const void * pElem1, const void * pElem2);	///< sort callback function

	void ResizeAllListCtrlCols(CListCtrl &list);

public:
	CWnd *				m_pNotifyWindow;
	ProjectProperties	m_ProjectProperties;
	WORD				m_wParam;
private:
	CString				m_sRelativeRoot;
	CString				m_sRepositoryRoot;
	CListCtrl			m_LogList;
	CListCtrl			m_LogMsgCtrl;
	CProgressCtrl		m_LogProgress;
	CMenuButton			m_btnShow;
	CTSVNPath			m_path;
	SVNRev				m_pegrev;
	SVNRev				m_startrev;
	SVNRev				m_LogRevision;
	SVNRev				m_endrev;
	bool				m_bSelectionMustBeContinuous;
	long				m_logcounter;
	bool				m_bCancelled;
	volatile LONG 		m_bThreadRunning;
	BOOL				m_bStrict;
	BOOL				m_bSaveStrict;
	LogChangedPathArray * m_currentChangedArray;
	LogChangedPathArray m_CurrentFilteredChangedArray;
	CTSVNPathList		m_currentChangedPathList;
	CPtrArray			m_arShownList;
	bool				m_hasWC;
	int					m_nSearchIndex;
	static const UINT	m_FindDialogMessage;
	CFindReplaceDialog *m_pFindDialog;
	CFont				m_logFont;
	CString				m_sMessageBuf;
	CSplitterControl	m_wndSplitter1;
	CSplitterControl	m_wndSplitter2;
	CString				m_sFilterText;
	int					m_nSelectedFilter;
	volatile LONG		m_bNoDispUpdates;
	CDateTimeCtrl		m_DateFrom;
	CDateTimeCtrl		m_DateTo;
	DWORD				m_tFrom;
	DWORD				m_tTo;
	int					m_limit;
	int					m_limitcounter;
	CBitmapButton		m_cFilterCancelButton;
	CBitmapButton		m_cFilterIcon;
	int                 m_nSortColumn;
	bool                m_bAscending;
	static int			m_nSortColumnPathList;
	static bool			m_bAscendingPathList;
	CRegDWORD			m_regLastStrict;
	CButton				m_cHidePaths;
	bool				m_bShowedAll;
	CString				m_sTitle;
	bool				m_bSelect;
	bool				m_bShowBugtraqColumn;
	
	CTime				m_timFrom;
	CTime				m_timTo;
	CColors				m_Colors;
	CImageList			m_imgList;
	HICON				m_hModifiedIcon;
	HICON				m_hReplacedIcon;
	HICON				m_hAddedIcon;
	HICON				m_hDeletedIcon;

	CString sModifiedStatus, sReplacedStatus, sAddStatus, sDeleteStatus;

private:
    typedef struct LogEntryData
    {   
        DWORD dwRev;
        __time64_t tmDate;
        CString sDate;
        CString sAuthor;
        CString sMessage;
        CString sShortMessage;
        DWORD dwFileChanges;
        LogChangedPathArray* pArChangedPaths;
        BOOL bCopies;
        DWORD actions;
    } LOGENTRYDATA, *PLOGENTRYDATA;
    class CLogDataVector : 
        public std::vector<PLOGENTRYDATA>
    {
    public:
        // De-allocate log items.
        void ClearAll()
        {
            if(size() > 0)
            {
                for(iterator it=begin(); it!=end(); ++it)
                {
                    LogChangedPathArray * pPaths = (*it)->pArChangedPaths;
                    for(INT_PTR j=0; j<pPaths->GetCount(); ++j)
                    {
                        delete pPaths->GetAt(j);
                    }
                    pPaths->RemoveAll();
                    delete pPaths;
                    
                    delete *it;
                }     
                clear();
            }
        }
        // Ascending date sorting.
        struct AscDateSort
        {
            bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
            {
                return pStart->tmDate < pEnd->tmDate;
            }
        };
        // Descending date sorting.
        struct DescDateSort
        {
            bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
            {
                return pStart->tmDate > pEnd->tmDate;
            }
        };
        // Ascending revision sorting.
        struct AscRevSort
        {
            bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
            {
                return pStart->dwRev < pEnd->dwRev;
            }
        };
        // Descending revision sorting.
        struct DescRevSort
        {
            bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
            {
                return pStart->dwRev > pEnd->dwRev;
            }
        };
        // Ascending author sorting.
        struct AscAuthorSort
        {
            bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
            {
                return pStart->sAuthor < pEnd->sAuthor;
            }
        };
        // Descending author sorting.
        struct DescAuthorSort
        {
            bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
            {
                return pStart->sAuthor > pEnd->sAuthor;
            }
        };
        // Ascending message sorting.
        struct AscMessageSort
        {
            bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
            {
                return pStart->sShortMessage < pEnd->sShortMessage;
            }
        };
        // Descending message sorting.
        struct DescMessageSort
        {
            bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
            {
                return pStart->sShortMessage > pEnd->sShortMessage;
            }
        };
		// Ascending action sorting
		struct AscActionSort
		{
			bool operator() (PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
			{
				return pStart->actions < pEnd->actions;
			}
		};
		// Descending action sorting
		struct DescActionSort
		{
			bool operator() (PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
			{
				return pStart->actions > pEnd->actions;
			}
		};
    };
    CLogDataVector m_logEntries;
};
static UINT WM_REVSELECTED = RegisterWindowMessage(_T("TORTOISESVN_REVSELECTED_MSG"));
