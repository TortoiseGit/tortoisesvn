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

#include "StandAloneDlg.h"
#include "TSVNPath.h"
#include "SVN.h"
#include "Colors.h"
#include "afxwin.h"

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

/** 
 * Options which can be used to configure the way the dialog box works
 */
typedef enum
{
	ProgOptNone = 0,
	ProgOptRecursive = 0x01,
	ProgOptNonRecursive = 0x00,
	/// Don't actually do the merge - just practice it
	ProgOptDryRun = 0x04,
	ProgOptIgnoreExternals = 0x08,
	ProgOptKeeplocks = 0x10,
	/// for locking this means steal the lock, for unlocking it means breaking the lock
	ProgOptLockForce = 0x20,
	ProgOptSwitchAfterCopy = 0x40,
	ProgOptIncludeIgnored = 0x80,
	ProgOptIgnoreAncestry = 0x100,
	ProgOptEolDefault = 0x200,
	ProgOptEolCRLF = 0x400,
	ProgOptEolLF = 0x800,
	ProgOptEolCR = 0x1000,
	ProgOptSkipConflictCheck = 0x2000
} ProgressOptions;

typedef enum
{
	CLOSE_MANUAL = 0,
	CLOSE_NOERRORS,
	CLOSE_NOCONFLICTS,
	CLOSE_NOMERGES,
	CLOSE_LOCAL
} ProgressCloseOptions;

/**
 * \ingroup TortoiseProc
 * Handles different Subversion commands and shows the notify messages
 * in a listbox. Since several Subversion commands have similar notify
 * messages they are grouped together in this single class.
 */
class CSVNProgressDlg : public CResizableStandAloneDialog, SVN
{
public:
	typedef enum
	{
		SVNProgress_Checkout = 1,
		SVNProgress_Update = 2,
		SVNProgress_Commit = 3,
		SVNProgress_Add = 4,
		SVNProgress_Revert = 5,
		SVNProgress_Resolve = 6,
		SVNProgress_Import = 7,
		SVNProgress_Switch = 8,
		SVNProgress_Export = 9,
		SVNProgress_Merge = 10,
		SVNProgress_Copy = 11,
		SVNProgress_Relocate = 12,
		SVNProgress_Rename = 13,
		SVNProgress_Lock = 14,
		SVNProgress_Unlock = 15
	} Command;

private:
	class NotificationData
	{
	public:
		NotificationData() :
			action((svn_wc_notify_action_t)-1),
			kind(svn_node_none),
			content_state(svn_wc_notify_state_inapplicable),
			prop_state(svn_wc_notify_state_inapplicable),
			rev(0),
			color(::GetSysColor(COLOR_WINDOWTEXT)),
			bConflictedActionItem(false),
			bAuxItem(false),
			lock_state(svn_wc_notify_lock_state_unchanged)
		{
		}
	public:
		// The text we put into the first column (the SVN action for normal items, just text for aux items)
		CString					sActionColumnText;	
		CTSVNPath				path;
		CTSVNPath				basepath;
		CString					changelistname;

		svn_wc_notify_action_t	action;
		svn_node_kind_t			kind;
		CString					mime_type;
		svn_wc_notify_state_t	content_state;
		svn_wc_notify_state_t	prop_state;
		svn_wc_notify_lock_state_t lock_state;
		svn_revnum_t			rev;
		COLORREF				color;
		CString					owner;						///< lock owner
		bool					bConflictedActionItem;		// Is this item a conflict?
		bool					bAuxItem;					// Set if this item is not a true 'SVN action' 
		CString					sPathColumnText;	

	};

	DECLARE_DYNAMIC(CSVNProgressDlg)

public:

	CSVNProgressDlg(CWnd* pParent = NULL);
	virtual ~CSVNProgressDlg();

	virtual BOOL OnInitDialog();
	/**
	 * Sets the needed parameters for the commands to execute. Call this method
	 * before DoModal()
	 * \param cmd the command to execute on DoModal()
	 * \param path local path to the working copy (directory or file) or to a tempfile containing the filenames
	 * \param url the url of the repository
	 * \param revision the revision to work on or to get
	 */
	void SetParams(Command cmd, int options, const CTSVNPathList& pathList, const CString& url = CString(), const CString& message = CString(), SVNRev revision = -1); 

	void SetPegRevision(SVNRev pegrev = SVNRev()) {m_pegRev = pegrev;}

	void SetChangeList(const CString& changelist, bool keepchangelist) {m_changelist = changelist; m_keepchangelist = keepchangelist;}
	
	CString BuildInfoString();
	
	bool DidErrorsOccur() {return m_bErrorsOccurred;}

	enum { IDD = IDD_SVNPROGRESS };

protected:
	//implement the virtual methods from SVN base class
	virtual BOOL Notify(const CTSVNPath& path, svn_wc_notify_action_t action, 
		svn_node_kind_t kind, const CString& mime_type, 
		svn_wc_notify_state_t content_state, 
		svn_wc_notify_state_t prop_state, LONG rev,
		const svn_lock_t * lock, svn_wc_notify_lock_state_t lock_state,
		const CString& changelistname,
		svn_error_t * err, apr_pool_t * pool);
	virtual BOOL	Cancel();
	virtual void	OnCancel();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);

	afx_msg void	OnNMCustomdrawSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnNMDblclkSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnBnClickedLogbutton();
	afx_msg void	OnBnClickedOk();
	afx_msg void	OnHdnItemclickSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void	OnClose();
	afx_msg void	OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnSVNProgress(WPARAM wParam, LPARAM lParam);
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	afx_msg void	OnEnSetfocusInfotext();
	afx_msg void	OnLvnBegindragSvnprogress(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

	void			Sort();
	static bool		SortCompare(const NotificationData* pElem1, const NotificationData* pElem2);

	static BOOL	m_bAscending;
	static int	m_nSortedColumn;
	CStringList m_ExtStack;

private:
	static UINT ProgressThreadEntry(LPVOID pVoid);
	UINT		ProgressThread();
	virtual void OnOK();
	void		ReportSVNError();
	void		ReportError(const CString& sError);
	void		ReportWarning(const CString& sWarning);
	void		ReportNotification(const CString& sNotification);
	void		ReportString(CString sMessage, const CString& sMsgKind, COLORREF color = ::GetSysColor(COLOR_WINDOWTEXT));
	void		AddItemToList(const NotificationData* pData);

private:
	/**
	* Resizes the columns of the progress list so that the headings are visible.
	*/
	void		ResizeColumns();

	// Predicate function to tell us if a notification data item is auxiliary or not
	static bool NotificationDataIsAux(const NotificationData* pData);


public:
	DWORD		m_dwCloseOnEnd;
	SVNRev		m_RevisionEnd;

private:
	typedef std::map<CStringA, svn_revnum_t> StringRevMap;

	typedef std::vector<NotificationData *> NotificationDataVect;
	NotificationDataVect	m_arData;

	CListCtrl	m_ProgList;
	CWinThread* m_pThread;
	Command		m_Command;
	int			m_options;	// Use values from the ProgressOptions enum

	CTSVNPathList m_targetPathList;
	CTSVNPath	m_url;
	CString		m_sMessage;
	SVNRev		m_Revision;
	SVNRev		m_pegRev;

	CString		m_changelist;
	bool		m_keepchangelist;
	
	CTSVNPath	m_basePath;
	StringRevMap m_UpdateStartRevMap;
	StringRevMap m_FinishedRevMap;
	BOOL		m_bCancelled;
	volatile LONG m_bThreadRunning;
	bool		m_bConflictsOccurred;
	bool		m_bErrorsOccurred;
	bool		m_bMergesAddsDeletesOccurred;
	int			iFirstResized;
	BOOL		bSecondResized;
	CString		m_sTotalBytesTransferred;
	CColors		m_Colors;

	bool		m_bLockWarning;
	bool		m_bFinishedItemAdded;
	bool		m_bLastVisible;

private:
	// In preparation for removing SVN as base class
	// Currently needed to avoid ambiguities with the Command Enum
	SVN* m_pSvn;
};
