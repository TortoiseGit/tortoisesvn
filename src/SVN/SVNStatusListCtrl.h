// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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

#pragma once

#include "TSVNPath.h"
#include "SVNRev.h"

class SVNConfig;
class SVNStatus;

#define SVNSLC_COLSTATUS		0x000000002
#define SVNSLC_COLREMOTESTATUS	0x000000004
#define SVNSLC_COLTEXTSTATUS	0x000000008
#define SVNSLC_COLPROPSTATUS	0x000000010
#define SVNSLC_COLREMOTETEXT	0x000000020
#define SVNSLC_COLREMOTEPROP	0x000000040
#define SVNSLC_COLURL			0x000000080


#define SVNSLC_SHOWUNVERSIONED	0x000000001
#define SVNSLC_SHOWNORMAL		0x000000002
#define SVNSLC_SHOWMODIFIED		0x000000004
#define SVNSLC_SHOWADDED		0x000000008
#define SVNSLC_SHOWREMOVED		0x000000010
#define SVNSLC_SHOWCONFLICTED	0x000000020
#define SVNSLC_SHOWMISSING		0x000000040
#define SVNSLC_SHOWREPLACED		0x000000080
#define SVNSLC_SHOWMERGED		0x000000100
#define SVNSLC_SHOWIGNORED		0x000000200
#define SVNSLC_SHOWOBSTRUCTED	0x000000400
#define SVNSLC_SHOWEXTERNAL		0x000000800
#define SVNSLC_SHOWINCOMPLETE	0x000001000
#define SVNSLC_SHOWINEXTERNALS	0x000002000
#define SVNSLC_SHOWDIRECTS		0x000004000

#define SVNSLC_SHOWVERSIONED (SVNSLC_SHOWNORMAL|SVNSLC_SHOWMODIFIED|\
SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWCONFLICTED|SVNSLC_SHOWMISSING|SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED|\
SVNSLC_SHOWIGNORED|SVNSLC_SHOWOBSTRUCTED|SVNSLC_SHOWEXTERNAL|SVNSLC_SHOWINCOMPLETE|SVNSLC_SHOWINEXTERNALS)

#define SVNSLC_SHOWVERSIONEDBUTNORMAL (SVNSLC_SHOWMODIFIED|\
	SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWCONFLICTED|SVNSLC_SHOWMISSING|SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED|\
	SVNSLC_SHOWIGNORED|SVNSLC_SHOWOBSTRUCTED|SVNSLC_SHOWEXTERNAL|SVNSLC_SHOWINCOMPLETE|SVNSLC_SHOWINEXTERNALS)

#define SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALS (SVNSLC_SHOWMODIFIED|\
	SVNSLC_SHOWADDED|SVNSLC_SHOWREMOVED|SVNSLC_SHOWCONFLICTED|SVNSLC_SHOWMISSING|SVNSLC_SHOWREPLACED|SVNSLC_SHOWMERGED|\
	SVNSLC_SHOWIGNORED|SVNSLC_SHOWOBSTRUCTED|SVNSLC_SHOWINCOMPLETE)

#define SVNSLC_SHOWALL (SVNSLC_SHOWVERSIONED|SVNSLC_SHOWUNVERSIONED)

typedef int (__cdecl *GENERICCOMPAREFN)(const void * elem1, const void * elem2);

/**
 * \ingroup TortoiseProc
 * A List control, based on the MFC CListCtrl which shows a list of
 * files with their Subversion status. The control also provides a context
 * menu to do some Subversion tasks on the selected files.
 *
 * \par requirements
 * win98 or later\n
 * win2k or later\n
 * MFC\n
 *
 * \version 1.0
 * first version
 *
 * \date 07-04-2004
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CSVNStatusListCtrl : public CListCtrl
{
public: 
	
	/**
	 * Sent to the parent window (using ::SendMessage) after a context menu
	 * command has finished if the item count has changed.
	 */
	static const UINT SVNSLNM_ITEMCOUNTCHANGED;

	CSVNStatusListCtrl();
	~CSVNStatusListCtrl();

	/**
	 * \ingroup TortoiseProc
	 * Helper class for CSVNStatusListCtrl which represents
	 * the data for each file shown.
	 */
	class FileEntry
	{
	public:
		const CTSVNPath& GetPath() const					
		{
			return path;
		}
		const bool IsChecked() const
		{
			return checked;
		}
		CString GetRelativeSVNPath() const
		{
			return path.GetSVNPathString().Mid(basepath.GetSVNPathString().GetLength()+1);
		}
	private:
		CTSVNPath				path;					///< full path of the file
		CTSVNPath				basepath;				///< common ancestor path of all files
		CString					url;					///< Subversion URL of the file
	public:
		svn_wc_status_kind		status;					///< local status
	private:
		svn_wc_status_kind		textstatus;				///< local text status
		svn_wc_status_kind		propstatus;				///< local property status
		svn_wc_status_kind		remotestatus;			///< remote status
		svn_wc_status_kind		remotetextstatus;		///< remote text status
		svn_wc_status_kind		remotepropstatus;		///< remote property status
		bool					checked;				///< if the file is checked in the list control
		bool					inunversionedfolder;	///< if the file is inside an unversioned folder
		bool					inexternal;				///< if the item is in an external folder
		bool					direct;					///< directly included (TRUE) or just a child of a folder
		bool					isfolder;				///< TRUE if entry refers to a folder

		friend class CSVNStatusListCtrl;
	};

	/**
	 * Initializes the control, sets up the columns.
	 * \param dwColumns mask of columns to show. Use the SVNSLC_COLxxx defines.
	 * \param bHasCheckboxes TRUE if the control should show checkboxes on the left of each file entry
	 */
	void Init(DWORD dwColumns, bool bHasCheckboxes = TRUE);

	/**
	 * Fetches the Subversion status of all files and stores the information
	 * about them in an internal array.
	 * \param sFilePath path to a file which contains a list of files and/or folders for which to
	 *                  fetch the status, separated by newlines.
	 * \param bUpdate TRUE if the remote status is requested too.
	 * \return TRUE on success.
	 */
	BOOL GetStatus(const CTSVNPathList& pathList, bool bUpdate = false);

	/**
	 * Populates the list control with the previously (with GetStatus) gathered status information.
	 * \param dwShow mask of file types to show. Use the SVNSLC_SHOWxxx defines.
	 * \param dwCheck mask of file types to check. Use SVNLC_SHOWxxx defines. Default (0) means 'use the entry's stored check status'
	 */
	void Show(DWORD dwShow, DWORD dwCheck = 0);

	/**
	 * If during the call to GetStatus() some svn:externals are found from different
	 * repositories than the first one checked, then this method returns TRUE.
	 */
	BOOL HasExternalsFromDifferentRepos() {return m_bHasExternalsFromDifferentRepos;}

	/**
	 * If during the call to GetStatus() some svn:externals are found then this method returns TRUE.
	 */
	BOOL HasExternals() {return m_bHasExternals;}

	/**
	 * If unversioned files are found (but not necessarily shown) TRUE is returned.
	 */
	BOOL HasUnversionedItems() {return m_bHasUnversionedItems;}
	/**
	 * Returns the file entry data for the list control index.
	 */
	CSVNStatusListCtrl::FileEntry * GetListEntry(int index);

	/**
	 * Returns a String containing some statistics like number of modified, normal, deleted,...
	 * files.
	 */
	CString GetStatisticsString();

	/**
	 * Set a static control which will be updated automatically with
	 * the number of selected and total files shown in the list control.
	 */
	void SetStatLabel(CWnd * pStatLabel){m_pStatLabel = pStatLabel;};

	/**
	 * Set a tri-state checkbox which is updated automatically if the
	 * user checks/unchecks file entries in the list control to indicate
	 * if all files are checked, none are checked or some are checked.
	 */
	void SetSelectButton(CButton * pButton) {m_pSelectButton = pButton;}

	/**
	 * Select/unselect all entries in the list control.
	 * \param bSelect TRUE to check, FALSE to uncheck.
	 */
	void SelectAll(bool bSelect);

	/** Set a checkbox on an entry in the listbox
	 * Keeps the listctrl checked state and the FileEntry's checked flag in sync
	 */
	void SetEntryCheck(FileEntry* pEntry, int listboxIndex, bool bCheck);

	/** Write a list of the checked items' paths into a path list 
	 */
	void WriteCheckedNamesToPathList(CTSVNPathList& pathList);


public:
	CString GetLastErrorMessage() {return m_sLastError;}

	void Block(BOOL block) {m_bBlock = block;}

	LONG						m_nTargetCount;		///< number of targets in the file passed to GetStatus()

	CString						m_sURL;				///< the URL of the target or "(multiple targets)"

	SVNRev						m_HeadRev;			///< the HEAD revision of the repository if bUpdate was TRUE

	CString						m_sUUID;			///< the UUID of the associated repository

	DECLARE_MESSAGE_MAP()

private:
	void Sort();	///< Sorts the control by columns
	void AddEntry(const FileEntry * entry, WORD langID, int listIndex);	///< add an entry to the control
	void RemoveListEntry(int index);	///< removes an entry from the listcontrol and both arrays
	void BuildStatistics();	///< build the statistics
	void StartDiff(int fileindex);	///< start the external diff program
	CTSVNPath BuildTargetFile();		///< builds a temporary files containing the paths of the selected entries
	//static int __cdecl SortCompare(const void * pElem1, const void * pElem2);	///< sort callback function
	static bool CSVNStatusListCtrl::SortCompare(const FileEntry* entry1, const FileEntry* entry2);

	///< Process one line of the command file supplied to GetStatus
	bool FetchStatusForSingleTarget(SVNConfig& config, SVNStatus& status, const CTSVNPath& target, bool bFetchStatusFromRepository, CStringA& strCurrentRepositoryUUID, CTSVNPathList& arExtPaths); 

	///< Create 'status' data for each item in an unversioned folder
	void AddUnversionedFolder(const CTSVNPath& strFolderName, const CTSVNPath& strBasePath, apr_array_header_t *pIgnorePatterns);

	///< Read the all the other status items which result from a single GetFirstStatus call
	void ReadRemainingItemsStatus(SVNStatus& status, const CTSVNPath& strBasePath, CStringA& strCurrentRepositoryUUID, CTSVNPathList& arExtPaths, apr_array_header_t *pIgnorePatterns);

	///< Clear the status vector (contains custodial pointers)
	void ClearStatusArray();

	///< Sort predicate function - Compare the paths of two entries without regard to case
	static bool EntryPathCompareNoCase(const FileEntry* pEntry1, const FileEntry* pEntry2);

	///< Predicate used to build a list of only the versioned entries of the FileEntry array
	static bool IsEntryVersioned(const FileEntry* pEntry1);

	///< Look up the relevant show flags for a particular SVN status value
	DWORD GetShowFlagsFromSVNStatus(svn_wc_status_kind status);

	///< Build a FileEntry item and add it to the FileEntry array
	const FileEntry* AddNewFileEntry(
		const svn_wc_status_t* pSVNStatus,  // The return from the SVN GetStatus functions
		const CTSVNPath& path,				// The path of the item we're adding
		const CTSVNPath& basePath,			// The base directory for this status build
		bool bDirectItem,					// Was this item the first found by GetFirstFileStatus or by a subsequent GetNextFileStatus call
		bool bInExternal					// Are we in an 'external' folder
		);

	///< Adjust the checkbox-state on all descendents of a specific item
	void SetCheckOnAllDescendentsOf(const FileEntry* parentEntry, bool bCheck);

	///< Build a path list of all the selected items in the list (NOTE - SELECTED, not CHECKED)
	void FillListOfSelectedItemPaths(CTSVNPathList& pathList);

	afx_msg void OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

private:
	static bool					m_bAscending;		///< sort direction
	static int					m_nSortedColumn;	///< which column to sort
	bool						m_bHasExternalsFromDifferentRepos;
	bool						m_bHasExternals;
	BOOL						m_bHasUnversionedItems;
	typedef std::vector<FileEntry*> FileEntryVector;
	FileEntryVector				m_arStatusArray;
	std::vector<DWORD>			m_arListArray;
	CTSVNPathList				m_tempFileList;
	CString						m_sLastError;

	LONG						m_nUnversioned;
	LONG						m_nNormal;
	LONG						m_nModified;
	LONG						m_nAdded;
	LONG						m_nDeleted;
	LONG						m_nConflicted;
	LONG						m_nTotal;
	LONG						m_nSelected;

	DWORD						m_dwColumns;
	DWORD						m_dwShow;
	BOOL						m_bBlock;

	int							m_nIconFolder;

	CWnd *						m_pStatLabel;
	CButton *					m_pSelectButton;

};
