#include "stdafx.h"
#include "TortoiseMerge.h"
#include "FilePatchesDlg.h"
#include ".\filepatchesdlg.h"


IMPLEMENT_DYNAMIC(CFilePatchesDlg, CDialog)
CFilePatchesDlg::CFilePatchesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilePatchesDlg::IDD, pParent)
{
}

CFilePatchesDlg::~CFilePatchesDlg()
{
}

void CFilePatchesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_cFileList);
}

CString CFilePatchesDlg::GetFullPath(int nIndex)
{
	CString temp = m_pPatch->GetFilename(nIndex);
	temp.Replace('/', '\\');
	temp = temp.Mid(temp.Find('\\')+1);
	temp = m_sPath + temp;
	return temp;
}

BOOL CFilePatchesDlg::Init(CPatch * pPatch, CPatchFilesDlgCallBack * pCallBack, CString sPath)
{
	if ((pCallBack==NULL)||(pPatch==NULL))
	{
		return FALSE;
	}
	m_pPatch = pPatch;
	m_pCallBack = pCallBack;
	m_sPath = sPath;
	if (m_sPath.Right(1).Compare(_T("\\"))==0)
		m_sPath = m_sPath.Left(m_sPath.GetLength()-1);

	m_sPath = m_sPath + _T("\\");

	m_cFileList.SetExtendedStyle(LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT);
	m_cFileList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_cFileList.DeleteColumn(c--);
	m_cFileList.InsertColumn(0, _T(""));

	m_cFileList.SetRedraw(false);

	for(int i=0; i<m_pPatch->GetNumberOfFiles(); i++)
	{
		CString sFile = m_pPatch->GetFilename(i);
		sFile.Replace('/', '\\');
		sFile = sFile.Mid(sFile.ReverseFind('\\')+1);
		m_cFileList.InsertItem(i, sFile);
		m_arFileStates.Add(m_pPatch->PatchFile(GetFullPath(i)));
	} // for(int i=0; i<m_pPatch->GetNumberOfFiles(); i++) 
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_cFileList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_cFileList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	m_cFileList.SetRedraw(true);
	return TRUE;
}

BEGIN_MESSAGE_MAP(CFilePatchesDlg, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_GETINFOTIP, IDC_FILELIST, OnLvnGetInfoTipFilelist)
	ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_FILELIST, OnNMCustomdrawFilelist)
END_MESSAGE_MAP()

void CFilePatchesDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (this->IsWindowVisible())
	{
		CRect rect;
		GetClientRect(rect);
		GetDlgItem(IDC_FILELIST)->MoveWindow(rect.left, rect.top , cx, cy);
		m_cFileList.SetColumnWidth(0, cx);
	} // if (this->IsWindowVisible()) 
}

void CFilePatchesDlg::OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);

	CString temp = m_pPatch->GetFilename(pGetInfoTip->iItem);
	temp.Replace('/', '\\');
	temp = m_sPath + temp;
	_tcsncpy(pGetInfoTip->pszText, temp, pGetInfoTip->cchTextMax);
	*pResult = 0;
}

void CFilePatchesDlg::OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (m_pCallBack)
	{
		m_pCallBack->PatchFile(GetFullPath(pNMLV->iItem), m_pPatch->GetRevision(pNMLV->iItem));
	} // if ((m_pCallBack)&&(!temp.IsEmpty())) 
}

void CFilePatchesDlg::OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult)
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

		COLORREF crText = ::GetSysColor(COLOR_WINDOWTEXT);

		if (m_arFileStates.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
		{
			if (!m_arFileStates.GetAt(pLVCD->nmcd.dwItemSpec))
			{
				crText = RGB(200, 0, 0);
			} 
			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
		} // if (m_arFileStates.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec) 

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;
	}
}
