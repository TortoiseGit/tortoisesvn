#pragma once

#include "DiffData.h"
#include "Patch.h"
#include "afxcmn.h"

/**
 * \ingroup TortoiseMerge
 * Virtual class providing the callback interface which
 * is used by CFilePatchesDlg.
 */
class CPatchFilesDlgCallBack
{
public:
	/**
	 * Callback function. Called when the user doubleclicks on a
	 * specific file to patch. The framework the has to process
	 * the patching/viewing.
	 * \param sFilePath the full path to the file to patch
	 * \param sVersion the revision number of the file to patch
	 * \return TRUE if patching was successful
	 */
	virtual BOOL PatchFile(CString sFilePath, CString sVersion) = 0;
};


/**
 * \ingroup TortoiseMerge
 *
 * Dialog class, showing all files to patch from a unified diff file.
 */
class CFilePatchesDlg : public CDialog
{
	DECLARE_DYNAMIC(CFilePatchesDlg)

public:
	CFilePatchesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFilePatchesDlg();

	/**
	 * Call this method to initialize the dialog.
	 * \param pPatch The CPatch object used to get the filenames from the unified diff file
	 * \param pCallBack The Object providing the callback interface (CPatchFilesDlgCallBack)
	 * \param sPath The path to the "parent" folder where the patch gets applied to
	 * \return TRUE if successful
	 */
	BOOL	Init(CPatch * pPatch, CPatchFilesDlgCallBack * pCallBack, CString sPath);

	enum { IDD = IDD_FILEPATCHES };
protected:
	CPatch *					m_pPatch;
	CPatchFilesDlgCallBack *	m_pCallBack;
	CString						m_sPath;
	CListCtrl					m_cFileList;
	CDWordArray					m_arFileStates;
	CImageList					m_ImgList;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

	CString GetFullPath(int nIndex);
public:
	afx_msg void OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult);
};
